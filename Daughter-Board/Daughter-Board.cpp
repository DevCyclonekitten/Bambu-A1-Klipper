/*//////////////////////////////////////////////////////////////////////////

Light up WS2812b Keyboard Daughter Board
Copyright (c) 2026 Isaac Tyley Miller
 ___      ___   _______  __   __  _______    __   __  _______ 
|   |    |   | |       ||  | |  ||       |  |  | |  ||       |
|   |    |   | |    ___||  |_|  ||_     _|  |  | |  ||    _  |
|   |    |   | |   | __ |       |  |   |    |  |_|  ||   |_| |
|   |___ |   | |   ||  ||       |  |   |    |       ||    ___|
|       ||   | |   |_| ||   _   |  |   |    |       ||   |    
|_______||___| |_______||__| |__|  |___|    |_______||___|    
 _     _  _______  _______   _____   ____   _______  _______  
| | _ | ||       ||       | |  _  | |    | |       ||  _    | 
| || || ||  _____||____   | | |_| |  |   | |____   || |_|   | 
|       || |_____  ____|  ||   _   | |   |  ____|  ||       | 
|       ||_____  || ______||  | |  | |   | | ______||  _   |  
|   _   | _____| || |_____ |  |_|  | |   | | |_____ | |_|   | 
|__| |__||_______||_______||_______| |___| |_______||_______| 


///////////////////////////////////////////////////////////////////////////

The MIT License (MIT)
Copyright (c) 2026 Isaac Tyley Miller

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

//////////////////////////////////////////////////////////////////////////*/

//General
#include <stdio.h>
#include "pico/stdlib.h"
#include "toml++/toml.hpp"
#include <vector>

//PIO
#include "hardware/pio.h" 
#include "hardware/clocks.h"
#include "midi_uart.pio.h"

///////////////////////////////////////////////////////////////////////////

// This auto manages firmware (cannot reprogram, but can detect and notify user)
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0

// Make sure to keep these Uarts on UART1 and UART0
#define DATA_UART_TX_PIN 4
#define DATA_UART_RX_PIN 5
#define MIDI_UART_TX_PIN 1
#define MIDI_UART_RX_PIN 0

//The led row that is at the bottom, should be first in index, so the last array index should be the led that appears first
#define NEOPIXEL_PINS []

//This is not neccesary
#define RELAY_PIN 6

//This may be changable in toml file, but is set in software
#define MIDI_BAUD 31250
#define MIDI_CYCLES_PER_BIT 15


///////////////////////////////////////////////////////////////////////////

//STRUCT DEFINING AREA, BECAUSE STRUCTS STUPIDLY HAVE TO GO ABOVE DEFINITIONS

struct MidiMessage {
    uint32_t timestamp;
    uint8_t data;
};

//Variable defining area
std::vector<MidiMessage> midi_messages;


///////////////////////////////////////////////////////////////////////////



int Core1(){
    //Configure MIDI UART PIO block
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &midi_uart_program);
    uint sm = pio_claim_unused_sm(pio, true);
    
    float hzcyclesperbit = (MIDI_BAUD*MIDI_CYCLES_PER_BIT);
    float clockdivisor = (float)clock_get_hz(clk_sys) / hzcyclesperbit;

    pio_sm_config c = midi_uart_program_get_default_config(offset);
    sm_config_set_in_pins(&c, MIDI_UART_RX_PIN); // Assuming GPIO 0 is MIDI RX
    sm_config_set_clkdiv(&c, clockdivisor);
    sm_config_set_in_shift(&c, true, false, 8); // Shift right, no autopush
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);


    printf("[1] Core 1 Establised");


    while (true) {
        static std::vector<uint8_t> buffer;

        int pc_rx = getchar_timeout_us(0);
        if (pc_rx != PICO_ERROR_TIMEOUT) {
            buffer.push_back((uint8_t)pc_rx);
        //
            // If we have 3 bytes, we have a full Note On/Off message
            if (buffer.size() == 3) {
                midi_messages.push_back({to_ms_since_boot(get_absolute_time()), buffer[0]});
                // You can now access buffer[1] for note and buffer[2] for velocity
                printf("Full MIDI Message: %02X %02X %02X\n", buffer[0], buffer[1], buffer[2]);
                buffer.clear(); 
            }
        }

        // --- PART B: Check Physical MIDI (PIO) ---
        if (!pio_sm_is_rx_fifo_empty(pio, sm)) {
            uint8_t pio_byte = (uint8_t)(pio_sm_get(pio, sm) >> 24);
            
            midi_messages.push_back({to_ms_since_boot(get_absolute_time()), pio_byte});
            
            printf("[UART -> MIDI] Received: 0x%02X\n", pio_byte);
        }



        // --- PART C: Cleanup list as before ---
        if (midi_messages.size() > 50) {
            midi_messages.erase(midi_messages.begin());
        }
    }
}

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    std::string_view config_data = R"(
        [pico_settings]
        led_brightness = 128
        sensor_name = "DHT22"midi_messages
    )";

    auto result = toml::parse(config_data);

    if(!result) {
        printf("[0] Parse failed! %s\n", result.error().description());
    }else {
        auto tbl = result.table();
        auto brightness = tbl["pico_settings"]["led_brightness"].value_or(0);
        printf("[0] LED Brightness set to: %d\n", brightness);
    }
        
    
    ::Core1();
}