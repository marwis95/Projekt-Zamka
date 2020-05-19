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

volatile uint64_t RFID_data;//zmienna globalna przechowuj�ca ostatnio odebrane 64 bity danych

volatile uint8_t RFID_decoded_flag;//flaga ustawiana w celu sygnalizacji odebrania ramki

volatile uint8_t RFID_id[5];//tablica w kt�rej zapisywane s� zdekodowane dane

void RFID_init() {
    //OUT_DDR &= ~(1<<OUT_PIN);//pin OUT jako wej�cie z podci�ganiem
    //OUT_PORT |= (1<<OUT_PIN);


    /*
    // 1. Pin MOD na sta�e zwarty do GND
    // 2. Pin SHD podpi�ty do uk�adu RC generuj�cego sygna� resetuj�cy POR:
    // VCC-----||---+--[====]-----GND
    //        100nF |   10k
    //              +------------->SHD
    */
    // w poprzedniej wersji wszystkie piny uk�adu by�y podpi�te do procesora, kt�ry nimi sterowa�:

    CLK_DDR &= ~(1<<CLK_PIN);//pin CLK jako wej�cie
    CLK_PORT |= (1<<CLK_PIN);//podci�ganie do VCC na pinie CLK
    SHD_DDR |= (1<<SHD_PIN);//piny SHD i MOD jako wyj�cia
    MOD_DDR |= (1<<MOD_PIN);


    MOD_PORT &= ~(1<<MOD_PIN);
    SHD_PORT &= ~(1<<SHD_PIN);
    _delay_ms(100);
    SHD_PORT |= (1<<SHD_PIN);
    _delay_ms(100);
    SHD_PORT &= ~(1<<SHD_PIN);//*/

    //taktowanie z wyj�cia CLK EM4095 by�o stosowane w pierwszej wersji oprogramowania
    TCCR1B |=(1<<CS10)|(1<<CS11)|(1<<CS12);//taktowanie timera z wyj�cia CLK em4095 - synchronizacja czasowa

    //taktowanie z preskalera 16MHz/8=2MHz, timer pracuje w trybie Input Capture (ICR)
    //TCCR1B |=(1<<CS11);//preskaler 8
    TCCR1B &= ~(1<<ICES1);//zbocze opadaj�ce do pierwszego wyzwolenia

    TIMSK |= (1<<TICIE1);//aktywowanie przerwania
    RFID_decoded_flag= 0;//zerowanie flagi
}

/*
funkcja operujaca na zmiennej globalenj RFID_data
wyszukuje bity startu. Je�li je odnajdzie to zwraca 1, a w zmiennej RFID_data
mamy wyr�wnan� do lewej strony ramk�
w przeciwnym wypadku fukcja zwraca 0
*/
uint8_t header_align(){
    uint8_t mbit;//bit tymczasowy
    for(uint8_t i=0;i<64;i++){//maksymalnie mo�emy obr�ci� ca�� ramk� - 64 bity
        if((RFID_data&SBIT_MASK)==SBIT_MASK){//sprawdzamy czy ramka jest OK
            return 1;//je�li tak to przerywamy dzia�anie
            //break;
        }//je�li ramka nie jest ok
        if(RFID_data&MBIT_MASK)mbit=1; else mbit=0;//sprawdzamy najwy�szy bit
        RFID_data=RFID_data<<1;//przes�wamy ramk� o 1 bit w lewo
        if(mbit)RFID_data|=LBIT_MASK;//dopisujemy wcze�niejszy najwy�szy bit na koniec
    }
    if((RFID_data&SBIT_MASK)==SBIT_MASK){//sprawdzamy po ostatnim przesuni�ciu
        return 1;
    }//je�li nadal nie mamy headeru to ramka by�a b��dna
    return 0;
}

/*
szybkie obliczanie even parity dla czterech bit�w
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
funkcja sprawdza parzysto�� poziom� (P0-P9)
i przy okazji ��czy 4-ro bitowe fragmenty danych w pe�ne bajty
*/
uint8_t h_parity(){
    uint8_t ok=1,hbyte=0;
    for(uint8_t i=0;i<10;i++){
        uint8_t par_c,par_r;
        hbyte=(RFID_data&((uint64_t)D10_MASK)<<(i*5))>>(D10_SHIFT+(i*5));//�ci�gamy po��wk� bajtu
        par_c=parity_cal(hbyte);//obliczamy parzysto�� even
        par_r=(RFID_data&((uint64_t)P10_MASK)<<(i*5))>>(P10_SHIFT+(i*5));//�ci�gamy odczytan� parzysto��
        if(par_c!=par_r){//je�li si� nie zgadza to mamy b��d
            ok=0;//parity_error
        }
        if(i%2==0){//przy okazji ��czmy po��wki bajt�w w bajty i zapisujemy zdekodowany numer tagu
            RFID_id[i/2]=hbyte;
        }else{
            RFID_id[i/2]|=hbyte<<4;
        }
    }
    return ok;//zwracamy czy parzysto�� si� zgadza
}

/*
funkcja sprawdza bity parzysto�ci pionowej (CP0-CP3)
*/
uint8_t v_parity(){//obliczamy bit parzysto�ci pionowej
    for(uint8_t i=0;i<4;i++){
        uint8_t vpar_c=0;
        uint8_t vpar_r;
        for(uint8_t j=0;j<5;j++){//obliczamy na podstawie po��czonych bajt�w
            if(RFID_id[j]&(0b10000000>>i))vpar_c^=0x01;
            if(RFID_id[j]&(0b00001000>>i))vpar_c^=0x01;
        }
        //�ci�gamy odczytany bit parzysto�ci
        vpar_r=(RFID_data&((uint64_t)CP4_MASK)<<(3-i))>>(CP4_SHIFT+(3-i));
        if(vpar_r!=vpar_c){//sprawdzamy - pierwszy b��d i nie ma sensu sprawdza� dalej
            return 0;
        }
    }
    return 1;
}

//dekodowanie w przerwaniu ICR1
ISR(TIMER1_CAPT_vect){

    static uint16_t LastICR;//ostatnia warto�� przechwycenia
           uint16_t PulseWidth;//szeroko�� impulsu mi�dzy zboczami
    static uint8_t  EdgeCt;//licznik zbocz
    static uint8_t  BitCt;//licznik bit�w odebranych
    static uint8_t  BitVal;//warto�� aktualnie przetwarzanego bitu
           uint8_t  decode_ok;//zmienna tymczasowa pomagaj�ca obs�ugiwa� kontrol� poprawno�ci danych
    static uint64_t RFID_tmp;//tymczasowa zmienna do kt�rej zapisywane s� kolejno odbierane bity

    PulseWidth = ICR1 - LastICR;//szeroko�� impulsu
    LastICR=ICR1;//zapisujemy dane tego zbocza

    TCCR1B ^= (1<<ICES1);//zmiana zbocza wyzwalaj�cego na przeciwne


    if(EdgeCt == 0){//je�li system dekodowania zosta� wyzerowany
        BitCt=0;//resetujemy wszystkie zmienne
        BitVal=1;
        RFID_tmp=0;
    }

    if(PulseWidth < MIN_HALF_BIT || PulseWidth > MAX_BIT){//impuls za d�ugi lub za kr�tki - reset dekodowania
        EdgeCt=0;
    }else if(PulseWidth >= MIN_HALF_BIT && PulseWidth <= MAX_HALF_BIT){//impuls kr�tki (1/2 CLK)
        if(EdgeCt % 2  == 0){//je�li to parzyste zbocze
            RFID_tmp<<=1;//to zapisujemy bit
            RFID_tmp|=(uint64_t)BitVal;
            BitCt++;//i zwi�kszamy licznik odebranych bit�w
        }
        EdgeCt++;//zwi�kszamy licznik zbocz
    }else{//przeciwny wypadek - (PulseWidth > MAX_HALF_BIT && PulseWidth < MAX_BIT)
        //czyli d�ugi impuls (1 CLK)
        BitVal^=0x01;//zmieniamy warto�� aktualnie przetwarzanego bitu
        RFID_tmp<<=1;//i zapisujemy bit
        RFID_tmp|=(uint64_t)BitVal;
        BitCt++;//i zwi�kszamy licznik odebranych bit�w
        EdgeCt+=2;//i zwi�kszamy licznik zbocz o 2 (dla porz�dku)
    }

    if(BitCt>64){//je�li odebrali�my ca�� ramk�
        EdgeCt=0;//resetujemy system do obdioru kolejnej
        if (RFID_decoded_flag == 0) {//i je�li poprzednie dekodowanie zosta�o odebrane w programie
            RFID_data=RFID_tmp;//to zapisujemy ramk� w formacie niezdekodowanym
            decode_ok=header_align();//i dekodujemy j�
            if(decode_ok)decode_ok=h_parity();
            if(decode_ok)decode_ok=v_parity();
            if(!decode_ok){//je�li dekodowanie posz�o nie tak jak trzeba
                RFID_data=~RFID_data;//to negujemy ramk� (mo�e zacz�li�my dekodowanie od niew�a�ciwego zbocza)
                decode_ok=header_align();//i ponawiamy pr�b� dekodowania
                if(decode_ok)decode_ok=h_parity();
                if(decode_ok)decode_ok=v_parity();
            }
            RFID_decoded_flag=decode_ok;//i przypisujemy fladze to czy zdekodowano poprawnie ramk�, czy nie
        }
    }

}
