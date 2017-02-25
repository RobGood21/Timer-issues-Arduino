/*
*****************************************
Name:		Timertesten.ino
Created:	2/3/2017 9:49:42 PM

Author:	Rob Antonisse
*****************************************

Beschrijvingen uitgebreid.
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

//Decalarties voor void TRAIN()
volatile boolean BUT1OS = true;
volatile boolean BUT1 = false;
volatile int SCOUNTER = 0;
int CS = 0; //Commandstatus
int i; //teller voor loop functies in train
int nc;


//TRAINLOOP
//declaraties tbv TRAIN() in LOOP()
boolean NEWCOMMAND = false;
byte AD = B11111111;
byte DT = B00000000;
byte ER = B11111111;
int BC = 0; //=ByteCount Aantal verzonden bytes
int BT = 3; //ByteTotal =aantal te verzenden bytes
boolean BF;

//xxxdeclaraties voor debugging en testen
boolean DEBUG = false; //false; //toont alle teksten als true

//straks naar eeprom voor commando bytes tijdelijk array voor 16 commands
byte MEMORIE[16];

//Declaraties voor INPUTCOMMAND
byte OLDCP = 0; //Oude status van C port register
byte CHANGECP = 0; //veranderde bits

byte RIJ=0;   //8 rijen onder 8 kolommen schakelaars kan array worden bij MEER dan 64 schakelaars
byte KOLOM=0; //voorlopig 1 kolom
byte PREG=0; // Public register voor 8 booleans

//unsigned int CADRES[10]; //gevonden adres van de knop, corresponderend met een geheugen adres waar command instaat
byte CREG[10]; //status van dit command adres
byte CMSB[10]; //msb van command komt uit eeprom geheugen ADRESCOMMAND
byte CLSB[10]; //LSB van command komt uit eeprom geheugen ADRESCOMMAND + 1
byte CERROR[10]; //XOR van MSB en LSB 

byte KNOPSTATUS[8];  //onthoud de laatste stand van de accessoire door deze knop aangestuurd. (aan of uit)


//Tijdelijk voor flikkerledje
int tic; //Teller loop functies in INPUTCOMMAND
int til; //Teller voor INPUTLOOP()
//int FLASH = 0; //hoevaak led moet flashen
//int FLASHTIMER = 0; //duur van de flash

void RUNTRAIN(boolean onoff) { //start of stopt de controller

	digitalWrite(7, onoff); //zet gewoon de H-brug uit

//cli(); //disable interrupts
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

	//PORTB |= (1 << PORTB2); //Zet PIN10 hoog


	 //interrupts tijdelijk ondebreken
	TCCR1A = 0; //initialiseer register NODIG!
	TCCR1B = 0; //initialiseer register NODIG!
	
	TCCR1B |= (1 << WGM12); //CTC mode	
	TCCR1B |= (1 << CS11); //CS12 zet prescaler (256?) CS11 voor prescaler 8, CS10 voor geen prescaler

	OCR1AH = B00000000;
	OCR1AL = B01110011; //112;//   116; //zet TOP waarde counter bij prescaler 256 1sec (standard timing true bit)	
	//TCNT1 = 50; //set timer1 op 0 BOTTOM waarde counter



	TCCR1A |= (1 << COM1A0);//zet PIN 9 aan de comparator, bij true toggle output
	//TCCR1A |= (1 << COM1B0);



	TIMSK1 |= (1 << OCIE1A); //interrupt op bereiken TOPwaarde

	//PORTB |= (1 << PORTB1); //clear port B1 (pin9)	//
	//PORTB |= (1 << PORTB2); //set portb2 1 (pin10)
	sei(); //interupts weer toestaan
	
}

ISR(TIMER1_COMPA_vect)  {// timer compare interrupt service routine, verzorgt het verzenden van de DCC pulsen.
	//Variabele DEBUG toont op de serial monitor de bits en bytes zoals bepaald en verzonden. Controller doet het dan
	//niet meer vanwege timing. 
//	cli(); //disable interrupts
	// PORTB ^= (1 << PORTB2); //toggles PIN10, better do this with a hardware inverter. 

	GPIOR2 ^= (1 << 0); //toggle het BITPART deel, als BITPART=false dan is het bit klaar >> nieuw bit bepalen.
	if (bitRead(GPIOR2, 0) == true) { //bit is klaar .. verder

		switch (CS) { //CS=Commandstatus ) 0=Wacht op Command; 1=Preample zenden; 2=Tussenbit zenden; 3=Byte verzenden 

		case 0: //Wacht	//Topwaarde wordt niet aangepast, timer gaat gewoon door met trues zenden	//test Commandready
			i++; //verhoog preample teller, preample tijdens wachten op command

//if (DEBUG==true) Serial.println("wacht op COMMAND");			

				if (bitRead(GPIOR2, 1) == true) //COMMANDready == true
			{
				CS = 1; //nu volgende true bit preample verzenden 
				// i = 0; //teller reset niet meer 25feb
			}
			break;//(0)  

		case 1: //send preample
			//registers resetten
			bitClear(GPIOR2, 3); //Laatste byte is false, nieuw command

//if (DEBUG == true) {
	//Serial.print("Preample: "); 
	//Serial.println(i);
//}
			if (i < 14) { //is lengte preample dus minimaal 14
				i++;
			}
			else {
				CS = 2; //15 true bits verzonden nu een false bit als volgend bit
			//	i = 7; //25feb dubbel? teller al instellen voor 1e byte //	Serial.println("");
			}
			break; //(1)

		case 2: //send Tussenbit

//if (DEBUG == true) {
//	Serial.print("Laatstebyte LOOP():");	
//	Serial.print(bitRead(GPIOR2, 2)); 
//	Serial.print("    Laatste Byte interrupt:  "); 
//	Serial.println(bitRead(GPIOR2, 3));
//}

			if (bitRead(GPIOR2, 3) == true) { //Laatste byte flag = true, laatste byte is verzonden nu uitspringen

				bitClear(OCR1AL, 7); //instellen op 1 bit
				bitClear(GPIOR2, 1); //COMMANDready naar false, dus GEEN command in behandeling, LOOP() zet deze weer true als een command in de byte registers is gezet.
				CS = 0; //volgende doorlopp wachten op nieuw command..
				GPIOR2 |= (1 << 4); //reset Bytefree register GPIOR0
				GPIOR2 |= (1 << 5); //reset BYTEfree register GPIOR1	


				//???????????????????????????????????????????????????
				//bitSet(GPIOR2, 6);		
				//GPIOR2 &= (0 << 6); //?????  moet zijn clear BYTEpointer naar GPIOR0 of zo??
				GPIOR2 = 0; //B0000000;  bovenstaande doet dit.... dit klopt niet ...toch?



//if(DEBUG==true) Serial.println("Command ready");
										
			}
			else { //Laatste byte niet verzonden
				bitSet(OCR1AL, 7); //verhoog teller met 128


//if (DEBUG==true) Serial.println("tussenbit");

				CS = 3; //volgende doorloop naar byte verzenden.
				i = 7; //reset bitcounter
				if (bitRead(GPIOR2, 2) == true) bitSet(GPIOR2, 3); //Laatste byte  flag true, verkregen uit LOOP() bij volgende doorloop van T(ussenbit) uitspringen.
			}
			break;//(2)

		case 3: //send Byte MSB first

//if (DEBUG == true) {
//	Serial.print("byte:  ");	
//	Serial.print(bitRead(GPIOR2, 6)); 
//	Serial.print("   Bit:  "); Serial.print(i);
//}

			if (bitRead(GPIOR2, 6) == false) { //BYTEpointer false= GPIOR0, true = GPIOR1

//if (DEBUG == true) {
//	Serial.print("   Bitwaarde:  ");
//	Serial.println(bitRead(GPIOR0, i));
//}

				if (bitRead(GPIOR0, i) == true) {
					bitClear(OCR1AL, 7);
					//if (bitRead(OCR1AL, 7) == true) OCR1A = (OCR1A >> 1);

				}
				else {
					bitSet(OCR1AL, 7);
						//if (bitRead(OCR1AL, 7) == false) OCR1A = (OCR1A << 1);
				}
				i--;
				if (i < 0) bitSet(GPIOR2, 4); //dit was het laatste bit dus BYTEfree wordt true
			}
			else { //GPIOR1 register sturen

//if (DEBUG == true) {
//	Serial.print("   Bitwaarde:  "); 
//	Serial.println(bitRead(GPIOR1, i));
//}
				if (bitRead(GPIOR1, i) == true) {
					bitClear(OCR1AL, 7);
					//if (bitRead(OCR1AL, 7) == true) OCR1A = (OCR1A >>1);
				}
				else {
					bitSet(OCR1AL, 7);
					//if (bitRead(OCR1AL, 7) == false) OCR1A = (OCR1A << 1);
				}
				i--;
				if (i < 0) bitSet(GPIOR2, 5);
			}
			if (i < 0) {
				GPIOR2 ^= (1 << 6); //toggle bytepointer				
				CS = 2; //naar tussenbit
//if (DEBUG == true)	Serial.println("Bytepointer getoggeld");
			}
			break; //(3)
			}
	} //bit is niet klaar, gebeurt gewoon niks
//	sei(); //Enable interrupts
}

ISR(TIMER2_COMPB_vect) {
	PREG ^= (1 << 0); //bit0 in register PREG is timer op ongeveer een 4ms.
	SLOWEVENTS();
	INPUTCOMMAND();
	}

void SHOWBYTE(byte beit) {
	static int b; //tellers
	b = 7;
	//	Serial.println();
		Serial.print("BYTE= ");
			while (b >= 0) {
			Serial.print(bitRead(beit, b));
			b--;

		}
			Serial.println();
}

void WRITECOMMANDS() {

	//Toont de inhoud van de commandregisters
	static int w, b; //tellers
	w = 0;
	while (w < 10) {	
		Serial.println();
		Serial.print("CREG ");
		Serial.print(w);
		Serial.print(" : ");
		b = 7;
		while (b >= 0) {
			Serial.print(bitRead(CREG[w], b));
			b--;
		}
		b = 7;
		Serial.println();
		Serial.print("MSB= :   ");
		while (b >= 0) {
			Serial.print(bitRead(CMSB[w],b));
			b--;
			}

		b = 7;

			Serial.println();
			Serial.print("LSB= :   ");
			while (b >= 0) {
				Serial.print(bitRead(CLSB[w], b));
				b--;
			}
			b = 7;
			Serial.println();
			Serial.print("ERROR=:  ");
			while (b >= 0) {
				Serial.print(bitRead(CERROR[w], b));
				b--;
			}
			Serial.println();
		w++;
	}

}


void TRAINSETUP() { //instellen interrups voor knoppen en zo
	//Startstop knop staat op PIN4
	cli(); //disable interrupts	
//Gebruikte timer = timer 2 voor timerinterrupts voor de SLOWEVENTS (switches..)

	DDRB |= (1 << DDB4); //Pin12, PORTB4 als Output
	PORTB |= (1 << PORTB4); //Zet PIN12 hoog. (groene led)
	DDRD |= (1 << DDD7); //PIN7 H-Bridge enable als output
	PORTD |= (1 << PORTD7); //Enable H-bridge
	DDRD &= ~(1 << DDD5); //PIN 5 (PORTD4) input met pullup weerstand stellen
	PORTD |= (1 << PORTD5); // pull up intern inschakelen zodat de SHORT functie optioneel wordt. Als PIN7 Hoog is draait de controller
	// Timer2, slowevents and inputs control
	TIMSK2 |= (1 << OCIE2B); // enable de OCR2B interrupt van Timer2
	TCNT2 = 0; //Set Timer 2 bottom waarde
	TCCR2B |= (1 << CS22); //set prescaler 
	TCCR2B |= (1 << CS21); //set prescaler
	TCCR2B |= (1 << CS20); //set prescaler op 1024, result overflow every 16 millisec.
	OCR2B = 100; //timerclock wordt NIET gereset dus frequency bepaald door overflow en prescaler

	sei(); //enable interrupts
}

void INPUTSETUP() {

	//Alleen voor knoppen ontwerp mag weg straks
	//pinMode(12, OUTPUT);
	//digitalWrite(12, HIGH);
	//******************************

}

void setup()
{
	//tbv INPUTCOMMAND, tijdelijke pinnen tbv schakelaars en Leds. 
		//Voor schakelaars PORTC bit 0:5  (PINA0:PINA5) instellen als input dus false
	DDRC = 0; //alle PORTC pins als input. ook de reset PORTC6
	DDRB |= (1 << DD3); //set PIN11 as output, Led voor INPUTCOMMAND
	PORTB |= (1 << PORTB3); //Set high, led off
	PINC = 0; //clear alle inputs
	OLDCP = PINC; //kopieer huidige PINC	

//voor ontwerp knoppen en command berekening even uitzetten

	TRAINSETUP();
	TRAIN();
	//INPUTSETUP(); //Setup routine voor inputzaken kan weg...

//tijdelijk even commandoos in een byte array plaatsen
	MEMORIE[0] = B10000000; 
	MEMORIE[1] = B11110000; //adres0 activate off

	MEMORIE[2] = B10000000;
	MEMORIE[3] = B11110001; //adres1 deactivate off

	MEMORIE[4] = B10000000;
	MEMORIE[5] = B11110010; //adres2 activate off

	MEMORIE[6] = B10000000;
	MEMORIE[7] = B11110011; //adres3 activate off

	MEMORIE[8] = B10000000;
	MEMORIE[9] = B11110100; //adres4 deactivate off

	MEMORIE[10] = B10000101;
	MEMORIE[11] = B11110101; //adres5 activate off

	MEMORIE[12] = B10000000;
	MEMORIE[13] = B11110110; //adres6 activate off

	MEMORIE[14] = B10000000;
	MEMORIE[15] = B11110111; //adres7 deactivate off
//totaal even een 8 schakelaars, commandoos.
	
}

void INPUTCOMMAND() { //leest schakelaars PORTC, called from timer2

	tic = 0; //reset  teller input command

	//tijdelijk gebruik van het input register van PORTC dit moet straks 1 van de 8 uit shiftregister verkregen knopstatus bytes worden.
	//Ook OLDCP wordt een array van 8 bytes dan. Voorlopig even met 1 byte werken. 
	//dus een while die de 8 registers doorloopt, PINC is dan die verkregen byte uit het shiftregister

	CHANGECP = OLDCP ^ PINC; //XOR nieuwe situatie C port met situatie vorige doorloop, straks data uit shiftregisters

	RIJ = CHANGECP & PINC; //veranderde pinstatus and pinstatus is true, dus ingedrukte schakelaar. Byte doorgeven aan loop
	
	KOLOM = 0; // voorlopig slechts 1 kolom dus 8 schakelaars, hier starks een while loop voor alle kolommen. 

	if (RIJ > 0) INPUTFC(); //INPUT FIND COMMAND, //SWREG |= (1 << 0); //Als een true switch event set flag nieuwe switch, verwerken in Loop. 	  

	//supersimplele feedback, de echte moet hier ergens komen? Of beter als het Command is verzonden.
	 if (CHANGECP > 0) {
		 OLDCP = PINC;
		 PORTB ^= (1 << 3); //toggle het signaal ledje op PIN 11
	 }
}

void SLOWEVENTS() {
	SCOUNTER++;  //Counter voor het aantal doorlopen van deze VOID
	if (bitRead(PIND,PIND4)!= BUT1) {//  (digitalRead(4) != BUT1) { //knopstatus veranderd
			if (BUT1 == true) BUT1 = false;
		else { //was false wordt nu true, nu output omzetten 
			BUT1 = true;
			BUT1OS= ! BUT1OS;
			digitalWrite(12, BUT1OS);
			RUNTRAIN(BUT1OS);
			if (BUT1OS == true) SCOUNTER = 7;
		}
	}
	if (SCOUNTER > 10) {
		
		if (bitRead(PIND,PIND5)==false) { // (digitalRead(5) == LOW) { = veel sneller
			//Serial.println(SCOUNTER);	
			//Serial.println();
			BUT1OS = false;
			RUNTRAIN(BUT1OS);
			digitalWrite(12, BUT1OS);
			SCOUNTER = 10;
		}
	}


}

void INPUTFC() {
	static byte r, c, ad; //teller kolom, Rij, teller commands en adres max 255 ...!

	//KOLOM hoeft niet, in INPUTCOMMAND() worden de kolommen afgelopen, in KOLOM staat nu het actieve kolom waar de switch event in is gevonden. 
	
	//if (bitRead(SWREG, 0) == true) { // nieuwe switches, ingedrukte schakelaars.
	//	RIJ en KOLOM hebben de port met nieuwe drukknop.
			//Serial.println(RIJ);			
	//Serial.println("hier...");
	//SHOWBYTE(RIJ);

			r = 0; 
			c = 0;
			ad = 0;	
			while (r < 8) {
			
				if(bitRead(RIJ, r) == true) {

					//Serial.print(r);

					//Vrije plek voor een command zoeken in CREG[] tsnn cccc bit T=false  
					while (c < 10) { //voorlopig met 10 'registers 'voor commands werken
						if (bitRead(CREG[c], 7) == false) { //vrije command, adres=( kolom*8 + rij) command staat in eeprom byte adres*2 en adres*2 +1

							//Serial.println("dit mag maar 1 keer");

							bitClear(RIJ, r); //Schakelevent verwerkt dus clear flag
							bitSet(CREG[c], 7); //claim dit command geheugen
							bitSet(CREG[c], 2); //zet uitvoer counter op 4.
							ad = ((KOLOM * 8) + r) * 2;//adres decimaal bepalen kolom x 6 + gevonden bit in RIJ
							CMSB[c] = MEMORIE[ad]; //haal MSB uit geheugen straks EEPROM
							CLSB[c] = MEMORIE[ad + 1];  //Haal LSB							
							KNOPSTATUS[KOLOM] ^= (1 << r); //zet on/off invert, voor deze schakelaar
							if (bitRead(KNOPSTATUS[KOLOM], r) == true) CLSB[c] |= (1<<3) ; //set ON (off is default uit Memorie)
							CERROR[c] = CMSB[c] ^ CLSB[c];
							c = 10;
						}
						c++; //Als geen vrij commandplek wordt gevonden, bij volgende doorloop wordt opnieuw gezocht.
					}
				}	
			r ++;
			}

			//WRITECOMMANDS(); //Voor debug, toont de bytes in de command arrays, kills dcctrain
			//bitSet(SWREG, 0); //set status flag weer false
	//}
}

void TRAINLOOP() {

	static byte CBYTE[3]; //static geheugen voor het actieve Commando
    int b; //teller en flag voor new command helaas hele byte?
	byte part;

	if (bitRead(GPIOR2, 1) == false) { //nieuw command nodig
		b = 0;
		nc = 0;

	
		while (b < 10) { //Loop testing Command array
			if (bitRead(CREG[b], 7) == true) { //(a)    command gevonden

				if (bitRead(CREG[b],5) == true) { //(b) Timerbit

					bitClear(CREG[b], 5);
					bitClear(CREG[b], 4); //reset de pauze timer

					CREG[b] = CREG[b] - 1; //verlaagt de counter. 
					part = CREG[b];
					part = part << 5;

					//SHOWBYTE(part);

					if (part == 0) CREG[b] = 0; //counter op nul, CREG en command array vrij geven.

					//SHOWBYTE(CREG[b]);


					CBYTE[0] = CMSB[b];
					CBYTE[1] = CLSB[b];
					CBYTE[2] = CERROR[b];
					b = 10; //uitspingen als command is gevonden
					nc = 1;

					// Serial.println("command geladen");

				}
				else { //(b) timerbit5 = false

					if (bitRead(PREG, 0) == true) { //timer hoog

						CREG[b] |= (1 << 4); // bitSet(CREG[b], 4);

					}
					else { //timer laag

						if (bitRead(CREG[b], 4) == true) CREG[b] |= (1 << 5); //bitSet(CREG[b], 5);
					}
				}
			} //(a)
			b++;
		}


		if (nc == 0) {
			CBYTE[0] = B11111111; //laad idle command
			CBYTE[1] = B00000000;
			CBYTE[2] = B11111111;
		}

		GPIOR0 = CBYTE[0];  //laad eerste byte van command
		GPIOR1 = CBYTE[1]; //laad tweede byte van command

		BC = 2;
		bitClear(GPIOR2, 4); //clear flag byte free (dus niet meer vrij)
		bitClear(GPIOR2, 5); //Clear flag byte free

		//******dit moet aan
		 GPIOR2 |= (1 << 1);  //COMMAND READY naar waar, kan misschien hogerop? Scheelt een byte

		// if (nc == 1) SHOWBYTE(GPIOR0); if (nc==1) SHOWBYTE(GPIOR1);

	}
	else
	{ //geen nieuw commando nodig maar misschien wel een nieuw BYTE
		if (BC <= BT) { //geen Bytes meer te verzenden	//check het NIET acieve register

			if (bitRead(GPIOR2, 6) == false) { //Register GPIOR0 = actieve register   //check of register vrij is:

				if (bitRead(GPIOR2, 5) == true) { //alleen als BYte vrij is

					GPIOR1 = CBYTE[2]; //Laad Byte3 van command

					//if (nc == 1) SHOWBYTE(GPIOR1);

					bitClear(GPIOR2, 5); //Register GPIOR1 is nu niet meer vrij
					BC++;
				}
			}
			else { //Register GPIOR1= actieve register, dus de andere vullen 
				if (bitRead(GPIOR2, 4) == true) { //Alleen als bytefree true is
					
					GPIOR0 = CBYTE[2];
					// if (nc == 1) SHOWBYTE(GPIOR0);

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


void loop()
{
	
	//tijdens ontwerp knoppen even uit
	TRAINLOOP();
	//INPUTLOOP();

	}



