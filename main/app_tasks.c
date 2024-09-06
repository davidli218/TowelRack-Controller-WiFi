#include <math.h>
#include <stdlib.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "app_tasks.h"
#include "bsp/towelrack_controller_a1.h"

__unused static const char* TAG = "app_tasks";

static const int fe_task_hold_time = 10 * 1000;   // 前台任务保持时间
static const int target_temperature_default = 50; // 默认开机目标温度
static const int target_time_hours_default = 3;   // 默认开机目标时间
static const int target_temperature_min = 40;     // 目标温度范围_下限
static const int target_temperature_max = 60;     // 目标温度范围_上限
static const int target_time_hours_min = 0;       // 目标时间范围_下限
static const int target_time_hours_max = 24;      // 目标时间范围_上限

static QueueHandle_t bsp_input_queue = NULL;              // 输入事件队列
static TaskHandle_t app_fe_status_watchdog_handle = NULL; // 前台状态看门狗任务句柄

typedef enum {
    APP_FE_STATUS_IDLE,
    APP_FE_STATUS_TEMP_INTERACT,
    APP_FE_STATUS_TIMER_INTERACT,
} app_frontend_status_t;

/* 系统状态变量 */
static struct {
    bool be_status_on;                    // 后台状态
    app_frontend_status_t fe_status;      // 前台状态
    bsp_led_strip_mode_t idle_strip_mode; // 空闲状态灯带模式
    int target_temperature;               // 目标温度
    int target_time_hours;                // 目标时间
    bool target_time_dirty;               // 目标时间是否被修改过
} app_context = {
    .be_status_on = false,
    .fe_status = APP_FE_STATUS_IDLE,
    .idle_strip_mode = BSP_STRIP_OFF,
    .target_temperature = 0,
    .target_time_hours = 0,
    .target_time_dirty = true,
};

/**
 * @brief 根据系统状态刷新显示内容
 */
static void app_refresh_display(void) {
    bsp_display_set_c_flag(
        app_context.fe_status == APP_FE_STATUS_TEMP_INTERACT ||
        (app_context.be_status_on == true && app_context.fe_status == APP_FE_STATUS_IDLE)
    );
    bsp_display_set_h_flag(app_context.fe_status == APP_FE_STATUS_TIMER_INTERACT);

    switch (app_context.fe_status) {
        case APP_FE_STATUS_TEMP_INTERACT:
            bsp_display_write_int(app_context.target_temperature);
            break;
        case APP_FE_STATUS_TIMER_INTERACT:
            bsp_display_write_int(app_context.target_time_hours);
            break;
        case APP_FE_STATUS_IDLE:
            app_context.be_status_on
                ? bsp_display_write_int(app_context.target_temperature)
                : bsp_display_write_str(NULL);
            break;
        default:
            ESP_LOGE(TAG, "Invalid app frontend status to display: %d", app_context.fe_status);
    }
}

/**
 * @brief 切换应用前台状态
 */
static void app_fe_switch_status(const app_frontend_status_t status) {
    /* 喂狗 */
    xTaskNotifyGive(app_fe_status_watchdog_handle);

    /* 更新任务状态并重置输入队列 */
    app_context.fe_status = status;
    xQueueReset(bsp_input_queue);

    /* 更新灯带状态 */
    switch (app_context.fe_status) {
        case APP_FE_STATUS_IDLE:
            bsp_led_strip_write(app_context.idle_strip_mode);
            break;
        case APP_FE_STATUS_TEMP_INTERACT:
            bsp_led_strip_write(BSP_STRIP_RED);
            break;
        case APP_FE_STATUS_TIMER_INTERACT:
            bsp_led_strip_write(BSP_STRIP_BLUE);
            break;
        default:
            break;
    }

    /* 强制更新显示内容 */
    app_refresh_display();
}

/**
 * @brief 切换应用后台状态
 */
static void app_be_toggle_status(void) {
    /* 切换后台状态 */
    app_context.be_status_on = !app_context.be_status_on;

    /* 恢复应用目标参数 */
    if (app_context.be_status_on) {
        app_context.idle_strip_mode = BSP_STRIP_ORANGE;
        app_context.target_temperature = target_temperature_default;
        app_context.target_time_hours = target_time_hours_default;
    } else {
        app_context.idle_strip_mode = BSP_STRIP_OFF;
        app_context.target_temperature = 0;
        app_context.target_time_hours = 0;
    }
    app_context.target_time_dirty = true;

    /* 更新灯带状态 */
    bsp_led_strip_write(app_context.idle_strip_mode);

    /* 更新前台状态, 开关机后默认进入空闲状态 */
    app_fe_switch_status(APP_FE_STATUS_IDLE);
}

/**
 * @brief 温度交互事件处理
 *
 * @param event 设备输入事件
 */
static void temp_inter_handler(const bsp_input_event_t event) {
    switch (event) {
        case BSP_KNOB_ENCODER_ACW:
            app_context.target_temperature--;
            break;
        case BSP_KNOB_ENCODER_CW:
            app_context.target_temperature++;
            break;
        default:
            return;
    }

    xTaskNotifyGive(app_fe_status_watchdog_handle);

    if (app_context.target_temperature < target_temperature_min) {
        app_context.target_temperature = target_temperature_max;
    }
    if (app_context.target_temperature > target_temperature_max) {
        app_context.target_temperature = target_temperature_min;
    }

    ESP_LOGI(TAG, "Target temperature changed: %d", app_context.target_temperature);
}

/**
 * @brief 定时交互事件处理
 *
 * @param event 设备输入事件
 */
static void timer_inter_handler(const bsp_input_event_t event) {
    switch (event) {
        case BSP_KNOB_ENCODER_ACW:
            app_context.target_time_hours--;
            break;
        case BSP_KNOB_ENCODER_CW:
            app_context.target_time_hours++;
            break;
        default:
            return;
    }

    app_context.target_time_dirty = true;
    xTaskNotifyGive(app_fe_status_watchdog_handle);

    if (app_context.target_time_hours < target_time_hours_min) {
        app_context.target_time_hours = target_time_hours_max;
    }
    if (app_context.target_time_hours > target_time_hours_max) {
        app_context.target_time_hours = target_time_hours_min;
    }

    ESP_LOGI(TAG, "Target time changed: %d", app_context.target_time_hours);
}

/**
 * @brief 处理系统开启状态下的用户输入事件
 *
 * @param event 设备输入事件
 */
static void app_be_active_status_input_handler(const bsp_input_event_t event) {
    /* 处理按钮切换前台任务事件 */
    if (event == BSP_TOUCH_BUTTON_L_CLICK || event == BSP_TOUCH_BUTTON_R_CLICK) {
        app_fe_switch_status(
            event == BSP_TOUCH_BUTTON_L_CLICK ? APP_FE_STATUS_TEMP_INTERACT : APP_FE_STATUS_TIMER_INTERACT
        );
        return;
    }

    /* 其余输入事件根据前台任务状态分发 */
    switch (app_context.fe_status) {
        case APP_FE_STATUS_TEMP_INTERACT:
            temp_inter_handler(event);
            break;
        case APP_FE_STATUS_TIMER_INTERACT:
            timer_inter_handler(event);
            break;
        default:
            break;
    }

    app_refresh_display();
}

/**
 * @brief 处理系统休眠状态下的用户输入事件
 *
 * @param event 设备输入事件
 */
static void app_be_sleep_status_input_handler(const bsp_input_event_t event) {
    /* 处理显示系统信息事件 */
    if (app_context.fe_status == APP_FE_STATUS_IDLE && event == BSP_KNOB_MT8_CLICK) {
        ESP_LOGI(TAG, "System version: %s", esp_get_idf_version());
        return;
    }

    /* 处理按钮切换前台任务事件, 休眠状态下只允许定时任务 */
    if (event == BSP_TOUCH_BUTTON_R_CLICK) {
        app_fe_switch_status(APP_FE_STATUS_TIMER_INTERACT);
        return;
    }

    /* 其余输入事件根据前台任务状态分发 */
    switch (app_context.fe_status) {
        case APP_FE_STATUS_TIMER_INTERACT:
            timer_inter_handler(event);
            break;
        default:
            break;
    }

    app_refresh_display();
}

/**
 * @brief [RT任务]用户输入处理
 *
 * 该任务负责从输入队列中获取用户输入事件，并根据当前应用状态进行处理。
 */
_Noreturn static void input_redirect_task(__attribute__((unused)) void* pvParameters) {
    bsp_input_event_t event;

    while (1) {
        if (xQueueReceive(bsp_input_queue, &event, portMAX_DELAY) != pdTRUE) { continue; }

        ESP_LOGI(TAG, "[InputRedirectTask] Received input event: %d:%s", event, bsp_input_event_to_string(event));

        /* 拦截处理长按按钮事件，实现系统开关 */
        if (event == BSP_KNOB_LONG_PRESS) {
            app_be_toggle_status();
            continue;
        }

        /* 根据当前后端状态分发事件处理 */
        app_context.be_status_on
            ? app_be_active_status_input_handler(event)
            : app_be_sleep_status_input_handler(event);
    }
}

/**
 * @brief [RT任务]前台状态看门狗
 *
 * 监视前台任务，如果一段时间前台任务没有被控制，就把app_context.fe_status设为APP_FE_STATUS_IDLE
 */
_Noreturn static void fe_status_watchdog(__attribute__((unused)) void* pvParameters) {
    while (true) {
        const uint32_t notify_value = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(fe_task_hold_time));

        if (notify_value == 0 && app_context.fe_status != APP_FE_STATUS_IDLE) {
            app_fe_switch_status(APP_FE_STATUS_IDLE);
        }
    }
}

/**
 * @brief [RT任务]加热控制任务
 */
_Noreturn void heating_task(__attribute__((unused)) void* pvParameters) {
    /* 升温模式: [0]干柴烈火, [1]贤者模式 */
    uint8_t heating_status = 0;
    while (1) {
        if (!app_context.be_status_on) {
            bsp_heating_disable();
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        const int current_temperature = bsp_heating_get_temp();
        ESP_LOGI(TAG, "Current temperature: %d", current_temperature);

        switch (heating_status) {
            case 0:
                bsp_heating_enable();
                if (current_temperature >= app_context.target_temperature) {
                    app_context.idle_strip_mode = BSP_STRIP_GREEN;
                    if (app_context.fe_status == APP_FE_STATUS_IDLE) {
                        bsp_led_strip_write(app_context.idle_strip_mode);
                    }
                    heating_status = 1;
                }
                break;
            case 1:
                bsp_heating_disable();
                if (current_temperature < app_context.target_temperature - 5) {
                    app_context.idle_strip_mode = BSP_STRIP_ORANGE;
                    if (app_context.fe_status == APP_FE_STATUS_IDLE) {
                        bsp_led_strip_write(app_context.idle_strip_mode);
                    }
                    heating_status = 0;
                }
                break;
            default:
                ESP_LOGE(TAG, "Invalid heating status: %d", heating_status);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief [RT任务]定时开关机任务
 */
_Noreturn void power_on_off_task(__attribute__((unused)) void* pvParameters) {
    int rest_3sec_counter = 0; // 3秒计数器
    while (1) {
        if (app_context.target_time_hours == 0) {
            vTaskDelay(pdMS_TO_TICKS(3000)); // 3秒检查一次
            continue;
        }

        if (app_context.target_time_dirty) {
            rest_3sec_counter = app_context.target_time_hours * 20 * 60; // 20次/分钟
            app_context.target_time_dirty = false;
        }

        vTaskDelay(pdMS_TO_TICKS(3000)); // 3秒检查一次
        rest_3sec_counter--;

        if (app_context.target_time_dirty) { continue; }

        if (rest_3sec_counter <= 0) {
            app_be_toggle_status();
            continue;
        }

        app_context.target_time_hours = ceil((float)rest_3sec_counter / 20 / 60);
        if (app_context.fe_status == APP_FE_STATUS_TIMER_INTERACT) { app_refresh_display(); }
    }
}

/**
 * @brief 初始化系统任务
 */
void app_tasks_init(void) {
    bsp_input_queue = bsp_input_get_queue();
    assert(bsp_input_queue != NULL); // 输入事件队列必须存在

    xTaskCreate(
        // 创建前台状态看门狗任务
        fe_status_watchdog, "FeStatusWatchdog", 2048, NULL, 10, &app_fe_status_watchdog_handle
    );
    xTaskCreate(
        // 创建设备输入处理任务
        input_redirect_task, "InputRedirectTask", 2048, NULL, 10, NULL
    );
    xTaskCreate(
        // 创建加热控制任务
        heating_task, "HeatingTask", 2048, NULL, 10, NULL
    );
    xTaskCreate(
        // 创建定时开关机任务
        power_on_off_task, "PowerOnOffTask", 2048, NULL, 10, NULL
    );
}
