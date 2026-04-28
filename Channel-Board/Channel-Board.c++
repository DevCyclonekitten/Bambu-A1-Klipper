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


#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h" //To get similar to millis
#include "pico/multicore.h" //Self explanitory
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/uart.h"

#include "hardware/pio.h"
#include "ws2812.pio.h" // Generated from your .pio file


#include "hardware/adc.h"
#include <map>
#include <vector>
#include <cstdint>

// Global Definitions
#define NEOPIXEL_PIO 0
#define I2C_BAUD_RATE 400000
// Pin Definitions
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define SLAVE_ADDRESS 0x40 //need to change or something

#define A_BTN1 14
#define A_BTN2 10
#define A_ENCD 11
#define A_ENCC 12
#define A_ENCPB 13
#define A_SLIDER 26
#define A_SLIDER_INDEX 0
#define A_NEOPIXEL 0


#define B_BTN1 17
#define B_BTN2 21
#define B_ENCD 20
#define B_ENCC 19
#define B_ENCPB 18
#define B_SLIDER 27
#define B_SLIDER_INDEX 1
#define B_NEOPIXEL 1
#define ENCODER_DEBOUNCE_MS 5

// Global Blocks
PIO pio;
// Global Variables
uint32_t timems;
int temperature = 0;
int lasttemperature = 0;

//Commands
bool prepareNextDataTransmission = false;
std::vector<uint8_t> dataToTransmit;

// A
bool A_BTN1_STATE = false;
bool A_BTN2_STATE = false;

uint16_t A_SLIDER_VALUE = 0;
int A_LREG_SLID_VAL = 0;

int A_ENC_STATE = 0;
bool A_ENCPB_STATE = false;
bool A_ENC_LAST_CK = false;
uint32_t A_ENC_LAST_TIME = 0;


// B
bool B_BTN1_STATE = false;
bool B_BTN2_STATE = false;

uint16_t B_SLIDER_VALUE = 0;
int B_LREG_SLID_VAL = 0;

int B_ENC_STATE = 0;
bool B_ENCPB_STATE = false;
bool B_ENC_LAST_CK = false;
uint32_t B_ENC_LAST_TIME = 0;





void InitializeGpio(){
    stdio_init_all();
    // I2C
    i2c_init(I2C_PORT, I2C_BAUD_RATE);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    i2c_set_slave_mode(I2C_PORT, true, SLAVE_ADDRESS);

    // Channel A
    gpio_set_function(A_SLIDER,GPIO_FUNC_SIO);
    
    gpio_set_function(A_ENCD,GPIO_FUNC_SIO);
    gpio_set_function(A_ENCC,GPIO_FUNC_SIO);
    gpio_set_function(A_ENCPB,GPIO_FUNC_SIO);
    gpio_set_dir(A_ENCD, GPIO_IN);
    gpio_set_dir(A_ENCC, GPIO_IN);
    gpio_set_dir(A_ENCPB, GPIO_IN);
    gpio_pullup(A_ENCD);
    gpio_pullup(A_ENCC);
    gpio_pullup(A_ENCPB);

    gpio_set_function(A_BTN1,GPIO_FUNC_SIO);
    gpio_set_function(A_BTN2,GPIO_FUNC_SIO);
    gpio_set_dir(A_BTN1, GPIO_IN);
    gpio_set_dir(A_BTN2, GPIO_IN);
    gpio_pullup(A_BTN1);
    gpio_pullup(A_BTN2);

    
}
void InitializeNeopixelPIO(uint pin, uint pV){
    pio = (pV==0 ? pio0 : pio1);
    uint offset = pio_add_program(pio, &ws2812_program);
    

    ws2812_program_init(pio, 0, offset, pin, 800000, false);

    //pio_sm_put_blocking(pio, 0, 0xFF000000); // Red
    //sleep_ms(1000);
    
}
void InitializeADC(){
    adc_init();

    adc_gpio_init(A_SLIDER);
    adc_gpio_init(B_SLIDER);
}
void SetNeopixelPIO(int pin, uint32_t colour){
    pio_sm_put_blocking(pio, pin, colour);
}

void ReadChannelData(){
    //Channel A
    A_BTN1_STATE= (A_BTN1_STATE ? true : !gpio_get(A_BTN1));
    A_BTN2_STATE= (A_BTN2_STATE ? true : !gpio_get(A_BTN2));
    A_ENCPB_STATE= (A_ENCPB_STATE ? true : !gpio_get(A_ENCPB));

    if(timems + ENCODER_DEBOUNCE_MS>= A_ENC_LAST_TIME){
        if(A_ENC_LAST_CK != gpio_get(A_ENCC)){
            A_ENC_LAST_CK=!A_ENC_LAST_CK;
            if(A_ENC_LAST_CK != digitalRead(A_ENCD)){
                A_ENC_LAST_TIME=timems;
                A_ENC_STATE+=1;
            }
            if (A_ENC_LAST_CK == digitalRead(A_ENCD)){
                A_ENC_LAST_TIME=timems;
                A_ENC_STATE-=1;
            }
        }
    }

    adc_select_input(A_SLIDER_INDEX); 
    A_SLIDER_VALUE = adc_read();

    // Channel B
    B_BTN1_STATE= (B_BTN1_STATE ? true : !gpio_get(B_BTN1));
    B_BTN2_STATE= (B_BTN2_STATE ? true : !gpio_get(B_BTN2));
    B_ENCPB_STATE= (B_ENCPB_STATE ? true : !gpio_get(B_ENCPB));

    if(timems + ENCODER_DEBOUNCE_MS>= B_ENC_LAST_TIME){
        if(B_ENC_LAST_CK != gpio_get(B_ENCC)){
            B_ENC_LAST_CK=!B_ENC_LAST_CK;
            if(B_ENC_LAST_CK != digitalRead(B_ENCD)){
                B_ENC_LAST_TIME=timems;
                B_ENC_STATE+=1;
            }
            if (B_ENC_LAST_CK == digitalRead(B_ENCD)){
                B_ENC_LAST_TIME=timems;
                B_ENC_STATE-=1;
            }
        }
    }

    adc_select_input(B_SLIDER_INDEX); 
    B_SLIDER_VALUE = adc_read();


    // Debugging
    adc_select_input(4);
    temperature = (int)(27.0f - (adc_read(); * 3.3f / 4095.0f - 0.706f) / 0.001721f); //got this from site, also i hope casting exists

    // Transmitting
    if(prepareNextDataTransmission){
        dataToTransmit = ReportChannelData();
    }
}

std::vector<uint8_t> ReportChannelData(){
    uint8_t i2c_adr = 0x30;//example
    uint8_t channel_a_adr = 0b00;
    uint8_t channel_b_adr = 0b01;
    uint8_t channel_debug_adr = 0b10;
    std::vector<uint8_t> data;
    uint8_t pck;
    uint8_t dpck;
    /*
    Encoding

    0babcdefgh
    a=new object
    b=state boolean, or for data, signed value ()
    c,d = channel adress
    e,f,g,h = object adress
    
    0b10001000
    */
    /////////////// STATES OF CHANNEL A ///////////////
    if(A_BTN1_STATE){
        pck = (0xC0 + channel_a_adr<<4 + 0b0000);
        data.push(pck);
    }
    if(A_BTN2_STATE){
        pck = (0xC0 + channel_a_adr<<4 + 0b0001);
        data.push(pck);
    }
    if(A_ENCPB_STATE){
        pck = (0xC0 + channel_a_adr<<4 + 0b0010);
        data.push(pck);
    }
    if(A_ENC_STATE){
        pck = (0x80 + ((A_ENC_STATE>=0 ? 1 : 0)<<6) + channel_a_adr<<4 + 0b0011);
        dpck = absint8(A_ENC_STATE) >0x7F ? 0x7F : absint8(A_ENC_STATE); //clamp to 0-127, so MSB is 0
        data.push(pck);
        data.push(dpck);
        A_ENC_STATE = 0;
    }
    if(A_SLIDER_VALUE!=A_LREG_SLID_VAL){
        pck = (0xC0 + channel_a_adr<<4 + 0b0100);
        dpck = absint8(A_SLIDER_VALUE) >0x7F ? absint8(A_SLIDER_VALUE)>>5: absint8(A_SLIDER_VALUE);
        data.push(pck);
        data.push(dpck);
    }
    /////////////// STATES OF CHANNEL A ///////////////
    if(B_BTN1_STATE){
        pck = (0xC0 + channel_b_adr<<4 + 0b0000);
        data.push(pck);
    }
    if(B_BTN2_STATE){
        pck = (0xC0 + channel_b_adr<<4 + 0b0001);
        data.push(pck);
    }
    if(B_ENCPB_STATE){
        pck = (0xC0 + channel_b_adr<<4 + 0b0010);
        data.push(pck);
    }
    if(B_ENC_STATE){
        pck = (0x80 + ((B_ENC_STATE>=0 ? 1 : 0)<<6) + channel_b_adr<<4 + 0b0011);
        dpck = absint8(B_ENC_STATE) >0x7F ? 0x7F : absint8(B_ENC_STATE);
        data.push(pck);
        data.push(dpck);
        B_ENC_STATE = 0;
    }
    if(B_SLIDER_VALUE!=B_LREG_SLID_VAL){
        pck = (0xC0 + channel_b_adr<<4 + 0b0100);
        dpck = absint8(B_SLIDER_VALUE) >0x7F ?  absint8(B_SLIDER_VALUE)>>5: absint8(B_SLIDER_VALUE);
        data.push(pck);
        data.push(dpck);
    }
    if(temperature!=lasttemperature){
        lasttemperature=temperature;
        pck = (0xC0 + channel_debug_adr<<4 + 0b0000);
        dpck = absint8(temperature) >0x7F ? 0x7F : absint8(temperature); //cutoff
        data.push(pck);
        data.push(dck);
    }
    return data;
}
uint8 absint8(int v){
    if(v<0){
        return -v;
    }
    return v;
}

int main(){

    InitializeGpio();
    InitializeNeopixelPIO(2);
    InitializeADC();

    while (true){
        timems = to_ms_since_boot(get_absolute_time());
        ReadChannelData();
    }

    /*
    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_puts(UART_ID, " Hello, UART!\n");*/

}



void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
        case I2C_SLAVE_RECEIVE:
            uint8_t byte = i2c_read_byte_raw(i2c); 
            if(byte == 0xFE){ //Prepare long transmission, SLAVE_REQUEST should come maybe in next 200-500ns
                prepareNextDataTransmission=true; 
            }
            break;
        case I2C_SLAVE_REQUEST:
            i2c_write_raw_blocking(i2c,dataToTransmit.data(),dataToTransmit.size());
            break;
        case I2C_SLAVE_FINISH:
            break;
    }
}