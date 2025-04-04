#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

SemaphoreHandle_t semaforo_LED;

const uint pin_led_vermelho = 13;
const uint pin_led_azul = 12;

void setup() {
    stdio_init_all();
    gpio_init(pin_led_vermelho);
    gpio_set_dir(pin_led_vermelho, GPIO_OUT);
    gpio_init(pin_led_azul);
    gpio_set_dir(pin_led_azul, GPIO_OUT);
}

void pisca_led_vermelho(void *pvParametros) {
    uint *led = (uint *)pvParametros;
    while(1) {
        if(xSemaphoreTake(semaforo_LED, portMAX_DELAY == pdTRUE)) {
            gpio_put(*led, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_put(*led, 0);
            xSemaphoreGive(semaforo_LED);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void pisca_led_azul(void *pvParametros) {
    uint *led = (uint *)pvParametros;
    while(1) {
        if(xSemaphoreTake(semaforo_LED, portMAX_DELAY == pdTRUE)) {
            gpio_put(*led, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_put(*led, 0);
            xSemaphoreGive(semaforo_LED);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

int main() {
    setup();
    semaforo_LED = xSemaphoreCreateBinary();
    xSemaphoreGive(semaforo_LED);
    if (semaforo_LED != NULL) {
        xTaskCreate(pisca_led_vermelho, "LED Vermelho", 128, (void *)&pin_led_vermelho, 1, NULL);
        xTaskCreate(pisca_led_azul, "LED azul", 128, (void *)&pin_led_azul, 1, NULL);
        vTaskStartScheduler();
    }
    for(;;)
    ;
}