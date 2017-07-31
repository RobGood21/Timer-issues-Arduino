/*
 Name:		SchakelBord.ino
 Created:	7/7/2017 10:20:39 PM
 Author:	Rob Antonisse
 Info:		www.wisselmotor.nl
*/

//tijdelijke en hulp declaraties en functies
unsigned long tijd = 0;
boolean DEBUG = false; //false; //toont alle teksten als true  //straks naar eeprom voor commando bytes tijdelijk array voor 16 commands
byte MEMORIE[16];
//byte RIJ = 0;   //8 rijen onder 8 kolommen schakelaars kan array worden bij MEER dan 64 schakelaars
//int tic; //Teller loop functies in INPUTCOMMAND
byte KOLOM = 0; //wordt gebruikt in Testinput
void Tijdelijk()
{
	/* Al gekopieerd naar setup
	//tbv INPUTCOMMAND, tijdelijke pinnen tbv schakelaars en Leds.
	//Voor schakelaars PORTC bit 0:5  (PINA0:PINA5) instellen als input dus false
	DDRC = 0; //alle PORTC pins als input. ook de reset PORTC6
	DDRB |= (1 << DD3); //set PIN11 as output, Led voor INPUTCOMMAND
	PORTB |= (1 << PORTB3); //Set high, led off
	PINC = 0; //clear alle inputs
	OLDCP = PINC; //kopieer huidige PINC	//voor ontwerp knoppen en command berekening even uitzetten
	TRAINSETUP();
	TRAIN();
	*/
	//INPUTSETUP(); //Setup routine voor inputzaken kan weg...

	//tijdelijk even commandoos in een byte array plaatsen

	/*
	Hoe zit ut in elkaar
	0=Byte 1
	1=byte 2
	Van Byte 1 bit7=1 bit6=0 bit5-0 = LSB van adress dus 1000 0001 = adres 1 1000 0010 = adres 2 let wel van een set van 4 dubbel adressen 
	Van Byte 2 bit7=1 bit6-5-4 = de MSB van het adres dus de hoge waardes geinverteerd, bit3 = aan uit van het adres bit2-1 welke van de vier dubbeladressen bit0 welke van het dubbeladres 
	Zeg maar links of rechts.
	Onderstaand voorbeelden tijdelijke bewaar dit want morgen ben je het weer vergeten....
	*/


	MEMORIE[0] = B10000001; //adres 1 van 4 dubbeladressen dus 1-2-3-4
	MEMORIE[1] = B11111000; //laatste bit bepaald WELKE van de twee dus recht of afbuigend

	MEMORIE[2] = B10000001;//2
	MEMORIE[3] = B11111010; 

	MEMORIE[4] = B10000001;//3
	MEMORIE[5] = B11111100; 

	MEMORIE[6] = B10000001;//4
	MEMORIE[7] = B11111110; 

	//tweede

	MEMORIE[8] = B10000010; // adres 2 van 4 dubbeladressen dus 5-6-7-8
	MEMORIE[9] = B11111000; //

	MEMORIE[10] = B10000010; //6
	MEMORIE[11] = B11111010; 

	MEMORIE[12] = B10000010;//7
	MEMORIE[13] = B11111100; 

	MEMORIE[14] = B10000010;//8
	MEMORIE[15] = B11111110; 
							

}

//declaraties Switches Leds
byte SwitchRow[8];// = { 0, 0, 0, 0, 0, 0, 0,0 };
byte LedRow[8];// = { 0,0,0,0,0,0,0,0 };
byte SwitchChanged;
byte LCDbyte = 0;
int CColums = 0; //column in processing

const int AantalPORTS = 5; //Aantal aangesloten schuifregisters. Xtra seriele poorten. 
int CSB = 1; //CSb=count schakelbord, hoeveel aangesloten schakebord printen

unsigned int LCDDelay = 0; // 350; //delay periodes voor de LCD initieel op 350ms
unsigned long LCDTime;
byte PORTS[AantalPORTS]; // declareer array
						 //0=PISO 1=SIPO voor switches 2=rijen ledmatrix 3=kolommen ledmatrix 4=SIPO for LCD
byte RijRegister = B00000000; // bevat ingelezen rij switches
byte IORegist = B00000000; //er zijn nog vrije bits te gebruiken als flag
//bit7=nc  
//bit6=(LCD) txt changed
//bit5=(LCD) send byte, byte staat klaar te verzenden, wordt verzonden, bij false klaar, wacht op nieuw byte
//bit4=(LCD) =RS true is send caracter false is send command  
//bit 3= (LCD) Send TXT true text aan het verzenden, false klaar niks verzenden
//bit2=OpstartTest true=aan false=uit  bit1=SSC switch status changed  bit0=vraag lezen PISO register. (rijen van switches) true is bezig false =klaar

//Declaraties DCC CS
volatile boolean BUT1OS = true;
volatile boolean BUT1 = false;
volatile unsigned int SCOUNTER = 0; 
unsigned int CS = 0; //Commandstatus beter unsigned integer (aanpassen)
int i; //teller voor loop functies in train, noet signed zijn...wordt negatief,  hier nog naar kijken is onhandig.
int nc;
int BC = 0; //=ByteCount Aantal verzonden bytes
int BT = 3; //ByteTotal =aantal te verzenden bytes, //xxx declaraties voor debugging en testen
byte PREG = 0; // Public register voor 8 booleans
byte CREG[10]; //status van dit command adres
byte CMSB[10]; //msb van command komt uit eeprom geheugen ADRESCOMMAND
byte CLSB[10]; //LSB van command komt uit eeprom geheugen ADRESCOMMAND + 1
byte CERROR[10]; //XOR van MSB en LSB 
byte SwitchState[8];  //Bevat actuele stand accessoire. aantal rijen per kolom hier aanpassen bij meer schakelbord pcb's
// functions

//declaraties LCD
//char l1[16] = "SchakelBord"; //l1=line 1
//char l2[16] = "Versie 1.01";

String l1 = String(16);
String l2 = String(16);

ISR(TIMER1_COMPA_vect) {// timer compare interrupt service routine, verzorgt het verzenden van de DCC pulsen.
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

//	}
//	else { //bit is niet klaar, gebeurt gewoon niks
//		IOLoop();

	}
	
	sei(); //Enable interrupts
}
void SLOWEVENTS() {	
	SCOUNTER++;  //Counter, creates pauze for short detectection

	if (bitRead(PIND, PIND4) != BUT1) { //DCC on/off switch
		if (BUT1 == true) BUT1 = false;
		else { 
			BUT1 = true;
			BUT1OS = !BUT1OS;
			digitalWrite(12, BUT1OS);
			digitalWrite(7, BUT1OS); //Switch H-bridge
			if (BUT1OS == true) SCOUNTER = 7;
		}
	}
	if (SCOUNTER > 10) {
		if (bitRead(PIND, PIND5) == false) { //short detection 
			BUT1OS = false;
			digitalWrite(7, BUT1OS); //Switch H-bridge
			digitalWrite(12, BUT1OS);
			SCOUNTER = 10;
		}
	}
}
void DCCLOOP() {
	static byte CBYTE[3]; //static geheugen voor het actieve Commando
	int b; //teller en flag voor new command helaas hele byte?
	byte part;
	if (bitRead(GPIOR2, 1) == false && bitRead(IORegist,3)==false) { //nieuw command gevraagt, ioregist,3 voorkomt zenden DCC tijdens zenden LCD text.
		b = 0;
		nc = 0;
		while (b < 10) { //Loop testing Command array
			if (bitRead(CREG[b], 7) == true) { //(a)    command gevonden
				if (bitRead(CREG[b], 5) == true) { //(b) Timerbit
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
	else { //geen nieuw commando nodig maar misschien wel een nieuw BYTE
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
			if (bitRead(GPIOR2, 3) == false) bitSet(GPIOR2, 2); //laatste byte is doorgegeven, alleen zetten als laatste byte in interrupt nog false is
		}
	}
}
ISR(TIMER2_COMPB_vect) { //timer interupt timed en start de 'langzame' events. 
PREG ^= (1 << 0); //bit0 in register PREG is timer op ongeveer een 4ms.
SwitchLoop();
SLOWEVENTS();
//LCDLoop();
}
void LCDLoop() {  //aanroep uit isr timer 2 ??? 
	//lcddelay
	//current=100 = clockpuls
	static int count = 0; 
	static int current = 0; //current fase van proces
	static int next =0; //volgende fase
	static int cc = 0; //character count
	static int lc = 0; //line count as int because of larger lcd possibility
	count++;
	//if (millis() - LCDTime >LCDDelay) {
	if (count > LCDDelay) {
		//start init LCD
		switch (current) {

		case 0: //begin, send 3 times 0x3
			PORTS[4] |= (1 << 0);
			PORTS[4] |= (1 << 1);
			LCDDelay = 10;
			current = 1;
			break;

		case 1:
			current = 100;
			next = 2;
			break;

		case 2:
			current = 100;
			next = 3;
			break;

		case 3:
			current = 100;
			next = 4;
			break;

		case 4: //set in 4-bit mode
			bitClear(PORTS[4], 0);
			current = 5;
			break;
		case 5:
			current = 100;
			next = 6;
			break;

		case 6: // init display off msn
			for (i = 0; i < 4; i++) {
				bitClear(PORTS[4], i);
			}
			current = 100;
			next = 7;
			break;


		case 7: //init display off lsn
			bitSet(PORTS[4], 3);
			bitSet(PORTS[4], 2);
			bitSet(PORTS[4], 1); //cursor on
			bitSet(PORTS[4], 0); //cursor blink
			current = 100;
			next = 8;
			break;

		case 8: //init display clear
			for (i = 0; i < 4; i++) {
				bitClear(PORTS[4], i);
			}
			current = 100;
			next = 9;
			break;

		case 9:
			bitSet(PORTS[4], 0);
			current = 100;
			next = 10;
			break;
		case 10: //end of init
			current = 200; //start txt mode
			LCDDelay = 100; //check for new txt once 10ms
			break;
			
			
		case 100: //clockpuls aansluiting op aparte poort A3
			LCDDelay = 1;
			if (digitalRead(A3) == true) {
				digitalWrite(A3, LOW);
			}
			else {
				digitalWrite(A3, HIGH);
				current = next;
			}
			break;

			/*	
		case 100: // voor poort op schuifregister. 

		dit wil niet werken waarschijnlijk door een timing probleem, vertragen van deze poort met hardware, 555 timer? is waarschijnlijk de oplossing
		
			
			PORTS[4] ^= (1 << 6); //toggle enable
			if (bitRead(PORTS[4], 6) == true) current = next;
			break;
*/
		case 110: //clear LCD
			bitClear(PORTS[4], 7); //reset RS send command
			for (i = 0; i < 5; i++) {
				bitClear(PORTS[4], i);
			}
			current = 100;
			next = 111;
			break;

		case 111: //lsn van clear lcd
			bitSet(PORTS[4], 0);
			current = 100;
			next = 112;
			break;

		case 112: //send msn 'home'
			for (i = 0; i < 5; i++) {
				bitClear(PORTS[4], i);
			}
			current = 100;
			next = 113;
			break;

		case 113: //send lsn 'home'
			bitSet(PORTS[4], 1);
			current = 100;
			next = 210; //naar verzenden 1e teken
			break;

		case 200: //start txt mode
			//is er een veranderde txt?
			//byte klaar?
			//ioregist, bit 3= (LCD) Send TXT true text aan het verzenden, false klaar niks verzenden
			//ioregist, bit5=(LCD) send byte, byte staat klaar te verzenden, wordt verzonden, bij false klaar, wacht op nieuw byte
			//text in l1 en l2 

			if (bitRead(IORegist, 3) == true) { //nieuwe text, bij false gebeurt er gewoon niks, ieder lcddelay wordt hierop getest
				LCDDelay = 0; //raise speed
				cc = 0;
				lc = 0;
				//adressen? clear? home?
					//current = 210;
				current = 110;
			}
			break;

		case 210: //begin txt send
			bitSet(PORTS[4], 7); //set rs (character send)	

			switch (lc) {
			case 0:
				//LCDbyte = l1[cc];
				LCDbyte = l1.charAt(cc);
				break;
			case 1:
				//LCDbyte = l2[cc];
			LCDbyte = l2.charAt(cc);
				break;
			}

			if (LCDbyte == 0)LCDbyte = 32;
			current = 211;
			break;

		case 211: //send msn
			for (i = 0; i < 5; i++) { //clear data nibble
				bitClear(PORTS[4], i);
			}
			for (i = 4; i < 8; i++) { //copy msn to datanibble
				if (bitRead(LCDbyte, i) == true) bitSet(PORTS[4], i - 4);
			}
			current = 100;
			next = 212;
			break;

		case 212: //send lsn
			for (i = 0; i < 5; i++) {
				if (bitRead(LCDbyte, i) == true) {
					bitSet(PORTS[4], i);
				}
				else {
					bitClear(PORTS[4], i);
				}
			}
			current = 100;
			next = 213;
			break;

		case 213:
			cc++; //next character
			current = 210;
			if (cc > 16) { //l1 verzonden
				cc = 0;
				lc++;


				if (lc > 1) { //beide lines verzonden
					bitClear(IORegist, 3); //txt klaar
					lc = 0;
					LCDDelay = 100;
					current = 200;
				}
				else { //line 2 verzenden
					current = 250;
				}
			}
					
				break;

		case 250: //cursor verplaatsen naar tweede regel, send adres 56 ? Hex40=64 toch?
			bitClear(PORTS[4], 7); //rs low, send command
			for (i = 0; i < 5; i++) {
				bitClear(PORTS[4], i);
			}
			bitSet(PORTS[4], 2); //msn adres
			bitSet(PORTS[4], 3);
			current = 100;
			next = 251;
			break;

		case 251:
			for (i = 0; i < 5; i++) { //lsn 
				bitClear(PORTS[4], i);
			}
			//bitSet(PORTS[4], 3);
			current = 100;
			next = 210;
			break;

			//} 		
		}//end of switch	
		count = 0;
	}
}
void LCDWrite() { //niet in gebruik...
/*
verzend het klaargezette byte (character)
Vanuit IOloop aangeroepen telkens als alle bytes zijn ingeschoven. 
LCDbyte bevat te verzenden byte, te versturen naar de LCD
ioregist booleans.   
bit5=Zend Byte als false niks doen..
bit4=RS (command false of caracter true
bit3=text verzenden (true) klaar voor nieuwe text(false)
*/

	static int pnt = 0; //pnt=point waar in het proces
	byte Temp;
	if (bitRead(IORegist, 5) == true) {
//Serial.println (LCDbyte);
		switch (pnt) {
		case 0: //begin 
			Temp = LCDbyte;
			PORTS[4] = Temp >> 4; //load  msn 
			if (bitRead(IORegist, 4) == true) bitSet(PORTS[4], 7);
			bitSet(PORTS[4], 6);
			pnt = 10;
			break;

		case 10: //clock in 1e nibble
		bitClear(PORTS[4], 6);
		pnt = 20;
			break;

		case 20:
			Temp = LCDbyte;
			PORTS[4] = Temp << 4;
			PORTS[4] = PORTS[4] >> 4; //load lsn, clear msn
			if (bitRead(IORegist, 4) == true) bitSet(PORTS[4], 7);
			bitSet(PORTS[4], 6);// ??
			pnt = 30;
			break;
		case 30:
			//ends sending this byte
			bitClear(PORTS[4], 6); //???????????
			bitClear(IORegist, 5); //reset flag
			pnt =0;//reset point counter
			break;
		}
	}
}
void TestInput() { //dient als een soort van DCC monitor, schakel adres 0 pe 6 seconden om.t

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
	//Serial.print("jo");

	//Vrije plek voor een command zoeken in CREG[] tsnn cccc bit T=false  
	while (c < 10) { //voorlopig met 10 'registers 'voor commands werken
		if (bitRead(CREG[c], 7) == false) { //vrije command, adres=( kolom*8 + rij) command staat in eeprom byte adres*2 en adres*2 +1

											//Serial.println("dit mag maar 1 keer");

											//bitClear(RIJ, r); //Schakelevent verwerkt dus clear flag
			bitSet(CREG[c], 7); //claim dit command geheugen
			bitSet(CREG[c], 2); //zet uitvoer counter op 4.
			ad = 0;// ((KOLOM * 8) + r) * 2;//adres decimaal bepalen kolom x 8 + gevonden bit in RIJ
			CMSB[c] = MEMORIE[ad]; //haal MSB uit geheugen straks EEPROM
			CLSB[c] = MEMORIE[ad + 1];  //Haal LSB							
			SwitchState[KOLOM] ^= (1 << r); //zet on/off invert, voor deze schakelaar
			if (bitRead(SwitchState[KOLOM], r) == true) CLSB[c] |= (1 << 0); //set r/l adres +1 
			CERROR[c] = CMSB[c] ^ CLSB[c];
			c = 10;
		}
		c++; //Als geen vrij commandplek wordt gevonden, bij volgende doorloop wordt opnieuw gezocht.
	}

}
void IOLoop() { //ioloop= in-out loop
				/*
				zet continue de PORTS[n] in schuifregisters, toont deze.En leest de schakelaars uit.
				Per doorloop wordt maar 1 instructie gedaan.
				theroretisch op max snelheid=ieder 0.0625us
				*/

	static int STATUS = 100;  //State of process
	static int SBiT = 0; //bitcounter
	static byte SendByte = 0;//Current byte in process
	static int ByteCount = AantalPORTS - 1; //number of to shift bytes
	static byte PISO = B00000000; // Temporary register for reading of row switches

	bitClear(PORTC, 0); //clear shiftclock

	switch (STATUS) {
	case 100: //start new cycle
		PISO = B00000000;
		if (ByteCount >= 0) {
			SendByte = PORTS[ByteCount];
			STATUS = 101;
			SBiT = 0;
			ByteCount--;
		}
		else { //alle bytes verzonden //outputs shiftregisters tonen; PISO register leest de inputs (schakelaars) per rij. 
			bitSet(PORTC, 1); //set registerclock
			STATUS = 102;
		}
		break;
	case 101: //bits sturen
		if (bitRead(SendByte, SBiT) == true) {
			bitSet(PORTC, 2); //set outputs SIPO, read inputs PISO
		}
		else {
			bitClear(PORTC, 2);
		}
		if (bitRead(PINB, PB0) == true)bitSet(PISO, SBiT); //Get value output shiftregisters	
		bitSet(PORTC, 0); //set shift clock
		SBiT++; //next bit
		if (SBiT > 7) { //sending current byte ready
			STATUS = 100;
			if (bitRead(IORegist, 0) == true) { //Request for row read
				if (ByteCount + 1 == 0) { // select PISO register, negative numbers not allowed.
					RijRegister = PISO; //return PISO register read
					bitClear(IORegist, 0); //clear flag request
				}
			}
		}
		break;
	case 102: // shift ready, clock in.
		//snelheid hier max het aantal bytes te verzenden x8 x 0.0625us
		//bij 5 shiftregisters dus minimum 2,5us 
		bitClear(PORTC, 1); //clear register clock
		STATUS = 100;
		ByteCount = AantalPORTS - 1;

		//LedTest(); //continue leds even late lopen

		//hier het laden van de default, opgeslagen stand uitvoeren???

		if (bitRead(IORegist, 2) == true) {
			LedTest();
		}
		else {
			LedLoop();			
		}
		LCDLoop();
		break;
	default:
		break;
	}
}
void Switched(byte k, byte r) {//handles switches

int c = 0;
int adress = 0;
int sw = 0;

	for (int i = 0; i < 8; i++) { //kolom bepalen
		//Serial.println(k);
		if (bitRead(k, i) == true) CColums = i;
	}
	//xor het gemeten rij met de vorige meting
	SwitchChanged = r ^ SwitchRow[CColums];
	SwitchRow[CColums] = r; //save new rowstate
	
	for (int i = 0; i < 8; i++) {
		if (bitRead(SwitchChanged, i) == true) {
			if (bitRead(r, i) == true) { //found switch has become true
				LedRow[CColums] ^= 1 << i; //toggle indicatorled		

				
				//schakelaar bepaald en stand txt weergeven op LCD
				sw = (i*8) + (CColums + 1);
				l1 = "Knop: ";
				l1.concat(sw);
				if (bitRead(LedRow[CColums],i) == true) {
					l2 = "Aan";
				}
				else {
					l2 = "Uit";
				}

				bitSet(IORegist, 6); //vragen om nieuwe text (vertraagd via loop())


				while (c < 10) { //voorlopig met 10 'registers 'voor commands werken
					if (bitRead(CREG[c], 7) == false) { //vrije command, adres=( kolom*8 + rij) command staat in eeprom byte adres*2 en adres*2 +1
													

						//bitClear(RIJ, r); //Schakelevent verwerkt dus clear flag
						bitSet(CREG[c], 7); //claim dit command geheugen
						bitSet(CREG[c], 2); //zet uitvoer counter op 4.
						adress = ((i * 8) + CColums) * 2;//adres decimaal bepalen kolom x 8 + gevonden bit in RIJ
						//MERK OP =1 lager dan de aanduiding op de LCD

						CMSB[c] = MEMORIE[adress]; //haal MSB uit geheugen straks EEPROM
						CLSB[c] = MEMORIE[adress + 1];  //Haal LSB							
						SwitchState[CColums] ^= (1 << i); //zet on/off invert, voor deze schakelaar
						if (bitRead(SwitchState[CColums], i) == true) CLSB[c] |= (1 << 0); //set r/l adres +1 

						//Serial.println(CMSB[c]);//("dit mag maar 1 keer");
						//Serial.println(CLSB[c]);

						CERROR[c] = CMSB[c] ^ CLSB[c];
						c = 10;
					}
					c++; //Als geen vrij commandplek wordt gevonden, bij volgende doorloop wordt opnieuw gezocht.
				}
			}

			/*
			Serial.print("kolom   :");
			Serial.println(CColums);
			Serial.print("rij   :");
			Serial.println(i);
			delay(1000);

			*/			
		}
	}

}
void SwitchLoop() {	//leest de schakelaars
	static int fase = 0;
	static byte Rk = B1000000; //ReadKolom
	switch (fase) {
	case 0: //load column to SIPO shiftregister for switches
		PORTS[1] = Rk;
		fase = 1;
		break;
	case 1: // kolom geplaatst; rij laten laden in IOloop.
		bitSet(IORegist, 0); //set flag
		fase = 2;
		break;
	case 2:
		if (bitRead(IORegist, 0) == false) { //flag cleared, row loaded
											 //uitzoeken welke schakelaars zijn verandered
			Switched(Rk, RijRegister); //Move column and row..
			fase = 3;
		}
		break;
	case 3: //next column
		switch (Rk) {
		case B00000000:
			Rk = B00000001;
			break;
		case B10000000:
			Rk = B00000001;
			break;
		default:
			Rk = Rk << 1;
			break;
		}
		fase = 0; //reset new cycle
		break;
	}
}
void LedTest() { //opstarten, initialiseren leds en LCD
	//doorloop ongeveer 1 x per 2.5us
	static int t = 0;
	static int Doorloop = 0;
	static int Bitcount = 0;
	static byte Temp = B00000000;
	static int SPEED = 50;
	t++;
	if (t > SPEED) {
		//Serial.println(Doorloop);
		t = 0;
		switch (Doorloop) {
		case 0:
			PORTS[3] = B00000001;
			Temp = B00000001;
			PORTS[2] = ~Temp;
			Doorloop = 10;
			Bitcount = 0;
			break;
		case 10:
			PORTS[3] = PORTS[3] << 1;
			Bitcount++;
			if (Bitcount == 8) {
				Bitcount = 0;
				Temp = Temp << 1;

				PORTS[3] = B00000001;
				PORTS[2] = ~Temp;

				//if (Temp == B00000000)Doorloop = 20; //leave ledtest
				//if (Temp == B00000000)Doorloop = 0;
				if (Temp == B00000000) {
					
					Doorloop = 20; //initialiseren LCD
					PORTS[4] = B01000000;

				//bitcount is nu vrij gebruiken als teller voor LCD init		
					//snelheid aanpassen
					--SPEED = 200; 
					Bitcount = 0;
				}
				break;
				/*
				

		case 100: //LCD opstarten
			bitSet(PORTS[4], 6);//set e high
			Doorloop = 101;
			break;
		case 101:
			bitSet(PORTS[4], 0);
			bitSet(PORTS[4], 1);
			Doorloop = 102;
			break;

		case 102:
			bitClear(PORTS[4], 6); //clock 1
			Doorloop = 103;
			break;

		case 103: 
			bitSet(PORTS[4], 6);//set e high
			Doorloop = 104;
			break;

		case 104:
			bitClear(PORTS[4], 6); //clock 2
			Doorloop = 105;
			break;

		case 105: //LCD opstarten
			bitSet(PORTS[4], 6);//set e high
			Doorloop = 106;
			break;

		case 106:
			bitClear(PORTS[4], 6); //clock 3
			Doorloop = 107;
			break;

		case 107: 
			bitSet(PORTS[4], 6);//set e high
			Doorloop = 110;
			break;
			
		case 110: //nu waarde twee sturen, voor 4 bits mode
			bitClear(PORTS[4], 0);
			Doorloop = 111;
			break;

		case 111: //clock puls 
			bitClear(PORTS[4], 6); //clock
			Doorloop = 112;	
			break;

		case 112:
			bitSet(PORTS[4], 6);
			Doorloop = 120; //stop
			break;

//nu zou die in 4 bit mode moeten staan

		case 120:
//
			PORTS[4] = B01000000;

			Doorloop = 121;
			break;

		case 121:
			bitClear(PORTS[4], 6);
			Doorloop = 122;
			break;

		case 122:
			bitSet(PORTS[4], 6);
			Doorloop = 123;
			break;


		case 123:
			bitSet(PORTS[4], 0);
			bitSet(PORTS[4], 1);
			bitSet(PORTS[4], 2);
			bitSet(PORTS[4], 3);
			Doorloop = 124;
			break;
			
		case 124:
			bitClear(PORTS[4], 6);
			Doorloop = 125;

			break;

		case 125:
			bitSet(PORTS[4], 6);
			Doorloop = 200;
			break;

		case 200:
			break;
			*/

		case 20:
			PORTS[2] = B11111111;
			PORTS[3] = 0;
			bitClear(IORegist, 2); //verlaat test mode
			break;
			}
		}
	}

}
void LedLoop() {
		static int t = 0;
	byte r; //ports[3] (geinverteerd)
	static byte kolom; // = B00000000; //ports[4]
	switch (t) {
	case 0:
		kolom = B00000001;
		break;
	default:
		kolom = kolom << 1;
		break;
	}
		//Serial.println(t);
	//Serial.println(kolom);
	r = LedRow[t];
	PORTS[3] = kolom; //colums to ledmatrix	  //PORTS[3] = B11100111;
	PORTS[2] = ~r; //load inverted row to led matrix
				   /*
				   if (r > 0) {
				   Serial.print("kolom....: ");
				   Serial.println(PORTS[4]);
				   Serial.print("rij....: ");
				   Serial.println(PORTS[3]);
				   delay(1000);
				   }
				   */
	t++;
	if (t == 8)t = 0;
}
void SetupDCC() { //Setup ports and timers
	cli(); // geen interrupts
		   //setup ports
	GPIOR0 = 0; //General purpose register 0, holds Byte to be send
	GPIOR1 = 0; //General purpose register 0, holds Byte to be send
	GPIOR2 = 0; //General Purpose register holds Flags, booleans.
	DDRB |= (1 << DDB1); //set PIN 9 als output Aangestuurd uit OCRA1
	DDRB |= (1 << DDB2); //set PIN 10 als output, wordt getoggled in de ISR	
	DDRB |= (1 << DDB4); //Pin12, PORTB4 als Output
	PORTB |= (1 << PORTB4); //Zet PIN12 hoog. (groene led)
	DDRD |= (1 << DDD7); //PIN7 H-Bridge enable als output
	PORTD |= (1 << PORTD7); //Enable H-bridge
	DDRD &= ~(1 << DDD5); //PIN 5 (PORTD4) input met pullup weerstand stellen
	PORTD |= (1 << PORTD5); // pull up inschakelen zodat de SHORT functie optioneel wordt.
							//setup ports IO shiftregisters
	DDRC |= (1 << PC0); // set A0 as output = shiftclock  (pin11, shiftregister)
	DDRC |= (1 << PC1); //set A1 as output = shift Latch  (pin12,shiftregister)
	DDRC |= (1 << PC2); //set A2 as output =Data in (Pin14, shiftregister)

	DDRC |= (1 << PC3); //st pin A3 as output tbv Enable LCD
	
	DDRB &= ~(1 << PB0); // set PORTB0 (PIN8-Arduino) as input
	for (int i = 0; i < AantalPORTS; i++) { //clear all used ports
		PORTS[i] = B00000000;
	}
	//Setup Timer 1
	TCCR1A = 0; //initialiseer register 
	TCCR1B = 0; //initialiseer register	
	TCCR1B |= (1 << WGM12); //CTC mode	
	TCCR1B |= (1 << CS11); //CS12 zet prescaler (256?) CS11 voor prescaler 8, CS10 voor geen prescaler
	OCR1AH = B00000000;
	OCR1AL = B01110011; //116 zet TOP waarde timer
	TCCR1A |= (1 << COM1A0);//zet PIN 9 aan de comparator, toggle op TOPwaarde
	TIMSK1 |= (1 << OCIE1A); //interrupt enable
							 // Timer2, slowevents and inputs control
	TIMSK2 |= (1 << OCIE2B); // enable de OCR2B interrupt van Timer2
	TCNT2 = 0; //Set Timer 2 bottom waarde
	TCCR2B |= (1 << CS22); //set prescaler 
	TCCR2B |= (1 << CS21); //set prescaler
	//prescaler op 256 bij 16Mhz dus ISR iedere 16us.
						   //TCCR2B |= (1 << CS20); //set prescaler op 1024, result overflow every 16 millisec.
	OCR2B = 100; //timerclock wordt NIET gereset dus frequency bepaald door overflow en prescaler
	sei(); //interupts weer toestaan	
}
void setup() {
	// test en hulp instellingen
	Serial.begin(9600);
	Tijdelijk();

	l1 = "SchakelBord";
	l2 = "Versie 1.01";

	
	bitSet(IORegist, 2); //test modus inschakelen
	DDRB |= (1 << DD3); //set PIN11 as output, Led voor INPUTCOMMAND
	PORTB |= (1 << PORTB3); //Set high, led off
							//PINC = 0; //clear alle inputs
							//OLDCP = PINC; //kopieer huidige PINC	//voor ontwerp knoppen en command berekening even uitzetten
							// TRAINSETUP();
	
	PORTC |= (1 << PORTC3);//enable LCD hoog
	//bitSet(PORTS[4], 6); //LCD enable hoog via shiftregister
	SetupDCC();
}
void loop() {
	DCCLOOP();
	IOLoop();
	//LCDLoop();

	
	//timer voor zetjes
	if (millis() - tijd > 300) {

		//LCD txt vervangen
		if (bitRead(IORegist, 6) == true) {
			bitClear(IORegist, 6);
			bitSet(IORegist, 3);
		}
		tijd = millis();
	}

}





