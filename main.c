#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "hd44780.h"

#include "uart.h"

//INT0 interrupt
ISR(INT0_vect )
{
    if(!bit_is_clear(PIND, PD3))
    {
            LCD_Clear();
    LCD_GoTo(0,0);
    LCD_WriteText("+");
    }
    else
    {
            LCD_Clear();
    LCD_GoTo(0,0);
    LCD_WriteText("-");
    }
}

//INT1 interrupt
ISR(INT1_vect )
{
    if(!bit_is_clear(PIND, PD2))
    {
            LCD_Clear();
    LCD_GoTo(0,0);
    LCD_WriteText("+");
    }
    else
    {
           LCD_Clear();
    LCD_GoTo(0,0);
    LCD_WriteText("-");
    }
}

int main(void)
{
    LCD_Initalize();
    LCD_Home();
    LCD_Clear();
    LCD_GoTo(0,0);
    LCD_WriteText("INIT");

  DDRD &=~ (1 << PD2);
  DDRD &=~ (1 << PD3);
  PORTD |= (1 << PD3)|(1 << PD2);

  GICR |= (1<<INT0)|(1<<INT1);
  MCUCR |= (1<<ISC01)|(1<<ISC11)|(1<<ISC10);

  sei();


   while(1)
   {
    _delay_ms(1);
   }

  return 0;
}


/*
#include <avr/io.h>
#include "hd44780.h"
#include <stdio.h>
#include <util/delay.h>

int main(void){
    LCD_Initalize();
    LCD_Home();
    LCD_Clear();
    LCD_GoTo(0,0);
    LCD_WriteText("test");
    LCD_GoTo(0,1);
    LCD_WriteText("xD");
    for(;;);
}
*/