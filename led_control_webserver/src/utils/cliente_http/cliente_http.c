#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cliente_http.h"
#include "pico/cyw43_arch.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
#include "sensores/sensores.h"

// Informações do TCP Proxy do Railway
#define PROXY_HOST "maglev.proxy.rlwy.net"
#define PROXY_PORT 48443

// --- Função para processar resposta do servidor ---
static err_t callback_resposta_recebida(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    // ... (código existente sem alterações) ...
    if (!p) {
        printf("Conexão fechada pelo servidor.\n");
        tcp_close(pcb);
        return ERR_OK;
    }

    printf("Resposta do servidor:\n");
    char *dados = (char *)malloc(p->tot_len + 1);
    if (dados) {
        pbuf_copy_partial(p, dados, p->tot_len, 0);
        dados[p->tot_len] = '\0';
        printf("%s\n", dados);
        free(dados);
    }

    pbuf_free(p);
    return ERR_OK;
}

// --- Callback quando a conexão for estabelecida ---
static err_t callback_conectado(void *arg, struct tcp_pcb *pcb, err_t err) {
    if (err != ERR_OK) {
        printf("Erro ao conectar: %d\n", err);
        tcp_abort(pcb);
        return err;
    }

    tcp_recv(pcb, callback_resposta_recebida);

    // Coletar dados
    uint8_t x = ler_joystick_x();
    uint8_t y = ler_joystick_y();
    bool botao_a = botao_a_pressionado();
    bool botao_b = botao_b_pressionado();

    char corpo_json[128];
    snprintf(corpo_json, sizeof(corpo_json),
             "{\"botao_a\": %d, \"botao_b\": %d, \"x\": %d, \"y\": %d}",
             botao_a, botao_b, x, y);

    char requisicao[512];
    // Usar PROXY_HOST no cabeçalho Host
    snprintf(requisicao, sizeof(requisicao),
             "POST /dados HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             PROXY_HOST, strlen(corpo_json), corpo_json);

    cyw43_arch_lwip_begin();
    err_t erro_envio = tcp_write(pcb, requisicao, strlen(requisicao), TCP_WRITE_FLAG_COPY);
    if (erro_envio == ERR_OK) {
        tcp_output(pcb);
        printf("Requisição enviada para %s:%d:\n%s\n", PROXY_HOST, PROXY_PORT, requisicao);
    } else {
        printf("Erro ao enviar dados: %d\n", erro_envio);
        tcp_abort(pcb);
    }
    cyw43_arch_lwip_end();

    return ERR_OK;
}

// --- Callback quando DNS for resolvido ---
static void callback_dns_resolvido(const char *nome_host, const ip_addr_t *ip_resolvido, void *arg) {
    if (!ip_resolvido) {
        printf("Erro: DNS falhou para %s\n", nome_host);
        return;
    }

    printf("DNS resolveu %s para %s\n", nome_host, ipaddr_ntoa(ip_resolvido));

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) {
        printf("Erro ao criar pcb\n");
        return;
    }

    // Conectar à porta do PROXY
    err_t erro = tcp_connect(pcb, ip_resolvido, PROXY_PORT, callback_conectado);
    if (erro != ERR_OK) {
        printf("Erro ao conectar a %s:%d: %d\n", nome_host, PROXY_PORT, erro);
        tcp_abort(pcb);
    }
}

// --- Função principal chamada no loop ---
void enviar_dados_para_nuvem() {
    ip_addr_t endereco_ip;
    // Usar PROXY_HOST para resolução DNS
    err_t resultado_dns = dns_gethostbyname(PROXY_HOST, &endereco_ip, callback_dns_resolvido, NULL);

    if (resultado_dns == ERR_OK) {
        // Se já resolvido (cache), conectar diretamente à porta do PROXY
        printf("Host %s já resolvido para %s\n", PROXY_HOST, ipaddr_ntoa(&endereco_ip));

        struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
        if (!pcb) {
            printf("Erro ao criar pcb (cache)\n");
            return;
        }

        // Conectar à porta do PROXY
        err_t erro = tcp_connect(pcb, &endereco_ip, PROXY_PORT, callback_conectado);
        if (erro != ERR_OK) {
            printf("Erro ao conectar (cache) a %s:%d: %d\n", PROXY_HOST, PROXY_PORT, erro);
            tcp_abort(pcb);
        }
    } else if (resultado_dns == ERR_INPROGRESS) {
        printf("Resolução DNS em andamento para %s...\n", PROXY_HOST);
    } else {
        printf("Erro ao iniciar DNS para %s: %d\n", PROXY_HOST, resultado_dns);
    }
}