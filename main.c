#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "hd44780.h"
#include "uart.h"

#define KEY (1<<PC0)

uint16_t key_lock;
unsigned int encoder = 0;


//INT0 interrupt
ISR( INT0_vect ) {
    if ( !bit_is_clear( PIND, PD3 ) ) {
        encoder ++;
    } else {
        encoder --;
    }
}

//INT1 interrupt
ISR( INT1_vect ) {
    if ( !bit_is_clear( PIND, PD2 ) ) {
        encoder ++;
    } else {
        encoder --;
    }
}

int main( void ) {

    int securityNumber = 1;

    int encoderResult[3];
    int encoderCount = 0;

    char bufor [4];
    char buforEnc0 [4];
    char buforEnc1 [4];
    char buforEnc2 [4];

    int keypressed=0;
    int keyboardCount=0;
    char keyboardResult[4];
//    uint8_t locked = 0;

    PORTC |= KEY;

    DDRD &= ~( 1 << PD2 );
    DDRD &= ~( 1 << PD3 );
    PORTD |= ( 1 << PD3 ) | ( 1 << PD2 );

    GICR |= ( 1 << INT0 ) | ( 1 << INT1 );
    MCUCR |= ( 1 << ISC01 ) | ( 1 << ISC11 ) | ( 1 << ISC10 );

    sei();

    LCD_Initalize();
    LCD_Home();
    LCD_Clear();
    LCD_GoTo( 0, 0 );

    _delay_ms( 10 );

    while ( 1 ) {

          if(securityNumber == 0){

          if( !key_lock && !(PINC & KEY ) ) {
           key_lock = 65000;

           encoderResult[encoderCount] = encoder;

           if(encoderCount == 0){
               itoa(encoderResult[encoderCount],buforEnc0,10);
               LCD_GoTo( 0, 1 );
               LCD_WriteText( buforEnc0 );
           } else if (encoderCount == 1){
               itoa(encoderResult[encoderCount],buforEnc1,10);
               LCD_GoTo( 3, 1);
               LCD_WriteText( buforEnc1 );
           } else if (encoderCount == 2){
               itoa(encoderResult[encoderCount],buforEnc2,10);
               LCD_GoTo( 6, 1);
               LCD_WriteText( buforEnc2 );
           }

           if(encoderCount < 2){
               encoderCount = encoderCount + 1;
           }else{
               if(encoderResult[0] == 20 && encoderResult[1] == 40 && encoderResult[2] == 60){
                    LCD_GoTo(10,1);
                    LCD_WriteText("OPEN");
                    _delay_ms( 1000 );

                    securityNumber++;
                    LCD_Clear();

               }else{
                    LCD_GoTo(10,1);
                    LCD_WriteText("WRONG");
                    encoderCount = 0;
                    _delay_ms( 1000 );
                    LCD_Clear();
               }
           }

          } else if( key_lock && (PINC & KEY ) ) key_lock++;

        if(encoder > 99){
             encoder = 99;
        }

        if(encoder < 1){
            encoder = 1;
        }


        LCD_GoTo( 0, 0 );
        itoa(encoder,bufor,10);
        if(encoder < 10){
            bufor[1] = bufor[0];
            bufor[0] = '0';
            bufor[2] = ' ';
            bufor[3] = ' ';
        }

        LCD_WriteText( bufor );

        LCD_GoTo( 2, 0 );
        LCD_WriteText("               ");

        }else if (securityNumber == 1){

             LCD_GoTo(0,0);
             LCD_WriteText("Keyboard");

             DDRB=0xF0;
             _delay_ms(1);
             PORTB=0x0F;
             _delay_ms(1);

             keyboardResult[0] = '_';
             keyboardResult[1] = '_';
             keyboardResult[2] = '_';
             keyboardResult[3] = '_';

             LCD_GoTo(0,1);
             LCD_WriteText(keyboardResult);
             LCD_GoTo(4,1);
             LCD_WriteText("              ");


             while(1){

                 if (PINB!=0b11110000){

                     _delay_ms(5);
                     keypressed = PINB;
                     DDRB ^=0b11111111;
                     _delay_ms(1);
                     PORTB ^= 0b11111111;
                     _delay_ms(1);
                     keypressed |=PINB;

                     if (keypressed==0b01111110 && keyboardCount < 4){
                        LCD_GoTo(0,1);
                        keyboardResult[keyboardCount] = '7';
                        LCD_WriteText(keyboardResult);
                        LCD_GoTo(4,1);
                        LCD_WriteText("              ");
                        keyboardCount++;
                     }

                     if (keypressed==0b10111110 && keyboardCount < 4){
                        LCD_GoTo(0,1);
                        keyboardResult[keyboardCount] = '8';
                        LCD_WriteText(keyboardResult);
                        LCD_GoTo(4,1);
                        LCD_WriteText("              ");
                        keyboardCount++;
                     }

                     if (keypressed==0b11011110 && keyboardCount < 4){
                        LCD_GoTo(0,1);
                        keyboardResult[keyboardCount] = '9';
                        LCD_WriteText(keyboardResult);
                        LCD_GoTo(4,1);
                        LCD_WriteText("              ");
                        keyboardCount++;
                     }

                     if (keypressed==0b01111101 && keyboardCount < 4){
                        LCD_GoTo(0,1);
                        keyboardResult[keyboardCount] = '4';
                        LCD_WriteText(keyboardResult);
                        LCD_GoTo(4,1);
                        LCD_WriteText("              ");
                        keyboardCount++;
                     }

                     if (keypressed==0b10111101 && keyboardCount < 4){
                        LCD_GoTo(0,1);
                        keyboardResult[keyboardCount] = '5';
                        LCD_WriteText(keyboardResult);
                        LCD_GoTo(4,1);
                        LCD_WriteText("              ");
                        keyboardCount++;
                     }

                     if (keypressed==0b11011101 && keyboardCount < 4){
                        LCD_GoTo(0,1);
                        keyboardResult[keyboardCount] = '6';
                        LCD_WriteText(keyboardResult);
                        LCD_GoTo(4,1);
                        LCD_WriteText("              ");
                        keyboardCount++;
                     }

                     if (keypressed==0b01111011 && keyboardCount < 4){
                        LCD_GoTo(0,1);
                        keyboardResult[keyboardCount] = '1';
                        LCD_WriteText(keyboardResult);
                        LCD_GoTo(4,1);
                        LCD_WriteText("              ");
                        keyboardCount++;
                     }

                     if (keypressed==0b10111011 && keyboardCount < 4){
                        LCD_GoTo(0,1);
                        keyboardResult[keyboardCount] = '2';
                        LCD_WriteText(keyboardResult);
                        LCD_GoTo(4,1);
                        LCD_WriteText("              ");
                        keyboardCount++;
                     }

                     if (keypressed==0b11011011 && keyboardCount < 4){
                        LCD_GoTo(0,1);
                        keyboardResult[keyboardCount] = '3';
                        LCD_WriteText(keyboardResult);
                        LCD_GoTo(4,1);
                        LCD_WriteText("              ");
                        keyboardCount++;
                     }

                     if (keypressed==0b10110111 && keyboardCount < 4){
                        LCD_GoTo(0,1);
                        keyboardResult[keyboardCount] = '0';
                        LCD_WriteText(keyboardResult);
                        LCD_GoTo(4,1);
                        LCD_WriteText("              ");
                        keyboardCount++;
                     }

                     keypressed=0;
                     DDRB ^=0b11111111;
                     _delay_ms(1);
                     PORTB ^= 0b11111111;
                     _delay_ms(220);

                 }

             }

        }


        _delay_ms( 1 );
    }

    return 0;
}