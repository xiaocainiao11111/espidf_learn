#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"

#define GPIO_LED_NUM 2
#define GPIO_KEY_NUM 0

/* 定义LED闪烁定时器句柄*/
TimerHandle_t LED_Timer_Handle;
/* 声明定时器回调函数 */
void LED_Timer_Callback(TimerHandle_t xTimer);

/* 定义按键句柄*/
TimerHandle_t Key_Timer_Handle;
/* 声明定时器回调函数 */
void KEY_Timer_Callback(TimerHandle_t xTimer);

/* 定义一个按键值消息队列句柄 */
QueueHandle_t Key_Queue;

/* gpio中断处理函数*/
static void gpio_isr_handler(void *arg)
{
    xTimerResetFromISR(Key_Timer_Handle, NULL); // 复位软件定时器
}

void app_main(void)
{
    BaseType_t ret = 0;
    /* 打印Hello world! */
    printf("Hello world!\n");

    /* 定义一个gpio配置结构体 */
    gpio_config_t gpio_config_structure;

    /* 初始化gpio配置结构体*/
    gpio_config_structure.pin_bit_mask = (1ULL << GPIO_LED_NUM); /* 选择gpio2 */
    gpio_config_structure.mode = GPIO_MODE_OUTPUT;               /* 输出模式 */
    gpio_config_structure.pull_up_en = 0;                        /* 不上拉 */
    gpio_config_structure.pull_down_en = 0;                      /* 不下拉 */
    gpio_config_structure.intr_type = GPIO_INTR_DISABLE;         /* 禁止中断 */

    /* 根据设定参数初始化并使能 */
    gpio_config(&gpio_config_structure);

    /* 初始化gpio配置结构体*/
    gpio_config_structure.pin_bit_mask = (1ULL << GPIO_KEY_NUM); /* 选择gpio0 */
    gpio_config_structure.mode = GPIO_MODE_INPUT;                /* 输入模式 */
    gpio_config_structure.pull_up_en = 0;                        /* 不上拉 */
    gpio_config_structure.pull_down_en = 0;                      /* 不下拉 */
    gpio_config_structure.intr_type = GPIO_INTR_NEGEDGE;         /* 下降沿触发中断 */

    /* 根据设定参数初始化并使能 */
    gpio_config(&gpio_config_structure);

    /* 开启gpio中断服务 */
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1); /* LEVEL1为最低优先级 */
    /* 设置GPIO的中断回调函数 */
    gpio_isr_handler_add(GPIO_KEY_NUM, gpio_isr_handler, (void *)GPIO_KEY_NUM);

    /* 输出高电平，点亮LED*/
    gpio_set_level(GPIO_LED_NUM, 1);

    LED_Timer_Handle = xTimerCreate((const char *)"LED Timer",                    /* 软件定时器名称 */
                                    (TickType_t)(1000 / portTICK_PERIOD_MS),      /* 定时周期，单位为时钟节拍 */
                                    (UBaseType_t)pdTRUE,                          /* 定时器模式，是否为周期定时模式 */
                                    (void *)1,                                    /* 定时器ID号 */
                                    (TimerCallbackFunction_t)LED_Timer_Callback); /*定时器回调函数 */
    if ((LED_Timer_Handle != NULL))
        ret = xTimerStart(LED_Timer_Handle, 0); /* 创建成功，开启定时器*/
    else
        printf("LED Timer Create failure !!! \n"); /* 定时器创建失败 */

    if (ret == pdPASS)
        printf("LED Timer Start OK. \n"); /* 定时器启动成功*/
    else
        printf("LED Timer Start err. \n"); /* 定时器启动失败*/

    /*
    创建按键检测定时器，单次定时50ms后触发回调函数
    单次模式用pdFALSE，周期模式用pdTRUE
    */
    Key_Timer_Handle = xTimerCreate("Key Timer", (50 / portTICK_PERIOD_MS), pdFALSE, (void *)1, KEY_Timer_Callback);

    /* 创建按键检测消息队列 */
    Key_Queue = xQueueCreate((UBaseType_t)10,  /* 队列长度，这里是队列的项目数，即这个队列可以接受多少个消息*/
                             (UBaseType_t)50); /* 队列中每个消息的长度，单位为字节 */

    char msg[50];
    while (1)
    {
        if (xQueueReceive(Key_Queue, msg, portMAX_DELAY))
        {
            printf("in app_main : %s \n", msg);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); /* 延时100ms*/
    }
}

/* 定时器回调函数 */
void LED_Timer_Callback(TimerHandle_t xTimer)
{
    static int led_flag = 0;
    led_flag = !led_flag;                   /* led电平翻转*/
    gpio_set_level(GPIO_LED_NUM, led_flag); /* 根据led_flag设置led电平，实现LED闪烁*/
}

// 按键回调函数
void KEY_Timer_Callback(TimerHandle_t xTimer)
{
    static int key_times = 0;
    char msg[50];
    key_times++;
    printf("BOOT KEY have pressed. \n");
    sprintf(msg, "BOOT KEY have pressed %d times.", key_times); // 把内容存到msg中，但不打印
    xQueueSendFromISR(Key_Queue, msg, NULL);
}
