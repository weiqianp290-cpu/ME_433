#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// ----- SPI and pin definitions -----
#define SPI_PORT spi0

#define PIN_CS_DAC 17   // MCP4912 CS -> Pico GP17
#define PIN_CS_RAM 13   // 23K256 CS  -> Pico GP13
#define PIN_SCK    18   // SCK shared by MCP4912 and 23K256
#define PIN_MOSI   19   // MOSI: Pico -> MCP4912 SDI and 23K256 SI
#define PIN_MISO   16   // MISO: 23K256 SO -> Pico GP16

// ----- HW8 waveform settings -----
#define SAMPLES 1000    // 1000 samples for one sine cycle

// ----- 23K256 commands -----
#define RAM_WRITE 0x02
#define RAM_READ  0x03
#define RAM_WRMR  0x01

// 23K256 mode register
#define RAM_MODE_SEQUENTIAL 0x40

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

// ----- Make sure only one SPI chip is selected -----
static inline void select_dac(void) {
    gpio_put(PIN_CS_RAM, 1);     // RAM off
    cs_select(PIN_CS_DAC);       // DAC on
}

static inline void deselect_dac(void) {
    cs_deselect(PIN_CS_DAC);
}

static inline void select_ram(void) {
    gpio_put(PIN_CS_DAC, 1);     // DAC off
    cs_select(PIN_CS_RAM);       // RAM on
}

static inline void deselect_ram(void) {
    cs_deselect(PIN_CS_RAM);
}

// ----- Build MCP4912 command -----
// channel: 0 = Channel A, 1 = Channel B
// value: 0 ~ 1023
//
// MCP4912 16-bit command format:
//   Bit 15:   A/B  (0=A, 1=B)
//   Bit 14:   BUF  (1=buffered)
//   Bit 13:   GA   (1=1x gain)
//   Bit 12:   SHDN (1=active)
//   Bit 11-2: D9-D0 (10-bit data)
//   Bit 1-0:  don't care
uint16_t buildDACCommand(uint8_t channel, uint16_t value) {
    if (value > 1023) {
        value = 1023;
    }

    uint16_t command = 0;
    command |= (channel & 0x01) << 15;   // channel select
    command |= 1 << 14;                  // BUF = 1
    command |= 1 << 13;                  // GA = 1, 1x gain
    command |= 1 << 12;                  // SHDN = 1, output active
    command |= (value & 0x03FF) << 2;    // 10-bit data in bits 11:2

    return command;
}

// ----- Write full command to MCP4912 -----
void writeDACCommand(uint16_t command) {
    uint8_t data[2];

    data[0] = (command >> 8) & 0xFF;
    data[1] = command & 0xFF;

    select_dac();
    spi_write_blocking(SPI_PORT, data, 2);
    deselect_dac();
}

// ----- Write to MCP4912 -----
// Same style as your HW7 code
void writeDAC(uint8_t channel, uint16_t value) {
    uint16_t command = buildDACCommand(channel, value);
    writeDACCommand(command);
}

// ----- Initialize 23K256 RAM -----
// Set RAM to sequential mode
void initRAM(void) {
    uint8_t data[2];

    data[0] = RAM_WRMR;
    data[1] = RAM_MODE_SEQUENTIAL;

    select_ram();
    spi_write_blocking(SPI_PORT, data, 2);
    deselect_ram();
}

// ----- Write bytes to 23K256 RAM -----
void writeRAM(uint16_t address, uint8_t *data, uint16_t length) {
    uint8_t header[3];

    header[0] = RAM_WRITE;
    header[1] = (address >> 8) & 0xFF;
    header[2] = address & 0xFF;

    select_ram();
    spi_write_blocking(SPI_PORT, header, 3);
    spi_write_blocking(SPI_PORT, data, length);
    deselect_ram();
}

// ----- Read bytes from 23K256 RAM -----
void readRAM(uint16_t address, uint8_t *data, uint16_t length) {
    uint8_t header[3];

    header[0] = RAM_READ;
    header[1] = (address >> 8) & 0xFF;
    header[2] = address & 0xFF;

    select_ram();
    spi_write_blocking(SPI_PORT, header, 3);
    spi_read_blocking(SPI_PORT, 0x00, data, length);
    deselect_ram();
}

// ----- Store one DAC command into RAM -----
// one DAC command = 16 bits = 2 bytes
void storeDACCommandInRAM(uint16_t sample_index, uint16_t command) {
    uint8_t data[2];

    data[0] = (command >> 8) & 0xFF;
    data[1] = command & 0xFF;

    uint16_t address = sample_index * 2;

    writeRAM(address, data, 2);
}

// ----- Read one DAC command from RAM -----
uint16_t readDACCommandFromRAM(uint16_t sample_index) {
    uint8_t data[2];

    uint16_t address = sample_index * 2;

    readRAM(address, data, 2);

    return ((uint16_t)data[0] << 8) | data[1];
}

// ----- Build sine wave and store into 23K256 -----
// HW8 requirement:
// calculate 1000 sine values during initialization
// convert each one to unsigned 16-bit DAC command
// store command bytes into external RAM
void buildSineTableInRAM(void) {
    for (int i = 0; i < SAMPLES; i++) {
        float angle = 2.0f * (float)M_PI * (float)i / (float)SAMPLES;

        // sine: -1~1
        // shift to 0~1
        // scale to 0~1023, same as your HW7 DAC code
        uint16_t dac_value = (uint16_t)((sinf(angle) + 1.0f) * 0.5f * 1023.0f);

        // Channel A command
        uint16_t command = buildDACCommand(0, dac_value);

        // store two command bytes into 23K256
        storeDACCommandInRAM(i, command);
    }
}

int main() {
    stdio_init_all();

    // ----- Initialize CS pins first -----
    gpio_init(PIN_CS_DAC);
    gpio_set_dir(PIN_CS_DAC, GPIO_OUT);
    gpio_put(PIN_CS_DAC, 1);  // DAC CS idle high

    gpio_init(PIN_CS_RAM);
    gpio_set_dir(PIN_CS_RAM, GPIO_OUT);
    gpio_put(PIN_CS_RAM, 1);  // RAM CS idle high

    sleep_ms(100);

    // ----- Initialize SPI -----
    // 50 kHz is slow and stable for breadboard wiring
    spi_init(SPI_PORT, 50000);

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    sleep_ms(100);

    // ----- Initialize RAM -----
    initRAM();

    // ----- HW8 initialization step -----
    // build 1000-point sine wave and save it into external RAM
    buildSineTableInRAM();

    // ----- HW8 main loop -----
    // read two bytes from RAM and send directly to DAC
    // 1000 samples * 1 ms = 1 second period = 1 Hz sine wave
    while (true) {
        for (int i = 0; i < SAMPLES; i++) {
            uint16_t command = readDACCommandFromRAM(i);
            writeDACCommand(command);
            sleep_ms(1);
        }
    }

    return 0;
}