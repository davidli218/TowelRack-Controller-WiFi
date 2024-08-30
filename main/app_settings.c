#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "app_settings.h"

static const char* TAG = "app_settings";

#define NAME_SPACE "sys_param"
#define KEY "param"
#define MAGIC_HEAD 0xEE

static sys_param_t g_sys_param = {0};

static const sys_param_t g_default_sys_param = {
    .magic = MAGIC_HEAD,
    .dev_adopted = false,
};

/**
 * @brief 检查系统参数是否合法，不合法时设置为默认值
 */
static esp_err_t settings_check(sys_param_t* param) {
    esp_err_t ret = ESP_OK;
    ESP_GOTO_ON_FALSE(param->magic == MAGIC_HEAD, ESP_ERR_INVALID_ARG, reset, TAG, "magic incorrect");
    return ret;
reset:
    ESP_LOGW(TAG, "Set to default");
    memcpy(&g_sys_param, &g_default_sys_param, sizeof(sys_param_t));
    return ret;
}

/**
 * @brief 从NVS中读取系统参数
 */
esp_err_t settings_read_parameter_from_nvs(void) {
    ESP_LOGI(TAG, "Loading settings");

    nvs_handle_t my_handle = 0;
    esp_err_t ret = nvs_open(NAME_SPACE, NVS_READONLY, &my_handle);

    /* 第一次启动，没有找到参键值，设置为默认值 */
    if (ESP_ERR_NVS_NOT_FOUND == ret) {
        ESP_LOGW(TAG, "Not found, Set to default");
        memcpy(&g_sys_param, &g_default_sys_param, sizeof(sys_param_t));
        settings_write_parameter_to_nvs();
        return ESP_OK;
    }

    /* NVS打开失败 */
    ESP_GOTO_ON_FALSE(ESP_OK == ret, ret, err, TAG, "NVS open failed (0x%x)", ret);

    /* 读取系统参数 */
    size_t len = sizeof(sys_param_t);
    ret = nvs_get_blob(my_handle, KEY, &g_sys_param, &len);
    ESP_GOTO_ON_FALSE(ESP_OK == ret, ret, err, TAG, "Can't read param");

    nvs_close(my_handle);

    settings_check(&g_sys_param);
    return ret;
err:
    if (my_handle) { nvs_close(my_handle); }
    return ret;
}

/**
 * @brief 将系统参数写入NVS
 */
esp_err_t settings_write_parameter_to_nvs(void) {
    ESP_LOGI(TAG, "Saving settings");

    settings_check(&g_sys_param);

    nvs_handle_t my_handle = {0};
    esp_err_t err = nvs_open(NAME_SPACE, NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        err = nvs_set_blob(my_handle, KEY, &g_sys_param, sizeof(sys_param_t));
        err |= nvs_commit(my_handle);
        nvs_close(my_handle);
    }

    return ESP_OK == err ? ESP_OK : ESP_FAIL;
}

/**
 * @brief 获取 dev_adopted 位
 */
bool settings_get_dev_adopted(void) { return g_sys_param.dev_adopted; }

/**
 * @brief 设置 dev_adopted 位为 true
 */
void settings_set_dev_adopted(void) { g_sys_param.dev_adopted = true; }
