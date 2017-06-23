/*
 Name:		Leds.ino
 Created:	6/20/2017 8:39:57 AM
 Author:	Rob Antonisse
 description: Many different tests and try-outs. For using many to infinite leds. Knowledge needed for various projects in Wisselmotor. nl. 
 
*/
unsigned long Tijd;
unsigned long Periode;

void setup() {
	/* Poort instellen, PORTC  Pins A0...A5  Poort PC0...PC5

	pinMode(A0, OUTPUT);
	pinMode(A1, OUTPUT);
	pinMode(A2, OUTPUT);
	
	*/
	LedSetup();
	Serial.begin(9600);

	//indicatie ledje. gebeurt er wel wat...?
	pinMode(13, OUTPUT);
	digitalWrite(13, HIGH);

	Tijd = millis();
}

//************* LEDS

byte WAARDE = B00000000;

const int AantalBytes = 2;
byte BYTEout[AantalBytes]; // aantal te verzenden bytes

//hoort eigenlijk in de ledloop
	int STATUS = 100;  //voortgang proces
	int SBiT = 0; //gestuurd bit, bitcounter
	byte SendByte=0;
	int ByteCount = 0;





void LedSetup() { //leds aansturen dmv een shiftregister 

	DDRC |= (1 << PC0); // set A0 as output = shiftclock 
	DDRC |= (1 << PC1); //set A1 as output = shift Latch 
	DDRC |= (1 << PC2); //set A2 as output =Data in 

	//nulstellen alle Byteout
	for (int i = 0; i < AantalBytes; i++) {
		BYTEout[i] = B00000000;
	}


}

void GetByte(int n) { // bepaal het volgende te zenden byte
	

	switch (n) {
		/*
		Het aantal shiftregisters staat hier in Bytes
		dus byteout[0] het eerste register in de rij, byteout[1] de tweede en byteout[n] de n-de (-1) 
		De verschillende bits in deze bytes zijn de pins, outputs die we willen uitschuiven. 
		Eigenlijk hoeft hier dus niks te gebeuren. Als andere programma delen deze bytes aanpasse om een gewenste functie uit te sturen
		Alleen als we bv, een teller of een andere automatische beweging willen kan hier deze op de bytes worden toepast
		
		*/
	case 0:
		// 1e byte wordt ff doorgeschoven dus 
		if (bitRead(BYTEout[1], 1) == true) {
			BYTEout[0] = B00000000;
		}
		else {
			BYTEout[0] = B11111111;
		}
				//BYTEout[0] = B11111111; // eereste byte
		break;
	case 1: // dit is een teller op de tweede shift register
		BYTEout[1] = BYTEout[1] + B00000001;  //tweede byte
		if (BYTEout[1] == B11111111) BYTEout[1] = B00000000;

		break;
	}
}

void LedLoop() {
		//aan/uit status van 8 leds via een shiftregister instellen Data staat in out
		bitClear(PORTC, 0); //shiftclock false

	// Serial.println(STATUS);
	// Serial.println(WAARDE);

	switch (STATUS) {
		case 100 : //begin proces
//te verzenden Byte ophalen , voorlopig maar 1 byte

			if (ByteCount  < AantalBytes) {
				SendByte = BYTEout[ByteCount]; // BYTEout[ByteCount-1]; dit komt straks
			STATUS = 101;
			SBiT = 0;
			ByteCount ++;
			GetByte(ByteCount);
			}
			else { //alle bytes verzonden
				//outputs shiftregisters tonen
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
			delay(300); //voor test hier ff wachten

			//WAARDE = WAARDE + B00000001;
			//if (WAARDE == B11111111) WAARDE = B00000000;

			//cyclus opnieuw
			STATUS = 100;
			ByteCount = 0;
			GetByte(ByteCount); //bepaal waarde van eerste byte

			//indicatie ledje 
			if (digitalRead(13) == HIGH) {
				digitalWrite(13, LOW);
			}
			else {
				digitalWrite(13, HIGH);
			}
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
	i++;
	switch(i){
	case 0:
		BYTEout[0] = B11111100;
		break;
	case 1:

		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:
		break;
		case 7:
			break;
		case 8:
			break;
		case 9:
			break;
		case 10:
			i = 0;
			break;


}


void loop() {
//	LoopLicht();
	LedLoop(); 
if (millis()-Tijd>1000){ //'iedere seconde dus'
	KloK();
	Tijd = millis();
}
