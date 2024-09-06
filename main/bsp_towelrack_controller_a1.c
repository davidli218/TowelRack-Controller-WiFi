#include "iot_button.h"
#include "iot_knob.h"
#include "led_strip.h"
#include "ntc_driver.h"

#include "bsp/seg_display_driver.h"
#include "bsp/towelrack_controller_a1.h"

static const char* TAG = "towelrack-controller-a1";

/**************************************************************************************************
 * Config // 74HC595 IC & 7-Segment Display
 **************************************************************************************************/

const display_config_t bsp_display_config = {
    .ds = BSP_DISP_IC_DS,
    .shcp = BSP_DISP_IC_SHCP,
    .stcp = BSP_DISP_IC_STCP,
    .u1_ctrl = BSP_DISP_U1_CTRL,
    .u2_ctrl = BSP_DISP_U2_CTRL,
    .max_lens = 2,
};

/**************************************************************************************************
 * Config // Input Devices
 **************************************************************************************************/

static const button_config_t config_touch_button_left = {
    .type = BUTTON_TYPE_GPIO,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
    .gpio_button_config =
    {
        .gpio_num = BSP_TOUCH_BUTTON_LEFT,
        .active_level = 1,
    },
};

static const button_config_t config_touch_button_right = {
    .type = BUTTON_TYPE_GPIO,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
    .gpio_button_config =
    {
        .gpio_num = BSP_TOUCH_BUTTON_RIGHT,
        .active_level = 1,
    },
};

static const knob_config_t config_knob_encoder_a_b = {
    .default_direction = 0,
    .gpio_encoder_a = BSP_KNOB_ENCODER_A,
    .gpio_encoder_b = BSP_KNOB_ENCODER_B,
};

static const button_config_t config_knob_btn = {
    .type = BUTTON_TYPE_GPIO,
    .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
    .gpio_button_config =
    {
        .gpio_num = BSP_KNOB_BUTTON,
        .active_level = 0,
    },
};

/**************************************************************************************************
 * Config // LED Strip
 **************************************************************************************************/

#define BSP_LED_STRIP_NUM 4

const led_strip_config_t strip_config = {
    .strip_gpio_num = BSP_LED_STRIP_GPIO,
    .max_leds = BSP_LED_STRIP_NUM,
    .led_pixel_format = LED_PIXEL_FORMAT_GRB,
    .led_model = LED_MODEL_SK6812,
    .flags.invert_out = false,
};

const led_strip_rmt_config_t rmt_config = {
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = 10 * 1000 * 1000,
    .flags.with_dma = false,
};

/**************************************************************************************************
 * Config // NTC Temperature Sensor + Heating Control
 **************************************************************************************************/

static ntc_config_t ntc_config = {
    .b_value = 3950,
    .r25_ohm = 10000,
    .fixed_ohm = 10000,
    .vdd_mv = 3300,
    .circuit_mode = CIRCUIT_MODE_NTC_GND,
    .atten = ADC_ATTEN_DB_12,
    .channel = BSP_NTC_ADC_CHANNEL,
    .unit = BSP_NTC_ADC_UNIT,
};

static gpio_config_t heating_ctrl_config = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = (1ULL << BSP_HEATING_CTRL_PORT),
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pull_up_en = GPIO_PULLUP_DISABLE,
};

/**************************************************************************************************
 * Implementation // 74HC595 IC & 7-Segment Display
 **************************************************************************************************/

static display_device_handle_t display_device = NULL;

void bsp_display_init(void) {
    display_init(&bsp_display_config, &display_device);
    display_enable_all(display_device);
}

void bsp_display_write_str(const char* str) { display_write_str(display_device, str); }

void bsp_display_write_int(const int num) { display_write_int(display_device, num); }

void bsp_display_set_c_flag(const bool flag) { display_set_c_flag(display_device, flag); }

void bsp_display_set_h_flag(const bool flag) { display_set_h_flag(display_device, flag); }

/**************************************************************************************************
 * Implementation // Input Devices
 **************************************************************************************************/

static QueueHandle_t bsp_input_queue = NULL;

static const button_event_config_t btn_mt8_click_config = {
    .event = BUTTON_MULTIPLE_CLICK,
    .event_data =
    {
        .multiple_clicks = {.clicks = 8},
    },
};

static void bsp_input_event_cb(void* _, void* usr_data) {
    const bsp_input_event_t event = (uintptr_t)usr_data;
    xQueueSendFromISR(bsp_input_queue, &event, NULL);
}

void bsp_input_init(void) {
    /* 初始化旋钮编码器 */
    const knob_handle_t kb_ec_handle = iot_knob_create(&config_knob_encoder_a_b);
    iot_knob_register_cb(kb_ec_handle, KNOB_LEFT, bsp_input_event_cb, (void*)BSP_KNOB_ENCODER_ACW);
    iot_knob_register_cb(kb_ec_handle, KNOB_RIGHT, bsp_input_event_cb, (void*)BSP_KNOB_ENCODER_CW);

    /* 初始化旋钮按钮 */
    const button_handle_t kb_btn_handle = iot_button_create(&config_knob_btn);
    iot_button_register_cb(kb_btn_handle, BUTTON_LONG_PRESS_START, bsp_input_event_cb, (void*)BSP_KNOB_LONG_PRESS);
    iot_button_register_event_cb(kb_btn_handle, btn_mt8_click_config, bsp_input_event_cb, (void*)BSP_KNOB_MT8_CLICK);

    /* 初始化触摸按键 */
    const button_handle_t tc_btn_l_handle = iot_button_create(&config_touch_button_left);
    const button_handle_t tc_btn_r_handle = iot_button_create(&config_touch_button_right);
    iot_button_register_cb(tc_btn_l_handle, BUTTON_SINGLE_CLICK, bsp_input_event_cb, (void*)BSP_TOUCH_BUTTON_L_CLICK);
    iot_button_register_cb(tc_btn_r_handle, BUTTON_SINGLE_CLICK, bsp_input_event_cb, (void*)BSP_TOUCH_BUTTON_R_CLICK);

    /* 创建输入设备输入事件队列 */
    bsp_input_queue = xQueueCreate(10, sizeof(bsp_input_event_t));
}

QueueHandle_t bsp_input_get_queue(void) { return bsp_input_queue; }

char* bsp_input_event_to_string(const bsp_input_event_t event) {
    switch (event) {
        case BSP_KNOB_ENCODER_ACW:
            return "BSP_KNOB_ENCODER_ACW";
        case BSP_KNOB_ENCODER_CW:
            return "BSP_KNOB_ENCODER_CW";
        case BSP_KNOB_LONG_PRESS:
            return "BSP_KNOB_LONG_PRESS";
        case BSP_KNOB_MT8_CLICK:
            return "BSP_KNOB_MT8_CLICK";
        case BSP_TOUCH_BUTTON_L_CLICK:
            return "BSP_TOUCH_BUTTON_L_CLICK";
        case BSP_TOUCH_BUTTON_R_CLICK:
            return "BSP_TOUCH_BUTTON_R_CLICK";
        default:
            return "BSP_UNKNOWN_INPUT_EVENT";
    }
}

/**************************************************************************************************
 * Implementation // LED Strip
 **************************************************************************************************/

static led_strip_handle_t led_strip = NULL;
static int led_strip_brightness = 50;

void bsp_led_strip_init(void) {
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    bsp_led_strip_write(BSP_STRIP_WHITE);
}

void bsp_led_strip_write(const bsp_led_strip_mode_t mode) {
    int red, green, blue;

    if (mode == BSP_STRIP_OFF) {
        ESP_ERROR_CHECK(led_strip_clear(led_strip));
        return;
    }

    switch (mode) {
        case BSP_STRIP_WHITE:
            red = 255 * led_strip_brightness / 100;
            green = 255 * led_strip_brightness / 100;
            blue = 255 * led_strip_brightness / 100;
            break;
        case BSP_STRIP_ORANGE:
            red = 255 * led_strip_brightness / 100;
            green = 76 * led_strip_brightness / 100;
            blue = 10 * led_strip_brightness / 100;
            break;
        case BSP_STRIP_RED:
            red = 255 * led_strip_brightness / 100;
            green = 0;
            blue = 0;
            break;
        case BSP_STRIP_GREEN:
            red = 76 * led_strip_brightness / 100;
            green = 255 * led_strip_brightness / 100;
            blue = 10 * led_strip_brightness / 100;
            break;
        case BSP_STRIP_BLUE:
            red = 0;
            green = 0;
            blue = 255 * led_strip_brightness / 100;
            break;
        default:
            red = 0;
            green = 0;
            blue = 0;
            break;
    }

    for (int i = 0; i < BSP_LED_STRIP_NUM; i++) {
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, red, green, blue));
    }

    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

/**************************************************************************************************
 * Implementation // NTC Temperature Sensor + Heating Control
 **************************************************************************************************/

static ntc_device_handle_t ntc_device = NULL;

void bsp_heating_init(void) {
    /* 初始化NTC */
    adc_oneshot_unit_handle_t adc_handle = NULL;
    ESP_ERROR_CHECK(ntc_dev_create(&ntc_config, &ntc_device, &adc_handle));
    ESP_ERROR_CHECK(ntc_dev_get_adc_handle(ntc_device, &adc_handle));

    /* 初始化加热器控制 */
    gpio_config(&heating_ctrl_config);
    gpio_set_level(BSP_HEATING_CTRL_PORT, 0);
}

int bsp_heating_get_temp(void) {
    float temp;

    if (ntc_dev_get_temperature(ntc_device, &temp) == ESP_OK) {
        return (int)temp;
    }

    ESP_LOGE(TAG, "Failed to get temperature");
    return 100;
}

void bsp_heating_enable(void) { gpio_set_level(BSP_HEATING_CTRL_PORT, 1); }

void bsp_heating_disable(void) { gpio_set_level(BSP_HEATING_CTRL_PORT, 0); }


/**************************************************************************************************
 * Implementation // Initialize All Peripherals
 **************************************************************************************************/
void bsp_init_all(void) {
    bsp_display_init();
    bsp_led_strip_init();
    bsp_heating_init();
    bsp_input_init();

    vTaskDelay(pdMS_TO_TICKS(2000));         // 等待2s (自检)
    display_write_str(display_device, NULL); // 熄灭数码管
    bsp_led_strip_write(BSP_STRIP_OFF);      // 熄灭LED灯带
}
