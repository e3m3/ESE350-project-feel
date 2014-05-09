#ifndef __SPI_INT_H__
#define __SPI_INT_H__


#define ADXL_X          0
#define ADXL_Y          1
#define ADXL_Z          2

#define DUMMY_BYTE      0x00
#define SEND_LO_BYTE    0xF0
#define SEND_HI_BYTE    0xF1

#define NUM_ADXL        0x03
#define NUM_FSR         0x07

#define M2_ACK_ADXL     0xAC
#define M2_ACK_FSR      0xAD

#define M2_IDLE         0x1D
#define M2_SEND_ADXL    0x1A
#define M2_SEND_FSR     0x1F
#define M2_DONE_ADXL    0xD0
#define M2_DONE_FSR     0xD1

#define M2_ADXL_X       0x00
#define M2_ADXL_Y       0x01
#define M2_ADXL_Z       0x02

#define M2_FSR_0        0x00
#define M2_FSR_1        0x01
#define M2_FSR_2        0x02
#define M2_FSR_3        0x03
#define M2_FSR_4        0x04
#define M2_FSR_5        0x05
#define M2_FSR_6        0x06

#define MBED_ADXL_REQ   0xB0
#define MBED_FSR_REQ    0xB1
#define MBED_ADXL_COMP  0xD0
#define MBED_FSR_COMP   0xD1
#define MBED_RESET_REQ  0x8E


#endif /*__SPI_INT_H__*/
