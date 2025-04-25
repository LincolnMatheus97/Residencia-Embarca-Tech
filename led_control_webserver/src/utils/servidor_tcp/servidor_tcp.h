#ifndef SERVIDOR_TCP_H
#define SERVIDOR_TCP_H

#include "lwip/tcp.h"  // Para usar TCP
#include "lwip/pbuf.h" // Para gerenciar pacotes de rede (pbuf)
#include <stddef.h>    // Para usar size_t
#include <stdbool.h>   // Para usar bool

#define TCP_SND_BUF_CHUNK_SIZE 512 // Tamanho do chunk

// --- Estrutura de estado ---
typedef struct ESTADO_CONEXAO_TCP  // Estrutura para armazenar informações sobre a conexão TCP
{
    struct tcp_pcb *pcb;
    char *dados_resposta;
    size_t tamanho_total;
    size_t tamanho_enviado;
    bool dados_alocados; // Rastrear memória alocada dinamicamente
} ESTADO_CONEXAO_TCP;

// --- Protótipos das funções ---

err_t nova_conexao_aceita(void *arg_aceite, struct tcp_pcb *pcb_cliente, err_t erro_aceite);
void callback_erro_conexao(void *estado_conexao, err_t codigo_erro);
err_t inicializar_servidor_tcp(uint16_t numero_porta);
err_t dados_recebidos_cliente(void *estado_conexao, struct tcp_pcb *pcb_cliente, struct pbuf *dados_recebidos, err_t erro_recv);
err_t callback_dados_enviados(void *estado_conexao, struct tcp_pcb *pcb_cliente, u16_t bytes_confirmados);
err_t enviar_chunk(ESTADO_CONEXAO_TCP *estado_conexao);
void fechar_conexao_cliente(ESTADO_CONEXAO_TCP *estado_conexao);

#endif