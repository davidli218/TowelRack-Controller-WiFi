#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi.h"

static const char *TAG = "TRC-W";

extern user_paired_status g_system_paired_flag;

static void system_init(void) {
    /* 初始化NVS (Non-Volatile Storage) 闪存 */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased. Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* 读取NVS中的配对标志 */
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle));

    uint8_t paired = USER_UNPAIRED;
    err = nvs_get_u8(my_handle, "paired", &paired);

    if (err == ESP_OK && paired == USER_PAIRED) {
        ESP_LOGI(TAG, "[System] Wi-Fi has been previous paired");
        g_system_paired_flag = USER_PAIRED;
    }

    nvs_close(my_handle);

    ESP_ERROR_CHECK(esp_event_loop_create_default()); // 创建系统事件任务循环
}

void app_main(void) {
    system_init();      // 初始化系统
    system_wifi_init(); // 初始化Wi-Fi
}
