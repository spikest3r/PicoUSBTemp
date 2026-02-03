#include <stdio.h>
#include "pico/stdlib.h"


#define CMD_CONVERTTEMP 0x44 // Initiate temperature conversion
#define CMD_RSCRATCH 0xBE    // Read Scratchpad
#define CMD_SKIP_ROM 0xCC    // Skip ROM matching command (for single device)

#define TEMP_PIN 26          // Data pin connected to DS18B20 (with 4.7k - 10k pullup to 3.3V)
#define SCRATCHPAD_SIZE 9    // Scratchpad is 9 bytes long

void pin_output() {
    gpio_set_dir(TEMP_PIN, true);
}


void pin_input() {
    gpio_set_dir(TEMP_PIN, false);
}

// Standard 8-bit CRC polynomial for DS18B20: x^8 + x^5 + x^4 + 1
uint8_t crc8_ds18b20(const uint8_t *data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (int j = 0; j < 8; j++) {
            uint8_t lsb_byte = byte & 0x01;
            uint8_t lsb_crc = crc & 0x01;
            
            // Check if LSBs differ (XOR operation)
            if (lsb_byte ^ lsb_crc) {
                crc = (crc >> 1) ^ 0x8C;
            } else {
                crc = crc >> 1;
            }
            byte >>= 1;
        }
    }
    return crc;
}

bool ds18b20_reset() {
    pin_output();
    gpio_put(TEMP_PIN, 0);
    sleep_us(480); // Master pulls low (480us minimum)
    pin_input();
    
    // Wait for the device to respond (Presence Pulse)
    sleep_us(70);
    // Presence pulse should be low here.
    bool presence = !gpio_get(TEMP_PIN); 
    
    // Wait for the rest of the reset slot
    sleep_us(410);
    
    return presence;
}

void ds18b20_write_bit(bool bit) {
    pin_output();
    gpio_put(TEMP_PIN, 0); // Start write slot (pull low)
    
    if (bit) {
        // Write '1' slot: short low time (6us) followed by release for 64us
        sleep_us(6); 
        pin_input();
        sleep_us(64); 
    } else {
        // Write '0' slot: long low time (60us) followed by release for 10us
        sleep_us(60); 
        pin_input();
        sleep_us(10); 
    }
}

bool ds18b20_read_bit() {
    pin_output();
    gpio_put(TEMP_PIN, 0);
    sleep_us(2); // Start read slot (pull low for min 1us)
    pin_input();
    
    sleep_us(5); // Total 7us delay before sampling
    bool bit = gpio_get(TEMP_PIN);
    
    sleep_us(53); // Wait for the rest of the slot (total 60us min)
    return bit;
}

void ds18b20_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        // Send LSB first
        ds18b20_write_bit(byte & 0x01);
        byte >>= 1;
    }
}

uint8_t ds18b20_read_byte() {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        // Read LSB first and shift up
        if (ds18b20_read_bit()) {
            byte |= (1 << i);
        }
    }
    return byte;
}

float ds18b20_read_temp() {
    // 1. Initiate Reset and Presence Check
    if (!ds18b20_reset()) {
        return -999.0f;
    }

    ds18b20_write_byte(CMD_SKIP_ROM);
    ds18b20_write_byte(CMD_CONVERTTEMP);
    
    // Wait for conversion (750ms for 12-bit resolution)
    sleep_ms(750); 

    // 3. Read Scratchpad Data
    if (!ds18b20_reset()) {
        return -999.0f;
    }

    ds18b20_write_byte(CMD_SKIP_ROM);
    ds18b20_write_byte(CMD_RSCRATCH);

    uint8_t scratchpad[SCRATCHPAD_SIZE];
    for (int i = 0; i < SCRATCHPAD_SIZE; i++) {
        scratchpad[i] = ds18b20_read_byte();
    }

    // Validate Data Integrity with CRC
    uint8_t read_crc = scratchpad[8];
    
    // Calculate CRC over the first 8 bytes (Temp LSB, MSB, Config, etc.)
    uint8_t calculated_crc = crc8_ds18b20(scratchpad, 8); 

    if (read_crc != calculated_crc) {
        return -1000.0f; // Use a distinct error code for CRC failure
    }

    int16_t raw_temp = (scratchpad[1] << 8) | scratchpad[0];
    
    // The temperature is 12-bit, signed. Multiply by 0.0625 (1/16)
    float temp = raw_temp * 0.0625f;
    
    return temp;
}