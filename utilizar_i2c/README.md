# Projeto de Comunicação I2C com Display SSD1306 na Raspberry Pi Pico W

Este projeto demonstra como utilizar a comunicação I2C (Inter-Integrated Circuit) em uma Raspberry Pi Pico W para interagir com um display OLED SSD1306. O código inicializa o barramento I2C, escaneia por dispositivos conectados, inicializa o display encontrado e exibe uma contagem incremental.

## O que é a Conexão I2C?

I2C (Inter-Integrated Circuit), pronunciado "I-quadrado-C" ou "I-dois-C", é um barramento de comunicação serial síncrono, multi-mestre, multi-escravo, desenvolvido para comunicação de curta distância entre circuitos integrados em uma mesma placa de circuito impresso (PCB). Ele utiliza apenas duas linhas de sinal bidirecionais:

1.  **SDA (Serial Data Line):** Linha para transferência de dados.
2.  **SCL (Serial Clock Line):** Linha para o sinal de clock que sincroniza a transferência de dados.

Ambas as linhas são do tipo "open-drain" (ou "open-collector"), o que significa que precisam de resistores de pull-up conectados à tensão de alimentação positiva (VCC) para funcionarem corretamente.

### História da Criação do I2C

- **Quando:** Início dos anos 80 (especificamente em 1982).
- **Por Quem:** Philips Semiconductors (agora NXP Semiconductors).
- **Por Quê:** A Philips precisava de um protocolo simples e eficiente para conectar vários CIs (como microcontroladores, memórias EEPROM, sensores, conversores A/D e D/A, drivers de display, etc.) dentro de seus produtos eletrônicos (como televisores) usando um número mínimo de pinos e fiação. Protocolos existentes como o SPI exigiam mais pinos, e a comunicação paralela era ainda mais complexa em termos de fiação. O I2C foi projetado para simplificar o design de hardware e reduzir custos.

### Comparativo com Outras Conexões Comuns

| Característica      | I2C                                                                                                 | SPI (Serial Peripheral Interface)        | UART (Universal Asynchronous Receiver/Transmitter) |
| :------------------ | :-------------------------------------------------------------------------------------------------- | :--------------------------------------- | :------------------------------------------------- |
| **Número de Fios**  | 2 (SDA, SCL) + GND + VCC                                                                            | 4 (MOSI, MISO, SCLK, CS) + GND + VCC     | 2 (RX, TX) + GND (+ VCC opcional)                  |
| **Velocidade**      | Padrão (100 kbps), Rápido (400 kbps), Rápido+ (1 Mbps), Alta Vel. (3.4 Mbps), Ultra Rápido (5 Mbps) | Geralmente mais rápido (dezenas de Mbps) | Variável, geralmente mais lento (até ~1 Mbps)      |
| **Sincronia**       | Síncrono (usa clock)                                                                                | Síncrono (usa clock)                     | Assíncrono (sem clock compartilhado)               |
| **Modo**            | Multi-Mestre, Multi-Escravo                                                                         | Mestre Único, Multi-Escravo              | Ponto-a-Ponto (Full-Duplex)                        |
| **Endereçamento**   | Endereço de 7 ou 10 bits por escravo                                                                | Seleção de Escravo (Chip Select - CS)    | Não aplicável (conexão direta)                     |
| **Complexidade HW** | Baixa                                                                                               | Moderada                                 | Baixa                                              |
| **Complexidade SW** | Moderada (protocolo com ACK/NACK)                                                                   | Baixa                                    | Baixa                                              |
| **Distância**       | Curta (mesma PCB ou poucos metros)                                                                  | Curta (mesma PCB ou poucos metros)       | Variável (pode ser longa com transceivers)         |

### Vantagens do I2C

- **Poucos Pinos:** Requer apenas duas linhas de sinal (SDA e SCL), economizando pinos do microcontrolador.
- **Multi-Escravo:** Permite conectar múltiplos dispositivos escravos ao mesmo barramento usando endereços únicos.
- **Multi-Mestre:** Suporta múltiplos dispositivos mestres no mesmo barramento (embora mais complexo de implementar).
- **Confirmação (ACK/NACK):** O protocolo inclui bits de confirmação (Acknowledge/Not Acknowledge) para garantir que os dados foram recebidos corretamente.
- **Padronizado:** Amplamente adotado pela indústria, com muitos sensores e periféricos disponíveis.

### Desvantagens do I2C

- **Velocidade Limitada:** Geralmente mais lento que o SPI.
- **Capacitância do Barramento:** O número de dispositivos e o comprimento do barramento são limitados pela capacitância total das linhas SDA e SCL.
- **Complexidade do Protocolo:** O protocolo de software é um pouco mais complexo que o SPI devido ao endereçamento e aos bits ACK/NACK.
- **Resistores de Pull-up:** Requer resistores externos de pull-up.

## O Que é Necessário para Funcionar?

1.  **Microcontrolador com Suporte I2C:** A Raspberry Pi Pico W possui hardware I2C dedicado.
2.  **Dispositivo(s) Escravo(s) I2C:** Neste caso, um display OLED SSD1306. Cada dispositivo deve ter um endereço I2C único no barramento.
3.  **Conexões Físicas:**
    - Conectar o pino SDA do microcontrolador ao pino SDA do display.
    - Conectar o pino SCL do microcontrolador ao pino SCL do display.
    - Conectar os pinos GND de ambos os dispositivos.
    - Alimentar o display (VCC).
4.  **Resistores de Pull-up:** Conectar um resistor entre SDA e VCC, e outro entre SCL e VCC. O valor típico varia (ex: 2.2kΩ a 10kΩ), dependendo da velocidade e da capacitância do barramento. _Nota: Muitos módulos de display já incluem esses resistores._
5.  **Software/Firmware:** Código no microcontrolador para inicializar o barramento I2C, configurar a comunicação e interagir com o dispositivo escravo (ler/escrever dados).

## Explicação do Código (`utilizar_i2c.c`)

O código implementa a comunicação I2C para controlar um display SSD1306.

1.  **Includes:**

    - `stdio.h`: Para funções de entrada/saída padrão (como `printf`).
    - `pico/stdlib.h`: Funções padrão do SDK da Pico (inicialização, `sleep_ms`).
    - `hardware/i2c.h`: Funções para controle do hardware I2C da Pico.
    - `ssd1306/ssd1306.h`: Biblioteca para controle do display OLED SSD1306.

2.  **Defines:**

    - `I2C_PORT`: Define qual instância de hardware I2C usar (`i2c1`). A Pico tem `i2c0` e `i2c1`.
    - `I2C_SDA`, `I2C_SCL`: Define os pinos GPIO a serem usados para SDA (GPIO 14) e SCL (GPIO 15). _Verifique o datasheet da Pico para ver quais pinos podem ser usados para `i2c1`._

3.  **Variável Global:**

    - `ssd1306_t display;`: Declara uma estrutura para armazenar o estado e a configuração do display SSD1306.

4.  **`i2c_bus_init()`:**

    - Inicializa a instância I2C especificada (`i2c`) com a taxa de transferência (`baudrate`) desejada (400kHz no `main`).
    - Configura os pinos GPIO (`sda_pin`, `scl_pin`) para a função I2C.
    - Habilita os resistores de pull-up internos nos pinos SDA e SCL (útil se não houver resistores externos, mas pull-ups externos são geralmente recomendados para melhor desempenho).
    - Imprime uma mensagem indicando que o I2C foi inicializado.

5.  **`i2c_bus_scan()`:**

    - Função para detectar dispositivos I2C conectados ao barramento.
    - Itera sobre a faixa de endereços I2C padrão (0x08 a 0x77). Endereços fora dessa faixa são geralmente reservados.
    - Para cada endereço (`addr`), tenta ler 1 byte (`i2c_read_blocking`). Esta função envia o endereço do dispositivo no modo de leitura.
    - Se a leitura for bem-sucedida (`result >= 0`), significa que um dispositivo respondeu (ACK) nesse endereço. O endereço encontrado é armazenado em `addr_device`. _Nota: Este scanner simples retorna apenas o endereço encontrado._
    - Um pequeno `sleep_ms(5)` é adicionado para dar tempo aos dispositivos responderem.
    - Retorna o endereço do dispositivo encontrado (ou 0x00 se nenhum for encontrado).
    - _Observação:_ O `printf("Scanner finalizado.\n");` está após o `return` e, portanto, nunca será executado.

6.  **`display_init()`:**

    - Inicializa a estrutura do display (`display`) usando a biblioteca `ssd1306`.
    - Passa as dimensões do display (128x64 pixels), o endereço I2C (`addr_device`) encontrado pelo scanner e a instância I2C (`i2c`) a ser usada.
    - Limpa o buffer interno do display (`ssd1306_clear`).
    - Imprime uma mensagem indicando que o display foi inicializado com o endereço encontrado.

7.  **`display_text()`:**

    - Função auxiliar para exibir texto no display.
    - Limpa o buffer do display (`ssd1306_clear`) para apagar o conteúdo anterior.
    - Desenha a string `text` no buffer do display nas coordenadas (x=10, y=32) com tamanho de fonte 2 (`ssd1306_draw_string`).
    - Envia o conteúdo do buffer para o display físico via I2C (`ssd1306_show`).

8.  **`main()`:**
    - `stdio_init_all()`: Inicializa a comunicação serial USB para `printf`.
    - `i2c_bus_init()`: Chama a função para inicializar o barramento I2C (`i2c1`, pinos 14/15, 400kHz).
    - `i2c_bus_scan()`: Chama a função para encontrar o endereço do dispositivo I2C conectado. O resultado é armazenado em `addr_device`.
    - `display_init()`: Inicializa o display SSD1306 usando o endereço encontrado.
    - `count = 0;`: Inicializa uma variável de contador.
    - **Loop Infinito (`while (true)`):**
      - `snprintf()`: Formata uma string (ex: "Count: 0", "Count: 1", ...) e a armazena no `buffer`.
      - `display_text()`: Exibe a string formatada no display.
      - `count++`: Incrementa o contador.
      - `sleep_ms(1000)`: Pausa a execução por 1000 milissegundos (1 segundo).
    - `return 0;`: Teoricamente nunca alcançado devido ao loop infinito.

Em resumo, o programa configura a comunicação I2C, encontra o display automaticamente, inicializa-o e entra em um loop que atualiza o display a cada segundo com um valor de contagem crescente.
