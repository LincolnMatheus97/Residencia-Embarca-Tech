# Projeto de Comunicação I2C com Display SSD1306 na Raspberry Pi Pico W

Este projeto demonstra como estabelecer comunicação I2C entre uma Raspberry Pi Pico W e um display OLED SSD1306 para exibir um contador incremental.

## Hardware Necessário

- Raspberry Pi Pico W
- Display OLED SSD1306 (com interface I2C)
- Fios/Jumpers para conexão

## Conexões (Wiring)

Conecte os pinos do display SSD1306 aos pinos da Raspberry Pi Pico W conforme definido no código:

- **SDA (Dados):** Display SDA -> Pico GP14 (Pino 19)
- **SCL (Clock):** Display SCL -> Pico GP15 (Pino 20)
- **VCC (Alimentação):** Display VCC -> Pico 3V3 (OUT) (Pino 36)
- **GND (Terra):** Display GND -> Pico GND (Qualquer pino GND, ex: Pino 38)

_Observação: Verifique a pinagem específica do seu modelo de display SSD1306._

## Software e Compilação

1.  **Pré-requisitos:**

    - Raspberry Pi Pico W SDK configurado no seu ambiente de desenvolvimento.
    - CMake
    - Compilador ARM GCC (geralmente incluído com o SDK)
    - Biblioteca `ssd1306` (presume-se que esteja corretamente incluída no seu projeto, geralmente como um submódulo ou diretório).

2.  **Estrutura de Arquivos (Exemplo):**

    ```
    conexao_i2c/
    ├── CMakeLists.txt
    ├── pico_sdk_import.cmake
    ├── ssd1306/          # Diretório da biblioteca do display
    │   └── ...
    └── conexao_i2c.c     # Seu arquivo principal
    ```

3.  **Compilação:**

    - Crie um diretório `build`: `mkdir build`
    - Navegue até ele: `cd build`
    - Execute o CMake: `cmake ..`
    - Execute o Make: `make`
    - Isso gerará um arquivo `.uf2` (ex: `conexao_i2c.uf2`) dentro do diretório `build`.

4.  **Execução:**
    - Pressione e segure o botão `BOOTSEL` na Pico enquanto a conecta ao computador via USB.
    - Solte o botão `BOOTSEL`. A Pico aparecerá como um dispositivo de armazenamento USB.
    - Copie o arquivo `.uf2` gerado para dentro desse dispositivo de armazenamento.
    - A Pico reiniciará automaticamente e executará o código. O display deverá mostrar o contador.

## Explicação Detalhada do Código (`conexao_i2c.c`)

```c
// Includes: Importa bibliotecas necessárias
#include <stdio.h>         // Para funções de entrada/saída padrão (printf)
#include "pico/stdlib.h"    // Funções padrão do SDK da Pico (inicialização, GPIO, etc.)
#include "hardware/i2c.h"   // Funções para controle do hardware I2C da Pico
#include "ssd1306/ssd1306.h"// Funções específicas para controlar o display SSD1306

// Defines: Constantes para configuração
#define CANAL_I2C i2c1     // Define qual periférico I2C da Pico usar (i2c0 ou i2c1)
#define SDA_I2C 14         // Define o pino GPIO para a linha de Dados (SDA)
#define SCL_I2C 15         // Define o pino GPIO para a linha de Clock (SCL)

// Variável Global: Armazena o estado do display
ssd1306_t display;         // Estrutura que contém informações sobre o display (tamanho, endereço, buffer, etc.)

// Função: Inicializa a comunicação I2C
void inic_comun_i2c(i2c_inst_t *i2c, uint sda, uint scl, uint baudrate)
{
    // Inicializa o periférico I2C especificado com a taxa de transferência (baudrate)
    i2c_init(i2c, baudrate);
    // Configura os pinos GPIO para funcionarem como pinos I2C
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    // Habilita resistores de pull-up internos nos pinos I2C.
    // I2C requer que as linhas SDA e SCL sejam mantidas em nível alto quando o barramento está livre.
    gpio_pull_up(sda);
    gpio_pull_up(scl);

    // Imprime uma mensagem de confirmação no console serial (se conectado)
    printf("Comunicao I2C estabelecida... SDA = %d, SCL = %d e BOUDRATE = %d\n", sda, scl, baudrate);
}

// Função: Detecta o endereço de um dispositivo I2C no barramento
uint8_t detec_disp_i2c(i2c_inst_t *i2c)
{
    uint8_t end_dispo = 0x00; // Variável para armazenar o endereço encontrado (inicializa com 0)
    // Endereços I2C padrão de 7 bits vão de 0x08 a 0x77.
    // O loop testa cada endereço possível nesta faixa.
    for (uint8_t end = 0x08; end <= 0x77; end++)
    {
        uint8_t dado_lidos; // Buffer temporário para receber dados (não usado aqui, mas necessário pela função)
        // Tenta ler 1 byte do dispositivo no endereço 'end'.
        // 'i2c_read_blocking' tentará se comunicar com o endereço 'end'.
        // Se um dispositivo responder (ACK), a função retornará o número de bytes lidos (0 ou 1 neste caso).
        // Se nenhum dispositivo responder (NACK) ou ocorrer um erro/timeout, retornará um valor negativo.
        // 'false' no final indica para enviar uma condição STOP após a tentativa de leitura.
        int bytes_lidos = i2c_read_blocking(i2c, end, &dado_lidos, 1, false);
        // Verifica se a leitura foi bem-sucedida (retorno >= 0)
        if (bytes_lidos >= 0)
        {
            end_dispo = end; // Armazena o endereço onde um dispositivo respondeu
            break; // Sai do loop, pois encontrou o primeiro dispositivo
        }
        sleep_ms(5); // Pequena pausa para não sobrecarregar o barramento
    }
    // Retorna o endereço encontrado (ou 0x00 se nenhum dispositivo foi detectado)
    return end_dispo;
}

// Função: Inicializa o display SSD1306
void inic_display(ssd1306_t *display, uint16_t largura_disp, uint16_t altura_disp, uint8_t end_disp, i2c_inst_t *i2c)
{
    // Chama a função da biblioteca ssd1306 para configurar o display
    // Passa a estrutura do display, dimensões, endereço I2C e a instância I2C a ser usada.
    ssd1306_init(display, largura_disp, altura_disp, end_disp, i2c);
    // Limpa o buffer interno do display (apaga qualquer lixo inicial)
    ssd1306_clear(display);
    // Nota: ssd1306_show(display) seria necessário aqui para realmente limpar a tela física,
    // mas como a próxima função (mostrar_display) já faz isso, pode ser omitido.
}

// Função: Escreve texto no display
void mostrar_display(ssd1306_t *display, uint32_t larg_texto, uint32_t altr_texto, uint32_t tamanho_font, const char *texto)
{
    // Limpa o buffer do display antes de desenhar o novo texto
    ssd1306_clear(display);
    // Desenha a string de texto no buffer do display nas coordenadas (x, y) e com o tamanho de fonte especificados.
    ssd1306_draw_string(display, larg_texto, altr_texto, tamanho_font, texto);
    // Envia o conteúdo do buffer para a memória do display físico, atualizando a tela.
    ssd1306_show(display);
}

// Função Principal
int main(){
    // Inicializa todas as E/S padrão (necessário para printf e USB serial)
    stdio_init_all();

    // Inicializa a comunicação I2C no canal i2c1, pinos 14 (SDA) e 15 (SCL), com baud rate de 40kHz
    inic_comun_i2c(CANAL_I2C, SDA_I2C, SCL_I2C, 400 * 1000); // 40kHz

    // Tenta detectar o endereço do display no barramento I2C
    uint8_t end_display = detec_disp_i2c(CANAL_I2C);
    // Se um endereço foi encontrado (diferente de 0), imprime-o. Caso contrário, informa que não foi encontrado.
    if (end_display != 0x00) {
        printf("Display encontrado no endereco: 0x%02X\n", end_display);
    } else {
        printf("Nenhum display encontrado.\n");
        // Poderia adicionar um loop infinito aqui ou tratamento de erro,
        // pois o resto do código depende do display.
        while(1) tight_loop_contents();
    }

    // Inicializa o display com as dimensões (128x64), o endereço detectado e o canal I2C
    inic_display(&display, 128, 64, end_display, CANAL_I2C);

    int contador = 0; // Variável para o contador

    // Loop infinito principal
    while (1)
    {
        char texto[20]; // Buffer para armazenar a string formatada
        // Formata a string "Count X" onde X é o valor atual do contador
        snprintf(texto, sizeof(texto), "Count %d", contador);

        // Mostra a string formatada no display na posição (10, 32) com tamanho de fonte 2
        mostrar_display(&display, 10, 32, 2, texto);

        // Incrementa o contador. Se chegar a 20, volta para 0. Senão, incrementa.
        (contador >= 20) ? contador = 0 : contador++;
        // Equivalente a:
        // if (contador >= 20) {
        //     contador = 0;
        // } else {
        //     contador++;
        // }

        // Pausa por 1000 milissegundos (1 segundo)
        sleep_ms(1000);
    }
    // O programa nunca chega aqui em condições normais
    return 0; // Boa prática incluir, embora inalcançável no loop infinito
}
```

## Explicação dos Tipos de Dados (`uint`, `uint8_t`, `uint16_t`, `uint32_t`)

Esses são tipos de dados inteiros sem sinal (unsigned integers) com tamanhos fixos, definidos no cabeçalho `<stdint.h>` (que é incluído indiretamente pelo SDK da Pico).

- **`u`**: Significa "unsigned", ou seja, o tipo só pode armazenar valores não-negativos (0 e positivos).
- **`int`**: Significa "integer" (inteiro).
- **`8`, `16`, `32`**: Indicam o número de bits usados para armazenar o valor.
  - `uint8_t`: Inteiro sem sinal de 8 bits. Faixa de valores: 0 a 255 (2⁸ - 1). Usado para dados que cabem em um único byte, como endereços I2C, caracteres ASCII, ou pequenos contadores. É eficiente em termos de memória e transmissão.
  - `uint16_t`: Inteiro sem sinal de 16 bits. Faixa de valores: 0 a 65.535 (2¹⁶ - 1). Usado para valores maiores, como as dimensões (largura/altura) do display.
  - `uint32_t`: Inteiro sem sinal de 32 bits. Faixa de valores: 0 a 4.294.967.295 (2³² - 1). Usado para valores ainda maiores, como coordenadas de texto, tamanhos de fonte na biblioteca `ssd1306`, ou contadores de tempo em milissegundos.
- **`_t`**: É uma convenção de nomenclatura que indica que se trata de uma definição de tipo (`typedef`).

- **`uint`**: Este tipo, usado em algumas funções do SDK da Pico (como `gpio_set_function`), geralmente é um `typedef` para `unsigned int`. O tamanho exato de `unsigned int` pode variar com a arquitetura, mas na Raspberry Pi Pico W (ARM Cortex-M0+), ele tem **32 bits**, sendo efetivamente o mesmo que `uint32_t`. É usado pelo SDK para parâmetros de funções de propósito geral, como números de pinos GPIO.

**Por que usar tamanhos diferentes?**

1.  **Eficiência de Memória:** Usar o menor tipo possível que acomode a faixa de valores necessária economiza memória RAM, que é limitada em microcontroladores.
2.  **Eficiência de Transmissão:** Em protocolos como I2C, os dados são frequentemente enviados byte a byte (`uint8_t`). Usar tipos maiores exigiria dividir o valor em múltiplos bytes para transmissão.
3.  **Hardware:** Registradores de hardware e protocolos de comunicação muitas vezes operam com larguras de bits específicas (8, 16 ou 32 bits). Usar os tipos correspondentes facilita a interação com o hardware.
4.  **Clareza:** Especificar o tamanho exato (`uint8_t`, `uint16_t`, `uint32_t`) torna o código mais explícito sobre a faixa de valores esperada para uma variável, melhorando a legibilidade e prevenindo erros sutis relacionados a estouro de capacidade (overflow).

No seu código:

- `uint8_t end_dispo`, `uint8_t end`: Endereços I2C são de 7 bits, cabendo perfeitamente em 8 bits.
- `uint16_t largura_disp`, `uint16_t altura_disp`: Dimensões do display (128, 64) cabem em 16 bits.
- `uint32_t larg_texto`, `uint32_t altr_texto`, `uint32_t tamanho_font`: A biblioteca `ssd1306` usa 32 bits para essas coordenadas e tamanho, oferecendo mais flexibilidade.
- `uint sda`, `uint scl`, `uint baudrate`: Parâmetros de funções do SDK que usam o tipo `uint` (32 bits na Pico).

## Explicação da Função `i2c_read_blocking`

`int i2c_read_blocking(i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop)`

- **Propósito:** Ler uma quantidade específica de bytes (`len`) de um dispositivo escravo I2C (`addr`) e armazená-los no buffer apontado por `dst`.
- **`blocking` (Bloqueante):** Significa que a função **não retorna** até que a operação de leitura seja concluída (todos os bytes lidos e recebido ACK/NACK do escravo) ou até que ocorra um erro ou timeout (tempo limite de espera por resposta). Enquanto a função está bloqueada, o restante do seu código não executa. Isso simplifica o fluxo do programa, pois você sabe que, quando a função retorna, a leitura (bem-sucedida ou não) terminou.
- **Parâmetros:**
  - `i2c_inst_t *i2c`: Ponteiro para a instância do hardware I2C a ser usada (ex: `i2c0` ou `i2c1`).
  - `uint8_t addr`: O endereço de 7 bits do dispositivo escravo I2C do qual você quer ler.
  - `uint8_t *dst`: Ponteiro para o array (buffer) onde os bytes lidos serão armazenados. Você precisa garantir que este buffer tenha espaço suficiente (`len` bytes).
  - `size_t len`: O número de bytes que você deseja ler do dispositivo escravo.
  - `bool nostop`:
    - `false`: O mestre (Pico) enviará uma condição de STOP no barramento I2C após a leitura. Isso libera o barramento para outras comunicações. É o comportamento padrão na maioria das leituras simples.
    - `true`: O mestre **não** enviará a condição de STOP, mantendo a conexão ativa. Isso é útil para operações combinadas (ex: escrever um comando e depois ler a resposta sem liberar o barramento entre as ações).
- **Retorno:**
  - **Valor >= 0:** Indica sucesso. O valor retornado é o número de bytes efetivamente lidos (deveria ser igual a `len` se tudo correu bem). No caso da função `detec_disp_i2c`, um retorno `>= 0` (mesmo que 0 bytes sejam lidos, mas a comunicação foi estabelecida) significa que um dispositivo respondeu (ACK) no endereço testado.
  - **Valor < 0:** Indica um erro. Pode ser `PICO_ERROR_GENERIC` para erros gerais ou `PICO_ERROR_TIMEOUT` se o dispositivo escravo não respondeu dentro do tempo esperado. Na função `detec_disp_i2c`, um retorno negativo significa que nenhum dispositivo respondeu no endereço testado.

**Uso em `detec_disp_i2c`:** A função é usada para "sondar" cada endereço. Ela tenta ler 1 byte (`len = 1`). Não importa o _dado_ lido (`dado_lidos`), apenas se a _tentativa_ de comunicação com aquele endereço (`end`) foi bem-sucedida (`bytes_lidos >= 0`). Se foi, significa que um dispositivo existe naquele endereço.

## Explicação sobre o Baud Rate no I2C

- **O que é:** I2C é um protocolo **síncrono**, o que significa que ele usa uma linha de clock (SCL - Serial Clock) para coordenar a transferência de dados na linha de dados (SDA - Serial Data). O **Baud Rate** (mais precisamente, a frequência do clock SCL) define a velocidade com que os bits de dados são transferidos. É medido em bits por segundo (bps) ou Hertz (Hz). Um baud rate de 100.000 Hz (ou 100 kHz) significa que o clock SCL pulsa 100.000 vezes por segundo, permitindo a transferência de até 100.000 bits por segundo (descontando bits de controle como start, stop, ACK/NACK).
- **Por que é necessário:** Tanto o mestre (Pico) quanto o escravo (display SSD1306) precisam operar na **mesma velocidade** (ou o escravo precisa ser capaz de operar na velocidade definida pelo mestre). O mestre controla a linha SCL e dita o ritmo da comunicação. O `baudrate` informado na função `i2c_init` configura o periférico I2C da Pico para gerar o sinal de clock SCL na frequência desejada.
- **Velocidades Comuns:**

  - Standard Mode (SM): 100 kHz (100.000 Hz)
  - Fast Mode (FM): 400 kHz (400.000 Hz)
  - Fast Mode Plus (FM+): 1 MHz (1.000.000 Hz)
  - High Speed Mode (HS): 3.4 MHz (3.400.000 Hz)
  - _Seu código usa `40 _ 1000` = 40 kHz.\* Isso é mais lento que o Standard Mode, mas geralmente funciona e pode ser mais robusto em condições com fios longos ou ruído elétrico. A maioria dos displays SSD1306 suporta pelo menos 100 kHz e frequentemente 400 kHz.

- **O que acontece se for muito baixo (< 100 Hz)?**

  - **Extremamente Lento:** A comunicação seria muito, muito devagar. Atualizar o display levaria muito tempo.
  - **Timeouts:** Alguns dispositivos I2C podem ter mecanismos internos de timeout. Se o clock for lento demais, o dispositivo pode "desistir" de esperar pelo próximo pulso de clock e a comunicação falhar.
  - **Limites do Hardware:** O próprio periférico I2C da Pico pode ter uma frequência mínima de operação definida em seu datasheet. Operar abaixo disso pode não ser possível ou gerar comportamento indefinido. Na prática, velocidades tão baixas raramente são usadas ou úteis.

- **O que acontece se for muito alto (> 400 kHz, por exemplo)?**
  - **Incompatibilidade do Escravo:** O fator limitante mais comum é a velocidade máxima suportada pelo **dispositivo escravo** (o display SSD1306). Se você configurar o mestre (Pico) para uma velocidade maior do que o escravo consegue processar (ex: 1 MHz para um display que só suporta 400 kHz), a comunicação falhará. O escravo pode não reconhecer os sinais, não conseguir enviar o bit de Acknowledge (ACK) a tempo, ou os dados podem ser corrompidos.
  - **Integridade do Sinal:** Em velocidades mais altas, a qualidade do sinal elétrico nas linhas SDA e SCL se torna crítica. Fatores como:
    - **Capacitância do Barramento:** A capacitância total das linhas (devido aos fios, trilhas na placa, pinos dos CIs) limita a rapidez com que os níveis de tensão podem subir e descer. Em alta velocidade, os sinais podem não ter tempo de atingir os níveis lógicos corretos, parecendo mais uma onda senoidal do que quadrada.
    - **Resistores de Pull-up:** O valor dos resistores de pull-up influencia a velocidade de subida do sinal. Valores inadequados podem limitar a velocidade máxima.
    - **Comprimento dos Fios:** Fios mais longos adicionam capacitância e indutância, degradando o sinal em altas frequências.
  - **Resultado:** Mesmo que o escravo _teoricamente_ suporte a velocidade, problemas de integridade do sinal podem causar erros de comunicação (NACKs inesperados, dados corrompidos).

**Em resumo:** Você deve escolher um baud rate que seja suportado por **todos** os dispositivos no barramento (no seu caso, a Pico e o display) e que funcione confiavelmente com a sua fiação e resistores de pull-up. Começar com 100 kHz ou 400 kHz é comum para displays SSD1306. Os 40 kHz usados no seu código são seguros, mas mais lentos que o necessário na maioria dos casos.

## Analogia para a Comunicação I2C

Imagine uma **sala de aula pequena e silenciosa** com um **professor** (o **Mestre**, sua Raspberry Pi Pico W) e vários **alunos** (os **Escravos**, como o seu display SSD1306, e talvez outros sensores no futuro).

1.  **O Barramento (Fios SDA e SCL):** Pense no **ar da sala** como o meio de comunicação compartilhado. Há duas "regras" principais para falar:

    - **Linha de Dados (SDA):** É como a **voz** usada para dizer as palavras (os bits de dados 0 ou 1). Apenas uma pessoa (professor ou o aluno chamado) fala por vez.
    - **Linha de Clock (SCL):** É como o **ritmo** ou a **batida** que o professor define para a conversa. Cada "batida" do clock indica quando todos devem prestar atenção para "ler" o próximo bit de informação que está sendo dito na linha de dados (voz). O professor controla esse ritmo.

2.  **Endereços Únicos:** Cada aluno na sala tem um **nome único** (o **Endereço I2C**). O display tem um nome (por exemplo, "Aluno 0x3C"), um sensor de temperatura poderia ter outro nome ("Aluno 0x48"), etc.

3.  **Iniciando a Conversa (Start Condition):** Quando o professor quer falar com um aluno específico, ele primeiro **chama a atenção** de toda a sala com um sinal especial (a **Condição de Start** no I2C - geralmente baixar o SDA enquanto o SCL está alto).

4.  **Chamando o Aluno (Endereçamento):** Logo após chamar a atenção, o professor **diz o nome** do aluno com quem quer falar (envia o **Endereço I2C** de 7 bits pela linha SDA, sincronizado pelo SCL). Ele também diz se quer **fazer uma pergunta** (Ler do aluno) ou **dar uma instrução** (Escrever para o aluno) - isso é o bit R/W.

5.  **Resposta do Aluno (Acknowledge - ACK):** O aluno que ouviu seu nome **levanta a mão brevemente** para dizer "Presente! Entendi que você quer falar comigo." (o escravo puxa a linha SDA para baixo por um pulso de clock, enviando um **ACK**). Se nenhum aluno reconhecer o nome, ninguém levanta a mão (a linha SDA permanece alta, resultando em um **NACK - Not Acknowledge**), e o professor sabe que o aluno não está lá ou não ouviu. É isso que a função `detec_disp_i2c` faz: chama cada nome (endereço) e vê se alguém levanta a mão (ACK).

6.  **Transferência de Dados:**

    - **Professor dando instruções (Escrita):** O professor começa a ditar a instrução (enviar bytes de dados, como comandos ou dados para exibir no display), um bit de cada vez, no ritmo do clock (SCL). Após cada byte completo (8 bits), o aluno que está ouvindo levanta a mão novamente (ACK) para dizer "Entendi esse pedaço".
    - **Professor fazendo pergunta (Leitura):** O professor indica que quer ouvir (após enviar o endereço com o bit de Leitura). Agora, o **aluno chamado** começa a falar (colocar os bits de dados na linha SDA), no ritmo ditado pelo professor (SCL). Após cada byte que o professor ouve, **ele** (o mestre) "acena com a cabeça" (envia um ACK) para dizer "Ok, recebi, pode continuar". Quando o professor recebeu tudo que queria, ele não acena na última vez (envia um NACK) para indicar ao aluno que a conversa acabou por enquanto.

7.  **Terminando a Conversa (Stop Condition):** Quando a troca de informações termina, o professor dá um sinal final para liberar a sala para outras conversas (a **Condição de Stop** - geralmente subir o SDA enquanto o SCL está alto). Agora o barramento (o ar da sala) está livre novamente.

**Blocking vs Non-Blocking (na Analogia):**

- **`i2c_read_blocking`:** O professor faz uma pergunta ao aluno e **espera em silêncio**, sem fazer mais nada, até que o aluno termine **toda** a sua resposta. Só então o professor continua com a aula.
- **Non-Blocking (Não usado no seu código):** O professor faz a pergunta e imediatamente continua a fazer outra coisa (ex: escrever no quadro). Ele combina com o aluno um jeito de ser avisado (uma interrupção) quando a resposta estiver pronta. Isso é mais complexo de gerenciar, mas permite que o professor (Pico) faça outras tarefas enquanto espera.

Essa analogia ajuda a entender a natureza compartilhada do barramento, a necessidade de endereços, o papel do clock e os conceitos básicos de leitura, escrita, ACK/NACK e start/stop.
