#include <stdlib.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "app_tasks.h"
#include "bsp_display.h"
#include "bsp_heating.h"
#include "bsp_input.h"
#include "bsp_led_strip.h"

__unused static const char* TAG = "app_tasks";

const int target_temperature_default = 50;
const int target_time_hours_default = 3;
const int target_temperature_min = 40;
const int target_temperature_max = 60;
const int target_time_hours_min = 0;
const int target_time_hours_max = 24;

extern QueueHandle_t bsp_input_queue;     // 输入事件队列
extern led_indicator_handle_t led_handle; // LED指示灯句柄

/* 系统状态 */
static struct {
    app_status_t status;    // 系统状态
    int target_temperature; // 目标温度
    int target_time_hours;  // 目标时间
} app_context = {
    .status = APP_STATUS_SLEEP,
    .target_temperature = target_temperature_default,
    .target_time_hours = target_time_hours_default
};

/**
 * @brief 刷新显示器内容
 */
static void refresh_display(const bool digit_only) {
    switch (app_context.status) {
        case APP_STATUS_TEMP_INTERACT:
            bsp_display_set_int(app_context.target_temperature);
            break;
        case APP_STATUS_TIMER_INTERACT:
            bsp_display_set_int(app_context.target_time_hours);
            break;
        default:
            break;
    }

    if (!digit_only) {
        bsp_display_set_c_flag(app_context.status == APP_STATUS_TEMP_INTERACT);
        bsp_display_set_h_flag(app_context.status == APP_STATUS_TIMER_INTERACT);
    }
}

/**
 * @brief 切换应用状态
 */
static void switch_app_status(const app_status_t status) {
    /* 如果状态未发生变化则直接返回 */
    if (app_context.status == status) { return; }

    /* 更新任务状态并重置输入队列 */
    app_context.status = status;
    xQueueReset(bsp_input_queue);

    refresh_display(false);
}

/**
 * @brief 应用状态级开机
 */
static void app_status_turn_on(void) {
    /* 1. 恢复默认系统状态参数 */
    app_context.target_temperature = target_temperature_default;
    app_context.target_time_hours = target_time_hours_default;

    /* 2. 启动LED指示灯 */
    // led_indicator_start(led_handle, BLINK_ORANGE);

    /* 3. 切换到温度控制交互任务 */
    switch_app_status(APP_STATUS_TEMP_INTERACT);

    /* 4. 恢复显示器 */
    bsp_display_resume();
}

/**
 * @brief 应用状态级关机
 */
static void app_status_turn_off(void) {
    /* 1. 关闭显示器 */
    bsp_display_pause();

    /* 2. 切换到休眠任务 */
    switch_app_status(APP_STATUS_SLEEP);

    /* 3. 关闭LED指示灯 */
    // led_indicator_stop(led_handle, BLINK_ORANGE);
}

/**
 * @brief 温度交互任务输入处理器
 */
static void app_inter_task_temp_input_handler(const bsp_input_event_t event) {
    switch (event) {
        case BSP_KNOB_ENCODER_ACW:
            app_context.target_temperature--;
            break;
        case BSP_KNOB_ENCODER_CW:
            app_context.target_temperature++;
            break;
        default:
            break;
    }

    if (app_context.target_temperature < target_temperature_min) {
        app_context.target_temperature = target_temperature_max;
    }
    if (app_context.target_temperature > target_temperature_max) {
        app_context.target_temperature = target_temperature_min;
    }

    ESP_LOGI(TAG, "Target temperature changed: %d", app_context.target_temperature);

    refresh_display(true);
}

/**
 * 定时交互任务输入处理器
 */
static void app_inter_timer_temp_input_handler(const bsp_input_event_t event) {
    switch (event) {
        case BSP_KNOB_ENCODER_ACW:
            app_context.target_time_hours--;
            break;
        case BSP_KNOB_ENCODER_CW:
            app_context.target_time_hours++;
            break;
        default:
            break;
    }

    if (app_context.target_time_hours < target_time_hours_min) {
        app_context.target_time_hours = target_time_hours_max;
    }
    if (app_context.target_time_hours > target_time_hours_max) {
        app_context.target_time_hours = target_time_hours_min;
    }

    ESP_LOGI(TAG, "Target time changed: %d", app_context.target_time_hours);

    refresh_display(true);
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
            app_context.status == APP_STATUS_SLEEP ? app_status_turn_on() : app_status_turn_off();
            continue;
        }

        /* 拦截处理关机状态下的事件 */
        if (app_context.status == APP_STATUS_SLEEP) {
            if (event == BSP_KNOB_MT8_CLICK) {
                ESP_LOGI(TAG, "System version: %s", esp_get_idf_version());
            }
            continue;
        }

        /* 拦截处理系统级别的输入事件 */
        if (event == BSP_TOUCH_BUTTON_L_CLICK) {
            switch_app_status(APP_STATUS_TEMP_INTERACT);
            continue;
        }
        if (event == BSP_TOUCH_BUTTON_R_CLICK) {
            switch_app_status(APP_STATUS_TIMER_INTERACT);
            continue;
        }

        /* 转发其他输入事件到前台交互任务 */
        switch (app_context.status) {
            case APP_STATUS_TEMP_INTERACT:
                app_inter_task_temp_input_handler(event);
                break;
            case APP_STATUS_TIMER_INTERACT:
                app_inter_timer_temp_input_handler(event);
                break;
            default:
                break;
        }
    }
}

/**
 * @brief [RT任务]加热控制任务
 */
_Noreturn void heating_task(__attribute__((unused)) void* pvParameters) {
    while (1) {
        const int current_temperature = bsp_heating_get_temp();

        ESP_LOGI(TAG, "Current temperature: %d", current_temperature);

        if (app_context.status == APP_STATUS_SLEEP) {
            bsp_heating_disable();
        } else {
            current_temperature < app_context.target_temperature ? bsp_heating_enable() : bsp_heating_disable();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief 初始化系统任务
 */
void app_tasks_init(void) {
    assert(bsp_input_queue != NULL); // 输入事件队列必须先初始化

    xTaskCreate(
        // 创建设备输入处理任务
        input_redirect_task, "InputRedirectTask", 2048, NULL, 10, NULL
    );
    xTaskCreate(
        // 创建加热控制任务
        heating_task, "HeatingTask", 2048, NULL, 10, NULL
    );
}
