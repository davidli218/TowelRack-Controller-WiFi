#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include <string.h>

/* 数码管引脚定义 */
#define DISP_PIN_1 GPIO_NUM_4
#define DISP_PIN_2 GPIO_NUM_5
#define DISP_PIN_3 GPIO_NUM_6
#define DISP_PIN_4 GPIO_NUM_7
#define DISP_PIN_5 GPIO_NUM_18
#define DISP_PIN_6 GPIO_NUM_19

/* 合并两个8位数据为一个HL16数据 */
#define HL16_CMD(high, low) (((high) << 8) | (low))

/* 获取HL16数据的高8位 */
#define HL16_GETHI(data) (((data) >> 8) & 0xFF)

/* 获取HL16数据的低8位 */
#define HL16_GETLO(data) ((data) & 0xFF)

void system_display_init(void);

void system_display_set_string(const char *str);
