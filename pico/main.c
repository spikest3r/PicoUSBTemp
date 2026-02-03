#include "pico/stdlib.h"
#include "tusb.h"
#include "temp.h"

int main() {
    tusb_init();

    gpio_init(26);

    while (1) {
        tud_task();

        if (tud_vendor_available()) {
            uint8_t cmd;
            tud_vendor_read(&cmd, 1);

            switch(cmd) {
                case 0xFE: {
                    float temp = ds18b20_read_temp();
                    char buf[64];
                    int n = snprintf(buf, sizeof(buf), "%f\n", temp);
                    tud_vendor_write((uint8_t*)buf, n);
                    tud_vendor_write_flush();
                    break;
                }
            }
        }
    }
    return 0;
}
