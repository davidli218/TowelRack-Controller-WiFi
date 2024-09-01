#pragma once

typedef enum {
    APP_STATUS_SLEEP = 0,
    APP_STATUS_TEMP_INTERACT,
    APP_STATUS_TIMER_INTERACT,
} app_status_t;

void app_tasks_init(void);
