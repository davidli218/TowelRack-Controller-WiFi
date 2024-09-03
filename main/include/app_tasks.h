#pragma once

typedef enum {
    APP_FE_STATUS_IDLE,
    APP_FE_STATUS_TEMP_INTERACT,
    APP_FE_STATUS_TIMER_INTERACT,
} app_frontend_status_t;

void app_tasks_init(void);
