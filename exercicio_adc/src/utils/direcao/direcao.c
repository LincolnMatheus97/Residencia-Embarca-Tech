#include "direcao.h"

void desenha_direcao_leste() {
    ws2812b_fill(10, 10, ws2812b_random_color(100));
    ws2812b_fill(11, 11, ws2812b_random_color(100));
    ws2812b_fill(12, 12, ws2812b_random_color(100));
    ws2812b_fill(13, 13, ws2812b_random_color(100));
    ws2812b_fill(14, 14, ws2812b_random_color(100));

    ws2812b_fill(8, 8, ws2812b_random_color(100));
    ws2812b_fill(2, 2, ws2812b_random_color(100));
    ws2812b_fill(18, 18, ws2812b_random_color(100));
    ws2812b_fill(22, 22, ws2812b_random_color(100));
}

void desenha_direcao_oeste() {
    ws2812b_fill(10, 10, ws2812b_random_color(100));
    ws2812b_fill(11, 11, ws2812b_random_color(100));
    ws2812b_fill(12, 12, ws2812b_random_color(100));
    ws2812b_fill(13, 13, ws2812b_random_color(100));
    ws2812b_fill(14, 14, ws2812b_random_color(100));

    ws2812b_fill(16, 16, ws2812b_random_color(100));
    ws2812b_fill(22, 22, ws2812b_random_color(100));
    ws2812b_fill(6, 6, ws2812b_random_color(100));
    ws2812b_fill(2, 2, ws2812b_random_color(100));
}

void desenha_direcao_norte() {
    ws2812b_fill(2, 2, ws2812b_random_color(100));
    ws2812b_fill(7, 7, ws2812b_random_color(100));
    ws2812b_fill(12, 12, ws2812b_random_color(100));
    ws2812b_fill(17, 17, ws2812b_random_color(100));
    ws2812b_fill(22, 22, ws2812b_random_color(100));

    ws2812b_fill(16, 16, ws2812b_random_color(100));
    ws2812b_fill(18, 18, ws2812b_random_color(100));
    ws2812b_fill(10, 10, ws2812b_random_color(100));
    ws2812b_fill(14, 14, ws2812b_random_color(100));
}

void desenha_direcao_sul() {
    ws2812b_fill(2, 2, ws2812b_random_color(100));
    ws2812b_fill(7, 7, ws2812b_random_color(100));
    ws2812b_fill(12, 12, ws2812b_random_color(100));
    ws2812b_fill(17, 17, ws2812b_random_color(100));
    ws2812b_fill(22, 22, ws2812b_random_color(100));

    ws2812b_fill(10, 10, ws2812b_random_color(100));
    ws2812b_fill(14, 14, ws2812b_random_color(100));
    ws2812b_fill(6, 6, ws2812b_random_color(100));
    ws2812b_fill(8, 8, ws2812b_random_color(100));
}

void desenha_direcao_noroeste() {
    ws2812b_fill(0, 0, ws2812b_random_color(100));
    ws2812b_fill(8, 8, ws2812b_random_color(100));
    ws2812b_fill(12, 12, ws2812b_random_color(100));
    ws2812b_fill(16, 16, ws2812b_random_color(100));
    ws2812b_fill(24, 24, ws2812b_random_color(100));

    ws2812b_fill(15, 15, ws2812b_random_color(100));
    ws2812b_fill(14, 14, ws2812b_random_color(100));
    ws2812b_fill(23, 23, ws2812b_random_color(100));
    ws2812b_fill(22, 22, ws2812b_random_color(100));
}

void desenha_direcao_sudeste() {
    ws2812b_fill(0, 0, ws2812b_random_color(100));
    ws2812b_fill(8, 8, ws2812b_random_color(100));
    ws2812b_fill(12, 12, ws2812b_random_color(100));
    ws2812b_fill(16, 16, ws2812b_random_color(100));
    ws2812b_fill(24, 24, ws2812b_random_color(100));

    ws2812b_fill(9, 9, ws2812b_random_color(100));
    ws2812b_fill(10, 10, ws2812b_random_color(100));
    ws2812b_fill(1, 1, ws2812b_random_color(100));
    ws2812b_fill(2, 2, ws2812b_random_color(100));
}

void desenha_direcao_nordeste() {
    ws2812b_fill(4, 4, ws2812b_random_color(100));
    ws2812b_fill(6, 6, ws2812b_random_color(100));
    ws2812b_fill(12, 12, ws2812b_random_color(100));
    ws2812b_fill(18, 18, ws2812b_random_color(100));
    ws2812b_fill(20, 20, ws2812b_random_color(100));
    
    ws2812b_fill(22, 22, ws2812b_random_color(100));
    ws2812b_fill(21, 21, ws2812b_random_color(100));
    ws2812b_fill(19, 19, ws2812b_random_color(100));
    ws2812b_fill(10, 10, ws2812b_random_color(100));
}

void desenha_direcao_sudoeste() {
    ws2812b_fill(4, 4, ws2812b_random_color(100));
    ws2812b_fill(6, 6, ws2812b_random_color(100));
    ws2812b_fill(12, 12, ws2812b_random_color(100));
    ws2812b_fill(18, 18, ws2812b_random_color(100));
    ws2812b_fill(20, 20, ws2812b_random_color(100));

    ws2812b_fill(2, 2, ws2812b_random_color(100));
    ws2812b_fill(3, 3, ws2812b_random_color(100));
    ws2812b_fill(5, 5, ws2812b_random_color(100));
    ws2812b_fill(14, 14, ws2812b_random_color(100));
}