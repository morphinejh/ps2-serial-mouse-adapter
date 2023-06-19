#pragma once
/*
 * Created by Jason Hill
 * 2023/06/18
 * 
 *     DEBUG =  0 or undefined - No Seiral debug, No information provided
 * 0 < DEBUG <= 1 - Some information
 *     DEBUG >  1 - Verbose - all information including PS/2 mouse reporting
 */
#define DEBUG 0
/****************************************************
 * The following defines assume default pinout
 * of Arduino ProMicro or similiar:
 * 
 * PS2_CLOCK = 2; //PD2 - Port D, bit 2
 * PS2_DATA  = 17;//PC3 - Port C, bit 3
 * 
 * RS232_RTS = 3; //PD3 - Port D, bit 3
 * RS232_TX  = 4; //PD4 - Port D, bit 5
 * 
 * 
 * The following defines Pin and Port addressing to
 * more quickly set individual pins to their desired
 * states.
 *PS/2 data pin operations***Bit:76543210************/
#define SETDATAHIGH   (PORTC |=0b00001000)      //0x08
#define SETDATALOW    (PORTC &=0b11110111)      //0xF7
#define SETDATAIN     (DDRC  &=0b11110111)      //0xF7
#define SETDATAOUT    (DDRC  |=0b00001000)      //0x08
#define READDATA      ((PINC &=0b00001000)>>3)  //0x08
/*PS/2 clock pin operations**Bit:76543210************/
#define SETCLOCKHIGH  (PORTD |=0b00000100)      //0x04
#define SETCLOCKLOW   (PORTD &=0b11111011)      //0xFB
#define SETCLOCKIN    (DDRD  &=0b11111011)      //0xFB
#define SETCLOCKOUT   (DDRD  |=0b00000100)      //0x04
#define READCLOCK     ((PIND &=0b00000100)>>2)  //0x04
/*RS-232 pin operations******Bit:76543210************/  
#define SETRSTXHIGH   (PORTD |=0b00010000)      //0x10
#define SETRSTXLOW    (PORTD &=0b11101111)      //0xEF
/****************************************************/
