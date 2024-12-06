#include "driver/gpio.h"
#include "esp_log.h"

#include "ic_74hc595_driver.h"

#define IC_DRIVER_CHECK_RETURN(a, str, ret_val)                   \
    if (!(a)) {                                                   \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                         \
    }

#define IC_DRIVER_CHECK_GOTO(a, str, label)                        \
    if (!(a)) {                                                    \
        ESP_LOGE(TAG, "%s:(%d): %s", __FUNCTION__, __LINE__, str); \
        goto label;                                                \
    }

static const char* TAG = "ic_74hc595_driver";

typedef struct {
    gpio_num_t mr_;  /* Master Reset                 (主复位) */
    gpio_num_t shcp; /* Shift Register Clock Pulse   (移位寄存器时钟脉冲) */
    gpio_num_t stcp; /* Storage Register Clock Pulse (存储寄存器时钟脉冲) */
    gpio_num_t oe_;  /* Output Enable                (输出使能) */
    gpio_num_t ds;   /* Data Serial                  (串行数据输入) */
} ic_driver_dev_t;

static esp_err_t ic_74hc595_oe_func_base(const ic_74hc595_handle_t handle, const uint32_t level) {
    IC_DRIVER_CHECK_RETURN(handle != NULL, "The handle is NULL", ESP_ERR_INVALID_ARG);

    const ic_driver_dev_t* dev = handle;

    IC_DRIVER_CHECK_RETURN(dev->oe_ >= 0, "Output Enable Pin is offline", ESP_ERR_INVALID_ARG);
    gpio_set_level(dev->oe_, level);

    return ESP_OK;
}

esp_err_t ic_74hc595_init(const ic_74hc595_config_t* config, ic_74hc595_handle_t* handle) {
    IC_DRIVER_CHECK_RETURN(config != NULL, "The pointer of config is NULL", ESP_ERR_INVALID_ARG);

    ic_driver_dev_t* dev = malloc(sizeof(ic_driver_dev_t));
    IC_DRIVER_CHECK_RETURN(dev != NULL, "Memory allocation failed", ESP_ERR_NO_MEM);

    dev->ds = config->ds;
    dev->shcp = config->shcp;
    dev->stcp = config->stcp;
    dev->mr_ = config->mr_ >= 0 ? config->mr_ : -1;
    dev->oe_ = config->oe_ >= 0 ? config->oe_ : -1;

    esp_err_t ret = ESP_OK;
    gpio_config_t io_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (1) {
        io_config.pin_bit_mask = (1ULL << dev->shcp) | (1ULL << dev->stcp) | (1ULL << dev->ds);
        ret = gpio_config(&io_config);
        IC_DRIVER_CHECK_GOTO(ret == ESP_OK, "GPIO configuration failed", clean_up);
    }
    if (dev->oe_ >= 0) {
        io_config.pin_bit_mask = (1ULL << dev->oe_);
        ret = gpio_config(&io_config);
        IC_DRIVER_CHECK_GOTO(ret == ESP_OK, "GPIO configuration failed", clean_up);
    }
    if (dev->mr_ >= 0) {
        io_config.pin_bit_mask = (1ULL << dev->mr_);
        ret = gpio_config(&io_config);
        IC_DRIVER_CHECK_GOTO(ret == ESP_OK, "GPIO configuration failed", clean_up);
    }

    /*
     * Set the initial state of the 74HC595 output to 0 / High-Z.
     * Clear the 74HC595 shift register & output register, setting all stored bits to 0.
     */
    if (dev->oe_ >= 0) { ic_74hc595_disable_output(dev); }
    ic_74hc595_reset(dev);

    *handle = (ic_74hc595_handle_t)dev;
    return ESP_OK;

clean_up:
    free(dev);
    return ret;
}

esp_err_t ic_74hc595_deinit(ic_74hc595_handle_t* handle) {
    IC_DRIVER_CHECK_RETURN(handle != NULL, "The pointer of handle is NULL", ESP_ERR_INVALID_ARG);
    IC_DRIVER_CHECK_RETURN(*handle != NULL, "The handle is NULL", ESP_ERR_INVALID_ARG);

    ic_driver_dev_t* dev = *handle;

    gpio_reset_pin(dev->shcp);
    gpio_reset_pin(dev->stcp);
    gpio_reset_pin(dev->ds);
    if (dev->oe_ >= 0) { gpio_reset_pin(dev->oe_); }
    if (dev->mr_ >= 0) { gpio_reset_pin(dev->mr_); }

    free(dev);
    *handle = NULL;

    return ESP_OK;
}

esp_err_t ic_74hc595_write(const ic_74hc595_handle_t handle, const uint8_t data) {
    IC_DRIVER_CHECK_RETURN(handle != NULL, "The handle is NULL", ESP_ERR_INVALID_ARG);

    const ic_driver_dev_t* dev = handle;

    for (int i = 0; i < 8; i++) {
        gpio_set_level(dev->ds, data >> i & 0x01);
        gpio_set_level(dev->shcp, 1);
        gpio_set_level(dev->shcp, 0);
    }

    return ESP_OK;
}

esp_err_t ic_74hc595_latch(const ic_74hc595_handle_t handle) {
    IC_DRIVER_CHECK_RETURN(handle != NULL, "The handle is NULL", ESP_ERR_INVALID_ARG);

    const ic_driver_dev_t* dev = handle;

    gpio_set_level(dev->stcp, 1);
    gpio_set_level(dev->stcp, 0);

    return ESP_OK;
}

esp_err_t ic_74hc595_enable_output(const ic_74hc595_handle_t handle) {
    return ic_74hc595_oe_func_base(handle, 0);
}

esp_err_t ic_74hc595_disable_output(const ic_74hc595_handle_t handle) {
    return ic_74hc595_oe_func_base(handle, 1);
}

esp_err_t ic_74hc595_reset(const ic_74hc595_handle_t handle) {
    IC_DRIVER_CHECK_RETURN(handle != NULL, "The handle is NULL", ESP_ERR_INVALID_ARG);

    const ic_driver_dev_t* dev = handle;

    if (dev->mr_ >= 0) {
        gpio_set_level(dev->mr_, 0);
        gpio_set_level(dev->mr_, 1);
    } else {
        ESP_LOGI(TAG, "IC Master Reset Pin is offline, using software reset instead");
        ic_74hc595_write(handle, 0x00);
        ic_74hc595_latch(handle);
    }

    return ESP_OK;
}
