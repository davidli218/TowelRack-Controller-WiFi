#pragma once

typedef enum {
    SYS_TASK_IDLE = 0,
    SYS_TASK_MAIN,
    SYS_TASK_TIMER,
} app_task_t;

void app_tasks_init(void);
