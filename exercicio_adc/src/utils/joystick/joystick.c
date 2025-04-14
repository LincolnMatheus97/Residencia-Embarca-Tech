#include "joystick.h"

/*
    Função para ler o valor do eixo X do joystick
    A função seleciona o canal ADC correspondente ao eixo X e lê o valor, retornando-o como uma porcentagem (0 a 100).
    O valor lido é dividido por 4095 (valor máximo do ADC) e multiplicado por 100 para obter a porcentagem.
*/
uint8_t leitura_joystick_x() {
    adc_select_input(1);

    return adc_read() * 100 / 4095;
}
/*
    Função para ler o valor do eixo Y do joystick
    A função seleciona o canal ADC correspondente ao eixo Y e lê o valor, retornando-o como uma porcentagem (0 a 100).
    O valor lido é dividido por 4095 (valor máximo do ADC) e multiplicado por 100 para obter a porcentagem.
*/
uint8_t leitura_joystick_y() {
    adc_select_input(0);

    return adc_read() * 100 / 4095;
}

/* 
    Função para verificar o movimento do joystick e desenhar a seta correspondente
    A função lê os valores dos eixos X e Y do joystick e verifica se eles estão acima ou abaixo dos limites definidos.
    Dependendo da direção do movimento, a função chama a função correspondente para desenhar a seta na tela.
    Se o joystick não estiver sendo pressionado, a tela é preenchida com a cor preta (apagada).
*/
void verificar_movimento() {
    uint8_t valor_x = leitura_joystick_x();
    uint8_t valor_y = leitura_joystick_y();

    ws2812b_fill_all(GRB_BLACK);

    if (valor_x > JOYSTICK_LIMITE_SUPERIOR && valor_y > JOYSTICK_LIMITE_SUPERIOR) {
        desenha_direcao_nordeste();
    } else if (valor_x > JOYSTICK_LIMITE_SUPERIOR && valor_y < JOYSTICK_LIMITE_INFERIOR) {
        desenha_direcao_sudeste();
    } else if (valor_x < JOYSTICK_LIMITE_INFERIOR && valor_y > JOYSTICK_LIMITE_SUPERIOR) {
        desenha_direcao_noroeste();
    } else if (valor_x < JOYSTICK_LIMITE_INFERIOR && valor_y < JOYSTICK_LIMITE_INFERIOR) {
        desenha_direcao_sudoeste();
    } else if (valor_x > JOYSTICK_LIMITE_SUPERIOR) {
        desenha_direcao_leste();
    } else if (valor_x < JOYSTICK_LIMITE_INFERIOR) {
        desenha_direcao_oeste();
    } else if (valor_y > JOYSTICK_LIMITE_SUPERIOR) {
        desenha_direcao_norte();
    } else if (valor_y < JOYSTICK_LIMITE_INFERIOR) {
        desenha_direcao_sul();
    } else {
        ws2812b_fill_all(GRB_BLACK);
    }
    ws2812b_render();
}

void inicializar_adc() {
    adc_init();
    adc_gpio_init(JOYSTICK_VRX);
    adc_gpio_init(JOYSTICK_VRY);

    ws2812b_init(pio0, 7, 25);
    ws2812b_set_global_dimming(5);
}