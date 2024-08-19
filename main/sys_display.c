#include "sys_display.h"

__unused static const char *TAG = "TRC-W::DISP";

static bool display_status_on = false;    // 数码管是否启动
static char display_pattern[7] = "";      // 数码管显示内容
static uint16_t display_buffer[24] = {0}; // 数码管显示缓冲区
static int display_buffer_len = 0;        // 数码管显示缓冲区长度

static gptimer_handle_t disp_gptimer_handle; // 数码管刷新定时器句柄

/* 数码管可显示的字符字典 */
static const uint8_t disp_pattern_segment_map[12][7] = {
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

/*
 * 数码管的引脚定义
 *
 * Key: [行号]代表从左到右数码管的编号，[列号]代表数码管的段号。
 * Val: [高8位]表示要设为高电平的引脚，[低8位]表示要设为低电平的引脚。
 */
static const uint16_t disp_segment_pin_map[3][8] = {
    {HL16_CMD(DISP_PIN_5, DISP_PIN_6), HL16_CMD(DISP_PIN_4, DISP_PIN_6), HL16_CMD(DISP_PIN_1, DISP_PIN_6),
     HL16_CMD(DISP_PIN_6, DISP_PIN_1), HL16_CMD(DISP_PIN_5, DISP_PIN_1), HL16_CMD(DISP_PIN_2, DISP_PIN_1),
     HL16_CMD(DISP_PIN_1, DISP_PIN_2), HL16_CMD(DISP_PIN_2, DISP_PIN_6)},
    {HL16_CMD(DISP_PIN_6, DISP_PIN_5), HL16_CMD(DISP_PIN_4, DISP_PIN_5), HL16_CMD(DISP_PIN_3, DISP_PIN_5),
     HL16_CMD(DISP_PIN_2, DISP_PIN_5), HL16_CMD(DISP_PIN_1, DISP_PIN_5), HL16_CMD(DISP_PIN_6, DISP_PIN_2),
     HL16_CMD(DISP_PIN_5, DISP_PIN_2), HL16_CMD(DISP_PIN_2, DISP_PIN_3)},
    {HL16_CMD(DISP_PIN_6, DISP_PIN_4), HL16_CMD(DISP_PIN_5, DISP_PIN_4), HL16_CMD(DISP_PIN_3, DISP_PIN_4),
     HL16_CMD(DISP_PIN_2, DISP_PIN_4), HL16_CMD(DISP_PIN_1, DISP_PIN_4), HL16_CMD(DISP_PIN_4, DISP_PIN_2),
     HL16_CMD(DISP_PIN_3, DISP_PIN_2), HL16_CMD(DISP_PIN_4, DISP_PIN_3)},
};

/**
 * @brief 重置数码管全部引脚状态为高阻态
 */
static void disp_reset(void) {
    gpio_set_direction(DISP_PIN_1, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(DISP_PIN_2, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(DISP_PIN_3, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(DISP_PIN_4, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(DISP_PIN_5, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(DISP_PIN_6, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(DISP_PIN_1, 1);
    gpio_set_level(DISP_PIN_2, 1);
    gpio_set_level(DISP_PIN_3, 1);
    gpio_set_level(DISP_PIN_4, 1);
    gpio_set_level(DISP_PIN_5, 1);
    gpio_set_level(DISP_PIN_6, 1);
}

/**
 * @brief 点亮数码管中的一段LED
 *
 * @param pin_config 数码管引脚配置
 */
static void disp_enable_one_segment(uint16_t pin_config) {
    disp_reset();

    gpio_set_direction(HL16_GETHI(pin_config), GPIO_MODE_OUTPUT);
    gpio_set_direction(HL16_GETLO(pin_config), GPIO_MODE_OUTPUT);
    gpio_set_level(HL16_GETHI(pin_config), 1);
    gpio_set_level(HL16_GETLO(pin_config), 0);
}

/**
 * @brief 获取字符在数码管字符字典中的索引
 *
 * @param character 查询的字符
 *
 * @return 字符在数码管字符字典中的索引, 无法识别的字符返回空白字符索引
 */
static int disp_get_pattern_index(char character) {
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
 * @brief 刷新数码管显示缓冲区
 *
 * 将`display_pattern`中的字符解析并缓存到`display_buffer`中
 */
static void disp_flush_buffer(void) {
    int buf_count = 0;
    int unit_ptr = 0;        // 当前正在处理的数码管单元
    bool prev_is_dot = true; // 上一个字符是否是小数点

    for (int i = 0; i < strlen(display_pattern); i++) {
        bool is_dot = display_pattern[i] == '.';

        if (unit_ptr + (is_dot ? 0 : 1) > 2) {
            ESP_LOGW(TAG, "Too many characters to display, only the first 3 will be displayed.");
            break;
        }

        if (is_dot) {
            display_buffer[buf_count++] = disp_segment_pin_map[unit_ptr][7];
            prev_is_dot = true;
            unit_ptr++;
        } else {
            if (!prev_is_dot) {
                unit_ptr++;
            }
            int index = disp_get_pattern_index(display_pattern[i]);
            for (int j = 0; j < 7; j++) {
                if (disp_pattern_segment_map[index][j] == 1) {
                    display_buffer[buf_count++] = disp_segment_pin_map[unit_ptr][j];
                }
            }
            prev_is_dot = false;
        }
    }

    display_buffer_len = buf_count;
}

/**
 * @brief (数码管刷新定时器回调函数) 刷新数码管
 */
static bool IRAM_ATTR disp_main_refresh_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *event,
                                           void *user_data) {
    static int display_buffer_ptr;

    if (display_buffer_ptr >= display_buffer_len) display_buffer_ptr = 0;

    if (display_buffer_len > 0) disp_enable_one_segment(display_buffer[display_buffer_ptr++]);

    return pdFALSE;
}

/**
 * @brief 设置数码管显示内容
 *
 * @param str 要显示的内容
 */
void system_display_set_string(const char *str) {
    // 如果显示内容没有变化，则不刷新
    if (strcmp(display_pattern, str) == 0) {
        return;
    }

    // 复制显示内容
    strncpy(display_pattern, str, sizeof(display_pattern));
    // 确保显示内容以'\0'结尾
    display_pattern[sizeof(display_pattern) - 1] = '\0';
    // 刷新数码管显示缓冲区
    disp_flush_buffer();

    // 如果显示内容为空，则暂停数码管刷新
    if (display_buffer_len == 0) system_display_pause();
    else if (!display_status_on) system_display_resume();
}

void system_display_set_int(int num) {
    char str[4];
    snprintf(str, sizeof(str), "%d", num);
    system_display_set_string(str);
}

/**
 * @brief 暂停数码管刷新
 */
void system_display_pause(void) {
    ESP_ERROR_CHECK(gptimer_stop(disp_gptimer_handle));
    disp_reset();
    display_status_on = false;
}

/**
 * @brief 恢复数码管刷新
 */
void system_display_resume(void) {
    ESP_ERROR_CHECK(gptimer_start(disp_gptimer_handle));
    display_status_on = true;
}

/**
 * @brief 初始化数码管
 */
void system_display_init(void) {
    // 初始化数码管所需的GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << DISP_PIN_1 | 1ULL << DISP_PIN_2 | 1ULL << DISP_PIN_3 | 1ULL << DISP_PIN_4 |
                        1ULL << DISP_PIN_5 | 1ULL << DISP_PIN_6,
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
        .on_alarm = disp_main_refresh_cb,
    };
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 50, // 0.5ms
        .flags.auto_reload_on_alarm = true,
    };

    // 开启数码管刷新定时器
    ESP_ERROR_CHECK(gptimer_new_timer(&gptimer_config, &disp_gptimer_handle));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(disp_gptimer_handle, &gptimer_callbacks, NULL));
    ESP_ERROR_CHECK(gptimer_enable(disp_gptimer_handle));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(disp_gptimer_handle, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(disp_gptimer_handle));
    system_display_set_string("");
}
