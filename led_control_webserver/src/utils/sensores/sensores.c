#include "sensores.h"

void inicializar_sensores() {
    // Configura os pinos dos botões como entrada com pull-up
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    // Inicializa o ADC
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN); // Inicializa o pino do eixo X do joystick
    adc_gpio_init(JOYSTICK_Y_PIN); // Inicializa o pino do eixo Y do joystick
}

uint8_t ler_joystick_x() {
    adc_select_input(1);                             // Canal ADC 1 é o GPIO 27
    uint16_t raw_value = adc_read();                 // Lê o valor bruto do ADC
    uint8_t scaled_value = (raw_value * 100) / 4095; // Escala para 0-100
    return scaled_value;
}

uint8_t ler_joystick_y() {
    adc_select_input(0);                             // Canal ADC 0 é o GPIO 26
    uint16_t raw_value = adc_read();                 // Lê o valor bruto do ADC
    uint8_t scaled_value = (raw_value * 100) / 4095; // Escala para 0-100
    return scaled_value;
}

bool botao_a_pressionado() {
    return !gpio_get(BUTTON_A_PIN); // Retorna verdadeiro se o botão A estiver pressionado (0)
}

bool botao_b_pressionado() {
    return !gpio_get(BUTTON_B_PIN); // Retorna verdadeiro se o botão B estiver pressionado (0)
}