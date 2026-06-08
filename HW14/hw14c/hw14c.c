#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#define HX711_SCK_PIN 15
#define HX711_DT_PIN 14

#define HX711_WAIT_TIMEOUT_US 200000

// First-order IIR low-pass:  y[n] = a*y[n-1] + (1-a)*x[n]
// Approx cutoff fc ~= fs*(1-a)/(2*pi*a).  At fs=80Hz, a=0.80 -> fc ~= 3.2Hz,
// which strongly attenuates the 25-30Hz touch noise while keeping the slow
// force signal. Lower a = less filtering, higher a = more filtering.
#define IIR_ALPHA 0.80f

// Max samples we can buffer in RAM before printing them back.
#define MAX_SAMPLES 4000

static uint32_t buf_time_ms[MAX_SAMPLES];
static int32_t  buf_raw[MAX_SAMPLES];
static float    buf_filtered[MAX_SAMPLES];

static void hx711_init(void) {
    gpio_init(HX711_SCK_PIN);
    gpio_set_dir(HX711_SCK_PIN, GPIO_OUT);
    gpio_put(HX711_SCK_PIN, 0);

    gpio_init(HX711_DT_PIN);
    gpio_set_dir(HX711_DT_PIN, GPIO_IN);
}

// HX711 pulls DT low when a fresh conversion is ready. Wait for that, with a
// timeout so a disconnected/silent sensor doesn't hang the firmware forever.
static bool hx711_wait_ready(void) {
    absolute_time_t start = get_absolute_time();

    while (gpio_get(HX711_DT_PIN)) {
        if (absolute_time_diff_us(start, get_absolute_time()) > HX711_WAIT_TIMEOUT_US) {
            return false;
        }
        tight_loop_contents();
    }

    return true;
}

// Clock out 24 data bits (MSB first), then one extra pulse to select
// channel A, gain 128 (25 total SCK pulses). Data on DT is valid while SCK
// is high, so we read after raising SCK.
static int32_t hx711_read(void) {
    uint32_t raw = 0;

    if (!hx711_wait_ready()) {
        return 0;
    }

    for (int i = 0; i < 24; i++) {
        gpio_put(HX711_SCK_PIN, 1);
        sleep_us(1);
        raw = (raw << 1) | gpio_get(HX711_DT_PIN);
        gpio_put(HX711_SCK_PIN, 0);
        sleep_us(1);
    }

    // 25th pulse -> channel A, gain 128
    gpio_put(HX711_SCK_PIN, 1);
    sleep_us(1);
    gpio_put(HX711_SCK_PIN, 0);
    sleep_us(1);

    // sign-extend 24-bit two's complement to 32-bit signed int
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }

    return (int32_t)raw;
}

static int read_sample_count(void) {
    char line[32];

    if (fgets(line, sizeof(line), stdin) == NULL) {
        return 0;
    }

    return atoi(line);
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);

    hx711_init();
    printf("HX711 ready. Send number of samples.\n");

    while (true) {
        int samples = read_sample_count();
        if (samples <= 0) {
            printf("Send a positive integer sample count.\n");
            continue;
        }
        if (samples > MAX_SAMPLES) {
            printf("Max %d samples; clamping.\n", MAX_SAMPLES);
            samples = MAX_SAMPLES;
        }

        // Heartbeat so the host knows the Pico is alive before the (silent)
        // collection loop begins.
        printf("collecting %d samples...\n", samples);

        // Discard one reading so the SCK channel/gain setting takes effect
        // and the filter starts from a real value.
        (void)hx711_read();

        float filtered = 0.0f;
        bool filter_initialized = false;
        uint32_t start_ms = to_ms_since_boot(get_absolute_time());

        // --- Collection loop: NO printf here, so the loop runs as fast as the
        //     HX711 produces data (up to 80Hz). All I/O happens afterwards. ---
        for (int i = 0; i < samples; i++) {
            int32_t raw = hx711_read();
            uint32_t now_ms = to_ms_since_boot(get_absolute_time()) - start_ms;

            if (!filter_initialized) {
                filtered = (float)raw;
                filter_initialized = true;
            } else {
                filtered = IIR_ALPHA * filtered + (1.0f - IIR_ALPHA) * (float)raw;
            }

            buf_time_ms[i] = now_ms;
            buf_raw[i] = raw;
            buf_filtered[i] = filtered;
        }

        // --- Print everything back after all data is collected. ---
        printf("time_ms,raw,filtered\n");
        for (int i = 0; i < samples; i++) {
            printf("%lu,%ld,%.2f\n", buf_time_ms[i], buf_raw[i], buf_filtered[i]);
        }
        printf("done\n");
    }
}
