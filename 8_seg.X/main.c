#include <xc.h>
#include <pic18f4520.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "conbits.h"
#include "uart_layer.h"

#define SEG_DATALINE LATB

#define SEG_DATA_DP (1 << 0) //0b00000001
#define SEG_DATA_A  (1 << 5) //0b00100000
#define SEG_DATA_B  (1 << 4) //0b00010000
#define SEG_DATA_C  (1 << 1) //0b00000010
#define SEG_DATA_D  (1 << 2) //0b00000100
#define SEG_DATA_E  (1 << 3) //0b00001000
#define SEG_DATA_F  (1 << 6) //0b01000000
#define SEG_DATA_G  (1 << 7) //0b10000000


#define SEG_SEL0 LATDbits.LATD7
#define SEG_SEL1 LATDbits.LATD6
#define SEG_SEL2 LATDbits.LATD5
#define SEG_SEL3 LATDbits.LATD4


#define SEG_DELAY 5000


static volatile uint16_t tmr0_overflow = 0;

const uint8_t program_start[18]="\r\nProgram start\n\r";
uint8_t print_buffer[33] = {0}; // buffer to print stuff to serial

volatile uint8_t uart_char = 0;
volatile bool uart_rcv_data = false;

void led_test(void){
    
    uint16_t k = 1;
    for(uint16_t i = 0; i < 8; i ++){
        LATB = k;
        k *= 2;
        if(k > 0x80){
            k = 1;
        }
        __delay_ms(500);
    }
    
}

void seg_numbers(uint8_t num){
    
    SEG_DATALINE &= 0x01;
    
    
    switch(num){
        case 0:
            SEG_DATALINE |= SEG_DATA_A | SEG_DATA_B | SEG_DATA_C | SEG_DATA_D | SEG_DATA_E | SEG_DATA_F;
        break;
        case 1:
            SEG_DATALINE |= SEG_DATA_B | SEG_DATA_C;
        break; 
        case 2:
            SEG_DATALINE |= SEG_DATA_A | SEG_DATA_B | SEG_DATA_D | SEG_DATA_E  | SEG_DATA_G;
        break;
        
        case 3:
            SEG_DATALINE |= SEG_DATA_A | SEG_DATA_B | SEG_DATA_C | SEG_DATA_D | SEG_DATA_G;
        break;
        case 4:
            SEG_DATALINE |= SEG_DATA_B | SEG_DATA_C | SEG_DATA_F | SEG_DATA_G;
        break;
        case 5:
            SEG_DATALINE |= SEG_DATA_A | SEG_DATA_C | SEG_DATA_D | SEG_DATA_F | SEG_DATA_G;
        break;
        case 6:
            SEG_DATALINE |= SEG_DATA_C | SEG_DATA_D | SEG_DATA_E | SEG_DATA_F | SEG_DATA_G;
        break;
        case 7:
            SEG_DATALINE |= SEG_DATA_A | SEG_DATA_B | SEG_DATA_C;
        break;
        case 8:
            SEG_DATALINE |= SEG_DATA_A | SEG_DATA_B | SEG_DATA_C | SEG_DATA_D | SEG_DATA_E | SEG_DATA_F | SEG_DATA_G;
        break;
        case 9:
            SEG_DATALINE |= SEG_DATA_A | SEG_DATA_B | SEG_DATA_C | SEG_DATA_F | SEG_DATA_G;
        break;
        default:
        break;
    }  
}

void seg_number_test(void){
    for(uint16_t i = 0; i < 10; i++){
        seg_numbers(i);
        __delay_ms(500);
    } 
}

void seg_convert_num(uint16_t num){
    
    uint16_t rem = num / 1000;
    SEG_SEL0 = 1;
    seg_numbers(rem);
    __delay_us(SEG_DELAY);
    SEG_SEL0 = 0;
    num = num - (rem * 1000);
    
    rem = num / 100;
    SEG_SEL1 = 1;
    seg_numbers(rem);
    __delay_us(SEG_DELAY);
    SEG_SEL1 = 0;
    num = num - (rem * 100);
    
    
    rem = num / 10;
    SEG_SEL2 = 1;
    seg_numbers(rem);
    __delay_us(SEG_DELAY);
    SEG_SEL2 = 0;
    num = num - (rem * 10);
    
    SEG_SEL3 = 1;
    SEG_DATALINE |= SEG_DATA_DP;
    seg_numbers(num);
    __delay_us(SEG_DELAY);
    SEG_SEL3 = 0;
    
    SEG_DATALINE &= 0xFE;
}


void main(void){

    OSCCONbits.IDLEN = 0;
    OSCCONbits.IRCF = 0x07;
    OSCCONbits.SCS = 0x03;
    while(OSCCONbits.IOFS!=1); // 8Mhz

    TRISB = 0x00;
    TRISD &= 0x0F;
    
    LATB = 0x00;
    
    SEG_SEL0 = 0;
    SEG_SEL1 = 0;
    SEG_SEL2 = 0;
    SEG_SEL3 = 0;
    
    uart_init(51,0,1,0);//baud 9600
    
    T0CONbits.TMR0ON=0; 
    T0CONbits.T08BIT=1;
    T0CONbits.T0CS = 0;
    T0CONbits.PSA = 0;
    T0CONbits.T0PS=2; //1:8
    INTCONbits.T0IE=1;
    INTCON2bits.TMR0IP=1;
    
    RCONbits.IPEN = 1;
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;// base interrupt setup
    
    __delay_ms(2000);
    uart_send_string(program_start);
    
    T0CONbits.TMR0ON=1;
    uint16_t seg_counter = 0;
    for(;;){
        
        
        if(tmr0_overflow >= 50){
            seg_counter++;
            tmr0_overflow = 0;
        }
        
        if(seg_counter > 9999){
            seg_counter = 0;
        }
        
        seg_convert_num(seg_counter);
        
        /*
        SEG_SEL0 = 1;
        //led_test();
        seg_number_test();
        SEG_SEL0 = 0;
        
        SEG_SEL1 = 1;
        //led_test();
        seg_number_test();
        SEG_SEL1 = 0;
        
        SEG_SEL2 = 1;
        seg_number_test();
       // led_test();
        SEG_SEL2 = 0;
        
        SEG_SEL3 = 1;
        seg_number_test();
        //led_test();
        SEG_SEL3 = 0;*/
        
        if(uart_rcv_data){
            uart_send(uart_char);
            uart_rcv_data = false;
        }
        
    } 
}



void __interrupt() high_isr(void){
    INTCONbits.GIEH = 0;


    if(PIR1bits.RCIF){
        uart_receiver(&uart_char,&uart_rcv_data);
        PIR1bits.RCIF=0;
    }
    
    if(INTCONbits.TMR0IF){
        tmr0_overflow++;
        INTCONbits.TMR0IF = 0;
    }

    INTCONbits.GIEH = 1;
}

void __interrupt(low_priority) low_isr(void){
    INTCONbits.GIEH = 0;

    if(0){

    }

    INTCONbits.GIEH = 1;
}
