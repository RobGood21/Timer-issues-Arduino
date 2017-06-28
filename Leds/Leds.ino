/*
 Name:		Leds.ino
 Created:	6/20/2017 8:39:57 AM
 Author:	Rob Antonisse
 description: Many different tests and try-outs. For using many to infinite leds. Knowledge needed for various projects in Wisselmotor. nl.

*/

//declaraties
unsigned long Tijd;
unsigned long Periode;
//tbv LEDS
const int AantalPORTS = 5; //Aantal aangesloten schuifregisters. Xtra seriele poorten. 
byte PORTS[AantalPORTS]; // declareer array

byte RijRegister = B00000000; // bevat ingelezen rij switches
byte IORegist = B00000000; //bevat booleans voor de schakelaar, leds processen
//bit7=nc  bit6=nc  bit5=nc  bit4=nc  bit 3=nc bit2=nc  bit1=nc  bit0=vraag lezen PISO register. (rijen van switches) true is bezig false =klaar



void setup() {
	// Poort instellen, PORTC  Pins A0...A5  Poort PC0...PC5
	LedSetup();
	Serial.begin(9600);
	Tijd = millis();
}
void LedSetup() { //leds aansturen dmv een shiftregister 
	DDRC |= (1 << PC0); // set A0 as output = shiftclock  (pin11)
	DDRC |= (1 << PC1); //set A1 as output = shift Latch  (pin12)
	DDRC |= (1 << PC2); //set A2 as output =Data in (Pin14)
	DDRB &= ~(1 << PB0); // set PORTB0 (PIN8-Arduino) as input
	//nulstellen alle Byteout
	for (int i = 0; i < AantalPORTS; i++) { //clear all used ports
		PORTS[i] = B00000000;
	}
	//PORTS[2] = ~PORTS[2]; //Invert Column port Matrix, not needed. 
}
void IOLoop() { //ioloop= in-out loop
/* 
zet continue de PORTS[n] in schuifregisters, toont deze.En leest de schakelaars uit. 
Per doorloop wordt maar 1 instructie gedaan.
*/		
		static int STATUS = 100;  //State of process
		static int SBiT = 0; //bitcounter
		static byte SendByte = 0;//Current byte in process
		static int ByteCount = AantalPORTS-1; //number of to shift bytes
		static byte PISO = B00000000; // Temporary register for reading of row switches

		bitClear(PORTC, 0); //clear shiftclock

		switch (STATUS) {
		case 100 : //start new cycle
			PISO = B00000000; 
			if (ByteCount  >= 0) {
				SendByte = PORTS[ByteCount];
			STATUS = 101;
			SBiT = 0;
			ByteCount --;
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
			if (bitRead(PINB,PB0)==true)bitSet(PISO, SBiT); //Get value output shiftregisters	
			bitSet(PORTC, 0); //set shift clock
			SBiT ++; //next bit
			if (SBiT > 7) { //sending current byte ready
				STATUS = 100;
				if (bitRead(IORegist, 0) == true) { //Request for row read
					if (ByteCount == 3) { // select PISO register
						RijRegister = PISO; //return PISO register read
						bitClear(IORegist, 0); //clear flag request
					}
				}
			}
			break;
		case 102: // lock in and outputs
			bitClear(PORTC, 1); //clear register clock
			STATUS = 100;
			ByteCount = AantalPORTS-1;
			break;
			default:
	break;
}
}
void KloK() {

	//PORTS[4] = B00000001;
	//PORTS[3] = B01111111;
	static int i = 0;
	static byte r=B11111110; // rijen voor uit HOOG
	static byte k; //kolommen voor uit LAAG
	byte b;
	i++;
		switch (i) {
	case 10:
		b = B11111100;
		i = 0;
		r = r << 1;
		r = r + B00000001;
		k = B00000000;

		//if (bitRead(PORTS[3],7)==false) r = B11111110;
		
		break;
	case 1:
		b = B01100000;
		k = B00000001;
		break;
	case 2:
		b = B11011010;
		k = B00000010;
		break;
	case 3:
		b = B11110010;
		k = B00000100;
		break;
	case 4:
		b = B01100110;
		k = k << 1;
		break;
	case 5:
		b = B10110110;
		k = k << 1;
		break;
	case 6:
		b = B10111110;
		k = k << 1;
		break;
	case 7:
		b = B11100000;
		k = k << 1; 
		break;
	case 8:
		b = B11111110;
		k = k << 1;
		break;
	case 9:
		b = B11110110;
		k = k << 1;
				break;

	}
PORTS[2] = b;

}
void SwitchLoop() {	//leest de schakelaars
	static int fase = 0;
	static byte Rk = B10000000; //ReadKolom
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
			PORTS[3] = ~RijRegister; //load inverted row to led matrix
			PORTS[4] = Rk; //colums to ledmatrix

			//Serial.print("Rijregister=   ");
			//Serial.println(PORTS[3]);

			//Serial.print("kolomregister=   "); 
			//Serial.println(PORTS[4]);

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
			Rk=Rk << 1;	
			break;
		}
		fase = 0; //reset new cycle
		break;		
	}
}
void loop() {

//	LoopLicht();
	IOLoop(); 
	if (millis() - Tijd > 1) { //'iedere seconde dus'
		//KloK();
		SwitchLoop();
		Tijd = millis();
	}
}
