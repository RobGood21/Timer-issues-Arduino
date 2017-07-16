/*
Programma scetch LCDScherm
Auteur: Rob Antonisse
dd: 13juli017

Programma voor testen en onderzoek van standaard LCD scherm gebaseed op de hitachi HD44780

*/

//4 pin mode
//declaraties

//pinnen dus in volgorde
const int DATAPIN = 2; //eerste in in oplopende rij
//Pin2=db7 =p+1
//pin3=db6 =p+2
//pin4=db5 =p+3
//pin5=db4 =p+4
//pin11=enable
const int rs = 12;
const int en = 11;


unsigned long klok = 0;
int C = 0;
char regel1[16]="Wisselmotor.nl";
char regel2[16]="Check it out";

void start() {
digitalWrite(rs, LOW);
digitalWrite(DATAPIN, LOW);
digitalWrite(DATAPIN + 1, LOW);
digitalWrite(DATAPIN + 2, HIGH);
digitalWrite(DATAPIN + 3, HIGH);
clk();
delay(2);
clk();
clk();
digitalWrite(DATAPIN + 3, LOW);
clk();
//LCD in 4-bit state now
//commands set rs=LOW

//display clear
//sendbyte(B00000001); 

//return home
// sendbyte(B00000010); 

//bit1=I/D 0=verplaats cursor naar rechts 1=verplaats naar links, bit0=S verplaats hele display ipv de cursor. 
//sendbyte(B00000100)  

//display control
//bit2=D 0=display off 1=display on, bit1=C show cursoe 1=on, bit0=B blinks the cursor
sendbyte(B00001100); 

//cursor or display shift ???? geen idee nu wat er mee te doen ...
/*
Cursor or display shift shifts the cursor position or display to the right or left without writing or reading
display data (Table 7). This function is used to correct or search the display. In a 2-line display, the cursor
moves to the second line when it passes the 40th digit of the first line. Note that the first and second line
displays will shift at the same time.
When the displayed data is shifted repeatedly each line moves only horizontally. The second line display
does not shift into the first line position.
The address counter (AC) contents will not change if the only action performed is a display shift.
*/
//bit3=S/C  bit2=R/L 
//sendbyte(B00010000);

//functions
/*
DL: Sets the interface data length. Data is sent or received in 8-bit lengths (DB7 to DB0) when DL is 1,
and in 4-bit lengths (DB7 to DB4) when DL is 0.When 4-bit length is selected, data must be sent or
received twice.
N: Sets the number of display lines.
F: Sets the character font.
Note: Perform the function at the head of the program before executing any instructions (except for the
read busy flag and address instruction). From this point, the function set instruction cannot be
executed unless the interface data length is changed.
*/
//sendbyte(B00100000);

//set CGRAM adress
/*
Set CGRAM address sets the CGRAM address binary AAAAAA into the address counter.
Data is then written to or read from the MPU for CGRAM.
*/
//sendbyte(B01000000);

//Set DDRAM address in address counter.
//Adres waar de cursor naar toe gaat 
//sendbyte(B01000000);


}

void sendbyte(byte data) {

	if (digitalRead(rs) == HIGH) if (data == 0)data = 32;


	int p;
	p = DATAPIN;

for (int i = 0; i < 8; i++) {
	if (bitRead(data, 7) == true) { 
		digitalWrite(p, HIGH);
	}
	else {
		digitalWrite(p, LOW);
	}
	p++; //volgende pin
	data = data << 1; //byte naar links schuiven
	//Serial.println(bitRead(data,7));
	//delay(500);
	if (i == 3) { //eerste nibble staat nu op de pinnen
		clk();
		p = DATAPIN; //terug naar eerste pin
		}
	}
	clk();
}

	
void clk() {
//stuurt een enable signaal
	digitalWrite(en, LOW);
	delay(1);
	digitalWrite(en, HIGH);
	delay(1);
}
void Doeiets() {
	static int teller = 0;
	static int t = 0;
	byte s;
		digitalWrite(rs, HIGH);
	//sendbyte(B01010111);
		int p;

		if (teller < 16) {
			sendbyte(regel1[t]);
			//p = 'A';
		}
		else {
			sendbyte(regel2[t]);
		}
		t++;
	//Serial.println(C);
		teller++;
	if (teller == 16) { //1e regel vol met atjes
		digitalWrite(rs, LOW);
		//text teller reset
		t = 0;
		//adres verzetten naar 40 (tweede lijn)
		s =184; //=128 (bit7=true + 56=adres =bin 111000
		//sendbyte(B10111000); //nieuw adres voor de cursor 
		sendbyte(s); //decimaal
		//Serial.println(s);
		digitalWrite(rs, HIGH);
	}
	if (teller>32){ //opnieuw

//delay(3000);
		digitalWrite(rs, LOW);
		sendbyte(B00000001);
		//delay(2);
		sendbyte(B00000010);
		delay(1000);
		t = 0;
		teller = 0;
		
	}
}
void setup() {
	delay(250);
	Serial.begin(9600);
//poorten instellen
	for (int i = 2; i < 13; i++) {
		pinMode(i, OUTPUT); // alle pinnen als output
	}
		digitalWrite(en, HIGH);
	//digitalWrite(10, LOW);
	start();
}

void loop() {
	if (millis() - klok >50) {
		 Doeiets();
		//digitalWrite(rs, HIGH);
		//sendbyte(B00010100);
		klok = millis();
	}	
}