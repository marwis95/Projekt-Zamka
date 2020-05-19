/*
 * rfid_em4095.c
 *
 *  Created on: 2012-04-07
 *       Autor: Piotr Rzeszut
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "rfid_em4095.h"

volatile uint64_t RFID_data;//zmienna globalna przechowuj¹ca ostatnio odebrane 64 bity danych

volatile uint8_t RFID_decoded_flag;//flaga ustawiana w celu sygnalizacji odebrania ramki

volatile uint8_t RFID_id[5];//tablica w której zapisywane s¹ zdekodowane dane

void RFID_init() {
    //OUT_DDR &= ~(1<<OUT_PIN);//pin OUT jako wejœcie z podci¹ganiem
    //OUT_PORT |= (1<<OUT_PIN);


    /*
    // 1. Pin MOD na sta³e zwarty do GND
    // 2. Pin SHD podpiêty do uk³adu RC generuj¹cego sygna³ resetuj¹cy POR:
    // VCC-----||---+--[====]-----GND
    //        100nF |   10k
    //              +------------->SHD
    */
    // w poprzedniej wersji wszystkie piny uk³adu by³y podpiête do procesora, który nimi sterowa³:

    CLK_DDR &= ~(1<<CLK_PIN);//pin CLK jako wejœcie
    CLK_PORT |= (1<<CLK_PIN);//podci¹ganie do VCC na pinie CLK
    SHD_DDR |= (1<<SHD_PIN);//piny SHD i MOD jako wyjœcia
    MOD_DDR |= (1<<MOD_PIN);


    MOD_PORT &= ~(1<<MOD_PIN);
    SHD_PORT &= ~(1<<SHD_PIN);
    _delay_ms(100);
    SHD_PORT |= (1<<SHD_PIN);
    _delay_ms(100);
    SHD_PORT &= ~(1<<SHD_PIN);//*/

    //taktowanie z wyjœcia CLK EM4095 by³o stosowane w pierwszej wersji oprogramowania
    TCCR1B |=(1<<CS10)|(1<<CS11)|(1<<CS12);//taktowanie timera z wyjœcia CLK em4095 - synchronizacja czasowa

    //taktowanie z preskalera 16MHz/8=2MHz, timer pracuje w trybie Input Capture (ICR)
    //TCCR1B |=(1<<CS11);//preskaler 8
    TCCR1B &= ~(1<<ICES1);//zbocze opadaj¹ce do pierwszego wyzwolenia

    TIMSK |= (1<<TICIE1);//aktywowanie przerwania
    RFID_decoded_flag= 0;//zerowanie flagi
}

/*
funkcja operujaca na zmiennej globalenj RFID_data
wyszukuje bity startu. Jeœli je odnajdzie to zwraca 1, a w zmiennej RFID_data
mamy wyrównan¹ do lewej strony ramkê
w przeciwnym wypadku fukcja zwraca 0
*/
uint8_t header_align(){
    uint8_t mbit;//bit tymczasowy
    for(uint8_t i=0;i<64;i++){//maksymalnie mo¿emy obróciæ ca³¹ ramkê - 64 bity
        if((RFID_data&SBIT_MASK)==SBIT_MASK){//sprawdzamy czy ramka jest OK
            return 1;//jeœli tak to przerywamy dzia³anie
            //break;
        }//jeœli ramka nie jest ok
        if(RFID_data&MBIT_MASK)mbit=1; else mbit=0;//sprawdzamy najwy¿szy bit
        RFID_data=RFID_data<<1;//przesówamy ramkê o 1 bit w lewo
        if(mbit)RFID_data|=LBIT_MASK;//dopisujemy wczeœniejszy najwy¿szy bit na koniec
    }
    if((RFID_data&SBIT_MASK)==SBIT_MASK){//sprawdzamy po ostatnim przesuniêciu
        return 1;
    }//jeœli nadal nie mamy headeru to ramka by³a b³êdna
    return 0;
}

/*
szybkie obliczanie even parity dla czterech bitów
*/
uint8_t parity_cal(uint8_t value){
    switch(value){
        case 0b0000: return 0;
        case 0b0001: return 1;
        case 0b0010: return 1;
        case 0b0011: return 0;
        case 0b0100: return 1;
        case 0b0101: return 0;
        case 0b0110: return 0;
        case 0b0111: return 1;
        case 0b1000: return 1;
        case 0b1001: return 0;
        case 0b1010: return 0;
        case 0b1011: return 1;
        case 0b1100: return 0;
        case 0b1101: return 1;
        case 0b1110: return 1;
        case 0b1111: return 0;
    }
    return 2;
}

/*
funkcja sprawdza parzystoœæ poziom¹ (P0-P9)
i przy okazji ³¹czy 4-ro bitowe fragmenty danych w pe³ne bajty
*/
uint8_t h_parity(){
    uint8_t ok=1,hbyte=0;
    for(uint8_t i=0;i<10;i++){
        uint8_t par_c,par_r;
        hbyte=(RFID_data&((uint64_t)D10_MASK)<<(i*5))>>(D10_SHIFT+(i*5));//œci¹gamy po³ówkê bajtu
        par_c=parity_cal(hbyte);//obliczamy parzystoœæ even
        par_r=(RFID_data&((uint64_t)P10_MASK)<<(i*5))>>(P10_SHIFT+(i*5));//œci¹gamy odczytan¹ parzystoœæ
        if(par_c!=par_r){//jeœli siê nie zgadza to mamy b³¹d
            ok=0;//parity_error
        }
        if(i%2==0){//przy okazji ³¹czmy po³ówki bajtów w bajty i zapisujemy zdekodowany numer tagu
            RFID_id[i/2]=hbyte;
        }else{
            RFID_id[i/2]|=hbyte<<4;
        }
    }
    return ok;//zwracamy czy parzystoœæ siê zgadza
}

/*
funkcja sprawdza bity parzystoœci pionowej (CP0-CP3)
*/
uint8_t v_parity(){//obliczamy bit parzystoœci pionowej
    for(uint8_t i=0;i<4;i++){
        uint8_t vpar_c=0;
        uint8_t vpar_r;
        for(uint8_t j=0;j<5;j++){//obliczamy na podstawie po³¹czonych bajtów
            if(RFID_id[j]&(0b10000000>>i))vpar_c^=0x01;
            if(RFID_id[j]&(0b00001000>>i))vpar_c^=0x01;
        }
        //œci¹gamy odczytany bit parzystoœci
        vpar_r=(RFID_data&((uint64_t)CP4_MASK)<<(3-i))>>(CP4_SHIFT+(3-i));
        if(vpar_r!=vpar_c){//sprawdzamy - pierwszy b³¹d i nie ma sensu sprawdzaæ dalej
            return 0;
        }
    }
    return 1;
}

//dekodowanie w przerwaniu ICR1
ISR(TIMER1_CAPT_vect){

    static uint16_t LastICR;//ostatnia wartoœæ przechwycenia
           uint16_t PulseWidth;//szerokoœæ impulsu miêdzy zboczami
    static uint8_t  EdgeCt;//licznik zbocz
    static uint8_t  BitCt;//licznik bitów odebranych
    static uint8_t  BitVal;//wartoœæ aktualnie przetwarzanego bitu
           uint8_t  decode_ok;//zmienna tymczasowa pomagaj¹ca obs³ugiwaæ kontrolê poprawnoœci danych
    static uint64_t RFID_tmp;//tymczasowa zmienna do której zapisywane s¹ kolejno odbierane bity

    PulseWidth = ICR1 - LastICR;//szerokoœæ impulsu
    LastICR=ICR1;//zapisujemy dane tego zbocza

    TCCR1B ^= (1<<ICES1);//zmiana zbocza wyzwalaj¹cego na przeciwne


    if(EdgeCt == 0){//jeœli system dekodowania zosta³ wyzerowany
        BitCt=0;//resetujemy wszystkie zmienne
        BitVal=1;
        RFID_tmp=0;
    }

    if(PulseWidth < MIN_HALF_BIT || PulseWidth > MAX_BIT){//impuls za d³ugi lub za krótki - reset dekodowania
        EdgeCt=0;
    }else if(PulseWidth >= MIN_HALF_BIT && PulseWidth <= MAX_HALF_BIT){//impuls krótki (1/2 CLK)
        if(EdgeCt % 2  == 0){//jeœli to parzyste zbocze
            RFID_tmp<<=1;//to zapisujemy bit
            RFID_tmp|=(uint64_t)BitVal;
            BitCt++;//i zwiêkszamy licznik odebranych bitów
        }
        EdgeCt++;//zwiêkszamy licznik zbocz
    }else{//przeciwny wypadek - (PulseWidth > MAX_HALF_BIT && PulseWidth < MAX_BIT)
        //czyli d³ugi impuls (1 CLK)
        BitVal^=0x01;//zmieniamy wartoœæ aktualnie przetwarzanego bitu
        RFID_tmp<<=1;//i zapisujemy bit
        RFID_tmp|=(uint64_t)BitVal;
        BitCt++;//i zwiêkszamy licznik odebranych bitów
        EdgeCt+=2;//i zwiêkszamy licznik zbocz o 2 (dla porz¹dku)
    }

    if(BitCt>64){//jeœli odebraliœmy ca³¹ ramkê
        EdgeCt=0;//resetujemy system do obdioru kolejnej
        if (RFID_decoded_flag == 0) {//i jeœli poprzednie dekodowanie zosta³o odebrane w programie
            RFID_data=RFID_tmp;//to zapisujemy ramkê w formacie niezdekodowanym
            decode_ok=header_align();//i dekodujemy j¹
            if(decode_ok)decode_ok=h_parity();
            if(decode_ok)decode_ok=v_parity();
            if(!decode_ok){//jeœli dekodowanie posz³o nie tak jak trzeba
                RFID_data=~RFID_data;//to negujemy ramkê (mo¿e zaczêliœmy dekodowanie od niew³aœciwego zbocza)
                decode_ok=header_align();//i ponawiamy próbê dekodowania
                if(decode_ok)decode_ok=h_parity();
                if(decode_ok)decode_ok=v_parity();
            }
            RFID_decoded_flag=decode_ok;//i przypisujemy fladze to czy zdekodowano poprawnie ramkê, czy nie
        }
    }

}
