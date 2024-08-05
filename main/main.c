#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "sys_display.h"
#include "sys_input.h"
#include "sys_wifi.h"

static const char *TAG = "TRC-W";

/* 全局变量 */
bool g_system_paired_flag = false; // [标识符] 系统是否已经配对过

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

    uint8_t temp = 0;
    err = nvs_get_u8(my_handle, "paired", &temp);
    if (err == ESP_OK) {
        g_system_paired_flag = true;
    }

    nvs_close(my_handle);

    ESP_ERROR_CHECK(esp_event_loop_create_default()); // 创建系统事件任务循环
}

void app_main(void) {
    system_init(); // 初始化系统

    system_display_init();            // 初始化数码管
    system_display_set_string("123"); // 设置数码管显示

    system_input_init(); // 初始化输入设备
    system_wifi_init();  // 初始化Wi-Fi
}
