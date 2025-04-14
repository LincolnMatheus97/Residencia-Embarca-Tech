#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ws2812b_animation.h"
#include "utils/direcao/direcao.h"

#define JOYSTICK_VRX 27 // Valor do pino do joystick X
#define JOYSTICK_VRY 26 // Valor do pino do joystick Y

/*
    Coloquei os valores agora como limites superior e inferior, para que utilize as leituras para todas as direções
    e não apenas para os limites de cada direção, como estava antes.
    Assim, o joystick pode ser utilizado para fazer movimentos diagonais, como noroeste e sudeste.
*/
#define JOYSTICK_LIMITE_SUPERIOR 80 
#define JOYSTICK_LIMITE_INFERIOR 20

uint8_t leitura_joystick_x(); 
uint8_t leitura_joystick_y(); 
void verificar_movimento(); 
void inicializar_adc(); 