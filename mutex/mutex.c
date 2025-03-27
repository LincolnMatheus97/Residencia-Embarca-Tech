#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "semphr.h"

SemaphoreHandle_t xMutex;
int contador_compartilhado = 0;

void vTarefa1(void *pvParameters)
{
    while (1)
    {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
        {
            contador_compartilhado++;
            printf("Tarefa 1: %d\n", contador_compartilhado);
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void vTarefa2(void *pvParameters)
{
    while (1)
    {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
        {
            contador_compartilhado++;
            printf("Tarefa 2: %d\n", contador_compartilhado);
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(700 / portTICK_PERIOD_MS);
    }
}

int main()
{
    stdio_init_all();
    xMutex = xSemaphoreCreateMutex();
    if (xMutex != NULL)
    {
        xTaskCreate(vTarefa1, "Tarefa 1", 256, NULL, 1, NULL);
        xTaskCreate(vTarefa2, "Tarefa 2", 256, NULL, 1, NULL);
        vTaskStartScheduler();
    }

    while (1){};
    return 0;
    
}