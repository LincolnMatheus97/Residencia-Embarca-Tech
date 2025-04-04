#ifndef DISPLAY_OLED_H
#define DISPLAY_OLED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/display_OLED/ssd1306.h"

// Definição do pino SDA e SCL do display OLED
#define I2C_SDA 14
#define I2C_SCL 15

// Estrutura do display OLED
extern ssd1306_t ssd;

// Buffer do OLED
extern uint8_t ssd_buffer[];
extern struct render_area frame_area;

// Display de mensagem no OLED
void display_mensagem(const char *linha1, const char *linha2, const char *linha3, bool limpar);

// Inicialização do display OLED
void inicializacao_display();

#endif