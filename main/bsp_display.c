#include <string.h>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "bsp_display.h"

__unused static const char *TAG = "bsp_display";

#define HL16_COMBI(high, low) (((high) << 8) | (low)) /* 合并两个8位数据为一个HL16数据 */
#define HL16_GETHI(data) (((data) >> 8) & 0xFF)       /* 获取HL16数据的高8位 */
#define HL16_GETLO(data) ((data) & 0xFF)              /* 获取HL16数据的低8位 */

/**
 * @brief 数码管引脚配置
 *
 * 此设备的LED数码管在微观时间内只能同时点亮一段LED。
 * 本宏用于定义点亮单段LED所需的引脚配置。
 *
 * @note 数据类型为`uint16_t`
 * @note 使用`SegCfg_Hi`与`SegCfg_Lo`宏分别获取高低电平引脚
 *
 * @param high 高电平引脚
 * @param low 低电平引脚
 */
#define SegCfg(high, low) HL16_COMBI(BSP_DISP_PIN_##high, BSP_DISP_PIN_##low)
#define SegCfg_Hi(seg_cfg) HL16_GETHI(seg_cfg)
#define SegCfg_Lo(seg_cfg) HL16_GETLO(seg_cfg)

static bool g_display_status = false;       // 显示是否开启
static char g_display_content[32] = "";     // 显示的内容
static uint16_t g_display_buffer[32] = {0}; // LED数码管显示缓冲区
static int g_display_buffer_len = 0;        // LED数码管显示缓冲区长度

static gptimer_handle_t display_gptimer_handle; // LED数码管刷新定时器句柄

/* LED数码管引脚定义映射表 */
static const uint16_t led_seg_cfg_map[3][8] = {
    {SegCfg(5, 6), SegCfg(4, 6), SegCfg(1, 6), SegCfg(6, 1), SegCfg(5, 1), SegCfg(2, 1), SegCfg(1, 2), SegCfg(2, 6)},
    {SegCfg(6, 5), SegCfg(4, 5), SegCfg(3, 5), SegCfg(2, 5), SegCfg(1, 5), SegCfg(6, 2), SegCfg(5, 2), SegCfg(2, 3)},
    {SegCfg(6, 4), SegCfg(5, 4), SegCfg(3, 4), SegCfg(2, 4), SegCfg(1, 4), SegCfg(4, 2), SegCfg(3, 2), SegCfg(4, 3)},
};

/* LED数码管字符手册 */
static const uint8_t led_seg_pattern_list[12][7] = {
    {0, 0, 0, 0, 0, 0, 0}, // None
    {0, 1, 1, 0, 0, 0, 0}, // 1
    {1, 1, 0, 1, 1, 0, 1}, // 2
    {1, 1, 1, 1, 0, 0, 1}, // 3
    {0, 1, 1, 0, 0, 1, 1}, // 4
    {1, 0, 1, 1, 0, 1, 1}, // 5
    {1, 0, 1, 1, 1, 1, 1}, // 6
    {1, 1, 1, 0, 0, 0, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1}, // 8
    {1, 1, 1, 1, 0, 1, 1}, // 9
    {1, 1, 1, 1, 1, 1, 0}, // 0
    {1, 0, 0, 1, 1, 1, 1}, // E
};

/**
 * @brief 获取字符在数码管字符手册中的索引
 *
 * @param character 查询的字符
 *
 * @return 字符在数码管字符手册中的索引, 无法识别的字符返回空白字符索引
 */
static int display_get_pattern_index(char character) {
    switch (character) {
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case '0': return 10;
        case 'E': return 11;
        default: return 0;
    }
}

/**
 * @brief 重置数码管全部引脚状态为高阻态
 */
static void display_disable_all_seg(void) {
    gpio_set_direction(BSP_DISP_PIN_1, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(BSP_DISP_PIN_2, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(BSP_DISP_PIN_3, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(BSP_DISP_PIN_4, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(BSP_DISP_PIN_5, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(BSP_DISP_PIN_6, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(BSP_DISP_PIN_1, 1);
    gpio_set_level(BSP_DISP_PIN_2, 1);
    gpio_set_level(BSP_DISP_PIN_3, 1);
    gpio_set_level(BSP_DISP_PIN_4, 1);
    gpio_set_level(BSP_DISP_PIN_5, 1);
    gpio_set_level(BSP_DISP_PIN_6, 1);
}

/**
 * @brief 点亮LED数码管中的一段LED
 *
 * @param pin_config 数码管引脚配置
 */
static void display_enable_one_seg(uint16_t pin_config) {
    display_disable_all_seg();

    gpio_set_direction(SegCfg_Hi(pin_config), GPIO_MODE_OUTPUT);
    gpio_set_direction(SegCfg_Lo(pin_config), GPIO_MODE_OUTPUT);
    gpio_set_level(SegCfg_Hi(pin_config), 1);
    gpio_set_level(SegCfg_Lo(pin_config), 0);
}

/**
 * @brief 刷新数码管显示缓冲区
 *
 * 将`display_content`中的字符解析并缓存到`display_buffer`中
 */
static void display_flush_buffer(void) {
    int next_buf_index = 0;
    int current_unit = -1;   // 当前正在处理的数码管单元
    bool prev_is_dot = true; // 上一个字符是否是小数点

    int buf_size = sizeof(g_display_buffer) / sizeof(g_display_buffer[0]);

    for (int i = 0; i < strlen(g_display_content); i++) {
        bool is_dot = g_display_content[i] == '.';

        /* 当上一个字符是小数点或者当前字符不是小数点时，数码管单元指针++ */
        if (prev_is_dot || !is_dot) current_unit++;

        /* 如果当前字符超出了数码管最大显示字符数，则停止解析 */
        if (current_unit >= BSP_DISP_MAX_CHAR) {
            ESP_LOGW(TAG, "Display content overflow, truncating display content");
            break;
        }

        /* 如果缓冲区已满，则停止解析 */
        if (next_buf_index >= buf_size) {
            ESP_LOGW(TAG, "Display buffer overflow, truncating display content");
            break;
        }

        if (is_dot) {
            g_display_buffer[next_buf_index++] = led_seg_cfg_map[current_unit][7];
        } else {
            int index = display_get_pattern_index(g_display_content[i]);
            for (int j = 0; j < 7; j++) {
                if (led_seg_pattern_list[index][j] == 1) {
                    g_display_buffer[next_buf_index++] = led_seg_cfg_map[current_unit][j];
                }
            }
        }

        prev_is_dot = is_dot;
    }

    g_display_buffer_len = next_buf_index;
}

/**
 * @brief (数码管刷新定时器回调函数) 刷新数码管
 */
static bool IRAM_ATTR display_refresh_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *event,
                                               void *user_data) {
    static int display_buffer_ptr;

    if (display_buffer_ptr >= g_display_buffer_len) display_buffer_ptr = 0;
    if (g_display_buffer_len > 0) display_enable_one_seg(g_display_buffer[display_buffer_ptr++]);

    return pdFALSE;
}

void bsp_display_set_string(const char *str) {
    if (str == NULL) return;

    // 复制显示内容
    memset(g_display_content, 0, sizeof(g_display_content));
    strncpy(g_display_content, str, sizeof(g_display_content) - 1);

    // 刷新数码管显示缓冲区
    display_flush_buffer();

    // 如果显示内容为空，则暂停数码管刷新
    if (g_display_buffer_len == 0) bsp_display_pause();
    else if (g_display_status == 0) bsp_display_resume();
}

void bsp_display_set_int(int num) {
    char str[BSP_DISP_MAX_CHAR + 1];
    snprintf(str, sizeof(str), "%d", num);
    bsp_display_set_string(str);
}

void bsp_display_pause(void) {
    ESP_ERROR_CHECK(gptimer_stop(display_gptimer_handle));
    display_disable_all_seg();
    g_display_status = false;
}

void bsp_display_resume(void) {
    ESP_ERROR_CHECK(gptimer_start(display_gptimer_handle));
    g_display_status = true;
}

void bsp_display_init(void) {
    // 初始化数码管所需的GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BSP_DISP_PIN_1 | 1ULL << BSP_DISP_PIN_2 | 1ULL << BSP_DISP_PIN_3 |
                        1ULL << BSP_DISP_PIN_4 | 1ULL << BSP_DISP_PIN_5 | 1ULL << BSP_DISP_PIN_6,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    // 初始化数码管刷新定时器
    gptimer_config_t gptimer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 100000, // 100kHz
    };
    gptimer_event_callbacks_t gptimer_callbacks = {
        .on_alarm = display_refresh_timer_cb,
    };
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 50, // 0.5ms
        .flags.auto_reload_on_alarm = true,
    };

    // 开启数码管刷新定时器
    ESP_ERROR_CHECK(gptimer_new_timer(&gptimer_config, &display_gptimer_handle));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(display_gptimer_handle, &gptimer_callbacks, NULL));
    ESP_ERROR_CHECK(gptimer_enable(display_gptimer_handle));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(display_gptimer_handle, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(display_gptimer_handle));
    bsp_display_set_string("");
}
