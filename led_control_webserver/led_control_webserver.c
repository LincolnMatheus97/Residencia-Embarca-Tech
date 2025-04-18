#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

// Configurações de Wi-Fi
#define WIFI_SSID "Larissa Lima"
#define WIFI_PASSWORD "larissalima33840"

// Definição do pino do Botão A
#define BUTTON_A_PIN 5 // GPIO 5 - Botão A

// Função de callback para processar requisições HTTP recebidas pelo servidor TCP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    tcp_recved(tpcb, p->tot_len);

    char *request = (char *)malloc(p->len + 1);
    if (request == NULL)
    {
        pbuf_free(p);
        return ERR_MEM;
    }
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    if (strstr(request, "GET /status") != NULL)
    {
        bool pin_state = gpio_get(BUTTON_A_PIN);
        bool button_pressed = !pin_state;

        printf("GPIO %d raw state: %d -> ", BUTTON_A_PIN, pin_state);
        printf("Botao pressionado: %s\n", button_pressed ? "vdd" : "falso");

        char response[128];
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "{\"button_a_pressed\": %s}",
                 button_pressed ? "true" : "false");

        err_t write_err = tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
        if (write_err != ERR_OK)
        {
        }
        else
        {
            err_t output_err = tcp_output(tpcb);
            if (output_err != ERR_OK)
            {
            }
        }
        tcp_close(tpcb);
    }
    else if (strstr(request, "GET / ") != NULL)
    {
        char *html = (char *)malloc(1024);
        if (html == NULL)
        {
            goto free_request;
        }

        snprintf(html, 1024,
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "<!DOCTYPE html>\n"
                 "<html>\n"
                 "<head>\n"
                 "<meta charset=\"UTF-8\">\n"
                 "<title>Button Status</title>\n"
                 "<style>\n"
                 "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; font-size: 48px; }\n"
                 "#buttonStatus { font-weight: bold; color: #555; }\n"
                 "</style>\n"
                 "</head>\n"
                 "<body>\n"
                 "<h1>Status do Botão A</h1>\n"
                 "<p id=\"buttonStatus\">Aguardando...</p>\n"
                 "<script>\n"
                 "const statusElement = document.getElementById('buttonStatus');\n"
                 "setInterval(() => {\n"
                 "  fetch('/status')\n"
                 "    .then(response => response.json())\n"
                 "    .then(data => {\n"
                 "      if (data.button_a_pressed) {\n"
                 "        statusElement.textContent = 'Pressionado!';\n"
                 "        statusElement.style.color = 'red';\n"
                 "      } else {\n"
                 "        statusElement.textContent = 'Solto';\n"
                 "        statusElement.style.color = '#555';\n"
                 "      }\n"
                 "    })\n"
                 "    .catch(error => {\n"
                 "      console.error('Erro ao buscar status:', error);\n"
                 "      statusElement.textContent = 'Erro ao ler';\n"
                 "      statusElement.style.color = 'orange';\n"
                 "    });\n"
                 "}, 1000);\n"
                 "</script>\n"
                 "</body>\n"
                 "</html>\n");

        err_t err_w = tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
        if (err_w != ERR_OK)
        {
            free(html);
            goto free_request;
        }
        else
        {
            err_t err_o = tcp_output(tpcb);
            if (err_o != ERR_OK)
            {
            }
            free(html);
        }
    }

free_request:
    free(request);
    pbuf_free(p);

    return ERR_OK;
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

int main()
{
    stdio_init_all();

    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    printf("Conectado ao Wi-Fi\n");

    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);

    printf("Servidor ouvindo na porta 80\n");

    while (true)
    {
        cyw43_arch_poll();
    }

    cyw43_arch_deinit();
    return 0;
}
