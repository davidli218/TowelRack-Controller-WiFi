#include "led.h"

char g_display_pattern[6] = {'1', 0, '2', 0, '3', 0};

static uint16_t display_buffer[24] = {0};
static int display_buffer_len = 0;

/*
 * 7段数码管引脚定义
 *
 * 行号代表从左到右数字的位置，列号代表段位置。
 * 高8位表示要设置为高电平的引脚，低8位表示要设置为低电平的引脚。
 */
static const uint16_t display_pin_map[3][8] = {
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
 * @brief 重置数码管引脚状态为高阻态
 */
static void display_reset(void) {
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
 * @brief 点亮数码管中的一个LED
 *
 * @param pin_config 7段数码管引脚配置
 */
static void display_set_bit(uint16_t pin_config) {
    display_reset();

    gpio_set_direction(HL16_GETHI(pin_config), GPIO_MODE_OUTPUT);
    gpio_set_direction(HL16_GETLO(pin_config), GPIO_MODE_OUTPUT);
    gpio_set_level(HL16_GETHI(pin_config), 1);
    gpio_set_level(HL16_GETLO(pin_config), 0);
}

/**
 * @brief 定时器回调函数，用于刷新数码管显示
 */
static bool IRAM_ATTR display_main_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *event,
                                      void *user_data) {
    BaseType_t high_task_awoken = pdFALSE;

    static int display_buffer_ptr;

    if (display_buffer_ptr >= display_buffer_len)
        display_buffer_ptr = 0;

    if (display_buffer_len > 0)
        display_set_bit(display_buffer[display_buffer_ptr++]);

    return (high_task_awoken == pdTRUE);
}

/**
 * @brief 将显示内容解码并存储到缓冲区中
 */
void display_flush_buffer(void) {
    int buf_count = 0;

    for (int i = 0; i < 3; i++) {
        // 处理主显示内容
        switch (g_display_pattern[i * 2]) {
        case '0':
            display_buffer[buf_count++] = display_pin_map[i][0];
            display_buffer[buf_count++] = display_pin_map[i][1];
            display_buffer[buf_count++] = display_pin_map[i][2];
            display_buffer[buf_count++] = display_pin_map[i][3];
            display_buffer[buf_count++] = display_pin_map[i][4];
            display_buffer[buf_count++] = display_pin_map[i][5];
            break;
        case '1':
            display_buffer[buf_count++] = display_pin_map[i][1];
            display_buffer[buf_count++] = display_pin_map[i][2];
            break;
        case '2':
            display_buffer[buf_count++] = display_pin_map[i][0];
            display_buffer[buf_count++] = display_pin_map[i][1];
            display_buffer[buf_count++] = display_pin_map[i][3];
            display_buffer[buf_count++] = display_pin_map[i][4];
            display_buffer[buf_count++] = display_pin_map[i][6];
            break;
        case '3':
            display_buffer[buf_count++] = display_pin_map[i][0];
            display_buffer[buf_count++] = display_pin_map[i][1];
            display_buffer[buf_count++] = display_pin_map[i][2];
            display_buffer[buf_count++] = display_pin_map[i][3];
            display_buffer[buf_count++] = display_pin_map[i][6];
            break;
        case '4':
            display_buffer[buf_count++] = display_pin_map[i][1];
            display_buffer[buf_count++] = display_pin_map[i][2];
            display_buffer[buf_count++] = display_pin_map[i][5];
            display_buffer[buf_count++] = display_pin_map[i][6];
            break;
        case '5':
            display_buffer[buf_count++] = display_pin_map[i][0];
            display_buffer[buf_count++] = display_pin_map[i][2];
            display_buffer[buf_count++] = display_pin_map[i][3];
            display_buffer[buf_count++] = display_pin_map[i][5];
            display_buffer[buf_count++] = display_pin_map[i][6];
            break;
        case '6':
            display_buffer[buf_count++] = display_pin_map[i][0];
            display_buffer[buf_count++] = display_pin_map[i][2];
            display_buffer[buf_count++] = display_pin_map[i][3];
            display_buffer[buf_count++] = display_pin_map[i][4];
            display_buffer[buf_count++] = display_pin_map[i][5];
            display_buffer[buf_count++] = display_pin_map[i][6];
            break;
        case '7':
            display_buffer[buf_count++] = display_pin_map[i][0];
            display_buffer[buf_count++] = display_pin_map[i][1];
            display_buffer[buf_count++] = display_pin_map[i][2];
            break;
        case '8':
            display_buffer[buf_count++] = display_pin_map[i][0];
            display_buffer[buf_count++] = display_pin_map[i][1];
            display_buffer[buf_count++] = display_pin_map[i][2];
            display_buffer[buf_count++] = display_pin_map[i][3];
            display_buffer[buf_count++] = display_pin_map[i][4];
            display_buffer[buf_count++] = display_pin_map[i][5];
            display_buffer[buf_count++] = display_pin_map[i][6];
            break;
        case '9':
            display_buffer[buf_count++] = display_pin_map[i][0];
            display_buffer[buf_count++] = display_pin_map[i][1];
            display_buffer[buf_count++] = display_pin_map[i][2];
            display_buffer[buf_count++] = display_pin_map[i][3];
            display_buffer[buf_count++] = display_pin_map[i][5];
            display_buffer[buf_count++] = display_pin_map[i][6];
            break;
        default:
            break;
        }

        // 处理小数点
        if (g_display_pattern[i * 2 + 1] != 0) {
            display_buffer[buf_count++] = display_pin_map[i][7];
        }
    }

    display_buffer_len = buf_count;
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
    gptimer_handle_t gptimer_handle;
    gptimer_config_t gptimer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 100000, // 100kHz
    };
    gptimer_event_callbacks_t gptimer_callbacks = {
        .on_alarm = display_main_cb,
    };
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 50, // 0.5ms
        .flags.auto_reload_on_alarm = true,
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&gptimer_config, &gptimer_handle));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer_handle, &gptimer_callbacks, NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer_handle));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer_handle, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(gptimer_handle));
}
