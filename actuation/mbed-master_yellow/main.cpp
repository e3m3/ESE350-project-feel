#include "mbed.h"
#include "MRF24J40.h"
#include <PwmOut.h>
#include "stdio.h"
#include <string.h>
#include "../../spi_int.h"

#include <string>
#include <cstdlib>
#include <ctime>

#define LINKED_PLAYER 0

#define SPI_PRD       10  // ms
#define PRINT_PRD     50  // ms
#define DEBOUNCE_TSH  500 // ms
#define SPI_WAIT_TIME 32  // us
#define SPI_FREQ      1000000  // MHz

#define ADDR_IMU      ((const int) 0xD0)
#define NUM_IMU       3
#define X_IMU         0
#define Y_IMU         1
#define Z_IMU         2
#define X_OFST_IMU    ((int16_t) 7)
#define Y_OFST_IMU    ((int16_t) -46)
#define Z_OFST_IMU    ((int16_t) -13)
#define IMU_XOUT_H    ((const char) 0x1D)
#define THIRTYHZ 33.0 // ms
#define THESH 0.1

#define MAX_ACCEL_VAL (float (1023.0))


// RF tranceiver to link with handheld.
MRF24J40 mrf(p11, p12, p13, p14, p21);

// Error LED
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

// Timer
Timer timer;
Timer print_adxl_timer;
Ticker spi_start_ticker;

// Serial port for showing RX data.
Serial pc(USBTX, USBRX);

// IMU I2C Setup
I2C i2c(p28, p27);
InterruptIn imu_int(p29);

volatile bool new_imu_int;
int16_t imu[NUM_IMU];

void ISR_imu_int(void);
void setup_imu(void);
///////////////////////

// M2 SPI Setup
SPI spi(p5, p6, p7);
DigitalOut ss(p8);
//InterruptIn reset_m2(p30);

int outgoing, incoming;
int adxl[NUM_ADXL];
bool read_adxl;

void SPI_start_handler(void);
//void m2_reset_handler(void);
int write_to_spi(int reg, bool no_read);
void read_adxl_from_m2(void);
///////////////////////

// Debouncing
Timer debounce;
InterruptIn button(p30);


// Used for sending and receiving
char txBuffer[128];
char rxBuffer[128];
int rxLen;

// Pwm Pins
//PwmOut pp1(p23);
//PwmOut pp2(p24);
//PwmOut pp3(p25);
//PwmOut pp4(p26);

DigitalOut pwm_out1(p26);
DigitalOut pwm_out2(p25);
DigitalOut pwm_out3(p24);
DigitalOut pwm_out4(p23);
DigitalOut pwm_out5(p15);
DigitalOut pwm_out6(p16);
DigitalOut pwm_out7(p17);
DigitalOut pwm_out_arr[] = {pwm_out1, pwm_out2, pwm_out3, pwm_out4, pwm_out5, pwm_out6, pwm_out7};

Ticker ticker_pwm1, ticker_pwm2, ticker_pwm3, ticker_pwm4, ticker_pwm5, ticker_pwm6, ticker_pwm7;
Ticker ticker_pwm[] = {ticker_pwm1, ticker_pwm2, ticker_pwm3, ticker_pwm4, ticker_pwm5, ticker_pwm6, ticker_pwm7};
Ticker ticker_30Hz;
volatile float pwm_duty_cycles[7];

void ticker_pwm_handler1(void);
void ticker_pwm_handler2(void);
void ticker_pwm_handler3(void);
void ticker_pwm_handler4(void);
void ticker_pwm_handler5(void);
void ticker_pwm_handler6(void);
void ticker_pwm_handler7(void);
void ticker_30Hz_handler(void);

void (*ticker_handler_arr[])(void) = {&ticker_pwm_handler1, &ticker_pwm_handler2, &ticker_pwm_handler3, &ticker_pwm_handler4, 
                                        &ticker_pwm_handler5, &ticker_pwm_handler6, &ticker_pwm_handler7};


// Green light Digital out pins
DigitalOut green_led1(p18);
DigitalOut green_led2(p19);
DigitalOut green_led3(p20);
DigitalOut green_led4(p36);
DigitalOut green_led5(p35);
DigitalOut green_led6(p34);
DigitalOut green_led7(p33);
DigitalOut green_channel[] = {green_led1, green_led2, green_led3, green_led4, green_led5, green_led6, green_led7};

char* token;            // Token parsing received data 
int inputs;             // Number of inputs 
float dutyValue;        // Vibration strength
char testString[128];
int player;
int testcheck;                  // For tests
bool training_mode;
int recMode;

// GUI controlled
Ticker tcontrol;
Timer t1;
Timer t2;
Timer t3;
Timer t4;
Timer t5;
Timer t6;
Timer t7;
Timer timerarray[] = {t1,t2,t3,t4,t5,t6,t7};
int tcount[] = {-1,-1,-1,-1,-1,-1,-1};

int vhit[] = {0,0,0,0,0,0,0};
int green[] = {0,0,0,0,0,0,0};

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
/*
void actuate(int vibNum, float value) {
    switch(vibNum) {
        case(1):if (value < 0.9) { 
                    pp1 = 1 - value;
                    green[0] = 0;
                }
                else {
                    pp1 = 0.0;
                    green[0] = 1;
                } break;
        case(2):if (value < 0.9) { 
                    pp2 = 1 - value;
                    green[1] = 0;
                }
                else {
                    pp2 = 0.0;
                    green[1] = 1;
                } break;
        case(3):if (value < 0.9) { 
                    pp3 = 1 - value;
                    green[2] = 0;
                }
                else {
                    pp3 = 0.0;
                    green[2] = 1;
                } break;
        case(4):if (value < 0.9) { 
                    pp4 = 1 - value;
                    green[3] = 0;
                }
                else {
                    pp4 = 0.0;
                    green[3] = 1;
                } break;
        default: break;
    }
}
*/
void ticker_pwm_handler1(void) {
  //pc.printf("pwm_handler1\r\n");
  ticker_pwm[0].detach();
  pwm_out1 = 0;
}


void ticker_pwm_handler2(void) {
  //pc.printf("pwm_handler2\r\n");
  ticker_pwm[1].detach();
  pwm_out2 = 0;
}


void ticker_pwm_handler3(void) {
  //pc.printf("pwm_handler3\r\n");
  ticker_pwm[2].detach();
  pwm_out3 = 0;
}

void ticker_pwm_handler4(void) {
  //pc.printf("pwm_handler1\r\n");
  ticker_pwm[3].detach();
  pwm_out4 = 0;
}


void ticker_pwm_handler5(void) {
  //pc.printf("pwm_handler2\r\n");
  ticker_pwm[4].detach();
  pwm_out5 = 0;
}


void ticker_pwm_handler6(void) {
  //pc.printf("pwm_handler3\r\n");
  ticker_pwm[5].detach();
  pwm_out6 = 0;
}

void ticker_pwm_handler7(void) {
  //pc.printf("pwm_handler3\r\n");
  ticker_pwm[6].detach();
  pwm_out7 = 0;
}


void ticker_30Hz_handler(void) {
  //pc.printf("handler \r\n");
  for (int i = 0; i < 7; i++) {
    if (pwm_duty_cycles[i] > THESH) {
      green[i] = 1;
      ticker_pwm[i].detach();
      ticker_pwm[i].attach_us(ticker_handler_arr[i],
          (unsigned int) (pwm_duty_cycles[i]*((float) THIRTYHZ * 1000)));
      pwm_out_arr[i] = 1;
    } else {
        pwm_out_arr[i] = 0;
        green[i] = 0;
      }
  }
}


void turnoffgreen() {
    for (int i = 0; i < 7; i++) {
        green[i] = 0;
    }
}

void greenlightact() {
    for (int i = 0; i < 7; i++)
        green_channel[i] = green[i];
}

void debounce_handler() {
    if (debounce.read_ms() >= 500) {
        debounce.reset();
        training_mode = !training_mode;
        led4 = !led4;
        turnoffgreen();
        greenlightact(); 
    }
}

void trigger(int motor) {
    //timerarray[motor-1].reset();
    timerarray[motor-1].start();
    tcount[motor-1] = 6;
}

void checkTimers() {
    for (int i = 0; i < 7; i++) {
        if (timerarray[i].read() >= 0.12 && tcount[i] > 0) {
            if (pwm_duty_cycles[i] == 0.5) {
                pwm_duty_cycles[i] = 0;
                vhit[i] = 0;
            }
            else if (!green[1]) {
                pwm_duty_cycles[i] = 0.5;
                vhit[i] = 1;
            }
            tcount[i]--;
            timerarray[i].reset();
        }
        else if (timerarray[i].read() >= 0.12 && tcount[i] == 0) {
            if (!green[i]) {
              pwm_duty_cycles[i] = 1.0;
              vhit[i] = 1;
            }
            tcount[i]--;
            timerarray[i].reset();
        }
        else if (timerarray[i].read() >= 3.0 && tcount[i] < 0) {
            pwm_duty_cycles[i] = 0;
            vhit[i] = 0;
            timerarray[i].reset();
            timerarray[i].stop();
        }
        //if (tcount[i] < 0 && vhit[i])
        //  vhit[i] = 0;
        //else if (tcount[i] == 0) {
        //    if (!green[i]) vhit[i] = 1;
        //    tcount[i]--;
        //}
        //else {
        //    pwm_duty_cycles[i] = 0;
        //}
    }
}

char gui_input_buf[15 + 2];

//***************** You can start coding here *****************//
int main (void)
{   
    // MRF SPI Init
    uint8_t channel = 12;
    print_adxl_timer.start();
    mrf.SetChannel(channel); //Set the Channel. 0 is default, 15 is max
    // ///////////

    // M2 SPI Init
    ss = 1;
    spi.format(8, 0);
    //spi.format(16, 0);
    spi.frequency(SPI_FREQ);
    spi_start_ticker.attach_us(&SPI_start_handler, SPI_PRD*1000);
    read_adxl = false;
    // ///////////
    
    // IMU I2C Init
    i2c.frequency(400000); // 400KHz
    setup_imu();
    // ///////////

    //Start the timer
    timer.start();
    debounce.start();
    strcpy(testString, "0,2,0.2,1,0.6,0.7,0.7,0.4,0");
    err();
    testcheck = 1;
    button.rise(debounce_handler);
    training_mode = false;
    
    // Initial Lights
    recMode = 0;
    
    // Timer initiate
    for (int i = 0; i < 7; i++) {
        //timerarray[i].start();   
        timerarray[i].stop();   
    }
    tcontrol.attach(checkTimers, 1);
    
    // Digital Pin Pwms
    for (int i = 0; i < 7; i++) {
        pwm_duty_cycles[i] = 0.0;
    }
    ticker_30Hz.attach_us(&ticker_30Hz_handler, THIRTYHZ*1000);

    
    pc.printf("Starting...\n");
    while(1) {
        if (training_mode) {
            if (read_adxl) read_adxl_from_m2();
    
            if (print_adxl_timer.read_ms() > PRINT_PRD) {
              print_adxl_timer.reset();
              pc.printf("dx: ");
              for (int i = 0; i < NUM_ADXL; i++)
                pc.printf("%d\t", adxl[i]);
              pc.printf("\tdtheta: ");
              for (int i = 0; i < NUM_IMU; i++)
                pc.printf("%d\t", imu[i]);
              pc.printf("\t");
              for (int i = 0; i < 7; i++) 
                pc.printf("%d\t", vhit[i]);
              pc.printf("\n");
            }
            
            // GUI controlled
            //TODO TODO TODO
            if (pc.readable()) {
                pc.gets(gui_input_buf, 16);
                sscanf(gui_input_buf, "%d,%d,%d,%d,%d,%d,%d,%d", &recMode,green,green+1,green+2,green+3,green+4,green+5,green+6);
                pc.printf("DEBUG: %d,%d,%d,%d,%d,%d,%d,%d\n", recMode,green[0],green[1],green[2],green[3],green[4],green[5],green[6]);
                //pc.printf("DEBUG: %d,%d,%d,%d,%d,%d,%d,%d\r\n", *recMode,green[0],green[1],green[2],green[3],green[4],green[5],green[6]);
                if (recMode > 0) {
                    trigger(recMode);
                }
            }
        }
        
        else {
            //Try to receive some data
            rxLen = rf_receive(rxBuffer, 128);
            if(rxLen > 0) {
                pc.printf("%s \r\n", rxBuffer );
                token = strtok(rxBuffer, " ,");
                player = atoi(token);
                if (player == LINKED_PLAYER) {
                    token = strtok(NULL, " ,");
                    inputs = atoi(token);
                    /*
                    for (int i = 1; i <= 4; i++) {
                        token = strtok(NULL, " ,");
                        // GHETTO TESTING STUFF
                        //if (i == 1 || i == 2) {
                        dutyValue = atof(token);
                        //} else {
                        //  dutyValue = 1.0;
                        //}
                        actuate(i, dutyValue);
                    }
                    */
                    for (int i = 0; i < inputs; i++) {
                        token = strtok(NULL, ",");
                        float temp = atof(token)/MAX_ACCEL_VAL;
                        //float temp = ((float) atoi(token))/MAX_ACCEL_VAL;
                        temp = 1.0  - temp;
                        //temp = temp * 0.02;
                        // GHETTO TESTING STUFF
                        //darrayVal[i-5] = 0.0;
                        pwm_duty_cycles[i] = temp;
                    }
                    //pc.printf("\r\n");
                }
            }
            greenlightact();
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
    while (1) {
        greenlightact();
    }*/
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


void SPI_start_handler(void) {
    read_adxl = true;
}


int write_to_spi(int reg, bool no_read) {
    int read_val = DUMMY_BYTE;
    outgoing = reg;
    if (no_read)
        spi.write(outgoing);
    else
        read_val = spi.write(outgoing);
    wait_us(SPI_WAIT_TIME);
    return read_val;
}


void read_adxl_from_m2(void) {
    read_adxl = false;
    ss = 0;
    write_to_spi(MBED_ADXL_REQ, true);
    //pc.printf("Sent: %d\t Received: %d\n", outgoing, incoming);
    for (int i = 0x00; i < NUM_ADXL; i += 0x01) {
        incoming = write_to_spi(SEND_LO_BYTE, false);
        adxl[i] = incoming << 8;
        incoming = write_to_spi(SEND_HI_BYTE, false);
        adxl[i] |= incoming;
    }
    incoming = write_to_spi(MBED_ADXL_COMP, false);
    incoming = write_to_spi(DUMMY_BYTE, false);
    ss = 1;
}


void ISR_imu_int(void) {
  char data[6];
  const char data_reg_addr = IMU_XOUT_H;
  new_imu_int = true;

  i2c.write(ADDR_IMU, &data_reg_addr, 1);
  i2c.read(ADDR_IMU, data, 6);

  imu[X_IMU] = (int16_t) (((data[0] << 8) + data[1]) + X_OFST_IMU);
  imu[Y_IMU] = (int16_t) (((data[2] << 8) + data[3]) + Y_OFST_IMU);
  imu[Z_IMU] = (int16_t) (((data[4] << 8) + data[5]) + Z_OFST_IMU);
}


void setup_imu(void) {
    char PWR_MGM[2] = {0x3E, 0x80};     // Reset
    char SMPLRT_DIV[2] = {0x15, 0x00};  // No sample rate division
    char DLPF_FS[2] = {0x16, 0x1B};     // Full range, 42Hz low pass
    char INT_CFG[2] = {0x17, 0x05};     // Interrupt on device and data ready
    char PWR_MGM2[2] = {0x3E, 0x00};    // Begin sampling

    i2c.write(ADDR_IMU, PWR_MGM, 2, true);
    i2c.write(ADDR_IMU, SMPLRT_DIV, 2, true);
    i2c.write(ADDR_IMU, DLPF_FS, 2, true);
    i2c.write(ADDR_IMU, INT_CFG, 2, true);
    i2c.write(ADDR_IMU, PWR_MGM2, 2, true);

    imu_int.rise(&ISR_imu_int);
}

