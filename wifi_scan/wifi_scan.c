/*
    * @file wifi_scan.c
    * @brief Exemplo de varredura de redes Wi-Fi usando a Raspberry Pi Pico com o módulo CYW43.
    * 
    * Este código demonstra como usar a biblioteca CYW43 para escanear redes Wi-Fi disponíveis
    * e imprimir informações sobre cada rede encontrada. O LED vermelho na placa pisca durante
    * o processo de varredura.
    *
    * @date 2025-10-01
    * @author Lincoln Matheus - Residência Embarca-Tech 2025
    * @version 1.0
*/

#include <stdio.h>
#include <pico/stdlib.h>
#include "pico/cyw43_arch.h"    // Inclui a biblioteca da Raspberry Pi Pico que fornece funções para gerênciar o modulo de Wi-Fi integrado na placa
#include "hardware/vreg.h"      // Inclui a biblioteca para configurar e controlar o regulador de tensão (Voltage Regulator) da placa
#include "hardware/clocks.h"

// Define o pino 13 como o pino do LED vermelho
const uint led_pin_vermelho = 13;

// Callback para o resultado da varredura de redes Wi-Fi
static int resultado_varredura(void *env, const cyw43_ev_scan_result_t *resultado)
{
    // Verifica se o ponteiro resultado é valido (não é nulo)
    if (resultado)
    {
        // Imprime os detalhes da rede Wi-Fi encontrada no console
        // Formato da impressão: SSID, RSSI, Canal, MAC e Modo de autenticação
        printf("\nSSID: %-32s RSSI: %4d CHAN: %3d MAC: %02x:%02x:%02x:%02x:%02x:%02x: SEC: %u\n",
               resultado->ssid,       // Nome da rede Wi-Fi (SSID)
               resultado->rssi,       // Intensidade do sinal (RSSI)
               resultado->channel,    // Canal utilizado pela rede Wi-Fi
               resultado->bssid[0],   // Endereço MAC primeiro byte
               resultado->bssid[1],   // Endereço MAC segundo byte
               resultado->bssid[2],   // Endereço MAC terceiro byte
               resultado->bssid[3],   // Endereço MAC quarto byte
               resultado->bssid[4],   // Endereço MAC quinto byte
               resultado->bssid[5],   // Endereço MAC sexto byte
               resultado->auth_mode); // Modo de autenticação da rede Wi-Fi (ex: WPA2, WPA3, etc.)
    }
    return 0; // Retorna 0 para continuar a varredura
}

int main()
{
    stdio_init_all();
    sleep_ms(1000);                           // Aguarda 1 segundo para garantir que o sistema esteja pronto
    printf("Wifi Scan Embarca-Tech 2025\n");  // Imprime uma mensagem de inicialização no console
    gpio_init(led_pin_vermelho);              // Inicializa o pino do LED vermelho
    gpio_set_dir(led_pin_vermelho, GPIO_OUT); // Configura o pino do LED vermelho como saída

    if (cyw43_arch_init())
    {                                             // Inicializa o módulo Wi-Fi integrado na placa
        printf("Falha ao inicializar o Wi-Fi\n"); // Imprime uma mensagem de erro se a inicialização falhar
        return 1;                                 // Retorna 1 para indicar falha
    }
    printf("Wi-Fi inicializado com sucesso\n");                // Imprime uma mensagem de sucesso se a inicialização for bem-sucedida
    cyw43_arch_enable_sta_mode();                              // Habilita o modo estação (STA) do Wi-Fi permitindo que a placa se conecte a redes Wi-Fi
    absolute_time_t tempo_varredura = make_timeout_time_ms(0); // Define o tempo limite para a varredura de redes Wi-Fi (0 segundos)
    bool progresso_de_varredura = false;                       // Variável para controlar o progresso da varredura de redes Wi-Fi

    while (true)
    {
        // Verifica se o tempo limite da varredura foi ultrapassou  o tempo definido para iniciar proxima varredura
        if (absolute_time_diff_us(get_absolute_time(), tempo_varredura) < 0)
        {
            // Se nenhuma varredura estiver em progresso, inicia uma nova varredura
            if (!progresso_de_varredura)
            {
                cyw43_wifi_scan_options_t scan_options = {0};
                // Cria uma estrutura para configurar as opções de varredura
                // Inicia a varredura de redes Wi-Fi usando as opções configuradas e define a função de callback para processar os resultados
                int erro = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, resultado_varredura); 
                if (erro == 0)
                {                                                         // Verifica se não houve erro ao iniciar a varredura
                    printf("Iniciando varredura de redes Wi-Fi...\n"); // Imprime uma mensagem indicando que a varredura foi iniciada
                    progresso_de_varredura = true;                        // Define que a varredura está em progresso
                }
                else
                {                                                                   // Se houve erro ao tentar iniciar a varredura
                    printf("Erro ao iniciar varredura de redes Wi-Fi: %d\n", erro); // Imprime uma mensagem de erro com o código de erro retornado
                    progresso_de_varredura = false;                                 // Define que a varredura não está em progresso
                    tempo_varredura = make_timeout_time_ms(10000);                  // Define o tempo limite para a próxima varredura (10 segundos)
                    
                }
            }
            // Verifica se a varredura em andamento foi concluída
            else if (!cyw43_wifi_scan_active(&cyw43_state))
            {
                printf("Varredura concluída\n");                // Imprime uma mensagem indicando que a varredura foi concluída
                tempo_varredura = make_timeout_time_ms(10000); // Define o tempo limite para a próxima varredura (10 segundos)
                progresso_de_varredura = false;                // Define que a varredura não está mais em progresso
            }
            gpio_put(led_pin_vermelho, true);  // Liga o LED vermelho
            sleep_ms(200);                     // Aguarda 200 milissegundos
            gpio_put(led_pin_vermelho, false); // Desliga o LED vermelho
            sleep_ms(200);                     // Aguarda 200 milissegundos
        }
#if PICO_CYW43_ARCH_POLL
        // Se o modo de polling estiver habilitado, executa as seguintes funções
        // Processa eventos pendentes no modulo Wi-Fi
        cyw43_arch_poll();
        // Aguarda o trabalho do Wi-Fi ser concluído
        cyw43_arch_wait_for_work_until(tempo_varredura);
#else 
        sleep_ms(1000); // Aguarda 1000 milissegundos se o modo de polling não estiver habilitado
#endif
    }
    cyw43_arch_deinit(); // Libera os recursos do módulo Wi-Fi integrado na placa
    return 0;
}