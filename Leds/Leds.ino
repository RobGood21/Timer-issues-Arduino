/*
 Name:		Leds.ino
 Created:	6/20/2017 8:39:57 AM
 Author:	Rob Antonisse
 description: Many different tests and try-outs. For using many to infinite leds. Knowledge needed for various projects in Wisselmotor. nl. 
 
*/



void setup() {
	/* Poort instellen, PORTC  Pins A0...A5  Poort PC0...PC5
	*/
	DDRC |= (1 << PC0); // set A0 as output = shiftclock 
	DDRC |= (1 << PC1); //set A1 as output = shift Latch 
	DDRC |= (1 << PC2); //set A2 as output =Data in 

	pinMode(13, OUTPUT);
	digitalWrite(13, HIGH);

}

void loop() {
	if (digitalRead(13) == HIGH) {
		digitalWrite(13, LOW);
	}else {
		digitalWrite(13, HIGH);
	}
	
	bitSet(PORTC, 2);
	delay(10);

	bitSet(PORTC, 0);
	//delay(10);
	bitClear(PORTC, 2);
	//delay(10);		
	bitClear(PORTC, 0);
	//delay(10);
	bitSet(PORTC, 1);
	//delay(10);
	bitClear(PORTC, 1);
	delay(100);

	
	for (int i = 0; i < 8; i++) { //dus 8 x doen
		bitSet(PORTC, 0);
		//delay(10);
		bitClear(PORTC, 0);
		//delay(10);
		bitSet(PORTC, 1);
		//delay(10);
		bitClear(PORTC, 1);
		delay(100);
}
	delay(1000);
}
