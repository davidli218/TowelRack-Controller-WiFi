#include "bsp_led_strip.h"
#include "led_indicator.h"

__unused static const char *TAG = "bsp_led_strip";

led_indicator_handle_t led_handle = NULL;

/* 红色呼吸慢 - 加热中状态 */
static const blink_step_t breath_red_blink[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 0, MAX_SATURATION, MAX_BRIGHTNESS), 0},
    {LED_BLINK_BRIGHTNESS, INSERT_INDEX(MAX_INDEX, LED_STATE_OFF), 0},
    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX, LED_STATE_ON), 1000},
    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX, LED_STATE_OFF), 1000},
    {LED_BLINK_LOOP, 0, 0},
};

/* 绿色呼吸慢 - 不加热状态 */
static const blink_step_t breath_green_blink[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 120, MAX_SATURATION, MAX_BRIGHTNESS), 0},
    {LED_BLINK_BRIGHTNESS, INSERT_INDEX(MAX_INDEX, LED_STATE_OFF), 0},
    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX, LED_STATE_ON), 1000},
    {LED_BLINK_BREATHE, INSERT_INDEX(MAX_INDEX, LED_STATE_OFF), 1000},
    {LED_BLINK_LOOP, 0, 0},
};

/* 橙色常亮 - 设置温度 */
static const blink_step_t orange_blink[] = {
    {LED_BLINK_RGB, SET_IRGB(MAX_INDEX, 255, 76, 10), 0},
    {LED_BLINK_LOOP, 0, 0},
};

/* 蓝色常亮 - 设置时间 */
static const blink_step_t blue_blink[] = {
    {LED_BLINK_HSV, SET_IHSV(MAX_INDEX, 240, MAX_SATURATION, MAX_BRIGHTNESS), 0},
    {LED_BLINK_LOOP, 0, 0},
};

/* 彩虹流动 - 加热完成 */
static const blink_step_t flowing_blink[] = {
    {LED_BLINK_HSV_RING, SET_IHSV(MAX_INDEX, 0, MAX_SATURATION, MAX_BRIGHTNESS), 2500},
    {LED_BLINK_HSV_RING, SET_IHSV(MAX_INDEX, MAX_HUE, MAX_SATURATION, MAX_BRIGHTNESS), 2500},
    {LED_BLINK_LOOP, 0, 0},
};

blink_step_t const *led_mode[] = {

    [BLINK_ORANGE] = orange_blink,
    [BLINK_BLUE] = blue_blink,
    [BLINK_RED_BREATHE] = breath_red_blink,
    [BLINK_GREEN_BREATHE] = breath_green_blink,
    [BLINK_FLOWING] = flowing_blink,

    [BLINK_MAX] = NULL,

};

void bsp_led_strip_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = BSP_LED_STRIP_GPIO,     // The GPIO that connected to the LED strip's data line
        .max_leds = BSP_LED_STRIP_NUM,            // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_SK6812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,            // different clock source can lead to different power consumption
        .resolution_hz = BSP_LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .flags.with_dma = false,                   // DMA feature is available on ESP target like ESP32-S3
    };

    led_indicator_strips_config_t strips_config = {
        .led_strip_cfg = strip_config,
        .led_strip_driver = LED_STRIP_RMT,
        .led_strip_rmt_cfg = rmt_config,
    };

    const led_indicator_config_t config = {
        .mode = LED_STRIPS_MODE,
        .led_indicator_strips_config = &strips_config,
        .blink_lists = led_mode,
        .blink_list_num = BLINK_MAX,
    };

    led_handle = led_indicator_create(&config);
    assert(led_handle != NULL);
}
