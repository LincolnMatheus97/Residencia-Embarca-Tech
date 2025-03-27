#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

const uint pino_led_vermelho = 13;
const uint pino_led_azul = 12;

void setup() 
{
    stdio_init_all();
    gpio_init(pino_led_vermelho);
    gpio_set_dir(pino_led_vermelho, GPIO_OUT);
    gpio_init(pino_led_azul);
    gpio_set_dir(pino_led_azul, GPIO_OUT);
}

void pisca_pisca(void *parametro_tarefa)
{   
    uint *led = (uint *)parametro_tarefa;
    for(;;)
    {
        gpio_put(*led, 1);

        vTaskDelay(250 / portTICK_PERIOD_MS);

        gpio_put(*led, 0);

        vTaskDelay(250 / portTICK_PERIOD_MS);

    }
}

int main()
{
    setup();

    xTaskCreate(pisca_pisca, "tarefa_pisca", 128, (void *)&pino_led_vermelho, 1, NULL);
    xTaskCreate(pisca_pisca, "tarefa_pisca_2", 128, (void *)&pino_led_azul, 1, NULL);

    vTaskStartScheduler();
    for (;;);
}