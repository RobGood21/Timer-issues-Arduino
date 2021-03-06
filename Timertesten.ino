/*
*****************************************
Name:		Timertesten.ino
Created:	2/3/2017 9:49:42 PM

Author:	Rob Antonisse
*****************************************

Beschrijvingen uitgebreid.
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

GPIOR0 � General Purpose I/O Register 0
GPIOR1  8bits 
GPIOR2

Altenatieve functies van belang: datasheet ATMEL 14.3.1 Alternate Functions of Port B

*/


//DECLARTATIES
//Decalarties voor void TRAIN()
volatile boolean BUT1OS = true;
volatile boolean BUT1 = false;
volatile unsigned int SCOUNTER = 0;
unsigned int CS = 0; //Commandstatus beter unsigned integer (aanpassen)
int i; //teller voor loop functies in train, noet signed zijn...wordt negatief,  hier nog naar kijken is onhandig.
int nc;

int BC = 0; //=ByteCount Aantal verzonden bytes
int BT = 3; //ByteTotal =aantal te verzenden bytes

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

void TRAIN() { //Setup van de DCC controller.
	cli(); // geen interrupts
	GPIOR0 = 0; //General purpose register 0, holds Byte to be send
	GPIOR1 = 0; //General purpose register 0, holds Byte to be send
	GPIOR2 = 0; //General Purpose register holds Flags, booleans.
    DDRB |= (1 << DDB1); //set PIN 9 als output Aangestuurd uit OCRA1
	DDRB |= (1 << DDB2); //set PIN 10 als output, wordt getoggled in de ISR	
	TCCR1A = 0; //initialiseer register 
	TCCR1B = 0; //initialiseer register	
	TCCR1B |= (1 << WGM12); //CTC mode	
	TCCR1B |= (1 << CS11); //CS12 zet prescaler (256?) CS11 voor prescaler 8, CS10 voor geen prescaler
	OCR1AH = B00000000;
	OCR1AL = B01110011; //116 zet TOP waarde timer
	TCCR1A |= (1 << COM1A0);//zet PIN 9 aan de comparator, toggle op TOPwaarde
	TIMSK1 |= (1 << OCIE1A); //interrupt enable
	sei(); //interupts weer toestaan	
}

ISR(TIMER1_COMPA_vect)  {// timer compare interrupt service routine, verzorgt het verzenden van de DCC pulsen.
	//Variabele DEBUG toont op de serial monitor de bits en bytes zoals bepaald en verzonden. Controller doet het dan
	//niet meer vanwege timing. Debug mag straks weg. 
	cli(); //disable interrupts
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
				CS = 0; //volgende doorlopp wachten op nieuw command..
				//GPIOR2 register vullen
				//bit0 = bitpart, is true, anders komen we niet hier, blijft true
				//bit1= Command ready, naar 0 
				//bit2=Laatste byte flag van uit LOOP, true=laatste byte
				//bit3=Laatste byte flag in de ISR, true is laatste byte
				//bit4=GP register te verzenden byte GPIOR0 true=vrij false is bezet
				//bit5=GP register te verzenden byte GPIOR1 true=vrij 0=bezet
				//bit6=Byte pointer true=GPIOR1 false = GPIOR0
				//bit 7=nc

	GPIOR2 = B00110001; //Flag register instellen voor nieuw command

//if(DEBUG==true) Serial.println("Command ready");
										
			}
			else { //Laatste byte niet verzonden
				bitSet(OCR1AL, 7); //verhoog OCR1A counter met 128, zend een 0 bit

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

				if (bitRead(GPIOR0, i) == true) { //een 1 bit
					bitClear(OCR1AL, 7);
				}
				else { //een 0 bit
					bitSet(OCR1AL, 7);
				}
				i--; //teller naar vorig bit
				if (i < 0) bitSet(GPIOR2, 4); //laatste bit verzonden, byte vrijgeven true =vrij
			}
			else { //Bytepointer=true, GPIOR1 register sturen

//if (DEBUG == true) {
//	Serial.print("   Bitwaarde:  "); 
//	Serial.println(bitRead(GPIOR1, i));
//}
				if (bitRead(GPIOR1, i) == true) { //een 1 bit
					bitClear(OCR1AL, 7);
					//if (bitRead(OCR1AL, 7) == true) OCR1A = (OCR1A >>1);
				}
				else { //een 0 bit
					bitSet(OCR1AL, 7);
					//if (bitRead(OCR1AL, 7) == false) OCR1A = (OCR1A << 1);
				}
				i--; //bitteller naar vorig bit
				if (i < 0) bitSet(GPIOR2, 5); //laatste bit verzonden, byte vrijgeven true =vrij
			}
			if (i < 0) { //laatste bit verzonden 
				GPIOR2 ^= (1 << 6); //toggle bytepointer				
				CS = 2; //naar tussenbit
//if (DEBUG == true)	Serial.println("Bytepointer getoggeld");
			}
			break; //(3)
			}
	} //bit is niet klaar, gebeurt gewoon niks
	sei(); //Enable interrupts
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
	cli(); //disable interrupts, 
	DDRB |= (1 << DDB4); //Pin12, PORTB4 als Output
	PORTB |= (1 << PORTB4); //Zet PIN12 hoog. (groene led)
	DDRD |= (1 << DDD7); //PIN7 H-Bridge enable als output
	PORTD |= (1 << PORTD7); //Enable H-bridge
	DDRD &= ~(1 << DDD5); //PIN 5 (PORTD4) input met pullup weerstand stellen
	PORTD |= (1 << PORTD5); // pull up inschakelen zodat de SHORT functie optioneel wordt.
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
	MEMORIE[0] = B10000001; //waarom hier een 1 op het laatste bit????
	MEMORIE[1] = B11111000; //adres0 activate off

	MEMORIE[2] = B10000001;
	MEMORIE[3] = B11111010; //adres1 deactivate off

	MEMORIE[4] = B10000001;
	MEMORIE[5] = B11111100; //adres2 activate off

	MEMORIE[6] = B10000001;
	MEMORIE[7] = B11111110; //adres3 activate off

	MEMORIE[8] = B10000000; //?? adressering verder uitzoeken
	MEMORIE[9] = B11110100; //adres4 deactivate off

	MEMORIE[10] = B10000101; //??
	MEMORIE[11] = B11110101; //adres5 activate off

	MEMORIE[12] = B10000000; //??
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
							if (bitRead(KNOPSTATUS[KOLOM], r) == true) CLSB[c] |= (1<<0) ; //set r/l adres +1 
							CERROR[c] = CMSB[c] ^ CLSB[c];
							c = 10;
						}
						c++; //Als geen vrij commandplek wordt gevonden, bij volgende doorloop wordt opnieuw gezocht.
					}
				}	
			r ++;
			}
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
					if (part == 0) CREG[b] = 0; //counter op nul, CREG en command array vrij geven.
					CBYTE[0] = CMSB[b];
					CBYTE[1] = CLSB[b];
					CBYTE[2] = CERROR[b];
					b = 10; //uitspingen als command is gevonden
					nc = 1;
				}
				else { //(b) timerbit5 = false
					if (bitRead(PREG, 0) == true) { //Slow timer clock
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
		BC = 2; //eerste twee bytes geladen
		bitClear(GPIOR2, 4); //clear flag byte free (dus niet meer vrij)
		bitClear(GPIOR2, 5); //Clear flag byte free
		 GPIOR2 |= (1 << 1);  //COMMAND READY naar waar
	}
	else	{ //geen nieuw commando nodig maar misschien wel een nieuw BYTE
		if (BC <= BT) { //geen Bytes meer te verzenden, anders check het NIET acieve register of het vrij is.
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
		else {// BC > BT Geen byte meer te verzenden 
			if (bitRead(GPIOR2,3) == false) bitSet(GPIOR2, 2); //laatste byte is doorgegeven, alleen zetten als laatste byte in interrupt nog false is
		}
	}
}

void loop()
{
	
	//tijdens ontwerp knoppen even uit
	TRAINLOOP();
	//INPUTLOOP();

	}



