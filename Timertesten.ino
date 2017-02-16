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

PINS: PD0=PIN0 PD1=PIN1 PD2=PIN2 PD4=PIN3 PD4=PIN4 PD5=PIN5 PD6=PIN6 PD7=PIN7
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

GPIOR0 – General Purpose I/O Register 0
GPIOR1  8bits 
GPIOR2

Altenatieve functies van belang: datasheet ATMEL 14.3.1 Alternate Functions of Port B

*/



//DECLARTATIES
//voor Registers
//byte registertest;

void TIMERTEST() {
	//Timer gebruik met phase correct
	//Void hoeft maar 1x uitgevoerd te worden in set up dus pinnen kunnen hier worden gedefinieerd
	//Eerst Pinnen aanzetten, pin 9 en 10 standaard al gedefinieerd.


	//Compare Output Mode, Phase Correct and Phase and Frequency Correct PWM(1)
	//Set in TCCR1A beide COM1A0 en COM1B0
	//TCCR1A – Timer/Counter1 Control Register A set beide COM1A0 COM1B0 voor toggle (omschakelen) bij bereiken Compare match.

	// Bit 1:0 – WGM11 : 0 : Waveform Generation Mode sets samen met TCCR1B. deze mode... hier kiezen voor mode
	//WGM13 1=TCCR1B bit4, WGM12 0=TCCR1B bit3, WGM11 1=TCCRA1 bit1, WGM10 1 =TCCRA1 bit0,
	//mode CTC 4 set WGM12
	//TCCR1A = 0; //initialiseren voor de zekerheid...
	//TCCR1B |= (1 << WGM13),

	//TCCR1A |= (1 << WGM12); //TCCR1A |= (1 << WGM11); TCCR1A |= (1 << WGM10);
		
	//TCCR1A |= (1 << COM1A0); TCCR1A |= (1 << COM1B0); 
	
	//TCCR1A |= (1 << COM1A1); TCCR1B |= (1 << COM1B1);


	
	
	//TCCR1B – Timer/Counter1 Control Register B 
	//prescaler even max zetten om de leds te kunnen zien knipperen, dus prescaler naar 1024 dus set CS12 en CS10
			//TCCR1B |= (1 << CS12); TCCR1B |= (1 << CS10);
	
	//16.11.3 TCCR1C – Timer/Counter1 Control Register C niet duidelijk of hier iets buikbaars bij is.
	//TCNT1H and TCNT1L – Timer/Counter1 de feitelijk counter, voor timer 1 16bits, timer 0 en 2 alleen 1 byte
	// bij voorkeur iets verzinnen dat deze timer niet opnieuw hoeft in te stellen, we kiezen 232, geeft gewenste tijd 116 micros  bij prescaler 8
	
	//TCNT1 = 6000;
	

	//OCR1AH and OCR1AL – Output Compare Register 1 A ;//vergelijk met waarde counter kanaal A
	//OCR1A = 3125;
	//OCR1B = 1160;

	//OCR1BH and OCR1BL – Output Compare Register 1 B ; //vergelijk met waarde counter kanaal B
	//ICR1H and ICR1L – Input Capture Register 1 

	//TIMSK1 – Timer/Counter1 Interrupt Mask Register
	//bit5=ICIE1 (enabled input interrupt) bit2=OCIE1B (enabled Output Compare B Match interrupt)  bit1 = OCIE1A (enabled Output Compare A Match interrupt) 
	//bit0=TOIE1 (enabled Timer/Counter1 Overflow interrupt)
	//TIFR1 – Timer/Counter1 Interrupt Flag Register bit5=ICF1  bit2=OCF1B bit1=OCF1A bit0=TOV1
	//PIN als output



	//Portten als output instellen
	//DDRB |= (1 << DDB0); // pinMode(8, OUTPUT);
	//DDRB |= (1 << DDB1); // pinMode(9, OUTPUT); maar veel sneller
	//DDRB |= (1 << DDB2);
	
	
	//noInterrupts(); //ff tegenhouden
	
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

//declaraties voor LOOP()

//Decalarties voor void TRAIN()
volatile boolean BUT1OS = true;
volatile boolean BUT1 = false;
volatile int SCOUNTER = 0;
int CS = 0; //Commandstatus
int i; //teller voor loop functies

//TRAINLOOP
//declaraties tbv TRAIN() in LOOP()
boolean NEWCOMMAND = false;
byte AD = B11111111;
byte DT = B00000000;
byte ER = B11111111;
int BC = 0; //=ByteCount Aantal verzonden bytes
int BT = 3; //ByteTotal =aantal te verzenden bytes
boolean BF;


//xxxdeclaraties voor testen
boolean DEBUG = false; //false; //toont alle teksten als true

void RUNTRAIN(boolean onoff) { //start of stopt de controller

	digitalWrite(7, onoff); //zet gewoon de H-brug uit

cli(); //disable interrupts
/*
	if (onoff == true) {

	TRAIN(); //start de controller
	}
	else {

		TCCR1A = 0; //initialiseer register NODIG!
		TCCR1B = 0; //initialiseer register NODIG!
		TIMSK1 = 0; //Interupts uitzetten

		TCCR1A &= ~(1 << COM1A0);//verbreek PIN 9 van de comparator
		//TCCR1A &= ~(1 << COM1A1);
		TIMSK1 &= ~(1 << OCIE1A); //ISR(TIMER1_COMPA_vect) disable
		
		 //PORTB &= ~(1 << PORTB1); //PIN9 low zetten. dit wer
		 //PORTB &= ~(1 << PORTB2); //PIN10 LOW zetten.

		//bitClear(PORTB, PORTB1); //Werkt ook
		//bitClear(PORTB, PORTB2);

			//digitalWrite(9, LOW); // werkt ook
			//digitalWrite(10, LOW);
	}
sei(); //enable interupts

*/
}

void TRAIN() { //set timer en outputs, de trein van datapulsen
	//De General Purpose I/O Registers op nul stellen
	cli(); // geen interrupts

	GPIOR0 = 0; //General purpose register 0, holds Byte to be send
	GPIOR1 = 0; //General purpose register 0, holds Byte to be send
	GPIOR2 = 0; //General Purpose register holds Flags, booleans.

	DDRB |= (1 << DDB1); //set PIN 9 als output Aangestuurd uit OCRA1
	DDRB |= (1 << DDB2); //set PIN 10 als output, wordt getoggled in de ISR


	 //interrupts tijdelijk ondebreken
	TCCR1A = 0; //initialiseer register NODIG!
	TCCR1B = 0; //initialiseer register NODIG!
	TCCR1B |= (1 << WGM12); //CTC mode	
	TCCR1B |= (1 << CS11); //CS12 zet prescaler (256?) CS11 voor prescaler 8
	OCR1A=116; //zet TOP waarde counter bij prescaler 256 1sec (standard timing true bit)
	TCNT1 = 0; //set timer1 op 0 BOTTOM waarde counter
	TCCR1A |= (1 << COM1A0);//zet PIN 9 aan de comparator, bij true toggle output
	TIMSK1 |= (1 << OCIE1A); //interrupt op bereiken TOPwaarde
	PORTB |= (1 << PORTB1); //clear port B1 (pin9)
	//PORTB |= (1 << PORTB2); //set portb2 1 (pin10)
	sei(); //interupts weer toestaan
	
}

ISR(TIMER1_COMPA_vect)  {// timer compare interrupt service routine

	cli();

	PORTB ^= (1 << PORTB2); //toggles PIN10


	GPIOR2 ^= (1 << 0); //toggle het BITPART deel, als BITPART=false dan is het bit klaar >> nieuw bit bepalen.


	if (bitRead(GPIOR2, 0) == true) { //bit is klaar .. verder

		switch (CS) { //CS=Commandstatus ) 0=Wacht op Command; 1=Preample zenden; 2=Tussenbit zenden; 3=Byte verzenden 
		case 0: //Wacht
			//Topwaarde wordt niet aangepast, timer gaat gewoon door met trues zenden	
			//test Commandready

if (DEBUG==true) Serial.println("wacht op COMMAND");
			
				if (bitRead(GPIOR2, 1) == true) //COMMANDready 
			{
				CS = 1; //na volgende true bit preample verzenden 
				i = 0; //teller reset
			}
			break;// (0)

		case 1: //send preample
			//registers resetten
			bitClear(GPIOR2, 3); // niet nodig?

if (DEBUG == true) {
	Serial.print("Preample: "); 
	Serial.println(i);
}
			if (i < 14) {
				i++;
			}
			else {
				CS = 2; //15 true bits verzonden nu een false bit
				i = 7; //teller al instellen voor 1e byte
			//	Serial.println("");
			}
			break; //(1)

		case 2: //send Tussenbit

if (DEBUG == true) {
	Serial.print("Laatstebyte LOOP():");	
	Serial.print(bitRead(GPIOR2, 2)); 
	Serial.print("    Laatste Byte interrupt:  "); 
	Serial.println(bitRead(GPIOR2, 3));
}

			if (bitRead(GPIOR2, 3) == true) { //Laatste byte interrupt = true, laatste byte is verzonden nu uitspringen
				if (bitRead(OCR1AL, 7) == true) OCR1A = (OCR1A >> 1);
				bitClear(GPIOR2, 1); //COMMANDready naar false, dus GEEN command in behandeling, LOOP() zet deze weer true als een command in de byte registers is gezet.
				CS = 0; //volgende doorlopp wachten op nieuw command..
				GPIOR2 |= (1 << 4); //reset Bytefree register GPIOR0
				GPIOR2 |= (1 << 5); //reset BYTEfree register GPIOR1
				GPIOR2 &= (0 << 6);

if(DEBUG==true) Serial.println("Command ready");
										
			}
			else {
				if (bitRead(OCR1AL, 7) == false) OCR1A = (OCR1A << 1);
if (DEBUG==true) Serial.println("tussenbit");

				CS = 3; //volgende doorloop naar byte verzenden.
				i = 7; //reset bitcounter
				if (bitRead(GPIOR2, 2) == true) bitSet(GPIOR2, 3); //Laatste byte  flag true, verkregen uit LOOP() bij volgende doorloop van T(ussenbit) uitspringen, 
				//true bits zenden wachten op nieuw command
			}


			break;//(2)

		case 3: //send Byte MSB first

if (DEBUG == true) {
	Serial.print("byte:  ");	
	Serial.print(bitRead(GPIOR2, 6)); 
	Serial.print("   Bit:  "); Serial.print(i);
}

			if (bitRead(GPIOR2, 6) == false) { //BYTEpointer true= GPIOR0, false = GPIOR1,  bepalen welke byteregister wordt verstuurd

if (DEBUG == true) {
	Serial.print("   Bitwaarde:  ");
	Serial.println(bitRead(GPIOR0, i));
}

				if (bitRead(GPIOR0, i) == true) {
					if (bitRead(OCR1AL, 7) == true) OCR1A = (OCR1A >> 1);

				}
				else {
					if (bitRead(OCR1AL, 7) == false) OCR1A = (OCR1A << 1);

				}

				i--;
				if (i < 0) bitSet(GPIOR2, 4); //dit was het laatste bit dus BYTEfree wordt true

			}
			else { //GPIOR1 register sturen

if (DEBUG == true) {
	Serial.print("   Bitwaarde:  "); 
	Serial.println(bitRead(GPIOR1, i));
}
				if (bitRead(GPIOR1, i) == true) {
					if (bitRead(OCR1AL, 7) == true) OCR1A = (OCR1A >>1);
				}
				else {
					if (bitRead(OCR1AL, 7) == false) OCR1A = (OCR1A << 1);
					
				}

				i--;
				if (i < 0) bitSet(GPIOR2, 5);				
			}			
			if (i < 0) {
				GPIOR2 ^= (1 << 6); //toggle bytepointer				
				CS = 2; //naar tussenbit
if(DEBUG==true)	Serial.println("Bytepointer getoggeld");
			}
					
			break; //(3)
			}

	} //bit is niet klaar, gebeurt gewoon niks
	
	//ff ledje laten branden op deze toggle
	//PORTB ^= (bitRead(GPIOR2, 0) << PORTB0); //  waarde van bit0 uit het flagregister kopieren naar DDB1 
	//PORTB ^= (1 << PORTB0);
	sei();
}

void TRAINLOOP() { 


	if (bitRead(GPIOR2, 1) == false) { 
		//command ready false
		//zoek nieuw commando 
		//als geen commando te vinden... dan boolean NEWCOMMAND=false 
		//dan versturen van een idle command 
		// do slow events (knoppen enzo)



		// hier dus een nieuw command zoeken.
		if (NEWCOMMAND == false) {
			GPIOR0 = AD;
			GPIOR1 = DT;
			//eerste twee bytes verzonden dus ByteCounter naar 2, clear ByteFree flags, zijn niet meer vrij
			BC = 2;
			bitClear(GPIOR2, 4);
			bitClear(GPIOR2, 5);

		}
		 //delay(5000); 

		GPIOR2 |= (1 << 1);  //COMMAND READY naar waar

	}
	else
	{ //geen nieuw commando nodig maar misschien wel een nieuw BYTE
		if (BC <= BT) { //geen Bytes meer te verzenden
						//check het NIET acieve register

if (bitRead(GPIOR2, 6) == false) { //Register GPIOR0 = actieve register
								   //check of register vrij is:
	if (bitRead(GPIOR2, 5) == true) { //alleen als BYte vrij is
		GPIOR1 = ER;
		bitClear(GPIOR2, 5); //Register GPIOR1 is nu niet meer vrij
		BC++;
	}
}
else { //Register GPIOR1= actieve register, dus de andere vullen 
	if (bitRead(GPIOR2, 4) == true) { //Alleen als bytefree true is
		GPIOR0 = ER;
		bitClear(GPIOR2, 4); //Register GPIOR0 is nu niet meer vrij
		BC++;
	}
}
		}
		else { //Geen byte meer te verzenden 
			bitSet(GPIOR2, 2); //laatste byte is doorgegeven.
		}
	}


}

void TRAININTERRUPTS() { //instellen interrups voor knoppen en zo
	//Startstop knop staat op PIN4
	cli(); //disable interrupts	
	
	TIMSK2 |= (1 << OCIE2B); // enable de OCR2B interrupt van Timer2
	TCNT2 = 0; //Set Timer bottom waarde
	TCCR2B |= (1 << CS22); //set prescaler
	TCCR2B |= (1 << CS21); //set prescaler
	TCCR2B |= (1 << CS20); //set prescaler
	OCR2B = 100; //timerclock wordt NIET gereset dus frequency bepaald door overflow en prescaler

	sei(); //enable interrupts
}
void setup()
{
	pinMode(12, OUTPUT); //But1 de aanuit knop zeg maar
	digitalWrite(12, HIGH);
	pinMode(13, OUTPUT); //ledje op 13 

	pinMode(7, OUTPUT);
	digitalWrite(7, HIGH);

	//TBV Output kortsluiting. 
	//We meten de uitgangs spanning gelijkgericht en voorien van een RC circuit van 1microF
//   pinMode(5, INPUT); //op pin 4 aansluiting pin3 van de 555 als schmitt trigger ingesteld.

	DDRD &= ~(1 << DDD5); //in C language
	PORTD |= (1 << PORTD5); // pull up intern inschakelen zodat de SHORT functie optioneel wordt, controller werkt ook ZONDER...

	TRAININTERRUPTS();
	TRAIN();


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


/*
registertest = B00000000;
pinMode(8, OUTPUT);
Serial.begin(9600);
*/
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

	GPIOR0 ^= (1 << 0); //toggle bit 0 in dit register....GPIOR1 GPIOR2
	delay(1000);
	digitalWrite(8, bitRead(GPIOR0, 0));

}

//Deze isr niet ingebruik toch?


ISR(TIMER2_COMPB_vect) {

	SLOWEVENTS();
	}

/*
ISR(PCINT2_vect) { 
	// is voor short en start/stopknop
// Start/stop knop
	//PCICR &= (1 << PCIE2); //disable interrupts for portD


	if(digitalRead(4)==true) {
		digitalWrite(13, ! digitalRead(13));
	}
	


	if (digitalRead(4) != BUT1) { //knopstatus veranderd
		if (BUT1 == true) BUT1 = false;
		else { //was false wordt nu true, nu output omzetten 
			BUT1 = true;
			BUT1OS = !BUT1OS;
			RUNTRAIN(BUT1OS); //start/stopt de controller
		}
	}

	// TIMSK0 |= (1 << OCIE0B); // enable de OCR0B interrupt van Timer0
}

*/

void TOGGLE() {
	// met PINB register leds omschakelen
	//bitSet(PINB, PINB1);bitSet(PINB, PINB2);
}



void SLOWEVENTS() {

	SCOUNTER++;  //		een test vertraging kijken of de boel blijft werken	

	if (bitRead(PIND,PIND4)!= BUT1) {//  (digitalRead(4) != BUT1) { //knopstatus veranderd

			if (BUT1 == true) BUT1 = false;
		else { //was false wordt nu true, nu output omzetten 
			BUT1 = true;
			digitalWrite(13,BUT1OS);
			BUT1OS= ! BUT1OS;
			digitalWrite(12, BUT1OS);
			RUNTRAIN(BUT1OS);
			if (BUT1OS == true) SCOUNTER = 7;
			/*
			Deze routine werkt een beetje slappy... gebruik van een zener lijkt me een beter plan. 

			De kortsluitdetectie moet even na herstart een preriode worden enables, deze constructie met de SCOUNTER
			Zorgt daarvoor. Hoe hoger de waarde hoe korter de enable tijd.
			*/
		}
	}


	if (SCOUNTER > 10) {
		
		if (bitRead(PIND,PIND5)==false) { // (digitalRead(5) == LOW) { = veel sneller
			//Serial.println(SCOUNTER);	
			//Serial.println();
			BUT1OS = false;
			RUNTRAIN(BUT1OS);
			digitalWrite(12, BUT1OS);
			digitalWrite(13, !BUT1OS);

			SCOUNTER = 10;
		}
	}

}

void loop()
{

	TRAINLOOP();


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
	//REGISTERS();

	

	/*
	if (millis() - SLOWTIMER > 100) {
		SLOWTIMER = millis();
			SLOWEVENTS();
	}



	// SLOWEVENTS();
	
	// test met bit schuiven
	delay(2000);
		Serial.println(SCHUIFBYTE);
		SCHUIFBYTE =SCHUIFBYTE << 1;
	delay(2000);
	Serial.println(SCHUIFBYTE);
	SCHUIFBYTE = SCHUIFBYTE >> 1;
	*/


	}



