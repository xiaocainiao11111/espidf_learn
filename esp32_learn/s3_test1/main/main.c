#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "driver/uart.h"

static const char *TAG = "example";

#define LED0(status) gpio_set_level(LED0_GPIO, status)
#define LED0_GPIO GPIO_NUM_10

void gpio_init(void)
{
    gpio_config_t io_conf;

    io_conf.pin_bit_mask = BIT64(LED0_GPIO);      // 设置引脚
    io_conf.mode = GPIO_MODE_OUTPUT;              // 设置模式
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;      // 上拉使能
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // 下拉失能
    io_conf.intr_type = GPIO_INTR_DISABLE;        // 配置中断
    gpio_config(&io_conf);
    // gpio_set_level(LED0_GPIO, 0);//默认让这个引脚输出什么电平
}

void uart_init()
{
    uart_config_t uart_cfg = {0};

    uart_cfg.baud_rate = 115200,
    uart_cfg.data_bits = UART_DATA_8_BITS;         // 8位数据
    uart_cfg.parity = UART_PARITY_DISABLE;         // 无校检
    uart_cfg.stop_bits = UART_STOP_BITS_1;         // 1位停止位
    uart_cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE; // 无硬件流控
    uart_cfg.source_clk = UART_SCLK_DEFAULT;       // 默认时钟源，APB时钟

    /* 初始化串口1 */
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 1, 2, -1, -1));


    // ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, 1024 * 2, 0, 0, NULL, 0));
    // ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_cfg));
    // ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, 21, 22, -1, -1));

}

void app_main(void)
{

    char *tag = "main";
    // esp_log_level_set(tag, ESP_LOG_DEBUG); // 设置日志等级
    gpio_init();
    uart_init();
    const char *str = "Hello, World!";
    while (1)
    {
        LED0(1);
        ESP_LOGI(tag, "GPIO_NUM_10 输出高电平");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        LED0(0);
        ESP_LOGI(tag, "GPIO_NUM_10 输出低电平");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        uart_write_bytes(UART_NUM_1, str, strlen(str));
        uart_wait_tx_done(UART_NUM_1, portMAX_DELAY);
    }
}
