
#define I2C_BAUD_RATE 400000 //Highest without high speed i2c
#define MIDI_BAUD_RATE 31500
#define I2C_PORT i2c1
#define I2C_SDA 6
#define I2C_SCL 7
#define SLAVE_ADDRESS 0x41

#define PORT1_UART_ID 1
#define PORT1_TX_PIN 4
#define PORT1_RX_PIN 5

#define PORT2_UART_ID 0
#define PORT2_TX_PIN 0
#define PORT2_RX_PIN 1


void InitializePorts(){
    i2c_init(I2C_PORT, I2C_BAUD_RATE);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    i2c_set_slave_mode(I2C_PORT, true, SLAVE_ADDRESS);
    i2c_slave_init(i2c0, 0x17, &I2CSlaveHandler);


    if(port1=Port.MIDI_RX||port1==Port.MIDI_TX){
        uart_init(PORT1_UART_ID, MIDI_BAUD_RATE);
        gpio_set_function(PORT1_TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(PORT1_RX_PIN, GPIO_FUNC_UART);
    }
    if(port2=Port.MIDI_RX||port2==Port.MIDI_TX){
        uart_init(PORT2_UART_ID, MIDI_BAUD_RATE);
        gpio_set_function(PORT2_TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(PORT2_RX_PIN, GPIO_FUNC_UART);
    }
}

void PollPorts(){
    // Port 1
    if(port1==Port.MIDI_RX){
        if (uart_is_readable(PORT1_UART_ID)) {
            uint8_t ch = uart_getc(PORT1_UART_ID);
            port1Data.push(ch);
            Note note = CheckMidiData(port1Data);
            if(note!=null){
                port1Stack.push(note);
            }
        }
    }
    else if(port1==Port.MIDI_TX){
        for(int i=0; i< port1Stack.size();i++){
            uart_puts(PORT1_UART_ID, NoteToMIDIByteStructure(port1Stack[i]).Data()); //probs wont work
        }
        port1Stack.clear();
       
    }


}


//Global

uint32_t timems;

// Ports
// Port is selected via two resistors, one for midi/usb, and one for rx-host/tx-device
Port port1 = Port.MIDI_RX;
Port port2 = Port.MIDI_RX;
std::vector<Note> port1Stack;
std::vector<note> port2Stack;
std::vector<uint8_t> port1Data;
std::vector<uint8_t> port2Data;

//I2C
std::vector<uint8_t> portI2CData;
std::vector<Note> portI2CStack;

void RecieveNoteFromMaster(int channel, Note n){
    switch{
        case 0:
            if(port1 == Port.MIDI_TX) port1Stack.push(n);
        case 1:
            if(port2 == Port.MIDI_TX) port2Stack.push(n);
        default:
            return;
    }
}

void I2CSlaveHandler(i2c_inst_t *i2c, i2c_slave_event_t event){
    switch (event) {
        case I2C_SLAVE_RECEIVE:
            uint8_t bit = i2c_read_byte_raw(i2c);
            portI2CData.push(bit);

            uint8_t firstbit = portI2CData[0]; //channel
            Note n = CheckMidiData(portI2CData,1); //note
            if(n!=null){RecieveNoteFromMaster(firstbit,n);
            
            break;
        case I2C_SLAVE_REQUEST:
            //This is to transmit recieved data to the main mcu
            if(CanRecieve(port1)){
                for(int i=0; i< port1Stack.size();i++){
                    std::vector<uint8_t> msg = NoteToMIDIByteStructure(port1Stack[i]);
                    for(int j=0; j< msg; j++){
                        i2c_write_byte_raw(i2c, msg[j]); 
                    }
                }
            }
            if(CanRecieve(port2)){
                for(int i=0; i< port1Stack.size();i++){
                    std::vector<uint8_t> msg = NoteToMIDIByteStructure(port2Stack[i]);
                    for(int j=0; j< msg; j++){
                        i2c_write_byte_raw(i2c, msg[j]); 
                    }
                }
            }
 

            break;
        case I2C_SLAVE_FINISH:
            break;
        }
    }
}
std::vector<uint8_t> NoteToMIDIByteStructure(Note note){
    std::vector<uint8_t> collector;
    collector.push(0x90+note.channel);
    collector.push(note.value);
    collector.push(note.velocity);
    return collector;
}
Note MIDIByteStructureToNote(std::vector<uint8_t>& bytes,bool status){
    Note n;
    n.value = bytes[1];
    n.velocity = bytes[2];
    n.channel = (bytes[0]-(0x90));
    n.time = timems;
    return n;
}
Note CheckMidiData(std::vector<uint8_t> stack,int byteoffset){
    uint8_t startindex = 0+byteoffset;
    switch(stack[startindex]){
        case(0xF0): //sysex
            if(stack[port1Data.size()-1+startindex]==0xF7){ //end sysex
                stack.clear();
            }
            return null;
        case(0xF8): //clock
            return null;
        case(0xFA): //start
            return null;
        case(0xFB): //continue
            return null;
        case(0xFC): //clock
            return null;
        default: //need to filter 0x9n and 0x8n
            if(stack.size()==3+startindex){
                if(stack[startindex] && (0b1001<<4) >> 4 == 0b1001){//it is noteOn 
                    return MIDIByteStructureToNote(stack,true);
                }
            }
        }
}
bool CanTransmit(Port p){
    if(p==Port.MIDI_TX ||p==Port.USB_D){
        return true;
    }
    return false;
}
bool CanRecieve(Port p){
    if(p==Port.MIDI_RX ||p==Port.USB_H){
        return true;
    }
    return false;
}
enum Port {
    USB_H,
    USB_D,
    MIDI_TX,
    MIDI_RX
}
struct Note {
    int value,
    int velocity,
    uint8_t channel,
    uint32_t time
}