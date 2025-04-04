#include "pico/stdio.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "inc/display_OLED/display_OLED.h"

// Definições do semáforo e do contador compartilhado
SemaphoreHandle_t xMutex;
int contador_compartilhado = 0;

// Função de inicialização do sistema e do display
void setup() {
    stdio_init_all();
    inicializacao_display();
}

// Funções da tarefa 1, que incrementa o contador e exibe a mensagem no display e libera o semáforo depois de usar o contador compartilhado
void vTarefa1 (void *pvParameters) {
    while(1) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            contador_compartilhado++;
            char valor_contador[5];
            sprintf(valor_contador, "%d", contador_compartilhado);
            display_mensagem("Tarefa 1", valor_contador, "Ola Mundo",true);
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(700 / portTICK_PERIOD_MS);
    }
}

// Funções da tarefa 2, que incrementa o contador e exibe a mensagem no display e libera o semáforo depois de usar o contador compartilhado
void vTarefa2 (void *pvParameters) {
    while(1) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            contador_compartilhado++;
            char valor_contador[5];
            sprintf(valor_contador, "%d", contador_compartilhado);
            display_mensagem("Tarefa 2", valor_contador, "EmbarcaTech",true);
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Função principal que inicializa o sistema, cria as tarefas e inicia o escalonador
int main() {
    setup();
    xMutex = xSemaphoreCreateMutex();

    if (xMutex != NULL) {
        xTaskCreate(vTarefa1, "Ola Mundo", 256, NULL, 1, NULL);
        xTaskCreate(vTarefa2, "EmbarcaTech", 256, NULL, 1, NULL);
        vTaskStartScheduler();
    }
    for (;;)
    ;
}