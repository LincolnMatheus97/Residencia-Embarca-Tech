#ifndef SENSORES_H
#define SENSORES_H

#include "pico/stdlib.h"
#include "hardware/adc.h"

// --- Definições de Pinos ---
#define BUTTON_A_PIN 5    // Pino GPIO 5 será o Botão A
#define BUTTON_B_PIN 6    // Pino GPIO 6 será o Botão B
#define JOYSTICK_X_PIN 27 // Pino GPIO 7 será o Eixo X do Joystick
#define JOYSTICK_Y_PIN 26 // Pino GPIO 8 será o Eixo Y do Joystick

// --- Funções para inicializar e ler sensores ---
void inicializar_sensores();
uint8_t ler_joystick_x();
uint8_t ler_joystick_y();
bool botao_a_pressionado();
bool botao_b_pressionado();

#endif