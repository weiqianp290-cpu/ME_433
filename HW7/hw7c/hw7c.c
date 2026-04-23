#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// ----- SPI and pin definitions -----
#define SPI_PORT spi0
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_MISO 16  // not used by DAC, but assigned to SPI

// ----- Waveform settings -----
#define SINE_FREQ     2    // 2 Hz sine wave
#define TRI_FREQ      1    // 1 Hz triangle wave
#define SAMPLES       200  // samples per waveform cycle (well above 50x minimum)

// Precomputed sine lookup table
static uint16_t sine_table[SAMPLES];

// ----- CS pin control -----
static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop");
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop");
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop");
}

// ----- Write to MCP4912 -----
// channel: 0 = Channel A, 1 = Channel B
// voltage: 0 ~ 1023 (10-bit value)
//
// MCP4912 16-bit command format:
//   Bit 15:   A/B  (0=A, 1=B)
//   Bit 14:   BUF  (1=buffered)
//   Bit 13:   GA   (1=1x gain, 0=2x gain)
//   Bit 12:   SHDN (1=active)
//   Bit 11-2: D9-D0 (10-bit data)
//   Bit 1-0:  don't care
void writeDAC(uint8_t channel, uint16_t voltage) {
    uint16_t command = 0;
    command |= (channel & 0x01) << 15;   // channel select
    command |= 1 << 14;                  // BUF = 1 (buffered)
    command |= 1 << 13;                  // GA = 1 (1x gain)
    command |= 1 << 12;                  // SHDN = 1 (output active)
    command |= (voltage & 0x03FF) << 2;  // 10-bit data in bits 11:2

    uint8_t data[2];
    data[0] = (command >> 8) & 0xFF;     // high byte first
    data[1] = command & 0xFF;

    cs_select(PIN_CS);
    spi_write_blocking(SPI_PORT, data, 2);
    cs_deselect(PIN_CS);
}

// ----- Build sine lookup table (0 ~ 1023) -----
void buildSineTable(void) {
    for (int i = 0; i < SAMPLES; i++) {
        float angle = 2.0f * (float)M_PI * i / SAMPLES;
        // sin returns -1~1, shift to 0~1, scale to 0~1023
        sine_table[i] = (uint16_t)((sinf(angle) + 1.0f) / 2.0f * 1023.0f);
    }
}

// ----- Compute triangle wave value (0 ~ 1023) -----
uint16_t triangleValue(int index, int total_samples) {
    if (index < total_samples / 2) {
        // rising: 0 -> 1023
        return (uint16_t)(1023.0f * index / (total_samples / 2));
    } else {
        // falling: 1023 -> 0
        return (uint16_t)(1023.0f * (total_samples - index) / (total_samples / 2));
    }
}

int main() {
    stdio_init_all();

    // ----- Initialize SPI -----
    // Start slow (12 kHz) for debugging with oscilloscope
    // Once working, increase to e.g. 1000000 (1 MHz)
    spi_init(SPI_PORT, 12000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // CS pin: manual GPIO control
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);  // CS idle high

    // Build lookup table
    buildSineTable();

    // ----- Timing calculation -----
    // 2 Hz sine, 200 samples/cycle -> update every 1/(2*200) = 2.5 ms
    // 1 Hz triangle, 200 samples/cycle -> update every 1/(1*200) = 5.0 ms
    // Use 2.5 ms as the base loop interval
    // Sine updates every loop iteration, triangle every 2nd iteration

    uint32_t sine_delay_us = 1000000 / (SINE_FREQ * SAMPLES);  // 2500 us = 2.5 ms
    int sine_idx = 0;
    int tri_idx  = 0;
    uint32_t loop_count = 0;

    while (true) {
        // Channel A: 2 Hz sine wave - update every iteration
        writeDAC(0, sine_table[sine_idx]);
        sine_idx = (sine_idx + 1) % SAMPLES;

        // Channel B: 1 Hz triangle wave - update every 2nd iteration
        // (since triangle is half the frequency of sine)
        if (loop_count % 2 == 0) {
            writeDAC(1, triangleValue(tri_idx, SAMPLES));
            tri_idx = (tri_idx + 1) % SAMPLES;
        }

        loop_count++;
        sleep_us(sine_delay_us);
    }

    return 0;
}
