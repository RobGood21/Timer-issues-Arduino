/*
Name:		Timertesten.ino
Created:	2/3/2017 9:49:42 PM
Author:	Rob Antonisse
*/

/* Beschrijvingen uitgebreid.
Registers voor pinacties

MCUCR � MCU Control Register Bit 4 � PUD: Pull-up Disable dus pull-up weerstenaden allemaal uitzetten, voorlopig nergens voor nodig feb2017

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

prescaler TCCR1B  bit2, CS12  bit1, CS11  bit0, CS10 1=001 8=010 64=011 256=100 1024=101


Bewerkekn van registers.
EECR = EECR | (1<<EEWE)
// logical OR what's in EECR with a 1 at the position of EEWE

(Registernaam) PORTB   |= B00000010  zet bit1 van PORTB  of  DDRB &=B11111101 zet bit1 laag... andere bits worden niet veranderd.
of 	bitSet (PORTB,PORTB1);

GPIOR0 � General Purpose I/O Register 0
GPIOR1  8bits 
GPIOR2

Altenatieve functies van belang: datasheet ATMEL 14.3.1 Alternate Functions of Port B

*/



//DECLARTATIES
//voor Registers
byte registertest;

void TIMERTEST() {
	//Timer gebruik met phase correct
	//Void hoeft maar 1x uitgevoerd te worden in set up dus pinnen kunnen hier worden gedefinieerd
	//Eerst Pinnen aanzetten, pin 9 en 10 standaard al gedefinieerd.


	//Compare Output Mode, Phase Correct and Phase and Frequency Correct PWM(1)
	//Set in TCCR1A beide COM1A0 en COM1B0
	//TCCR1A � Timer/Counter1 Control Register A set beide COM1A0 COM1B0 voor toggle (omschakelen) bij bereiken Compare match.

	// Bit 1:0 � WGM11 : 0 : Waveform Generation Mode sets samen met TCCR1B. deze mode... hier kiezen voor mode
	//WGM13 1=TCCR1B bit4, WGM12 0=TCCR1B bit3, WGM11 1=TCCRA1 bit1, WGM10 1 =TCCRA1 bit0,
	//mode CTC 4 set WGM12
	//TCCR1A = 0; //initialiseren voor de zekerheid...
	//TCCR1B |= (1 << WGM13),

	//TCCR1A |= (1 << WGM12); //TCCR1A |= (1 << WGM11); TCCR1A |= (1 << WGM10);
		
	//TCCR1A |= (1 << COM1A0); TCCR1A |= (1 << COM1B0); 
	
	//TCCR1A |= (1 << COM1A1); TCCR1B |= (1 << COM1B1);


	
	
	//TCCR1B � Timer/Counter1 Control Register B 
	//prescaler even max zetten om de leds te kunnen zien knipperen, dus prescaler naar 1024 dus set CS12 en CS10
			//TCCR1B |= (1 << CS12); TCCR1B |= (1 << CS10);
	
	//16.11.3 TCCR1C � Timer/Counter1 Control Register C niet duidelijk of hier iets buikbaars bij is.
	//TCNT1H and TCNT1L � Timer/Counter1 de feitelijk counter, voor timer 1 16bits, timer 0 en 2 alleen 1 byte
	// bij voorkeur iets verzinnen dat deze timer niet opnieuw hoeft in te stellen, we kiezen 232, geeft gewenste tijd 116 micros  bij prescaler 8
	
	//TCNT1 = 6000;
	

	//OCR1AH and OCR1AL � Output Compare Register 1 A ;//vergelijk met waarde counter kanaal A
	//OCR1A = 3125;
	//OCR1B = 1160;

	//OCR1BH and OCR1BL � Output Compare Register 1 B ; //vergelijk met waarde counter kanaal B
	//ICR1H and ICR1L � Input Capture Register 1 

	//TIMSK1 � Timer/Counter1 Interrupt Mask Register
	//bit5=ICIE1 (enabled input interrupt) bit2=OCIE1B (enabled Output Compare B Match interrupt)  bit1 = OCIE1A (enabled Output Compare A Match interrupt) 
	//bit0=TOIE1 (enabled Timer/Counter1 Overflow interrupt)
	//TIFR1 � Timer/Counter1 Interrupt Flag Register bit5=ICF1  bit2=OCF1B bit1=OCF1A bit0=TOV1
	//PIN als output



	//Portten als output instellen
	DDRB |= (1 << DDB0); // pinMode(8, OUTPUT);
	DDRB |= (1 << DDB1); // pinMode(9, OUTPUT); maar veel sneller
	DDRB |= (1 << DDB2);
	
	
	noInterrupts(); //ff tegenhouden
	
	//PORTB |= (1 << PORTB1);
	//prescaler instellen

	//TCCR1A = 0;
	//TCCR1B = 0;

	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1A |= (1 << COM1A0);// TCCR1A |= (1 << COM1B0);// gebruikt pin 9 en 10 als output

	TCCR1B |= (1 << CS12); // prescaler op 256 TCCR1B | (1 << CS10);
	TCCR1B |= (1 << WGM12); //CTC mode		
	
	OCR1A = 50000; //timer kanaal A
	OCR1B = 10000; //timer kanaal B
	TCNT1 = 0;
	// TIMSK1 |= (1 << TOIE1);TIMSK1 |= (1 << OCIE1A);TIMSK1 |= (1 << OCIE1B); // interrupts toestaan

	interrupts();
	
}

void VOORBEELD() {
	
		pinMode(8, OUTPUT);

		// initialize timer1 
		noInterrupts();           // disable all interrupts
		TCCR1A = 0;
		TCCR1B = 0;
		TCNT1 = 0;

		OCR1A = 31250;            // compare match register 16MHz/256/2Hz
		TCCR1B |= (1 << WGM12);   // CTC mode
		TCCR1B |= (1 << CS12);    // 256 prescaler 
		TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
		interrupts();             // enable all interrupts
	}

void TRAIN() { //set timer en outputs, de trein van datapulsen
	DDRB |= (1 << DDB1);
	DDRB |= (1 << DDB2);
	noInterrupts(); //interrupts tijdelijk ondebreken
	TCCR1A = 0; //initialiseer register NODIG!
	TCCR1B = 0; //initialiseer register NODIG!
	TCCR1B |= (1 << WGM12); //CTC mode	
	TCCR1B |= (1 << CS12); //zet prescaler (256?)
	OCR1A = 62500;//zet TOP waarde counter bij prescaler 256 1sec
	TCNT1 = 0; //set timer1 op 0 BOTTOM waarde counter
	TCCR1A |= (1 << COM1A0);//zet PIN 9 aan de comparator, bij true toggle output
	TIMSK1 |= (1 << OCIE1A); //inerrupt op bereiken TOPwaarde
	interrupts(); //interupts weer toestaan
}
ISR(TIMER1_COMPA_vect)  
// timer compare interrupt service routine
//If the interrupt is enabled, the interrupt handler routine can be used for updating the TOP value.
// dus hiermee een 1 of een 0 als volgende bit in te stellen???
{
	PORTB ^= (PORTB1 << PORTB2); //Sets Port 2 (PIN10) opposit to PIN9 
}



void setup()
{
	//TRAIN();
	//TIMERTEST();
	//VOORBEELD();
	// COMPARE();
	// OVERLOOP();
	// pin 9 is bit 2 in port b dus ... erg langzaam, 
	// pinMode(9, OUTPUT);  of 
	
	//DDRB |= (1 << DDB1); // pinMode(9, OUTPUT); maar veel sneller


    //DDRB |= B00000010; // of decimaal of met deze rare constructie blijkbaar hebben de bits een aanwijsbaar alias
	// DDRB = 2; // lastig  omdat je ALLE poorten goed in de gaten moet hebben dan, bovenstaand is beste of
	//voor toggle bij start
	//bitSet (PORTB, PORTB1);
	//werkt ook, maar als er nu ergens een vautje onstaat bijft alle fout gaan tot reset

registertest = B00000000;
pinMode(8, OUTPUT);
Serial.begin(9600);
}





void REGISTERS() {
	//kijken of een register als boolean kan worden gebruikt
	// eerst ff met DATAtype BYTE
	//BYTE in declareren
	/*
	registertest ^= (1 << 0);
	delay(1000);
	digitalWrite(8, bitRead(registertest, 0));
	Serial.println(bitRead(registertest, 0));
	*/
	//of met register...?

	GPIOR0 ^= (1 << 0); //toggle bit 0 in dit register....
	delay(1000);
	digitalWrite(8, bitRead(GPIOR0, 0));
	

}





ISR(TIMER1_COMPB_vect) {
PORTB ^= (1 << PORTB1);

}

ISR(TIMER1_OVF_vect) {
	PORTB = (1 << PORTB2);

}

void TOGGLE() {
	// met PINB register leds omschakelen
	//bitSet(PINB, PINB1);bitSet(PINB, PINB2);
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
	//PORTB ^= (1 << PORTB1);
	//delay(1000); 
	REGISTERS();

	
}


