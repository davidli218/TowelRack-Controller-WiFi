#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <stdlib.h>
#include <string.h>

static const char *TAG = "TowelRack-Controller-WiFi";

typedef enum {
    USER_WIFI_UNPAIRED = 0x00,
    USER_WIFI_PAIRED = 0xFF,
} user_wifi_paired_t;

static user_wifi_paired_t system_paired_flag = USER_WIFI_UNPAIRED;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

static const int ESPTOUCH_DONE_BIT = BIT1; // SmartConfig完成标志位

/* ------ 函数预声明 ------ */

static void smartconfig_task(void *parm);

/* ------ 函数定义 ------ */

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_STA_START) {
        /* Wi-Fi STA 模式启动事件 */
        ESP_LOGI(TAG, "[Wi-Fi.ER] Wi-Fi STA started");

        if (system_paired_flag == USER_WIFI_PAIRED) {
            ESP_LOGI(TAG, "[Wi-Fi.ER] Wi-Fi has been previous configured");
            ESP_ERROR_CHECK(esp_wifi_connect());
        } else {
            xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
        }
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        /* Wi-Fi STA 模式断开连接事件 */
        ESP_LOGI(TAG, "[Wi-Fi.ER] Wi-Fi STA disconnected");

        ESP_ERROR_CHECK(esp_wifi_connect());
    }
}

static void sc_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == SC_EVENT_SCAN_DONE) {
        /* SmartConfig 扫描AP完成事件 */
        ESP_LOGI(TAG, "[SmartConfig.ER] Scan APs done");
    } else if (event_id == SC_EVENT_FOUND_CHANNEL) {
        /* SmartConfig 找到目标AP的信道事件 */
        ESP_LOGI(TAG, "[SmartConfig.ER] Found target AP's channel");
    } else if (event_id == SC_EVENT_GOT_SSID_PSWD) {
        /* SmartConfig 获取到SSID和密码事件 */
        ESP_LOGI(TAG, "[SmartConfig.ER] Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data; // 类型转换事件数据
        wifi_config_t wifi_config; // 创建一个Wi-Fi配置实例

        bzero(&wifi_config, sizeof(wifi_config_t));                            // 清零Wi-Fi配置实例
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid)); // 拷贝SSID到Wi-Fi配置实例
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password)); // 拷贝密码到Wi-Fi配置实例

/* 是否设置目标AP的MAC地址，只有希望验证目标AP的MAC地址时才需要设置 */
#ifdef CONFIG_SET_MAC_ADDRESS_OF_TARGET_AP
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            ESP_LOGI(TAG, "[SmartConfig.E] Set MAC address of target AP: " MACSTR " ", MAC2STR(evt->bssid));
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }
#endif

        /* 打印连接信息 */
        uint8_t ssid[33] = {0};     // 创建一个SSID文本缓冲区，初始化为全0
        uint8_t password[65] = {0}; // 创建一个密码文本缓冲区，初始化为全0
        uint8_t rvd_data[33] = {0}; // 创建一个RVD数据缓冲区，初始化为全0

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "[SmartConfig.E] SSID:%s", ssid);
        ESP_LOGI(TAG, "[SmartConfig.E] PASSWORD:%s", password);

        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
            ESP_LOGI(TAG, "[SmartConfig.E] RVD_DATA:");
            for (int i = 0; i < 33; i++) {
                printf("%02x ", rvd_data[i]);
            }
            printf("\n");
        }

        ESP_ERROR_CHECK(esp_wifi_disconnect());                          // 断开当前Wi-Fi连接
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // 设置Wi-Fi配置
        ESP_ERROR_CHECK(esp_wifi_connect());                             // 连接Wi-Fi
    } else if (event_id == SC_EVENT_SEND_ACK_DONE) {
        /* SmartConfig 发送ACK完成事件 */
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

static void initialise_wifi(void) {
    s_wifi_event_group = xEventGroupCreate(); // 创建FreeRTOS WiFi事件组

    /* [1] Wi-Fi/LwIP 初始化阶段 */
    ESP_ERROR_CHECK(esp_netif_init()); // 创建一个 LwIP(TCP/IP协议栈) 核心任务

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta(); // 创建有 TCP/IP 堆栈的默认网络接口实例绑定STA
    assert(sta_netif);                                            // 断言网络接口实例是否创建成功

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // 加载默认的WiFi初始化配置
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));                // 初始化 Wi-Fi 驱动程序

    /* [2] Wi-Fi 配置阶段 */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // 注册Wi-Fi事件处理程序
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    /* [3] Wi-Fi 启动阶段 */
    ESP_ERROR_CHECK(esp_wifi_start());
}

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
    uint8_t paired;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle));
    err = nvs_get_u8(my_handle, "paired", &paired);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "[System] Error (%s) reading!", esp_err_to_name(err));
        esp_restart();
    }
    if (paired == USER_WIFI_PAIRED) {
        ESP_LOGI(TAG, "[System] Wi-Fi has been previous paired");
        system_paired_flag = USER_WIFI_PAIRED;
    }

    ESP_ERROR_CHECK(esp_event_loop_create_default()); // 创建系统事件任务循环
}

static void smartconfig_task(void *parm) {
    ESP_LOGI(TAG, "[SmartConfig.T] Task started");

    /* 注册SmartConfig事件处理程序 */
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &sc_event_handler, NULL));

    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2));      // 设置SmartConfig类型
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT(); // 加载默认的SmartConfig启动配置
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));                        // 启动SmartConfig

    while (1) {
        EventBits_t uxBits = xEventGroupWaitBits(s_wifi_event_group, ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if (uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "[SmartConfig.T] Done!");

            esp_smartconfig_stop();

            /* 向NVS中写入配对完成标志 */
            nvs_handle_t my_handle;
            ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle));
            ESP_ERROR_CHECK(nvs_set_u8(my_handle, "paired", USER_WIFI_PAIRED));
            ESP_ERROR_CHECK(nvs_commit(my_handle));

            /* 注销SmartConfig事件处理程序 */
            ESP_ERROR_CHECK(esp_event_handler_unregister(SC_EVENT, ESP_EVENT_ANY_ID, &sc_event_handler));
            /* 设置配对成功全局标志 */
            system_paired_flag = USER_WIFI_PAIRED;

            vTaskDelete(NULL);
        }
    }
}

void app_main(void) {
    system_init();     // 初始化系统
    initialise_wifi(); // 初始化Wi-Fi
}
