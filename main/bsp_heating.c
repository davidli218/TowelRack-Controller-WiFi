#include "bsp_heating.h"
#include "ntc_driver.h"

__unused static const char* TAG = "bsp_heating";

static ntc_device_handle_t ntc = NULL;

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

void bsp_heating_init(void) {
    /* 初始化NTC */
    adc_oneshot_unit_handle_t adc_handle = NULL;
    ESP_ERROR_CHECK(ntc_dev_create(&ntc_config, &ntc, &adc_handle));
    ESP_ERROR_CHECK(ntc_dev_get_adc_handle(ntc, &adc_handle));

    /* 初始化加热器控制 */
    gpio_config(&heating_ctrl_config);
    gpio_set_level(BSP_HEATING_CTRL_PORT, 0);
}

void bsp_heating_deinit(void) {
    gpio_reset_pin(BSP_HEATING_CTRL_PORT);
    if (ntc) {
        ntc_dev_delete(ntc);
        ntc = NULL;
    }
}

int bsp_heating_get_temp(void) {
    float temp;

    if (ntc_dev_get_temperature(ntc, &temp) == ESP_OK) {
        return (int)temp;
    }

    ESP_LOGE(TAG, "Failed to get temperature");
    return 100;
}

void bsp_heating_enable(void) { gpio_set_level(BSP_HEATING_CTRL_PORT, 1); }

void bsp_heating_disable(void) { gpio_set_level(BSP_HEATING_CTRL_PORT, 0); }
