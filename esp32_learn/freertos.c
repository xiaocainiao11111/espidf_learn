/*
  程序： FREERTOS - 单个参数传递
        大家在看本程序的时候，需要对指针非常的了解
        知道 * -> &的作用
  作业： 添加LED3_PIN 15
  公众号：孤独的二进制
*/
byte LED1_PIN = 23;
byte LED2_PIN = 21;

void task1(void *pt)
{
    byte *pbLEDPIN;
    pbLEDPIN = (byte *)pt;

    byte LEDPIN;
    LEDPIN = *pbLEDPIN;
    pinMode(LEDPIN, OUTPUT);
    while (1)
    {
        digitalWrite(LEDPIN, !digitalRead(LEDPIN));
        vTaskDelay(1000);
    }
}

void task2(void *pt)
{
    byte LEDPIN = *(byte *)pt;
    pinMode(LEDPIN, OUTPUT);
    while (1)
    {
        digitalWrite(LEDPIN, !digitalRead(LEDPIN));
        vTaskDelay(3000);
    }
}

void setup()
{
    Serial.begin(9600);

    byte *pbLED1PIN;
    pbLED1PIN = &LED1_PIN;

    void *pvLED1PIN;
    pvLED1PIN = (void *)pbLED1PIN;

    if (xTaskCreate(task1,
                    "Blink 23",
                    1024,
                    pvLED1PIN,
                    1,
                    NULL) == pdPASS)
        Serial.println("Task1 Created.");

    if (xTaskCreate(task2,
                    "Blink 21",
                    1024,
                    (void *)&LED2_PIN,
                    1,
                    NULL) == pdPASS)
        Serial.println("Task2 Created.");
}

void loop()
{
}

/*
  程序： FREERTOS - 结构体多参数传递
  作业： 添加LED3
  公众号：孤独的二进制
*/

typedef struct
{
    byte pin;
    int delayTime;
} LEDFLASH;

/*
void ledFlash(void *pt) {
  LEDFLASH * ptLedFlash = (LEDFLASH *)pt;
  LEDFLASH led = *ptLedFlash;

  pinMode(led.pin,OUTPUT);
  while (1) {
    digitalWrite(led.pin, !digitalRead(led.pin));
    vTaskDelay(led.delayTime);
  }
}
*/

void ledFlash(void *pt)
{
    LEDFLASH *ptLedFlash = (LEDFLASH *)pt;
    byte pin = ptLedFlash->pin;
    int delayTime = ptLedFlash->delayTime;

    pinMode(pin, OUTPUT);
    while (1)
    {
        digitalWrite(pin, !digitalRead(pin));
        vTaskDelay(delayTime);
    }
}

LEDFLASH led1, led2;

void setup()
{
    Serial.begin(9600);

    led1.pin = 23;
    led1.delayTime = 1000;
    led2.pin = 21;
    led2.delayTime = 3000;

    if (xTaskCreate(ledFlash,
                    "FLASH LED",
                    1024,
                    (void *)&led1,
                    1,
                    NULL) == pdPASS)
        Serial.println("led1 flash task Created.");

    if (xTaskCreate(ledFlash,
                    "FLASH LED",
                    1024,
                    (void *)&led2,
                    1,
                    NULL) == pdPASS)
        Serial.println("led2 flash task Created.");
}

void loop()
{
}

/*
  程序： Tasks之间数据传递
        有多任务同时写入，或者数据大小超过cpu内存通道时，或者对共享资源的访问时候，需要有防范机制
        使用MUTEX对数据对Cirtical Section的内容进行保护
        可以想象成MUTEX就是一把锁

  公众号：孤独的二进制

  语法：
  SemaphoreHandle_t xHandler; 创建Handler
  xHandler = xSemaphoreCreateMutex(); 创建一个MUTEX 返回NULL，或者handler
  xSemaphoreGive(xHandler); 释放
  xSemaphoreTake(xHanlder, timeout); 指定时间内获取信号量 返回pdPASS, 或者pdFAIL

  理解方法：
  MUTEX的工作原理可以想象成
  共享的资源被锁在了一个箱子里，只有一把钥匙，有钥匙的任务才能对改资源进行访问
*/

// 养成良好习惯，被多进程和中断调用的变量使用 volatile 修饰符
volatile uint32_t inventory = 100; // 总库存
volatile uint32_t retailCount = 0; // 线下销售量
volatile uint32_t onlineCount = 0; // 线上销售量

SemaphoreHandle_t xMutexInventory = NULL; // 创建信号量Handler

TickType_t timeOut = 1000; // 用于获取信号量的Timeout 1000 ticks

void retailTask(void *pvParam)
{
    while (1)
    {

        // 在timeout的时间内如果能够获取就继续
        // 通俗一些：获取钥匙
        if (xSemaphoreTake(xMutexInventory, timeOut) == pdPASS)
        {
            // 被MUTEX保护的内容叫做 Critical Section

            // 以下实现了带有随机延迟的 inventory减1；
            // 等效为 inventory--； retailCount++；
            uint32_t inv = inventory;
            for (int i; i < random(10, 100); i++)
                vTaskDelay(pdMS_TO_TICKS(i));
            if (inventory > 0)
            {
                inventory = inv - 1;
                retailCount++;

                // 释放钥匙
                xSemaphoreGive(xMutexInventory);
            }
            else
            {
                // 无法获取钥匙
            }
        };

        vTaskDelay(100); // 老板要求慢一些，客户升级后，可以再加快速度
    }
}

void onlineTask(void *pvParam)
{
    while (1)
    {

        // 在timeout的时间内如果能够获取二进制信号量就继续
        // 通俗一些：获取钥匙
        if (xSemaphoreTake(xMutexInventory, timeOut) == pdPASS)
        {
            // 被MUTEX保护的内容叫做 Critical Section
            // 以下实现了带有随机延迟的 inventory减1；
            // 等效为 inventory--； retailCount++；
            uint32_t inv = inventory;
            for (int i; i < random(10, 100); i++)
                vTaskDelay(pdMS_TO_TICKS(i));
            if (inventory > 0)
            {
                inventory = inv - 1;
                onlineCount++;

                // 释放钥匙
                xSemaphoreGive(xMutexInventory);
            }
            else
            {
                // 无法获取钥匙
            }
        };

        vTaskDelay(100); // 老板要求慢一些，客户升级后，可以再加快速度
    }
}

void showTask(void *pvParam)
{
    while (1)
    {

        printf("Inventory : %d\n", inventory);
        printf("  Retail : %d, Online : %d\n", retailCount, onlineCount);

        if (inventory == 0)
        {
            uint32_t totalSales = retailCount + onlineCount;
            printf("-----SALES SUMMARY-----\n");
            printf("  Total Sales:  %d\n", totalSales);
            printf("  OverSales:  %d\n", 100 - totalSales);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);

    xMutexInventory = xSemaphoreCreateMutex(); // 创建MUTEX

    if (xMutexInventory == NULL)
    {
        printf("No Enough Ram, Unable to Create Semaphore.");
    }
    else
    {
        xTaskCreate(onlineTask,
                    "Online Channel",
                    1024 * 4,
                    NULL,
                    1,
                    NULL);
        xTaskCreate(retailTask,
                    "Retail Channel",
                    1024 * 4,
                    NULL,
                    1,
                    NULL);
        xTaskCreate(showTask,
                    "Display Inventory",
                    1024 * 4,
                    NULL,
                    1,
                    NULL);
    }
}

void loop()
{
}

/*
   程序：  ESP32 OLED 使用 U8G2库，setup也是任务，跑完了可以删掉
          使用FREERTOS
   公众号：孤独的二进制
*/
#include <U8g2lib.h>
#include <Wire.h>

void oledTask(void *pvParam)
{
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
    u8g2.begin();
    for (;;)
    {
        u8g2.clearBuffer();                    // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);    // choose a suitable font
        u8g2.drawStr(15, 10, "LONELY BINARY"); // write something to the internal memory
        u8g2.sendBuffer();                     // transfer internal memory to the display
        vTaskDelay(1000);
    }
}

void setup()
{ // loopBack , Priority 1, Core 1
    xTaskCreatePinnedToCore(oledTask, "OLED Task", 1024 * 6, NULL, 1, NULL, 1);

    vTaskDelete(NULL);
}

void loop()
{
}

/*
    类时间片功能
  程序： 绝对任务延迟
  公众号：孤独的二进制
  API：
    vTaskDelayUntil(&xLastWakeTime, xFrequency)
      最后一次的唤醒时间是指针类型。
      本函数会自动更新xLastWakeTime为最后一次唤醒的时间
      所以无需手动在while循环内对其手动赋值
    xTaskGetTickCount()
      Tick Coun 和 Arduino Millis一样
      uint32_t类型 49天后overflow
*/

void showStockTask(void *ptParam)
{
    static float stockPrice = 99.57; // 股票价格

    // 最后一次唤醒的tick count，第一次使用需要赋值
    // 以后此变量会由vTaskDelayUntil自动更新
    TickType_t xLastWakeTime = xTaskGetTickCount();

    const TickType_t xFrequency = 3000; // 间隔 3000 ticks = 3 seconds

    for (;;)
    {
        // 恰恰算算，经过思考，既然我们叫做LastWakeTime，那么 vTaskDelayUntil 应该放在循环的第一句话
        // 如果放在循环的最后一句话，应该改为xLastSleepTime 才更加合适
        //  看懂的朋友， 请鼓掌
        //  哦，我无法听到掌声，干脆帮我按住 点赞三秒 对我的视频进行强力推荐吧
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // 验证当前唤醒的时刻tick count
        Serial.println(xTaskGetTickCount());
        // 验证xLastWake Time是否被vTaskDelayUntil更新
        // Serial.println(xLastWakeTime);

        // ------- 很复杂的交易股票计算，时间不定 ---------
        stockPrice = stockPrice * (1 + random(1, 20) / 100.0);
        vTaskDelay(random(500, 2000));

        Serial.print("Stock Price : $");
        Serial.println(stockPrice);

        // 使用vTaskDelay试试看会如何
        // vTaskDelay(xFrequency);
    }
}

void setup()
{
    Serial.begin(115200);
    xTaskCreate(showStockTask, "Show Stock Price", 1024 * 6, NULL, 1, NULL);
}

void loop()
{
}

/*
    软件定时器
  程序： Software Timer
  公众号：孤独的二进制
  API：
    xTimerCreate //创建时间
    xTimerStart //时间开始
    到时间后，会运行callback函数
*/

TimerHandle_t lockHandle, checkHandle;

void carKey(void *ptParam)
{
    byte lockPin = 23;
    pinMode(lockPin, INPUT_PULLUP);

    for (;;)
    {
        if (digitalRead(lockPin) == LOW)
        {
            // timeout 3000 ticks
            // xTimerStart 只是开启时间而已，而不是创造时间对象
            // 所以如果多次按按钮的话，不会有多个时间对象生成
            // 多次按按钮相当于每次对timer进行reset xTimerReset()
            if (xTimerStart(lockHandle, 3000) == pdPASS)
            {
                Serial.println("About to lock the car");
            }
            else
            {
                Serial.println("Unable to lock the car");
            };
            vTaskDelay(100); // very rude Button Debounce
        }
    }
}

void lockCarCallback(TimerHandle_t xTimer)
{
    Serial.println("Timer CallBack: Car is Locked");
}

void checkCallback(TimerHandle_t xTimer)
{
    // ------- 很复杂的检测汽车Sensors的方法，时间不定 ---------
    Serial.print(xTaskGetTickCount());
    Serial.println("  -  All Sensors are working.");
    vTaskDelay(random(10, 90));
}

void setup()
{
    Serial.begin(115200);
    xTaskCreate(carKey,
                "Check If Owner Press Lock Button",
                1024 * 1,
                NULL,
                1,
                NULL);

    lockHandle = xTimerCreate("Lock Car",
                              2000,
                              pdFALSE,
                              (void *)0,
                              lockCarCallback);

    checkHandle = xTimerCreate("Sensors Check",
                               100,
                               pdTRUE,
                               (void *)1,
                               checkCallback);

    // 必须要在 portMAX_DELAY 内开启 timer start
    // portMAX_DELAY is listed as value for waiting indefinitely
    // 实际上0xFFFFFFFF 2^32-1  49天 7周
    // 在此期间，此task进入Block状态
    xTimerStart(checkHandle, portMAX_DELAY);
}

void loop()
{
}

/*
  程序： 任务管理，创建，暂停，删除
  公众号：孤独的二进制
  API：
    BaseType_t xTaskCreate(,,,,,); //任务创建
    void vTaskDelete( TaskHandle_t xTask ); //任务删除
    void vTaskSuspend( TaskHandle_t xTaskToSuspend ); //任务暂停
    void vTaskResume( TaskHandle_t xTaskToResume ); //任务恢复
*/
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

TaskHandle_t biliHandle = NULL; // Task Handler

void control(void *ptParam)
{ // 按钮控制

    pinMode(32, INPUT_PULLUP);
    pinMode(33, INPUT_PULLUP);
    pinMode(25, INPUT_PULLUP);
    pinMode(26, INPUT_PULLUP);

    while (1)
    {
        // 创建任务
        if (digitalRead(32) == LOW)
        {
            // 判断是否之前已经创建了Bilibili channel task， 如果没有创建，则创建该Task
            if (biliHandle == NULL)
            {
                if (xTaskCreate(radioBilibili, "Bilibili Channel", 1024 * 8, NULL, 1, &biliHandle) == pdPASS)
                {
                    Serial.print(xTaskGetTickCount());
                    Serial.println(" - LOG: Task is Created.");
                }
                else
                {
                    Serial.print(xTaskGetTickCount());
                    Serial.println(" - WARNING: Unable to Create Task.");
                }
            }
            else
            {
                Serial.print(xTaskGetTickCount());
                Serial.println(" - WARNING: Task **WAS** Created.");
            }
            vTaskDelay(120); // 粗暴的Button Debounce
        }

        // 任务删除
        if (digitalRead(33) == LOW)
        {
            // 注意在删除任务前，一定要确保任务是存在的
            // 删除不存在的任务，比如连续删除两次，自动重启
            if (biliHandle != NULL)
            {
                vTaskDelete(biliHandle);
                lcdClear();        // 清空LCD
                biliHandle = NULL; // 手动将handler设置为空
            }

            if (biliHandle != NULL)
            {
                Serial.print(xTaskGetTickCount());
                Serial.println(" - WARNING: Unable to Delete Task.");
            }
            else
            {
                Serial.print(xTaskGetTickCount());
                Serial.println(" - LOG: Task is Deleted.");
            }
            vTaskDelay(120); // 粗暴的Button Debounce
        }

        // 任务暂停
        if (digitalRead(25) == LOW)
        {
            if (biliHandle != NULL)
            {
                vTaskSuspend(biliHandle);
                Serial.print(xTaskGetTickCount());
                Serial.println(" - LOG: Task is suspended.");
            }
            else
            {
                Serial.print(xTaskGetTickCount());
                Serial.println(" - WARNING: Unable to Suspend Task.");
            }
            vTaskDelay(120); // 粗暴的Button Debounce
        }

        // 任务恢复
        if (digitalRead(26) == LOW)
        {
            if (biliHandle != NULL)
            {
                vTaskResume(biliHandle);
                Serial.print(xTaskGetTickCount());
                Serial.println(" - LOG: Task is resumed.");
            }
            else
            {
                Serial.print(xTaskGetTickCount());
                Serial.println(" - WARNING: Unable to Resume Task.");
            }
            vTaskDelay(120); // 粗暴的Button Debounce
        }
    }
}

void radioBilibili(void *ptParam)
{ // 任务主体

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print(" Bilibili   Channel ");
    lcd.setCursor(0, 1);
    lcd.print("-FreeRTOS on EPS32- ");
    lcd.setCursor(0, 2);
    lcd.print("Study Hard  &  Smart");

    while (1)
    {
        lcd.setCursor(9, 3);
        lcd.print(xTaskGetTickCount() / 100);
        vTaskDelay(100);
    }
}

void lcdClear()
{ // 清空LCD
    lcd.clear();
    lcd.noBacklight();
    lcd.setCursor(0, 0);
    lcd.print("                                                                                                    ");
}

void setup()
{
    Serial.begin(115200);
    xTaskCreate(control, "control panel", 1024 * 8, NULL, 1, NULL);
}

void loop() {}