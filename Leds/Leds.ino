/*
 Name:		Leds.ino
 Created:	6/20/2017 8:39:57 AM
 Author:	Rob Antonisse
 description: Many different tests and try-outs. For using many to infinite leds. Knowledge needed for various projects in Wisselmotor. nl.

*/

//declaraties
unsigned long Tijd;
unsigned long T2 = 0;
unsigned int Periode=0;
//tbv LEDS
const int AantalPORTS = 4; //Aantal aangesloten schuifregisters. Xtra seriele poorten. 
byte PORTS[AantalPORTS]; // declareer array
//0=PISO 1=SIPO voor switches 2=rijen ledmatrix 3=kolommen ledmatrix
byte RijRegister = B00000000; // bevat ingelezen rij switches
byte IORegist = B00000000; //bevat booleans voor de schakelaar, leds processen
//bit7=nc  bit6=nc  bit5=nc  bit4=nc  bit 3=nc bit2=OpstartTest true=aan false=uit  bit1=SSC switch status changed  bit0=vraag lezen PISO register. (rijen van switches) true is bezig false =klaar

void TimerSetup() {
	cli();
	// Timer2, slowevents and inputs control
	TIMSK2 |= (1 << OCIE2B); // enable de OCR2B interrupt van Timer2
	TCNT2 = 0; //Set Timer 2 bottom waarde
	TCCR2B = 0;

	// TCCR2B |= (1 << CS22); //set prescaler 
	//TCCR2B |= (1 << CS21); //set prescaler
	//No Prescaler timing dus 1byte = system clock/255
	TCCR2B |= (1 << CS20); //set prescaler op 1024, result overflow every 16 millisec.

	OCR2B = 100; //timerclock wordt NIET gereset dus frequency bepaald door overflow en prescaler
	sei();
}
ISR(TIMER2_COMPB_vect) {

	IOLoop();
	/*
	// slow indicatie op led 13
	Periode++;
	if (Periode > 10000) {
		PORTB ^= (1 << PB5);

		Periode = 0;
	}
*/
}

void LedTest() { //bij opstarten alle leds even laten oplichten
	static int t = 0;
	static int Doorloop = 0;
	static int Bitcount = 0;
	static byte Temp=B00000000;
	static int SPEED = 50;
		t++;
		if (t > SPEED) { 
			t = 0;
			//PORTS[1] = B11111111;

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
			Bitcount ++;

			//Serial.println(PORTS[3]);

			if (Bitcount == 8) {
				Bitcount = 0;
				Temp = Temp << 1;

				PORTS[3] = B00000001;
				PORTS[2] = ~Temp;

				if (Temp == B00000000)Doorloop = 20; //alle rijen af gelopen

				//if (Temp == B00000000)Doorloop = 0;
								
			}
			break;
		case 20:
			PORTS[2] = B11111111;
			PORTS[3] = 0;
			bitClear(IORegist, 2); //verlaat test mode
			break;
	}
}
}
void setup() {
	// Poort instellen, PORTC  Pins A0...A5  Poort PC0...PC5
	LedSetup();
	Serial.begin(9600);
	Tijd = millis();
	T2 = millis();
	pinMode(13, OUTPUT);
	TimerSetup();

	bitSet(IORegist, 2); //test modus inschakelen
}
void LedSetup() { //leds aansturen dmv een shiftregister 
	DDRC |= (1 << PC0); // set A0 as output = shiftclock  (pin11)
	DDRC |= (1 << PC1); //set A1 as output = shift Latch  (pin12)
	DDRC |= (1 << PC2); //set A2 as output =Data in (Pin14)
	DDRB &= ~(1 << PB0); // set PORTB0 (PIN8-Arduino) as input
	//nulstellen alle Byteout
	for (int i = 0; i < AantalPORTS; i++) { //clear all used ports
		PORTS[i] = B00000000;

		//PORTS[1] = B11111111; //?????waarom
	}
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
					if (ByteCount+1 == 0) { // select PISO register, negative numbers not allowed.
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

			//LedTest(); //continue leds even late lopen

			//hier het laden van de default, opgeslagen stand uitvoeren???
			if (bitRead(IORegist, 2) == true) {
				LedTest();
			}
			else {
			SwitchLoop();
			LedLoop();
			}
		

			break;
			default:
	break;
}
}
void KloK() {
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

byte SwitchRow[8] = { 0, 0, 0, 0, 0, 0, 0,0 };
byte LedRow[8] = { 0,0,0,0,0,0,0,0 };
byte SwitchChanged;
int CColums = 0; //column in processing

void Switched(byte k, byte r) {//handles switches
	//uitzoeken welke column is gemeten

	for (int i = 0; i < 8; i++) {
		//Serial.println(k);
		if (bitRead(k, i) == true) CColums = i;
		//i = 10;
	}	
	//xor het gemeten rij met de vorige meting
	SwitchChanged = r ^ SwitchRow[CColums];
	SwitchRow[CColums] = r; //save new rowstate
	//bitSet(IORegist, 1); //set changed flag
	
	for (int i = 0; i < 8; i++) {
		if (bitRead(SwitchChanged, i) == true) { 
			if (bitRead(r, i) == true) { //found switch has become true
				LedRow[CColums] ^= 1 << i;
			}
			
			
			
			Serial.print("kolom   :");
			Serial.println(CColums);
			Serial.print("rij   :");
			Serial.println(i);
			delay(1000);

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
			Switched(Rk, RijRegister);
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
void LedLoop() { 

static int t = 0;

	byte r; //ports[3] (geinverteerd)
	static byte kolom; // = B00000000; //ports[4]

	switch (t){

	case 0:
		kolom = B00000001;	
		break;
	
	default:
		kolom=kolom << 1;
		break;
	}

//Serial.println(t);
//Serial.println(kolom);


	r = LedRow[t];

	

			PORTS[3] = kolom; //colums to ledmatrix
			//PORTS[3] = B11100111;
			PORTS[2] = ~ r; //load inverted row to led matrix
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
void loop() {

//	LoopLicht();
	//IOLoop(); 
	//delay(1);
	
	/*
	

	if (millis() - Tijd > 1) { //'iedere seconde dus'
		//KloK();
		SwitchLoop();
		Tijd = millis();
	}


	if (millis() - T2 > 0) {
		//LedLoop();
		T2 = millis();
	}

*/


}





