#pragma once

typedef enum {
    APP_TASK_SLEEP = 0,
    APP_TASK_TEMP_INTERACT,
    APP_TASK_TIMER_INTERACT,
} app_task_t;

void app_tasks_init(void);
