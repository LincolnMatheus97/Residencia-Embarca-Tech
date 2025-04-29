#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT_1 i2c1
#define I2C_SDA_1 14
#define I2C_SCL_1 15

#define I2C_PORT_2 i2c0 
#define I2C_SDA_2 8
#define I2C_SCL_2 9

#define OLED_ADDRESS 0x3C // Endereço padrão do OLED

void i2c_bus_init(i2c_inst_t *i2c, uint sda_pin, uint scl_pin, uint baudrate)
{
    i2c_init(i2c, baudrate);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    printf("I2C INICIALIZADO: SDA=%d, SCL=%d, BAUDRATE=%d Hz\n", sda_pin, scl_pin, baudrate);
}

void i2c_bus_scan(i2c_inst_t *i2c)
{
    printf("Iniciando scanner I2C...\n");

    uint8_t found_devices = 0;

    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        uint8_t rxdata;
        int result = i2c_read_blocking(i2c, addr, &rxdata, 1, false);
        if (result >= 0) {
            printf("Dispositivo encontrado no endereco 0x%02X\n", addr);
            found_devices++;
        }
        sleep_ms(5); // Pequena pausa para estabilidade
    }

    if (found_devices == 0) {
        printf("Nenhum dispositivo encontrado no barramento.\n");
    }

    printf("Scanner finalizado.\n");
}

int main()
{
    stdio_init_all();

    while (true) {
        printf("\n--- Testando I2C1  ---\n");
        // Inicializa o barramento I2C 1
        i2c_bus_init(I2C_PORT_1, I2C_SDA_1, I2C_SCL_1, 400000); // 400kHz
        i2c_bus_scan(I2C_PORT_1); // Executa o scanner I2C no barramento 1
        i2c_deinit(I2C_PORT_1); // Desinicializa o barramento I2C 1
        // Libera os pinos para que possam ser usados por outro periférico (embora não seja estritamente necessário aqui, é boa prática)
        gpio_set_function(I2C_SDA_1, GPIO_FUNC_NULL);
        gpio_set_function(I2C_SCL_1, GPIO_FUNC_NULL);
        sleep_ms(5000); // Aguarda 5 segundos


        printf("\n--- Testando I2C0 ---\n");
        // Inicializa o barramento I2C 0
        i2c_bus_init(I2C_PORT_2, I2C_SDA_2, I2C_SCL_2, 400000); // Re-inicializa o barramento I2C 0 com pinos corretos
        i2c_bus_scan(I2C_PORT_2); // Executa o scanner I2C no barramento 0
        i2c_deinit(I2C_PORT_2); // Desinicializa o barramento I2C 0
        // Libera os pinos
        gpio_set_function(I2C_SDA_2, GPIO_FUNC_NULL);
        gpio_set_function(I2C_SCL_2, GPIO_FUNC_NULL);
        sleep_ms(5000); // Aguarda 5 segundos

    }    
    return 0;
}
