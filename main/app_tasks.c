#include <stdlib.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

#include "app_tasks.h"
#include "bsp_display.h"
#include "bsp_heating.h"
#include "bsp_input.h"
#include "bsp_led_strip.h"

__unused static const char *TAG = "app_tasks";

#define TARGET_TEMPERATURE_DEFAULT (50)
#define TARGET_TIME_HOURS_DEFAULT (3)
#define TARGET_TEMPERATURE_MIN (40)
#define TARGET_TEMPERATURE_MAX (60)
#define TARGET_TIME_HOURS_MIN (0)
#define TARGET_TIME_HOURS_MAX (24)

extern QueueHandle_t bsp_input_queue;     // 输入事件队列
extern led_indicator_handle_t led_handle; // LED指示灯句柄

static app_task_t app_task = APP_TASK_SLEEP;                  // 前台任务
static TaskHandle_t app_task_temp_inter_handle = NULL;        // 温度交互任务.句柄
static TaskHandle_t app_task_timer_inter_handle = NULL;       // 定时交互任务.句柄
static QueueHandle_t app_task_temp_inter_input_queue = NULL;  // 温度交互任务.输入队列
static QueueHandle_t app_task_timer_inter_input_queue = NULL; // 定时交互任务.输入队列

/* 系统参数 */
static int target_temperature = TARGET_TEMPERATURE_DEFAULT;
static int target_time_hours = TARGET_TIME_HOURS_DEFAULT;

/**
 * @brief 切换系统前台交互任务
 */
static void sys_task_switch(app_task_t task) {
    if (app_task == task) return;

    app_task = task;
    xQueueReset(bsp_input_queue);

    switch (task) {
        case APP_TASK_TEMP_INTERACT: xTaskNotifyGive(app_task_temp_inter_handle); break;
        case APP_TASK_TIMER_INTERACT: xTaskNotifyGive(app_task_timer_inter_handle); break;
        default: break;
    }
}

/**
 * @brief 关闭系统任务
 */
static void sys_status_turn_off(void) {
    bsp_display_pause();
    sys_task_switch(APP_TASK_SLEEP);

    /* 关闭所有LED指示灯 */
    led_indicator_stop(led_handle, BLINK_ORANGE);
}

/**
 * @brief 复位系统任务
 */
static void sys_status_turn_on(void) {
    target_temperature = TARGET_TEMPERATURE_DEFAULT;
    target_time_hours = TARGET_TIME_HOURS_DEFAULT;

    bsp_display_resume();
    sys_task_switch(APP_TASK_TEMP_INTERACT);

    /* 启动LED指示灯 */
    led_indicator_start(led_handle, BLINK_ORANGE);
}

/**
 * @brief [RT任务]用户输入重定向
 *
 * 该任务负责接收用户输入事件。
 * 拦截并处理高级别事件，转发低级别事件到前台运行任务。
 */
static void input_redirect_task(void *pvParameters) {
    bsp_input_event_t event;
    while (1) {
        if (xQueueReceive(bsp_input_queue, &event, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "[InputRedirectTask] Received event: %s", bsp_input_event_to_string(event));

            /* 拦截长按按钮事件，实现系统开关 */
            if (event == BSP_KNOB_LONG_PRESS) {
                (app_task == APP_TASK_SLEEP) ? sys_status_turn_on() : sys_status_turn_off();
                continue;
            }

            /* 拦截关机状态下的特殊事件 */
            if (app_task == APP_TASK_SLEEP) {
                if (event == BSP_KNOB_MT8_CLICK) {
                    ESP_LOGI(TAG, "System version: %s", esp_get_idf_version());
                }
                continue;
            }

            /* 拦截处理系统级别的输入事件 */
            if (event == BSP_TOUCH_BUTTON_L_CLICK) {
                sys_task_switch(APP_TASK_TEMP_INTERACT);
                continue;
            } else if (event == BSP_TOUCH_BUTTON_R_CLICK) {
                sys_task_switch(APP_TASK_TIMER_INTERACT);
                continue;
            }

            /* 转发其他输入事件到前台任务 */
            switch (app_task) {
                case APP_TASK_TEMP_INTERACT: xQueueSend(app_task_temp_inter_input_queue, &event, 0); break;
                case APP_TASK_TIMER_INTERACT: xQueueSend(app_task_timer_inter_input_queue, &event, 0); break;
                default: break;
            }
        }
    }
}

/**
 * @brief [RT任务]温度交互任务
 *
 * 负责控制目标温度的调整。
 */
static void temp_inter_task(void *pvParameters) {
    bsp_input_event_t event;
    while (1) {
        /* 等待上位前台通知, 上位后立刻更新数码管显示 */
        if (app_task != APP_TASK_TEMP_INTERACT) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            ESP_LOGI(TAG, "[TempInterTask] Switched to foreground");
            bsp_display_set_int(target_temperature);
            bsp_display_set_h_flag(false);
            bsp_display_set_c_flag(true);
        }

        /* 非阻塞接收输入事件 */
        if (xQueueReceive(app_task_temp_inter_input_queue, &event, 0) == pdTRUE) {
            switch (event) {
                case BSP_KNOB_ENCODER_ACW: target_temperature--; break;
                case BSP_KNOB_ENCODER_CW: target_temperature++; break;
                default: break;
            }
            if (target_temperature < TARGET_TEMPERATURE_MIN) target_temperature = TARGET_TEMPERATURE_MIN;
            if (target_temperature > TARGET_TEMPERATURE_MAX) target_temperature = TARGET_TEMPERATURE_MAX;

            ESP_LOGI(TAG, "Target temperature changed: %d", target_temperature);
            bsp_display_set_int(target_temperature);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief [RT任务]定时交互任务
 *
 * 负责控制目标时间的调整。
 */
static void timer_inter_task(void *pvParameters) {
    bsp_input_event_t event;
    while (1) {
        /* 等待上位前台通知, 上位后立刻更新数码管显示 */
        if (app_task != APP_TASK_TIMER_INTERACT) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            ESP_LOGI(TAG, "[TimerInterTask] Switched to foreground");
            bsp_display_set_int(target_time_hours);
            bsp_display_set_h_flag(true);
            bsp_display_set_c_flag(false);
        }

        /* 非阻塞接收输入事件 */
        if (xQueueReceive(app_task_timer_inter_input_queue, &event, 0) == pdTRUE) {
            switch (event) {
                case BSP_KNOB_ENCODER_ACW: target_time_hours -= 1; break;
                case BSP_KNOB_ENCODER_CW: target_time_hours += 1; break;
                default: break;
            }
            if (target_time_hours < TARGET_TIME_HOURS_MIN) target_time_hours = TARGET_TIME_HOURS_MAX;
            if (target_time_hours > TARGET_TIME_HOURS_MAX) target_time_hours = TARGET_TIME_HOURS_MIN;

            ESP_LOGI(TAG, "Target time changed: %d hours", target_time_hours);
            bsp_display_set_int(target_time_hours);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief 加热控制任务
 */
void app_task_heating(void *pvParameters) {
    while (1) {
        int current_temperature = bsp_heating_get_temp();
        ESP_LOGI(TAG, "Current temperature: %d", current_temperature);

        if (app_task == APP_TASK_SLEEP) {
            bsp_heating_disable();
        } else {
            (current_temperature < target_temperature) ? bsp_heating_enable() : bsp_heating_disable();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief 初始化系统任务
 */
void app_tasks_init(void) {
    assert(bsp_input_queue != NULL); // 输入事件队列必须先初始化
    xQueueReset(bsp_input_queue);    // 重置输入事件队列

    app_task_temp_inter_input_queue = xQueueCreate(10, sizeof(bsp_input_event_t));  // 创建主要任务输入队列
    app_task_timer_inter_input_queue = xQueueCreate(10, sizeof(bsp_input_event_t)); // 创建定时任务输入队列

    xTaskCreate(temp_inter_task, "TempInterTask", 2048, NULL, 10, &app_task_temp_inter_handle); // 创建温度交互任务
    xTaskCreate(timer_inter_task, "TimerInterTask", 2048, NULL, 10, &app_task_timer_inter_handle); // 创建定时交互任务
    xTaskCreate(input_redirect_task, "InputRedirectTask", 2048, NULL, 10, NULL); // 创建输入重定向任务
    xTaskCreate(app_task_heating, "HeatingTask", 2048, NULL, 10, NULL);          // 创建加热控制任务

    sys_status_turn_on(); // 启动系统
}
