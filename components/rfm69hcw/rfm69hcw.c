#include <stdio.h>
#include "rfm69hcw.h"

/**
 * @brief RFM base SPI command
 * @param spi SPI Device Handle
 * @param cmd Register to interrogate
 * @param write Flag to indicate a write procedure
 * @param buf Data buffer for reading into or writing from
 * @param len Data buffer length in bytes
 */
void rfm_cmd(spi_device_handle_t spi, const uint8_t cmd, uint8_t write, uint8_t *buf, size_t len) 
{
    esp_err_t err;
    err = spi_device_acquire_bus(spi, portMAX_DELAY);
    assert(err == ESP_OK);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.cmd = (write << 7) | cmd;
    
    t.length = 8 * len;
    if (write) {
        t.tx_buffer = buf;
    } else {
        t.rx_buffer = buf;
        t.rxlength = 8 * len;
    }
    err = spi_device_polling_transmit(spi, &t);
    assert(err == ESP_OK);

    spi_device_release_bus(spi);
}

/**
 * @brief RFM Read with single byte returned
 * @param spi SPI Device Handle
 * @param reg Register to interrogate
 * @return `uint8_t` Data
 */
uint8_t rfm_read_single(spi_device_handle_t spi, uint8_t reg) {
    uint8_t buf;
    rfm_cmd(spi, reg, 0, &buf, 1);

    return buf;
}

/**
 * @brief RFM Write with single byte
 * @param spi SPI Device Handle
 * @param reg Register to interrogate
 * @param value Data to write
 */
void rfm_write_single(spi_device_handle_t spi, uint8_t reg, uint8_t value) {
    rfm_cmd(spi, reg, 1, &value, 1);
}

/**
 * @brief RFM initialisation procedure
 * @param spi SPI Device Handle
 * @param gpio_rst GPIO to reset the RFM
 * @param frequency Frequency in Hz to tune into
 * @param bitrate Bitrate in bits-per-second
 */
void rfm_init(spi_device_handle_t spi, gpio_num_t gpio_rst, uint32_t frequency, uint16_t bitrate) {
    // Perform Reset
    printf("[RFM69] Reset: Start\n");
    gpio_set_level(gpio_rst, 1);
    esp_rom_delay_us(10);
    gpio_set_level(gpio_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    printf("[RFM69] Reset: Done\n");

    // Check version
    if (rfm_read_single(spi, 0x10) != 0x24){
        printf("[RFM69] Version: FAILED\n");
        return;
    }
    printf("[RFM69] Version: OK\n");

    // Bring RFM into standby for configuration
    rfm_write_single(spi, 0x01, 0x04); // Set Standby
    while(!((rfm_read_single(spi, 0x27)) >> 7)); // Wait for Mode Ready

    // Data Mode & Modulation
    rfm_write_single(spi, 0x02, 0x48); // Continuous w/ Bit Sync & OOK

    // Bitrate
    uint16_t raw_bitrate = FXOSC / bitrate;
    rfm_write_single(spi, 0x03, (raw_bitrate >> 8) & 0xFF);
    rfm_write_single(spi, 0x04, raw_bitrate & 0xFF);

    // Frequency
    uint32_t raw_freq = (uint32_t)(frequency / (FXOSC / 524288));
    rfm_write_single(spi, 0x07, (raw_freq >> 16) & 0xFF);
    rfm_write_single(spi, 0x08, (raw_freq >> 8) & 0xFF);
    rfm_write_single(spi, 0x09, raw_freq & 0xFF);

    // Bandwidth
    rfm_write_single(spi, 0x19, DCCFREQ_4 << 5 | RXBWMANT_24 << 3 | RXBWEXP_0);
    // AFC bandwidth
    rfm_write_single(spi, 0x1A, DCCFREQ_4 << 5 | RXBWMANT_24 << 3 | RXBWEXP_0);

    // OOK
    rfm_write_single(spi, 0x1B, 0x40); // OOK Peak
    rfm_write_single(spi, 0x1D, 0x01); // OOK Peak Threshold

    // Recommended datasheet non-defaults (6.1. Table 23)
    rfm_write_single(spi, 0x18, 0x88); // Recommended non-default: LNA Settings
    rfm_write_single(spi, 0x6F, 0x30); // Recommended non-default: Fading Margin Improvement

    // Optional RSSI signaling
    rfm_write_single(spi, 0x25, 0x80); // Pin Mapping: DIO0 = RSSI
    rfm_write_single(spi, 0x29, 0xA0); // RSSI Threshold

    // Bring RFM out of standby into receiving mode
    rfm_write_single(spi, 0x01, 0x10); // Set receiver mode
    while(!((rfm_read_single(spi, 0x27)) >> 7)); // Wait for Mode Ready
}

/**
 * @brief RFM Status query - OpMode and Status
 * @param spi SPI Device Handle
 */
void rfm_status(spi_device_handle_t spi) {
    const char *modes[5] = { "Sleep", "Standby", "Frequency Synthesizer", 
                            "Transmitter", "Receiver" };

    const char *states[8] = { "ModeReady", "RxReady", "TxReady", "PllLock",
                            "Rssi", "Timeout", "AutoMode", "SyncAddressMatch" };

    printf("[RFM69] OpMode: %s", modes[(rfm_read_single(spi, 0x01)>> 2) & 0x07]);
    
    printf(", Status: "); 
    bool flag_set = 0;
    uint8_t value = rfm_read_single(spi, 0x27);
    for (int i = 7; i >= 0; i--) {
        if (value & 1 << (7 - i)) {
            if (flag_set) {
                printf(" | ");
            }
            flag_set = 1;
            printf("%s", states[7 - i]);
        }
    }
    printf("\n");
}

/**
 * @brief RFM RxRestart
 * @param spi SPI Device Handle
 */
void rfm_rxrestart(spi_device_handle_t spi) {
    uint8_t value = 0x04;
    rfm_cmd(spi, 0x3D, 1, &value, 1);
}