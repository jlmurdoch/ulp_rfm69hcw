/**
 * ESP32-S3 ULP-FSM RFM69HCW Continuous Data Reader
 *
 * This is the main program that sets up and feeds off the ULP program.
 */
#include <stdio.h> 
#include "rfm69hcw.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// RTC GPIO
#include "driver/rtc_io.h"
#include "soc/rtc_cntl_reg.h"

// ULP - see main/CMakeLists.txt
#include "ulp.h"
#include "ulp/config.h"
#include "ulp_rfm_dio2_data.h"

// Start and end addr of ULP program in 32bit chunks: 0 (0x0000) - 255 (0x03FF)
extern const uint8_t bin_start[] asm("_binary_ulp_rfm_dio2_data_bin_start");
extern const uint8_t bin_end[]   asm("_binary_ulp_rfm_dio2_data_bin_end");

// ESP32-S3 - [5.13 - RTC IO Mux Pin List]
// Adafruit Radio FeatherWing - Custom Wiring
#define PIN_NUM_DIO0    5   // IRQ
#define PIN_NUM_DIO2    11  // DATA
#define PIN_NUM_RST     9   // Radio reset
#define PIN_NUM_CS      6   // SPI Chip Select

// Adafruit ESP32-S3 Feather SPI
#define PIN_NUM_MOSI    35 // SPI2 - FSPID
#define PIN_NUM_CLK     36 // SPI2 - FSPICLK
#define PIN_NUM_MISO    37 // SPI2 - FSPIQ

// Fast SPI Host
#define RFM69HCW_HOST   SPI2_HOST

// RFM Clock speed
#define RFM69HCW_FSCK   SPI_MASTER_FREQ_8M

// RFM Radio Configuration
#define FREQUENCY       433730000
#define BITRATE         2000

// Flag to indicate ULP has finished processing
volatile bool ulp_rx_done_flag = 0;

/**
 * @brief Interrupt for when RSSI goes high on DIO2
 * @param arg Unused
 */
static void IRAM_ATTR dio0_rssi_isr(void *arg) {
    esp_err_t err;
    err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}

/**
 * @brief Interrupt for when the GPIO_RX_DONE goes low, indicating RX complete
 * @param arg SPI Device Handle
 */
static void IRAM_ATTR rx_done_isr(void *arg) {
    spi_device_handle_t spi = (spi_device_handle_t)arg;
    ulp_rx_done_flag = 1;

    rfm_rxrestart(spi);
}   

/**
 * @brief Initialise RTC and ULP
 * @param spi SPI Device Handle
 * @param reg Register to interrogate
 * @return `uint8_t` Data
 */
static void init_rtc_and_ulp(void)
{
    esp_err_t err;

    /*
     * RFM DATA GPIO
     */
    gpio_num_t gpio_num = PIN_NUM_DIO2;
    assert(rtc_gpio_is_valid_gpio(gpio_num) && "Not a valid RTC GPIO");
    printf("[ESP32-ULP] GPIO_%d = RTC_IO_%d\n", gpio_num, rtc_io_number_get(gpio_num));

    // Initialise GPIO as RTC GPIO
    err = rtc_gpio_init(gpio_num);
    ESP_ERROR_CHECK(err);

    // Set the RTC GPIO to INPUT
    rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_INPUT_ONLY);
    ESP_ERROR_CHECK(err);

    // Change GPIO, removing pullups and pulldowns
    rtc_gpio_pulldown_dis(gpio_num);
    rtc_gpio_pullup_dis(gpio_num);

    /*
     * ULP Receive Signalling GPIO
     */
    gpio_num = GPIO_RX_DONE;
    assert(rtc_gpio_is_valid_gpio(gpio_num) && "Not a valid RTC GPIO");
    printf("[ESP32-ULP] GPIO_%d = RTC_IO_%d\n", gpio_num, rtc_io_number_get(gpio_num));

    // Initialise GPIO as RTC GPIO
    err = rtc_gpio_init(gpio_num);
    ESP_ERROR_CHECK(err);

    // Set the RTC GPIO to INPUT & OUTPUT (main program & ULP respectively)
    rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_INPUT_OUTPUT);
    ESP_ERROR_CHECK(err);

    /*
     * ULP Program Upload
     */
    err = ulp_load_binary(0, bin_start, (bin_end - bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);
}

void app_main(void)
{
    esp_err_t err;
    spi_device_handle_t spi;

    // Wait for the serial console to start
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("Serial Console On\n"); 

    // Pause before start
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    // Initialize the SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };
    err = spi_bus_initialize(RFM69HCW_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(err);

    // Initialise the SPI device
    spi_device_interface_config_t devcfg = {
        .command_bits = 8,
        .clock_speed_hz = SPI_MASTER_FREQ_10M,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };
    err = spi_bus_add_device(RFM69HCW_HOST, &devcfg, &spi);

    /*
     * GPIOs outside of SPI
     */
    // RFM Reset GPIO
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);

    // RFM DATA GPIO - Read by ULP
    gpio_set_direction(PIN_NUM_DIO2, GPIO_MODE_INPUT);

    // RFM RSSI GPIO - ISR for signal detection
    gpio_set_direction(PIN_NUM_DIO0, GPIO_MODE_INPUT);
    gpio_set_intr_type(PIN_NUM_DIO0, GPIO_INTR_POSEDGE);
    gpio_pulldown_dis(PIN_NUM_DIO0);
    gpio_pullup_dis(PIN_NUM_DIO0);

    // RFM RSSI GPIO - ISR for ULP RX Complete
    gpio_set_direction(GPIO_RX_DONE, GPIO_MODE_INPUT);
    gpio_set_intr_type(GPIO_RX_DONE, GPIO_INTR_NEGEDGE);
    gpio_pulldown_en(GPIO_RX_DONE);

    // Set up interrupt service
    err = gpio_install_isr_service(0);
    ESP_ERROR_CHECK(err);

    // Attach the interrupt service routine, dio0_rssi_isr(), to DIO0
    err = gpio_isr_handler_add(PIN_NUM_DIO0, dio0_rssi_isr, NULL);
    ESP_ERROR_CHECK(err);

    // Attach the interrupt service routine, rx_done_isr(), to GPIO_RX_DONE
    err = gpio_isr_handler_add(GPIO_RX_DONE, rx_done_isr, (void*)spi);
    ESP_ERROR_CHECK(err);
    
    // Set up RTC & ULP
    init_rtc_and_ulp();

    // Initialise the RFM69HCW Radio
    rfm_init(spi, PIN_NUM_RST, FREQUENCY, BITRATE);

    while(1)
    {   
        // RFM Receive Processed
        if (ulp_rx_done_flag) {
            ulp_rx_done_flag = 0;

            // Inform the state of the RFM for debugging
            rfm_status(spi);

            // Print the RFM ULP variables
            printf("Raw Data: %04x,%04x\n", 
                (uint16_t)(&ulp_value)[0], (uint16_t)(&ulp_value)[1]);

            uint8_t id = ((uint16_t)(&ulp_value)[0] >> 8) & 0xFF;
            uint8_t battery = ((uint16_t)(&ulp_value)[0] >> 7) & 0x1; 
            uint8_t manual = ((uint16_t)(&ulp_value)[0] >> 6) & 0x1;
            uint8_t channel = (((uint16_t)(&ulp_value)[0]>> 4) & 0x3) + 1;
            float temp = ((((uint16_t)(&ulp_value)[0] & 0xF) << 16) |
                            (((uint16_t)(&ulp_value)[1] >> 8) & 0xFF)) * 0.1;
            uint8_t humidity = ((uint16_t)(&ulp_value)[1] & 0xFF);

            printf("Sensor - Id: 0x%02x, Bat: %d, Man: %d, Ch: %d, Temp: %.1f°C, RH: %d%%\n", 
                id, battery, manual, channel, temp, humidity);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
