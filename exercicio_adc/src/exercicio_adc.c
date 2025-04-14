#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ws2812b_animation.h"
#include "utils/joystick/joystick.h"

int main()
{
    stdio_init_all();
    inicializar_adc();

    while (true) {
        verificar_movimento();
        sleep_ms(100);
    }
}
