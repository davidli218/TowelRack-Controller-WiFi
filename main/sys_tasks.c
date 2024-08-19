#include "sys_tasks.h"

__unused static const char *TAG = "TRC-W::SysTasks";

extern QueueHandle_t sys_input_queue; // 输入事件队列

static bool sys_status_on = false; // 系统任务是否启动

static sys_task_t sys_task = SYS_TASK_IDLE;          // 前台任务
static TaskHandle_t sys_main_task_handle = NULL;     // 主要任务句柄
static TaskHandle_t sys_timer_task_handle = NULL;    // 定时任务句柄
static QueueHandle_t sys_main_task_in_queue = NULL;  // 主要任务输入队列
static QueueHandle_t sys_timer_task_in_queue = NULL; // 定时任务输入队列

/* 系统目标参数 */
static int target_temperature = 50;
static int target_time_minutes = 360;

/**
 * @brief 擦除所有数据并重启
 */
static void sys_erase_all(void) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    esp_restart();
}

/**
 * @brief 切换系统前台任务
 */
static void sys_task_switch(sys_task_t task) {
    sys_task = task;
    xQueueReset(sys_input_queue);
    switch (task) {
        case SYS_TASK_MAIN: xTaskNotifyGive(sys_main_task_handle); break;
        case SYS_TASK_TIMER: xTaskNotifyGive(sys_timer_task_handle); break;
        default: break;
    }
}

/**
 * @brief 关闭系统任务
 */
static void sys_status_turn_off(void) {
    system_display_pause();
    sys_task_switch(SYS_TASK_IDLE);
    sys_status_on = false;
}

/**
 * @brief 复位系统任务
 */
static void sys_status_turn_on(void) {
    target_temperature = 50;
    target_time_minutes = 360;

    system_display_resume();
    sys_task_switch(SYS_TASK_MAIN);
    sys_status_on = true;
}

/**
 * @brief [RT任务]用户输入重定向
 *
 * 该任务负责接收用户输入事件。
 * 拦截并处理高级别事件，转发低级别事件到前台运行任务。
 */
static void input_redirect_task(void *pvParameters) {
    sys_input_event_t event;
    while (1) {
        if (xQueueReceive(sys_input_queue, &event, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "[InputRedirectTask] Received event: %d", event);

            /* 拦截长按按钮事件，实现系统开关 */
            if (event == BUTTON_EVENT_LONG_PRESS) {
                if (sys_status_on) sys_status_turn_off();
                else sys_status_turn_on();
                continue;
            }

            /* 如果系统任务未启动，则忽略其他输入事件 */
            if (!sys_status_on) continue;

            /* 拦截处理系统级别的输入事件 */
            if (event == BUTTON_EVENT_SINGLE_CLICK) { // 长按按钮: 切换任务
                if (sys_task == SYS_TASK_MAIN) sys_task_switch(SYS_TASK_TIMER);
                else sys_task_switch(SYS_TASK_MAIN);
                continue;
            } else if (event == BUTTON_EVENT_PRESS_REPEAT) { // 重复按下按钮: 擦除所有数据
                sys_erase_all();
                continue;
            }

            /* 转发其他输入事件到前台任务 */
            switch (sys_task) {
                case SYS_TASK_MAIN: xQueueSend(sys_main_task_in_queue, &event, 0); break;
                case SYS_TASK_TIMER: xQueueSend(sys_timer_task_in_queue, &event, 0); break;
                default: break;
            }
        }
    }
}

/**
 * @brief [RT任务]主要任务
 *
 * 负责控制目标温度的调整。
 */
static void main_task(void *pvParameters) {
    sys_input_event_t event;
    while (1) {
        /* 等待上位前台通知, 上位后立刻更新数码管显示 */
        if (sys_task != SYS_TASK_MAIN) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            system_display_set_int(target_temperature);
        }

        /* 非阻塞接收输入事件 */
        if (xQueueReceive(sys_main_task_in_queue, &event, 0) == pdTRUE) {
            switch (event) {
                case KNOB_EVENT_LEFT: target_temperature--; break;
                case KNOB_EVENT_RIGHT: target_temperature++; break;
                default: break;
            }
            if (target_temperature < 30) target_temperature = 30;
            if (target_temperature > 60) target_temperature = 60;

            ESP_LOGI("TRC-W::Main", "Target temperature changed: %d", target_temperature);
            system_display_set_int(target_temperature);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief [RT任务]定时任务
 *
 * 负责控制目标时间的调整。
 */
static void timer_task(void *pvParameters) {
    sys_input_event_t event;
    while (1) {
        /* 等待上位前台通知, 上位后立刻更新数码管显示 */
        if (sys_task != SYS_TASK_TIMER) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            system_display_set_int(target_time_minutes);
        }

        /* 非阻塞接收输入事件 */
        if (xQueueReceive(sys_timer_task_in_queue, &event, 0) == pdTRUE) {
            switch (event) {
                case KNOB_EVENT_LEFT: target_time_minutes -= 10; break;
                case KNOB_EVENT_RIGHT: target_time_minutes += 10; break;
                default: break;
            }
            if (target_time_minutes < 0) target_time_minutes = 0;
            if (target_time_minutes > 960) target_time_minutes = 960;

            ESP_LOGI("TRC-W::Timer", "Target time changed: %d minutes", target_time_minutes);
            system_display_set_int(target_time_minutes);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief 初始化系统任务
 */
void system_tasks_init(void) {
    assert(sys_input_queue != NULL); // 输入事件队列必须先初始化
    xQueueReset(sys_input_queue);    // 重置输入事件队列

    sys_main_task_in_queue = xQueueCreate(10, sizeof(sys_input_event_t));  // 创建主要任务输入队列
    sys_timer_task_in_queue = xQueueCreate(10, sizeof(sys_input_event_t)); // 创建定时任务输入队列

    xTaskCreate(main_task, "MainTask", 2048, NULL, 10, &sys_main_task_handle);    // 创建主要任务
    xTaskCreate(timer_task, "TimerTask", 2048, NULL, 10, &sys_timer_task_handle); // 创建定时任务
    xTaskCreate(input_redirect_task, "InputRedirectTask", 2048, NULL, 10, NULL);  // 创建输入重定向任务
}
