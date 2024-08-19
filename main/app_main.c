#include "esp_event.h"
#include "esp_log.h"

#include "app_settings.h"
#include "app_wifi.h"
#include "sys_display.h"
#include "app_input.h"
#include "sys_tasks.h"

__unused static const char *TAG = "app_main";

static void system_init(void) {
    /* 初始化NVS (Non-Volatile Storage) 闪存 */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(settings_read_parameter_from_nvs());

    /* 创建系统事件任务循环 */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void app_main(void) {
    system_init(); // 初始化系统

    system_display_init(); // 初始化数码管
    system_input_init();   // 初始化输入设备
    system_tasks_init();   // 初始化应用任务
    system_wifi_init();    // 初始化Wi-Fi
}
