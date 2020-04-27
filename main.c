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

    char bufor [4];

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
          if( !key_lock && !(PINC & KEY ) ) {
           key_lock = 50000;
           LCD_GoTo( 0, 1 );
           LCD_WriteText( "PRESS" );
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
        _delay_ms( 1 );
    }

    return 0;
}