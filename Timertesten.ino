/*
Name:		Timertesten.ino
Created:	2/3/2017 9:49:42 PM
Author:	Rob Antonisse
*/

/* Beschrijvingen uitgebreid.
Registers voor pinacties

MCUCR – MCU Control Register Bit 4 – PUD: Pull-up Disable dus pull-up weerstenaden allemaal uitzetten, voorlopig nergens voor nodig feb2017

PINS: PB0=PIN8 PB1=PIN9 PB2=PIN10 PB3=PIN11 PB4=PIN12 PB5=PIN13 PB6=XTAL PB7=XTAL
PORTB  // Stand bij output 1=high, 0=low
DDRB direction, 0=input (3 standen) 1=output
PINB  Writing a logic one to PINxn toggles the value of PORTxn, independent on the value of DDRxn. Note that the
SBI instruction can be used to toggle one single bit in a port.

PINS:PC0=PINA0 PC1=PINA1 PC2=PINA2 PC3=PINA3 PC4=PINA4 PC5=PINA5 PC6=Reset PC7=nc
PORTC
DDRC
PINC

PINS: PD0=PIN0 PD1=PIN1 PD2=PIN2 PD#=PIN3 PD4=PIN4 PD5=PIN5 PD6=PIN6 PD7=PIN7
PORTD
DDRD
PIND

TIMERS, bespreking en test met TIMER 1
Registers:
(16bit)Timer/Counter (TCNT1)
(16bit)Output Compare Registers (OCR1A/B)
(16bit)Input Capture Register (ICR1)
(2x8bit)Timer/Counter Control Registers (TCCR1A/B)
Timer Interrupt Flag Register (TIFR1)
Timer Interrupt Mask Register (TIMSK1)
Output Compare pin (OC1A/B) OC1A=PB1=PIN9 OC1B=PB2=PIN10 (PIN 9 en 10 als output definieren )
Compare Match Flag (OCF1A/B)
Input Capture pin (ICP1)


Bewerkekn van registers.
EECR = EECR | (1<<EEWE)
// logical OR what's in EECR with a 1 at the position of EEWE

(Registernaam) PORTB   |= B00000010  zet bit1 van PORTB  of  DDRB &=B11111101 zet bit1 laag... andere bits worden niet veranderd.
of 	bitSet (PORTB,PORTB1);



Altenatieve functies van belang: datasheet ATMEL 14.3.1 Alternate Functions of Port B

*/

#define ledPin 10

void OVERLOOP() {

		pinMode(9, OUTPUT);

		// initialize timer1 
		noInterrupts();           // disable all interrupts
		TCCR1A = 0;
		TCCR1B = 0;

		TCNT1 = 34286;            // preload timer 65536-16MHz/256/2Hz
		TCCR1B |= (1 << CS12);    // 256 prescaler 
		TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
		interrupts();             // enable all interrupts
	}
void PHASECORRECT() {
	//Timer gebruik met phase correct
	//Void hoeft maar 1x uitgevoerd te worden in set up dus pinnen kunnen hier worden gedefinieerd
	//Eerst Pinnen aanzetten, pin 9 en 10 standaard al gedefinieerd.
	DDRB |= (1 << DDB2); // pinMode(10, OUTPUT);
	DDRB |= (1 << DDB1); // pinMode(9, OUTPUT); maar veel sneller
	//TCCR1A – Timer/Counter1 Control Register A set beide COM1A0/COM1B0 voor toggle (omschakelen) bij bereiken Compare match.

}

void setup()
{
	// COMPARE();
	// OVERLOOP();
	// pin 9 is bit 2 in port b dus ... erg langzaam, 
	// pinMode(9, OUTPUT);  of 
	
	
	DDRB |= (1 << DDB2); // pinMode(10, OUTPUT);
	DDRB |= (1 << DDB1); // pinMode(9, OUTPUT); maar veel sneller


    //DDRB |= B00000010; // of decimaal of met deze rare constructie blijkbaar hebben de bits een aanwijsbaar alias
	// DDRB = 2; // lastig  omdat je ALLE poorten goed in de gaten moet hebben dan, bovenstaand is beste of
	//voor toggle bij start
	//bitSet (PORTB, PORTB1);
	//werkt ook, maar als er nu ergens een vautje onstaat bijft alle fout gaan tot reset
	}

void COMPARE() {
	pinMode(10, OUTPUT);

	// initialize timer1 
	noInterrupts();           // disable all interrupts
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;

	OCR1A = 31250;            // compare match register 16MHz/256/2Hz

	TCCR1B |= (1 << WGM12);   // CTC mode
	TCCR1B |= (1 << CS12);    // 256 prescaler 
	TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
	interrupts();
}

ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
	digitalWrite(10, digitalRead(10) ^ 1);   // toggle LED pin
}

ISR(TIMER1_OVF_vect) {
	TCNT1 = 0;
	digitalWrite(9, digitalRead(9) ^ 1);   // toggle LED pin
}

void TOGGLE() {
	// met PINB register leds omschakelen
	bitSet(PINB, PINB1);bitSet(PINB, PINB2);
}

void loop()
{
	//PORTB |= B00000010;  //of
	// PORTB=PORTB |= (1 << PORTB1);
	//PORTB |= (1 << PORTB1);
	//PORTB &= B11111011;
	// PORTB &= (0 << PORTB2); dit werkt niet.... wel alles in 1 keer (1<<DDB3)|(1<<DDB2)|(1<<DDB1)|(1<<DDB0)
	//PORTB = B00000010; // of
	//bitSet(PORTB, PORTB1); bitClear(PORTB, PORTB2);
	//delay(1000);
	//PORTB &= B11111101;
	//PORTB |= (1 << PORTB2);
	//PORTB = B00000100;
	//bitSet(PORTB, PORTB2); bitClear(PORTB, PORTB1);
	//TOGGLE();
	//delay(1000); 
	
}


