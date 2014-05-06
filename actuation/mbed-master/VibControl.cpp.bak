#include "mbed.h"
#include "MRF24J40.h"
#include <PwmOut.h>
#include "stdio.h"

#include <string>
#include <cstdlib>
#include <ctime>

#define LINKED_PLAYER 0

// RF tranceiver to link with handheld.
MRF24J40 mrf(p11, p12, p13, p14, p21);

// Error LED
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

// Timer
Timer timer;

// Serial port for showing RX data.
Serial pc(USBTX, USBRX);

// Used for sending and receiving
char txBuffer[128];
char rxBuffer[128];
int rxLen;

// Pwm Pins
PwmOut pp1(p23);
PwmOut pp2(p24);
PwmOut pp3(p25);
PwmOut pp4(p26);
DigitalOut pp5(p5);
DigitalOut pp6(p6);
DigitalOut pp7(p7);

Ticker high;
Ticker low;
Timer duty;

DigitalOut darray[] = {pp5, pp6, pp7};
float darrayVal[] = {0,0,0};

char* token;            // Token parsing received data 
int inputs;             // Number of inputs 
float dutyValue;        // Vibration strength
char testString[128];
int player;
int testcheck;                  // For tests

//***************** Do not change these methods (please) *****************//

/**
* Receive data from the MRF24J40.
*
* @param data A pointer to a char array to hold the data
* @param maxLength The max amount of data to read.
*/
int rf_receive(char *data, uint8_t maxLength)
{
    uint8_t len = mrf.Receive((uint8_t *)data, maxLength);
    uint8_t header[8]= {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};

    if(len > 10) {
        //Remove the header and footer of the message
        for(uint8_t i = 0; i < len-2; i++) {
            if(i<8) {
                //Make sure our header is valid first
                if(data[i] != header[i])
                    return 0;
            } else {
                data[i-8] = data[i];
            }
        }

        //pc.printf("Received: %s length:%d\r\n", data, ((int)len)-10);
    }
    return ((int)len)-10;
}

/**
* Send data to another MRF24J40.
*
* @param data The string to send
* @param maxLength The length of the data to send.
*                  If you are sending a null-terminated string you can pass strlen(data)+1
*/
void rf_send(char *data, uint8_t len)
{
    //We need to prepend the message with a valid ZigBee header
    uint8_t header[8]= {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};
    uint8_t *send_buf = (uint8_t *) malloc( sizeof(uint8_t) * (len+8) );

    for(uint8_t i = 0; i < len+8; i++) {
        //prepend the 8-byte header
        send_buf[i] = (i<8) ? header[i] : data[i-8];
    }
    //pc.printf("Sent: %s\r\n", send_buf+8);

    mrf.Send(send_buf, len+8);
    free(send_buf);
}

void err() {
    led1 = 1;
    wait(0.3);
    led1 = 0;
    led2 = 1;
    wait(0.3);
    led2 = 0;
    led3 = 1;
    wait(0.3);
    led3 = 0;
    led4 = 1;
    wait(0.3);
    led4 = 0;
}

void actuate(int vibNum, float value) {
    switch(vibNum) {
        case(1): pp1 = (value > 0.8) ? 0.0 : (1 - value); break;
        case(2): pp2 = (value > 0.8) ? 0.0 : (1 - value); break;
        case(3): pp3 = (value > 0.8) ? 0.0 : (1 - value); break;
        case(4): pp4 = (value > 0.8) ? 0.0 : (1 - value); break;
        default: break; //err();
    }
}

void act() {
    for (int i = 0; i < 3; i++) {
        if (darrayVal[i] > (0.002)) {
            darray[i].write(1);
        }
        else {
            darrayVal[i] = 0;
        }
    }   
    duty.reset();
}
void csd() {
    float d = duty.read();
    if (d >darrayVal[2]) {
        darray[2].write(0);
    }
    if (d > darrayVal[0]) {
        darray[0].write(0);
    }
    if (d > darrayVal[1]) {
        darray[1].write(0);
    }
}

//***************** You can start coding here *****************//
int main (void)
{
    uint8_t channel = 12;

    //Set the Channel. 0 is default, 15 is max
    mrf.SetChannel(channel);

    //Start the timer
    timer.start();
    strcpy(testString, "0,7,0.2,0.4,0.6,0.7,0.7,0.4,0.3");
    err();
    testcheck = 1;
    
    
    duty.start();
    high.attach(&act,0.02);
    low.attach(&csd,0.002);
    
    pc.printf("started \r\n");
    while(1) {
        
        //Try to receive some data
        rxLen = rf_receive(rxBuffer, 128);
        //pc.printf("here");
        if(rxLen > 0) {
            pc.printf("%s \r\n", rxBuffer );
            token = strtok(rxBuffer, " ,");
            player = atoi(token);
            if (player == LINKED_PLAYER) {
                token = strtok(NULL, " ,");
                inputs = atoi(token);
                for (int i = 1; i <= 4; i++) {
                    token = strtok(NULL, " ,");
                    dutyValue = atof(token);
                    actuate(i, dutyValue);
                }
                for (int i = 5; i <= inputs; i++) {
                    token = strtok(NULL, ",");
                    float temp = atof(token);
                    temp = 1.0  - temp;
                    temp = temp * 0.02;
                    darrayVal[i-5] = temp;
                }
                pc.printf("\r\n");
            }
        }
    }
        
        /*
        // Testing
        token = strtok(testString, ",");
        player = atoi(token);
        if (player == LINKED_PLAYER) {
            token = strtok(NULL, " ,");
            inputs = atoi(token);
            for (int i = 1; i <= 4; i++) {
                token = strtok(NULL, ",");
                dutyValue = atof(token);
                pc.printf("%d: %f \r\n", i, dutyValue);
                actuate(i, dutyValue);
            }
            for (int i = 5; i <= inputs; i++) {
                token = strtok(NULL, ",");
                float temp = atof(token);
                pc.printf("%d: %f \r\n", i, temp);
                temp = 1.- temp;
                temp = temp * 0.02;
                darrayVal[i-5] = temp;
            }
            testcheck = 0;
        }
    }
    while (1);*/
}    
        /*
        //Send some data every second
        if(timer.read_ms() >= 1000) {
            //Reset the timer to 0
            timer.reset();
            // Toggle LED 2.
            led2 = led2^1;
            //Add to the buffer. You may want to check out sprintf
            strcpy(txBuffer, "Jeff is the best TA!");
            //Send the buffer
            rf_send(txBuffer, strlen(txBuffer) + 1);
            pc.printf("Sent: %s\r\n", txBuffer);
        }*/
