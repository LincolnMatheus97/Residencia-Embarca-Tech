// --- Includes ---
#include "pico/stdlib.h"     
#include "pico/cyw43_arch.h" 
#include <stdio.h>           
#include <string.h>          
#include <stdlib.h>          
#include "utils/sensores/sensores.h"
#include "utils/servidor_tcp/servidor_tcp.h"

#define WIFI_SSID "Larissa Lima"         // Nome da rede Wi-Fi
#define WIFI_PASSWORD "larissalima33840" // Senha da rede Wi-Fi

// --- Função Principal (início do programa) ---
int main()
{
    stdio_init_all();           // Inicializa a comunicação serial
    inicializar_sensores();     // Inicializa os sensores (botões e joystick)

    // Inicializa o Wi-Fi e verifica o erro
    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi '%s'...\n", WIFI_SSID);
    // Conecta ao Wi-Fi e verifica o erro
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");
    
    // Verifica o endereço IP do dispositivo
    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Inicializa o servidor e verifica o erro
    err_t server_err = inicializar_servidor_tcp(80);
    if (server_err != ERR_OK)
    {
        printf("main: Falha ao inicializar o servidor TCP (erro %d)\n", server_err);
        cyw43_arch_deinit(); // Limpar wifi
        return -1;           // Terminar se o servidor não iniciar
    }

    printf("Servidor ouvindo na porta 80\n");

    while (true)
    {
        cyw43_arch_poll();
    }

    cyw43_arch_deinit();
    return 0;
}
