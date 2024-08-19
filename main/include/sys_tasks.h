#ifndef TOWERRACK_CONTROLLER_SYS_TASKS_H
#define TOWERRACK_CONTROLLER_SYS_TASKS_H

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "app_display.h"
#include "app_input.h"

#include <stdlib.h>

typedef enum {
    SYS_TASK_IDLE = 0,
    SYS_TASK_MAIN,
    SYS_TASK_TIMER,
} sys_task_t;

void system_tasks_init(void);

#endif // TOWERRACK_CONTROLLER_SYS_TASKS_H
