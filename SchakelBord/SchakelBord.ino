/*
 Name:		SchakelBord.ino
 Version:	1.04
 Created:	17 dec 2017
 Author:	Rob Antonisse
 Info:		www.wisselmotor.nl
*/

//declaraties
#include <EEPROM.h>
/* Opmerkingen, remarks:
Een leeg, nog niet geprogrammeerd byte in EEPROM is altijd een Hex FF (B1111 1111) 
een nog niet gedefinieerd dcc kanaal = dus altijd adres 1024 (256-4)
*/

//versie
String versie = String(5)="1.04";
unsigned long tijd = 0;

//temp
int errcount = 0;
unsigned long counttijd;

//declaraties Switches Leds

byte SwitchRow[8];
byte LedRow[8];
byte SwitchChanged;
byte SwitchState[8];
byte CColums = 0;
byte waarom; //Nodig, compiler issue
byte SW1[255]; //byte1 switch data (8 bits adres)
byte SW2[255]; //byte2 switch data 
/*
bit7 Opstart state true =uit, false = aan.
bit6 dcc continue of puls
bit5 switch continu of puls
bit4 = wissel of apart
bit3 both=false single=true (standaard 0)
bit2 welke decoder van de 4 bit 1
bit1 welke decoder van de 4 bit 0
bit0= welke van het paar. True=afslaan, false is rechtdoor
*/
const int AantalPORTS = 6; //Aantal aangesloten schuifregisters. Xtra seriele poorten. 
unsigned int LCDDelay = 0; //teller voor vertraging van proces.
byte lcdnew=0; //new state LCD switches
byte lcdold = 0;//last state of lcd switches
byte lcdchanged; //Holds changed switch states
byte lcdon = 0; //holds active switch
int programmode = 0; //programma mode 0=normaal 1=adres 2=accessory type (single, dual) 3= switch type 4=CV
int programSw = 0; //Welke switch wordt geprogrammeerd
byte LCDbyte = 0; //Bevat actueel aan LCD te zenden byte
int lcdrefresh = 0; //Hoe moet de lcd worden aangepast?
int lcdadres = 0; //adres waar lcd naar toe moet
String l1 = String(16); //te tonen text 1e regel
String l2 = String(16); //2e regel
String l1mem = String(16); // geheugenplaats voor txt regel 1.
String l2mem = String(16);//idem regel 2
unsigned int PMmem; //geheugen programmode
byte CVnmem = 2; //memorie last CV number
byte CVvmem = 0; //memorie last CV value
int poortsw = 0; //dual or single mode of dcc adres
byte PORTS[AantalPORTS]; // aantal shiftregisters in serie
byte RijRegister = B00000000; // bevat ingelezen rij switches
byte IORegist = B00000000; //er zijn nog vrije bits te gebruiken als flag
//bit7=nc **11 okt divider, counter for slowing scan process
//bit6=(LCD) txt changed
//bit5=(LCD) send byte, byte staat klaar te verzenden, wordt verzonden, bij false klaar, wacht op nieuw byte
//bit4=(LCD) =Vraag lezen PISO(LCD) True is bezig, False is klaar
//bit 3= (LCD) Send TXT true text aan het verzenden, false klaar niks verzenden
//bit2=OpstartTest true=aan false=uit  
//bit1=SSC switch status changed  
//bit0=vraag lezen PISO register. (rijen van switches) true is bezig false =klaar
byte PrgRegist = 0; //flags tbv programmeren
//bit 0= encoder B channel
//bit 1= true =value mode,  false= parameter mode
//bit 2 = ja/nee voor bevestiging in CV programming
//bit3=CV sending, true
//bit4=true INITIALISE programSw has switch number, false normal operation
byte groep = 0; //bevat laatste groep adres
byte dc = 0;//bevat laatste decoder adres (wie van de vier)

//Declaraties DCC CS
unsigned long CP; //command pauze, time between commands 
volatile boolean BUT1OS = true;
volatile boolean BUT1 = false;
volatile unsigned int SCOUNTER = 0; 
unsigned int CS = 0; //Commandstatus beter unsigned integer (aanpassen) *****
int i; //teller voor loop functies in train, noet signed zijn...wordt negatief,  hier nog naar kijken is onhandig.
int nc;
int BC = 0; //=ByteCount Aantal verzonden bytes
int BT = 3; //ByteTotal =aantal te verzenden bytes, //xxx declaraties voor debugging en testen
byte PREG = 0; // Public register voor 8 booleans
byte CREG[10]; //status van dit command adres
byte CMSB[10]; //msb van command komt uit eeprom geheugen ADRESCOMMAND
byte CLSB[10]; //LSB van command komt uit eeprom geheugen ADRESCOMMAND + 1
byte CV1[10];
byte CV2[10];
byte CV3[10];
byte CERROR[10]; //XOR van MSB en LSB 

void LoadData(){ //gets data from EEPROM during power-up
	//laad SW1 en SW2 terug
	for (i = 0; i < 256; i++) {
		SW1[i] = EEPROM.read(i);
		SW2[i] = EEPROM.read(i + 256);
	}

}
ISR(TIMER1_COMPA_vect) {// timer compare interrupt service routine, verzorgt het verzenden van de DCC pulsen.

	cli(); //disable interrupts
	GPIOR2 ^= (1 << 0); //toggle het BITPART deel, als BITPART=false dan is het bit klaar >> nieuw bit bepalen.
	if (bitRead(GPIOR2, 0) == true) { //bit is klaar .. verder
		switch (CS) { //CS=Commandstatus ) 0=Wacht op Command; 1=Preample zenden; 2=Tussenbit zenden; 3=Byte verzenden 
		case 0: //Wacht	//Topwaarde wordt niet aangepast, timer gaat gewoon door met trues zenden	//test Commandready
			i++; //verhoog preample teller, preample tijdens wachten op command
			if (bitRead(GPIOR2, 1) == true) //COMMANDready == true
			{
				CS = 1; //nu volgende true bit preample verzenden 
						// i = 0; //teller reset niet meer 25feb
			}
			break;//(0)  

		case 1: //send preample
				//registers resetten
			bitClear(GPIOR2, 3); //Laatste byte is false, nieuw command
			if (i < 14) { //is lengte preample dus minimaal 14
				i++;
			}
			else {
				CS = 2; //15 true bits verzonden nu een false bit als volgend bit
			}
			break; //(1)
		case 2: //send Tussenbit
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
			}
			else { //Laatste byte niet verzonden
				bitSet(OCR1AL, 7); //verhoog OCR1A counter met 128, zend een 0 bit
				CS = 3; //volgende doorloop naar byte verzenden.
				i = 7; //reset bitcounter
				if (bitRead(GPIOR2, 2) == true) bitSet(GPIOR2, 3); //Laatste byte  flag true, verkregen uit LOOP() bij volgende doorloop van T(ussenbit) uitspringen.
			}
			break;//(2)
		case 3: //send Byte MSB first
			if (bitRead(GPIOR2, 6) == false) { //BYTEpointer false= GPIOR0, true = GPIOR1
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
			}
			break; //(3)
		}
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
//*********************
void DCCLOOP() { //called from loop() V1.04
					 /*
					 CREG register 10 mogelijke posities 0-9
					 bit7 true=bezet false=vrij
					 bit6 True = CV  false=bediening
					 bit5 timer
					 bit4 timer
					 bit3
					 bit2 count aantal te verzenden msb
					 bit1 count aantal te verzenden
					 bit0 count aantal te verzenden lsb
					 */
	static byte CBYTE[6]; //static geheugen voor het actieve Commando, switch modes uses 3, CV mode uses 6
	int b; //teller en flag voor new command helaas hele byte?
	byte part;
	if (bitRead(GPIOR2, 1) == false && bitRead(IORegist, 3) == false) { //nieuw command gevraagt, ioregist,3 voorkomt zenden DCC tijdens zenden LCD text.

		if (millis() - CP > 10) {//10ms pauze between commands
			CP = millis();
		b = 0;
		nc = 0;

		while (b < 10) { //Loop testing Command array
			if (bitRead(CREG[b], 7) == true) { //(a)    command gevonden

					CREG[b]--; //CREG[b] = CREG[b] - 1; //verlaagt de counter. 
					part = CREG[b];
					part=part << 4;
					part= part >> 4;
					if (part < 1) bitClear(CREG[b], 7);


				if (bitRead(CREG[b], 6) == true) {
					//CV command
					BT = 6;
					CBYTE[0] = CMSB[b];
					CBYTE[1] = CLSB[b];
					CBYTE[2] = CV1[b];
					CBYTE[3] = CV2[b];
					CBYTE[4] = CV3[b];
					CBYTE[5] = CERROR[b];
				}
				else {
					//switchcommand

					BT = 3; //send 3 bytes
					CBYTE[0] = CMSB[b];
					CBYTE[1] = CLSB[b];
					CBYTE[2] = CERROR[b];
					//version 1.04
					CBYTE[3] = 0;
					CBYTE[4] = 0;
					CBYTE[5] = 0;
				}
				b = 10; //uitspingen als command is gevonden
				nc = 1;
		} //Free register CREG bit 7
		b++;
} //WHILE
		}

		if (nc == 0) {
			BT = 3;
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
					GPIOR1 = CBYTE[BC]; // laad volgende byte uit command //CBYTE[2]; //Laad Byte3 van command //if (nc == 1) SHOWBYTE(GPIOR1); *****18aug2017***
					bitClear(GPIOR2, 5); //Register GPIOR1 is nu niet meer vrij
					BC++;
				}
			}
			else { //Register GPIOR1= actieve register, dus de andere vullen 
				if (bitRead(GPIOR2, 4) == true) { //Alleen als bytefree true is					
					GPIOR0 = CBYTE[BC];//laad volgend byte uit command //CBYTE[2]; ***AANPASSING 18AUG2017	
					bitClear(GPIOR2, 4); //Register GPIOR0 is nu niet meer vrij
					BC++;
				}
			}
			//if (nc==1) Serial.println(BC);
		}
		else {// BC > BT Geen byte meer te verzenden 
			if (bitRead(GPIOR2, 3) == false) bitSet(GPIOR2, 2); //laatste byte is doorgegeven, alleen zetten als laatste byte in interrupt nog false is
		}
	}
}
//****************************

//*********************
void DCCLOOP_old() { //called from loop() V1.03
	/*
	CREG register 10 mogelijke posities 0-9
	bit7 true=bezet false=vrij
	bit6 True = CV  false=bediening
	bit5 timer
	bit4 timer
	bit3
	bit2 count aantal te verzenden msb
	bit1 count aantal te verzenden 
	bit0 count aantal te verzenden lsb
	*/
	static byte CBYTE[6]; //static geheugen voor het actieve Commando, switch modes uses 3, CV mode uses 6
	int b; //teller en flag voor new command helaas hele byte?
	byte part;
	if (bitRead(GPIOR2, 1) == false && bitRead(IORegist,3)==false) { //nieuw command gevraagt, ioregist,3 voorkomt zenden DCC tijdens zenden LCD text.
		b = 0;
		nc = 0;
		while (b < 10) { //Loop testing Command array
			if (bitRead(CREG[b], 7) == true) { //(a)    command gevonden
errcount++;

				

				if (bitRead(CREG[b], 5) == true) { //(b) Timerbit


					bitClear(CREG[b], 5);
					bitClear(CREG[b], 4); //reset de pauze timer



					CREG[b]--; //CREG[b] = CREG[b] - 1; //verlaagt de counter. 

					if (bitRead(CREG[b], 6) == true) {
						//CV command

					BT = 6;
					CBYTE[0] = CMSB[b];
					CBYTE[1] = CLSB[b];
					CBYTE[2] = CV1[b];
					CBYTE[3] = CV2[b];
					CBYTE[4] = CV3[b];
					CBYTE[5]= CERROR[b];
					}
					else {
						//switchcommand

					BT = 3; //send 3 bytes
					CBYTE[0] = CMSB[b];
					CBYTE[1] = CLSB[b];
					CBYTE[2] = CERROR[b];
					//version 1.04
					CBYTE[3] = 0;
					CBYTE[4] = 0;
					CBYTE[5] = 0;
					}

					b = 10; //uitspingen als command is gevonden
					nc = 1;
					part = CREG[b];
					part = part << 4;
					if (part == 0) CREG[b] = B00000000; //creg altijd vrij geven


				}
				else { //(b) timerbit5 = false
					/*
					Preg(0) staat op timer 2 dit proces is denk ik te traag waardoor een ecos via de sniffer de reeks van commandoos als verschillende commandoos vertaald
					dit aanpassen met een standaard timerlus gebaseerd op de ingebouwde timer 1 dus micros of millis.

					met millis en micros gaat het het niet worden.
					Proberen met een 'teller' op timer 1
					
					*/
					//V1.03

					//if(micros()-DCCtime>4){
					//	DCCtime = micros();

					if (bitRead(PREG, 0) == true) { //Slow timer clock ..? Deze clock aanpassen gaat denk ik te langzaam, gewoon met microos?
						//CREG[b] |= (1 << 5); //bitSet(CREG[b], 5); V1.03		
						
						CREG[b] |= (1 << 4); // bitSet(CREG[b], 4);
					}
					else { //timer laag
						if (bitRead(CREG[b], 4) == true) CREG[b] |= (1 << 5); //bitSet(CREG[b], 5);
					}
				}
			} //(a) register free

			b++;

		}
		if (nc == 0) {
			BT = 3;
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
					GPIOR1 = CBYTE[BC]; // laad volgende byte uit command //CBYTE[2]; //Laad Byte3 van command //if (nc == 1) SHOWBYTE(GPIOR1); *****18aug2017***
					bitClear(GPIOR2, 5); //Register GPIOR1 is nu niet meer vrij
					BC++;
				}
			}
			else { //Register GPIOR1= actieve register, dus de andere vullen 
				if (bitRead(GPIOR2, 4) == true) { //Alleen als bytefree true is					
					GPIOR0 = CBYTE[BC];//laad volgend byte uit command //CBYTE[2]; ***AANPASSING 18AUG2017	
					bitClear(GPIOR2, 4); //Register GPIOR0 is nu niet meer vrij
					BC++;
				}
			}
			//if (nc==1) Serial.println(BC);
		}
		else {// BC > BT Geen byte meer te verzenden 
			if (bitRead(GPIOR2, 3) == false) bitSet(GPIOR2, 2); //laatste byte is doorgegeven, alleen zetten als laatste byte in interrupt nog false is
		}
	}
}
//****************************
ISR(TIMER2_COMPB_vect) { //timer interupt timed en start de 'langzame' events. 

//PREG ^= (1 << 0); //bit0 in register PREG is timer op ongeveer een 4ms. V1.04 kan weg

//**slows scanning process factor 2
//IORegist ^= 1 << 7;
//if (bitRead(IORegist,7)==true)SwitchLoop();

SwitchLoop();
lcdswitch();
SLOWEVENTS();
if (bitRead(PrgRegist, 4) == true)is(); //stand switches naar beginstand. 
}

void lcdtxt(int pm) { //schrijft texten naar lcd
	switch (pm){
	case 0:
		l1 = "SchakelBord";
		l2 = "Versie ";
		l2.concat(versie);
		lcdrefresh = 0;
		break;
	case 10:		
			l2 = " DCC adres";		
			lcdrefresh = 1;
		break;
	case 11: //dcc adres value
		l1 = "(";
		l1.concat(programSw);
		l1.concat(")-DCC adres");

		lcdrefresh=2;  //volledig vervangen
		programmode = 12;
		break;

	case 12: //value dcc veranderd
		lcdrefresh = 1; //vervang alleen tweede lijn en set cursor op knipperen
		break;

	case 15:
		bitClear(PrgRegist, 1); //parametermode
		txtmem(1);
		lcdrefresh = 2;
		break;

	case 20:
		l2 = " Aansturing";
		lcdrefresh = 1; //vervang alleen tweede lijn en set cursor op knipperen		
			break;
	case 21:
		l1 = "(";
		l1.concat(programSw);
		l1.concat(")-Aansturing");
		//type aansturing 
		//bit4 Coupled, zijn de poorten gekoppeld true = ja
		//	bit3 both = false single = true (standaard 0)
		//bit0 is welke poort
		if (bitRead(SW2[programSw], 3) == false) { //single mode
			if (bitRead(SW2[programSw], 4) == false) { //not coupled

				if (bitRead(SW2[programSw], 0) == true) {
					poortsw = 4;
				}
				else {
					poortsw = 5;
				}
			}
			else { //coupled
				if (bitRead(SW2[programSw], 0) == true) {
					poortsw = 2;
				}
				else {
					poortsw =3;
				}
			}
		}
		else { //dual mode wissel <>
			poortsw=1;
		}
		txtpoort();
		lcdrefresh = 2;  //volledig vervangen
		programmode = 22;
		break;

	case 22:
		lcdrefresh = 1;
		break;
	
	case 30:
			l2 = " Knop type";
			lcdrefresh = 1; //vervang alleen tweede lijn en set cursor op knipperen
			break;
	case 31:
		//schakelaartype bit5 van sw2 true is impuls, false is continue
		//regel 1
		l1 = "(";
		l1.concat(programSw);
		l1.concat(")-Knop type");

			//regel 2
		txtknoptype();
		lcdrefresh = 2;
		programmode = 32;
		break;

	case 40:
		//bit7 van SW2 true = off!!! false is on... dit ivm opstart van een schone arduino
			l2 = " stand bij start";
			lcdrefresh = 1; //vervang alleen tweede lijn en set cursor op knipperen
			//bitSet(IORegist, 6);
			break;

	case 41:
		l1 = "(";
		l1.concat(programSw);
		l1.concat(")-Bij start");

		//regel 2
		txtstart();
		lcdrefresh = 2;
		programmode = 42;

		break;
	
	case 50:
			l2 = " CV instellen";
			lcdrefresh = 1; //vervang alleen tweede lijn en set cursor op knipperen
			//bitSet(IORegist, 6);
			break;
	case 51: //enter cvinstellen, keuze CV
		//l1 instellen -switch-  -adres- "CV nummer: "
		l1 = "(";
		l1.concat(programSw);
		l1.concat(")");
		l1.concat(" CV nummer:");

		l2 = CVnmem +1; //getoonde nummer= 1 hoger dan werkelijke cv nummer adres.

		programmode = 52; //verder in programloop case 6 encoder
		break;

	case 52:
		l2 = CVnmem + 1;
		break;
	case 53:
		l1 = "(";
		l1.concat(programSw);
		l1.concat(")");
		l1.concat("CV");
		l1.concat(CVnmem+1);
		l1.concat(" Waarde:");
		txtCV();
		programmode = 54;
		break;
	case 54:
		txtCV();
		break;

	case 55:
		//prgregist bit 2 is ja of nee
		l1 = ("Zend? CV");
		l1.concat(CVnmem+1);
		l1.concat("-");
		l1.concat(CVvmem);
		bitClear(PrgRegist, 2);
		txtCVjn();	
		programmode = 56;
		break;

	case 56:
		txtCVjn();
		break;

	}
	bitSet(IORegist, 6);
}
void ProgramLoop(int s) { //called by pushed lcd switch
	/*
	Twee switch loop lopen hier door elkaar:
	S=welke schakelaar of encoder
	Programmode= waar in het programeerproces
Iedere swich heeft twee bytes opslag in EEPROM
en iederen switch heeft opslag in intern geheugen
by opstarten EEPROM geheugen in interngeheugen plaatsen
bij programmeren, intern geheugen EN EEprom aanpassen.

SW1   byte 1 = [programsw] SW2  byte 2= [programsw] + 255

byte 1= 8 bits adres bit7=msb bit0=lsb bit9 van het adres (bit 6 in byte2 van het dcc signaal wordt NIET opgeslagen) dus
max dcc adres = 255 (1024 wissel adressen, 2048 single adressen)

Als EEPROM adres nog NIET is geprogrammeerd bij bv. nieuwe arduino dan is waarde 0xFF
byte SW2= 
bit7 not used
bit6 dcc continue of puls
bit5 switch continu of puls
bit4 = wissel of apart
bit3 both=false single=true (standaard 0)
bit2 welke decoder van de 4 bit 1
bit1 welke decoder van de 4 bit 0
bit0= welke van het paar. True=afslaan, false is rechtdoor
	*/
int temp = 0;
static int adrs; //dcc adres actief switch
//Serial.println(s);
	switch (s) {

//**********Encoder switch*************
	case 7: //ok of encoderswitch	

		switch (programmode) {
		case 0: //normal 
			//switch to program mode 10
			programmode = 10; //dccadres programmeren. 
			bitClear(PrgRegist, 1); //parameter mode //lcdtxt(programmode);		
			break;

		case 10: // prgram DCC adres
			txtmem(0);
			bitSet(PrgRegist, 1); //valuemode			
			adrs=prgadres(); //het dcc adres opzoeken van gekozen switch
			adreslcd(adrs); //plaats adres code in l2
			programmode = 11;	
		break;

		case 12: //if s7 is pushed again leave value mode, save adress
			
			//groep en dc bevatten adres
			programmode = 15; //toon txt= lcdtxt
			//save
			SW1[programSw] = groep;

			bitClear(SW2[programSw],1); 
			bitClear(SW2[programSw],2);

			switch (dc) {
			case 1:
				bitSet(SW2[programSw], 1);
				break;
			case 2:
				bitSet(SW2[programSw], 2);
				break;
			case 3:
				bitSet(SW2[programSw], 1);
				bitSet(SW2[programSw], 2);
				break;
			default:				
				break;
			}
			//save to Eprom 
			EEPROM.write(programSw, SW1[programSw]);
			EEPROM.write(programSw + 256, SW2[programSw]);			
			break;

		case 20:  //mode aansturing
			txtmem(0);
			bitSet(PrgRegist, 1); //valuemode	
			programmode = 21;
			break;

		case 22: //leave programmode
				 /*
				 SW2
				 bit7 not used
				 bit6 dcc continue of puls
				 bit5 switch continu of puls
				 bit4 Coupled, zijn de poorten gekoppeld true = ja
				 bit3 both=false single=true (standaard 0)
				 bit2 welke decoder van de 4 bit 1
				 bit1 welke decoder van de 4 bit 0
				 bit0= welke van het paar. True=afslaan, false is rechtdoor
				 
				 EEn nog niet geprogrammeerd eeprom adrs is hex FF

				 */

		programmode = 15;
		//save poortsw
		switch (poortsw) {
			case 1://Wissel <>
				bitSet(SW2[programSw], 3); 
				bitSet(SW2[programSw], 4);
				break;
			case 2://wissel <
				bitClear(SW2[programSw], 3);
				bitSet(SW2[programSw], 4);
				bitSet(SW2[programSw], 0);
				break;
			case 3: //wissel >
				bitClear(SW2[programSw], 3);
				bitSet(SW2[programSw], 4);
				bitClear(SW2[programSw], 0);
				break;
			case 4: //apart  <
				bitClear(SW2[programSw], 3);
				bitClear(SW2[programSw], 4);
				bitSet(SW2[programSw], 0);
				break;
			case 5: //apart >
				bitClear(SW2[programSw], 3);
				bitClear(SW2[programSw], 4);
				bitClear(SW2[programSw], 0);
				break;
		}

		EEPROM.write(programSw + 256, SW2[programSw]);
			break;//for case 22		

		case 30:
			txtmem(0);
			bitSet(PrgRegist, 1); //valuemode	
			programmode = 31;
			break;

		case 32:
			programmode = 15;
			EEPROM.write(programSw + 256, SW2[programSw]);
			break;

		case 40:
			txtmem(0);
			bitSet(PrgRegist, 1); //valuemode	
			programmode = 41;
			break;
		case 42:
			programmode = 15;
			EEPROM.write(programSw + 256, SW2[programSw]);
			break;

	case 50: //switch 7 programmode 50
		//Serial.println("nu dus naar cv program");
		bitSet(PrgRegist, 1); //Value mode
		txtmem(0);
		lcdrefresh = 2; //lcd geheel vervangen, cursor op lijn 2
		programmode = 51; //verder in lcdtxt
		break;//switch 7 programmode 51 exit
	case 52: //switch 7 pm50 >> naar CV value
		//staat al in value mode
		lcdrefresh = 2;
		programmode = 53;
		break;

	case 54:
		lcdrefresh = 2;
		programmode = 55;
		break;
	case 56: //confirm CV programming Prgregist bit2
		if (bitRead(PrgRegist, 2) == true) { //verzenden CV 
			//hier verzenden
			CV();
		}
		//retour naar begin 
		programmode = 15;
		break;

//**************Afsluiting Case 7 decoder switch****************
		}
		break; //for case 7 switch


//***************encoder increment decrement************
	case 6: // encoder channel A wordt hoog

		if (bitRead(PrgRegist, 1) == false) { //parameter mode
			PORTS[4] ^= (1 << 6); // toggle led

			if (bitRead(PrgRegist, 0) == true) { //increment, channel B==high
				programmode=programmode + 10;
				if (programmode > 50)programmode = 10;
			}
			else {//decrement, channel B low				
			programmode = programmode - 10;
			if (programmode < 10) programmode = 50;
			}
			//lcdtxt(programmode);
		}
		else { //value mode 
			PORTS[4] ^= (1 << 5);
			
			switch (programmode) {
			case 12: //dcc adres
				if (bitRead(PrgRegist, 0) == true) { //inc
					adrs++;
					if (adrs > 1020)adrs = 1;
				}
				else { //dec
					adrs--;
					if (adrs < 1)adrs = 1020;
				}
				adreslcd(adrs); //plaats adres in l2		
				break;

			case 22: //aansturing 
				if (bitRead(PrgRegist, 0) == true) { //inc
					poortsw ++;
					if (poortsw > 5)poortsw = 0;
				}
				else { //dec
					poortsw--;
					if (poortsw < 0) poortsw = 5;
				}
				txtpoort();
				break;

			case 32: 
				lcdrefresh = 1;
				SW2[programSw] ^= (1 << 5); //toggle bit 5
				txtknoptype();
				break;
			case 42:
				lcdrefresh = 1;
				SW2[programSw] ^= (1 << 7); //toggle bit 5
				txtstart();
				break;


			case 52: //case 6 encoder, CV keuze 
				//Serial.println("nu dus cv kiezen");
				if (bitRead(PrgRegist, 0) == true) { //inc
					CVnmem++;
				}
				else { //dec
					CVnmem--;
				}
				lcdrefresh = 1;
				break;

			case 54: //Serial.println("nu dus cv kiezen");
				if (bitRead(PrgRegist, 0) == true) { //inc
					CVvmem++;
				}
				else { //dec
					CVvmem--;
				}
				lcdrefresh = 1;
				break;

			case 56:
				//toggle ja nee bit
				PrgRegist ^= (1 << 2);
				lcdrefresh = 1;
				break;
			}			
			//lcdrefresh = 100; //niks met txt doen
		}

//*********Afsluiting Case 6 encoder ********
		break;
	}
	lcdtxt(programmode);
}
int prgadres() { //haal adres uit geheugen
	int r;	
	r = SW1[programSw]*4+1; //adres 0 bestaat niet in DCC is niet geprogrammeerd
	if (bitRead(SW2[programSw], 1) == true) r = r + 1;
	if (bitRead(SW2[programSw], 2) == true) r = r + 2;
	return r;	
}
byte DCCmsb() {
	//bepaal msb van dcc command
	//opbouw bit 0 tot bit 5 van adres
	byte msb = SW1[programSw];
	bitSet(msb, 7);
	bitClear(msb, 6);
	msb++; //DCC adres 0 bestaat niet, dus Hex00 wordt Hex01 
	return msb;
}
byte DCClsb() {

	//bepaal lsb van dcc command
	/*
	bit7 not used
	bit6 not used
	bit5 switch continue of puls
	bit4 dcc continue of puls
	bit3 both=false single=true (standaard 0)
	bit2 welke decoder van de 4 bit 1
	bit1 welke decoder van de 4 bit 0
	bit0= welke van het paar. True=afslaan, false is rechtdoor
	*/

	byte lsb = 192; //set bit7,6 adressbit 9 not used. 
		if (bitRead(SW1[programSw], 7) == false) bitSet(lsb, 5); //adresbit 8 (inverted, one complement)
		if (bitRead(SW1[programSw], 6) == false) bitSet(lsb, 4); //adresbit 7
		
		if (bitRead(PrgRegist, 3) == false) {//only switch mode, CV mode bits 3-0 false		
		//bitSet(lsb, 3); //on/off	in switched
		if (bitRead(SW2[programSw],2) == true) bitSet(lsb, 2);
		if (bitRead(SW2[programSw], 1)  == true) bitSet(lsb, 1);
		//if (bitRead(SW2[programSw], 3) == false) { //not wissel <> mode
			//if (bitRead(SW2[programSw], 0) == true)bitSet(lsb, 0);
		//}
		//bit 0 TIJDELIJK ff false stellen
		}

	return lsb;
}
void txtpoort() { //txt voor l2 aansturing 
	switch (poortsw) {
	case 1:
		l2 = " Wissel <>";
		break;
	case 2:
		l2 = " Wissel <";
		break;
	case 3:
		l2 = " Wissel >";
		break;
	case 4:
		l2 = " Apart <";
		break;
	case 5:
		l2 = " Apart >";
		break;
	}
}
void txtCV() {
	//maakt txt regel aan de hand van CVvmem 
	byte dv = 128;
	byte rest = CVvmem;
	
	l2 = rest;
	l2.concat(" (b)");
	for (int i = 0; i < 8; i++) {
		if (rest - dv >= 0) {
			rest = rest - dv;
			l2.concat("1");
		}
		else {
			l2.concat("0");
		}
		dv = dv >> 1;
	}
}
void txtCVjn() {
	//geeft ja nee optie in CV programming
	if (bitRead(PrgRegist, 2) == false) {
		l2 = " Nee, exit";
	}
	else {
		l2 = " Ja";
	}
}
void txtmem(byte A) {
	if (A == 0) {
	l1mem = l1;
	l2mem = l2;
	PMmem = programmode;
	}
	else {
		l1 = l1mem;
		l2 = l2mem;
		programmode = PMmem;
	}

}
void txtknoptype() {
	//schakelaartype bit5 van sw2 true is impuls, false is continue
	if (bitRead(SW2[programSw],5) == true) {
		l2 = " Moment";
	}
	else {
		l2 = " Continue";
	}
}
void txtstart() {
	//initial state switch bit7 van sw2 true uit, false is aan
	if (bitRead(SW2[programSw], 7) == true) {
		l2 = " Uit";
	}
	else {
		l2 = " Aan";
	}
}
void adreslcd(int a) { //plaats adres in l2
	 int c = 512;
	 int adres;
	adres = a; // -1;
	int mainadres = 0;
	for (i = 0; i < 8; i++) { 

		if (adres - c > 0) {
			adres = adres - c;
			mainadres = mainadres + (c / 4);
		}
		c = c / 2;
	}
	dc = adres; //geeft decoder in groep, rest van bovenstaande berekening
	//main adres geeft de groep
	if (mainadres == 255) {
	l2 = "Niet bepaald";
	}
	else {	
	l2 = a;
	l2.concat(" (");
	l2.concat(mainadres+1);
	l2.concat("-");
	l2.concat(dc); //dc bevat rest
	l2.concat(")");
	groep = mainadres;
	dc = dc-1;
 }
}
void lcdswitch() { //reads LCD switches and encoder called from ISR timer 2
	static int proces = 0;
	byte swon = 0; //holds switches being switched on
	switch (proces) {
	case 0: //begin 
		bitSet(IORegist, 4); //request state LCD switches
		proces = 10;
		break;
	case 10: //wait for reply lcdswitches request from IOloop
		if (bitRead(IORegist, 4) == false) {
			lcdchanged = lcdnew^lcdold;
			if (lcdchanged > 0) {
				proces = 20;
			}
			else {
				proces = 0; //no change back to begin
			}
		}
		lcdold = lcdnew;
		break;

	case 20: //changed switches found
		//prgregist bit 0 = state encoder L bit1 = state encoder R
		proces = 0;
		/*
		switches:
		encoder switch = bit 7
		encoder chA=bit 6
		encoder CHB= bit 5
		switch Left= bit 2
		switch right= bit 3
		switch ok= bit 4
		*/

		//stand van encoder channel B overnemen in prgregist,0
		if (bitRead(lcdchanged, 5) == true) {
			if (bitRead(lcdnew, 5) == false) { // before 20nov2017 	if (bitRead(lcdnew, 5) == true)	// changes direction of encoder	
				bitSet(PrgRegist, 0);
			}
			else {
				bitClear(PrgRegist, 0);
			}
		}

		swon = lcdchanged & lcdnew;
			if (bitRead(swon, 7) == true) ProgramLoop(7); //is swich 7 aangezet 
			if ((bitRead(swon, 6) == true)&&(programmode > 0)) ProgramLoop(6); //encoder channel A wordt hoog
			break;
	}
}
void LCDLoop() {  //aanroep uit isr timer 2 ??? 
	//lcddelay
	//current=100 = clockpuls
	static int sc = 0;
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
			lcdrefresh = 0;
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


		case 110: //clear LCD
			bitClear(PORTS[4], 7); //reset RS send command
			for (i = 0; i < 4; i++) {
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
			for (i = 0; i < 4; i++) {
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
			// na initialisatie altijd 200, wacht op veranderde flag
			//is er een veranderde txt?
			//byte klaar?
			//ioregist, bit 3= (LCD) Send TXT true text aan het verzenden, false klaar niks verzenden
			//ioregist, bit5=(LCD) send byte, byte staat klaar te verzenden, wordt verzonden, bij false klaar, wacht op nieuw byte
			//text in l1 en l2 

			if (bitRead(IORegist, 3) == true) { //nieuwe text, bij false gebeurt er gewoon niks, ieder lcddelay wordt hierop getest
			LCDDelay = 0; //raise speed
			cc = 0;
			lc = 0;
			switch (lcdrefresh) {
			case 0: //geheel vervangen		
				current = 110; //text volledig aanpassen
				break;

			case 1: //Alleen l2 vervangen, cursor begin regel 2
				lc = 1;	
				current = 250; //send new line
				sc = 1; // setcursor na dat txt is geplaatst
			break; 

			case 2://geheel vervangen, cursor begin regel 2
				current = 110;
				sc = 1;
				break;

			case 100: //Niks doen, txt niet aanpassen
				bitClear(IORegist, 3);
				break;
				}		
			}
			break; //for case 200

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
			for (i = 0; i < 4; i++) { //clear data nibble
				bitClear(PORTS[4], i);
			}
			for (i = 4; i < 8; i++) { //copy msn to datanibble
				if (bitRead(LCDbyte, i) == true) bitSet(PORTS[4], i - 4);
			}
			current = 100;
			next = 212;
			break;

		case 212: //send lsn
			for (i = 0; i < 4; i++) {
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
					
					if (sc == 1) {
						current = 250;
						sc = 2;
					}
					else {
						current = 200;
					}				

				}
				else { //line 2 verzenden
					current = 250;
				}
			}
					
				break;

		case 250: //cursor verplaatsen naar tweede regel, send adres 56 ? Hex40=64 toch?
			bitClear(PORTS[4], 7); //rs low, send command
			for (i = 0; i < 4; i++) {
				bitClear(PORTS[4], i);
			}
			bitSet(PORTS[4], 2); //msn adres
			bitSet(PORTS[4], 3);
			current = 100;
			next = 251;
			break;

		case 251:
			for (i = 0; i < 4; i++) { //lsn 
				bitClear(PORTS[4], i);
			}
			//bitSet(PORTS[4], 3);
			current = 100;

			if (sc == 2) {
				next = 200;
				sc = 0;
			}
			else {
				next = 210;	
			}
					
			break;

			//} 		
		}//end of switch	
		count = 0;
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
			if (bitRead(IORegist, 0) == true) { //request new state switches
				if (ByteCount + 1 == 0) { // select PISO register, negative numbers not allowed. ????
					RijRegister = PISO; //return PISO register
					bitClear(IORegist, 0); //clear flag request
				}
			}

			if (bitRead(IORegist, 4) == true) { //request new state LCD switches
				if (ByteCount == 4) {
					lcdnew = PISO;
					bitClear(IORegist, 4);
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
void is() { //is= initialise switches
	static byte r = 0; //
	static byte c = 0;
	static byte teller = 2; //aantal malen dat init wordt doorlopen
	//dient om bij opstarten de standen van de wissels te initialiseren.
	//nodig column, nummer in de rij
	//bit 4 in progregist geeft aan of initialisatie bezig is. 
	//bit 7 in sw2 geeft switchstand false -aan   true -uit
	//run via timer 2??? slow events...

	//kolom=0 //rij=0
	programSw = (r * 8) + (c + 1); //calc var programsw

	//Serial.println(SW1[programSw]);

	if ((SW1[programSw] < 255)&& (bitRead(SW2[programSw], 5) == true)) { 
	
	//	Serial.print("switch:    ");
	//	Serial.println(programSw);
	//	Serial.print("------- ");
	//Serial.println(SW1[programSw]);	

	
	//adres between 1 and 254 and switch type is momentary bit5
		CColums = c;
		if (bitRead(SW2[programSw], 7) == false) bitSet(SwitchState[CColums], r); //set opgeslagen stand switch
		leds(r); //set leds
		DCC(r); //make DCC commands
	}
	r++;
	if (r == 8) {
		r = 0;
		c++;
		if (c == 8) {
			c = 0;
			teller--;
			if (teller==0)	bitClear(PrgRegist, 4);
		}
	}

}
void Switched(byte k, byte r) {//handles switches
	//switchtype =  SW2 bit5 true = moment false = continue
	//bit4 van prgregist = initialise, if true, programsw = dan te zetten switch
//byte as=0;
//int adress = 0;
	for (int i = 0; i < 8; i++) { //kolom bepalen
		if (bitRead(k, i) == true) CColums = i; //kolom 0-7 
	}
	//xor het gemeten rij met de vorige meting
	SwitchChanged = r ^ SwitchRow[CColums];
	SwitchRow[CColums] = r; //save new rowstate	

	for (int i = 0; i < 8; i++) {
		if (bitRead(SwitchChanged, i) == true) { //found switch status changed			
			programSw = (i*8) + (CColums + 1);
				programmode = 0;
				PMmem = 0;	
				l1 = "Knop: ";
				l1.concat(programSw);
			if (bitRead(SW2[programSw], 5) == true) { //moment switch
				if (bitRead(r, i) == true) { //found switch has become true//***
					SwitchState[CColums] ^= (1 << i); //zet on/off invert, voor deze schakelaar
				} // switch become true		
			}
			else { //continue switch

				if (bitRead(r, i) == true) { //found switch has become true//***
					bitSet(SwitchState[CColums], i);
				}
				else {
					bitClear(SwitchState[CColums], i);
				}
			}
			leds(i); //sets led status
			DCC(i);	//creates DCC commands		
		}//found switch status changed
	}
	bitClear(PrgRegist, 3); //reset CV programmode	
}
void leds(int i) { //switches indicator leds, i=switch in row
	//leds instellen en LCD
	//LedRow[CColums] ^= 1 << i; //toggle indicatorled
	if (bitRead(SwitchState[CColums], i) == true) {
		bitSet(LedRow[CColums], i);
		if (bitRead(PrgRegist, 4) == false) l2 = "Aan";
	}
	else {
		bitClear(LedRow[CColums], i);
		if (bitRead(PrgRegist, 4) == false) l2 = "Uit";
	}
	if (bitRead(PrgRegist, 4) == false) {
	lcdrefresh = 0;
	bitSet(IORegist, 6); //vragen om nieuwe text (vertraagd via loop())
	}

}
void DCC(int i) { //creates DCC commands i=switch in row
	int c = 0;
	while (c < 10) { //voorlopig met 10 'registers 'voor commands werken
		if (bitRead(CREG[c], 7) == false) { //vrije command, adres=( kolom*8 + rij) command staat in eeprom byte adres*2 en adres*2 +1	
			
			CREG[c] |= (1 << 7); //bitSet(CREG[c], 7); //claim dit command geheugen			
			bitClear(CREG[c], 6);//command voor bediening (true is voor CV zenden)
			CREG[c] |= (1 << 3);//Count aantal keren zenden
			
			
			CMSB[c] = DCCmsb();
			CLSB[c] = DCClsb();
			if (bitRead(SW2[programSw], 3) == true) { //both channels on 1 switch
				CLSB[c] |= (1 << 3);//always on						
				if (bitRead(SwitchState[CColums], i) == true) CLSB[c] |= (1 << 0); //switch channel			
			}
			else { //only 1 channel on switch
				   //twee mogelijkheden, wissel of apart 
				if (bitRead(SW2[programSw], 4) == true) {//wissel
					if (bitRead(SW2[programSw], 0) == true) CLSB[c] |= (1 << 0); //select channel
					CLSB[c] |= (1 << 3); //channel on always
				}
				else { //apart
					if (bitRead(SW2[programSw], 0) == true) CLSB[c] |= (1 << 0); //select channel
					if (bitRead(SwitchState[CColums], i) == true) CLSB[c] |= (1 << 3); //channel on/off
				}
			}
			CERROR[c] = CMSB[c] ^ CLSB[c]; //xor bytes
			c = 10;
		}
		c++; //Als geen vrij commandplek wordt gevonden, bij volgende doorloop wordt opnieuw gezocht.
	}
}
void CV() { //creates CV commands	
	
	bitSet(PrgRegist, 3); //CV mode needed for creating byte 2 of dcc command
	int c = 0;
	while (c < 10) { //voorlopig met 10 'registers 'voor commands werken
		//errcount++;
		if (bitRead(CREG[c], 7) == false) { //vrije command, adres=( kolom*8 + rij) command staat in eeprom byte adres*2 en adres*2 +1


			CREG[c] |= (1 << 7); //claim dit command geheugen

			CREG[c] |= (1 << 6); //command true is voor CV zenden
			CREG[c] |= (1 << 3); //zet uitvoer counter op 1.Twee keer zenden.

			CMSB[c] = DCCmsb(); //make first two adress bytes, programsw is allready known
			CLSB[c] = DCClsb();
			CV1[c] = B11101100;
			CV2[c] = CVnmem;
			CV3[c] = CVvmem;
			CERROR[c] = CMSB[c] ^ CLSB[c] ^ CV1[c] ^ CV2[c] ^ CV3[c]; //xor bytes
			c = 12;
		}
		c++; //Als geen vrij commandplek wordt gevonden, bij volgende doorloop wordt opnieuw gezocht.
	}
	bitClear(PrgRegist, 3); //reset CV mode to switch mode
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
					
					Doorloop = 20; 
					//PORTS[4] = B01000000;

				//bitcount is nu vrij gebruiken als teller voor LCD init		
					//snelheid aanpassen
					SPEED = 200; 
					Bitcount = 0;
				}
				break;

		case 20:
			PORTS[2] = B11111111;
			PORTS[3] = 0;
			bitClear(IORegist, 2); //verlaat test mode
			lcdtxt(0);
			bitSet(PrgRegist, 4); //start init stand switches
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
	//TCCR2B |= (1 << CS20); //set prescaler op 1024, result overflow every 16 millisec. **11okt2017**
	OCR2B = 100; //timerclock wordt NIET gereset dus frequency bepaald door overflow en prescaler
	sei(); //interupts weer toestaan	
}
void setup() {
	// test en hulp instellingen
	Serial.begin(9600);
	LoadData();	
	bitSet(IORegist, 2); //test modus inschakelen
	DDRB |= (1 << DD3); //set PIN11 as output, Led voor INPUTCOMMAND
	PORTB |= (1 << PORTB3); //Set high, led off
							//PINC = 0; //clear alle inputs
							//OLDCP = PINC; //kopieer huidige PINC	//voor ontwerp knoppen en command berekening even uitzetten
							// TRAINSETUP();
	PORTC |= (1 << PORTC3);//enable LCD hoog
	//bitClear(PORTS[4], 6); //LCD enable hoog via shiftregister
	SetupDCC();

}
void loop() {	
	DCCLOOP();
	IOLoop();
	if (millis() - tijd > 300) { // ververs periode van LCD
		//LCD txt vervangen
		if (bitRead(IORegist, 6) == true) {	
			waarom=(bitRead(LedRow[0],0)); ///niet te begrijpen wel noodzakelijk ander doen een hele rits leds in colum 0 het niet...vermodelijk probleem met de compiler
			bitClear(IORegist, 6);
			bitSet(IORegist, 3);			
		}
		//if (bitRead(PrgRegist, 4) == true)is(); //stand switches naar beginstand. 
		tijd = millis();
	}

	//temp
	if (millis() - counttijd > 3000) {
			Serial.print("count:  ");
			Serial.println(errcount);
			errcount = 0;
			counttijd = millis();
	}


}





