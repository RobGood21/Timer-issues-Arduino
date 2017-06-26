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


void setup() {
	// Poort instellen, PORTC  Pins A0...A5  Poort PC0...PC5
	LedSetup();
	Serial.begin(9600);

	//indicatie ledje. gebeurt er wel wat...?
	pinMode(13, OUTPUT);
	digitalWrite(13, HIGH);
	Tijd = millis();
}
void LedMatrixSetup(){
	/*
	Rijen R0 = A4   R1=B5  R2=B4  R3=A1  R4=B2  R5=A2  R6=A6 R7=A7
	koppen  K0=B3  K1=B6  K2=B1  K3=B7  K4=A3  K5=B0 K6=A5  K7A0

	alle rijen komen direct aan de ports Staan bij False Hoog.
	Alle koppen komen via 220ohm aan de ports Staan bij false laag.

	Rijen komen op PORTS[2]
	koppen komen op ports[3]
	*/
	PORTS[2] = B11111111; // alle hoog zodat alles uit is


}
void LedSetup() { //leds aansturen dmv een shiftregister 
	DDRC |= (1 << PC0); // set A0 as output = shiftclock  (pin11)
	DDRC |= (1 << PC1); //set A1 as output = shift Latch  (pin12)
	DDRC |= (1 << PC2); //set A2 as output =Data in (Pin14)
	//nulstellen alle Byteout

	for (int i = 0; i < AantalPORTS; i++) {
		PORTS[i] = B00000000;
			}
	LedMatrixSetup(); // volgorde belangrijk. 


}

void LedLoop() { /
/*
zet continue de PORTS[n] in schuifregisters, toont deze. 
Per doorloop wordt maar 1 instructie gedaan, bv schuif door-plaats bit- zet outputs schuifregisters gelijk aan de inhoud ervan. 
Hierdoor is de footprint van dit klein, zal waarschijnlijk geen timer issues geven. 
*/		
		bitClear(PORTC, 0); //shiftclock false
		static int STATUS = 100;  //voortgang proces
		static int SBiT = 0; //gestuurd bit, bitcounter
		static byte SendByte = 0;
		static int ByteCount = AantalPORTS-1;

	switch (STATUS) {
		case 100 : //begin proces
			if (ByteCount  >= 0) {
				SendByte = PORTS[ByteCount];
			STATUS = 101;
			SBiT = 0;
			ByteCount --;
			}
			else { //alle bytes verzonden //outputs shiftregisters tonen
				bitSet(PORTC, 1);
				STATUS = 102;
			}
	break;
		case 101: //bits sturen
			if (bitRead(SendByte, SBiT) == true) {
				bitSet(PORTC, 2);
			}
			else {
				bitClear(PORTC, 2);
			}
			bitSet(PORTC, 0); //set shift clock true
			SBiT ++; //volgende bit
			if (SBiT > 7) STATUS = 100; //bij volgende doorloop terug naar 100
			break;
		case 102: // Outputs worden getoond
			bitClear(PORTC, 1); 
			STATUS = 100; 
			ByteCount = AantalPORTS-1; //cyclus opnieuw
			break;
			default:
	break;
}
}
void LoopLicht() { // laad ledje lopen
	delay(100);
	//indicatie ledje 
	if (digitalRead(13) == HIGH) {
		digitalWrite(13, LOW);
	}
	else {
		digitalWrite(13, HIGH);
	}

	// LedLoop(); //bij iedere doorloop aanroepen LED functie

	//1 bit true maken
	bitSet(PORTC, 2); //data 1
	delay(100);

	bitSet(PORTC, 0); //clock shift
	delay(100);

	bitClear(PORTC, 0); //clock shift uit
	delay(100);

	bitClear(PORTC, 2); //data 0
	delay(100);

	bitSet(PORTC, 1); //toon 1
	delay(100);

	bitClear(PORTC, 1); //toon 0
						//delay(1000);
	
	for (int i = 0; i < 8; i++) { //dus 8 x doen
		bitSet(PORTC, 0); //shift 1
		delay(100);
		bitClear(PORTC, 0); //shift 0
		delay(100);
		bitSet(PORTC, 1); //toon 1
		delay(100);
		bitClear(PORTC, 1); //toon 0
		delay(100);
	}
}

void KloK() {
	static int i = 0;
	static byte r=B11111110; // rijen voor uit HOOG
	static byte k; //kollomen voor uit LAAG
	byte b;
	i++;
	switch (i) {
	case 10:
		b = B11111100;
		i = 0;
		r = r << 1;
		r = r + B00000001;
		k = B00000000;
		if (bitRead(PORTS[3],7)==false) r = B11111110;
		
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
// PORTS[0] = PORTS[0] + B00000001;

PORTS[3] = r;
PORTS[4] = k;

//teller op byte1


}


void loop() {

//	LoopLicht();
	LedLoop(); 
	if (millis() - Tijd > 100) { //'iedere seconde dus'
		KloK();
		Tijd = millis();
	}
}
