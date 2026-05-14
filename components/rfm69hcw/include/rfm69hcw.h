#include <string.h>             // memset()
#include "driver/spi_master.h"  // SPI
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"      // portMAX_DELAY
#include "driver/gpio.h"        // for reset

#define FXOSC 32000000.0

enum { FSK, OOK };
enum { DCCFREQ_16, DCCFREQ_8, DCCFREQ_4, DCCFREQ_2, DCCFREQ_1, DCCFREQ_0_5, DCCFREQ_0_25, DCCFREQ_0_125 };
enum { RXBWMANT_16, RXBWMANT_20, RXBWMANT_24 };
enum { RXBWEXP_0, RXBWEXP_1, RXBWEXP_2,  RXBWEXP_3,  RXBWEXP_4, RXBWEXP_5,  RXBWEXP_6,  RXBWEXP_7 };

void rfm_status(spi_device_handle_t spi);
void rfm_write_single(spi_device_handle_t spi, uint8_t reg, uint8_t value);
void rfm_init(spi_device_handle_t spi, gpio_num_t gpio_rst, uint32_t frequency, uint16_t bitrate);
void rfm_rxrestart(spi_device_handle_t spi);