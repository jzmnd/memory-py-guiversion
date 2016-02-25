/*
Memory testing sketch
memory_test_v3.ino
Use with memory_test_v2.py
Jeremy Smith
EECS, University of California, Berkeley
Version 3.0
*/

#include <eRCaGuy_Timer2_Counter.h>
#include <Memoryfunctions.h>

#define FASTADC 1
// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

const int ledPin = 13;      // LED pin number
unsigned int inByte;        // incoming serial byte for program number
unsigned int inBuffer[7];   // serial buffer for other data parsed from python
int state;                  // read state (1 or 0)

/*
Setup function
  Blinks LED
  Sets ADC smple rate
  Initializes pins and sets initial voltages
  Sends establish contact byte 'A'
*/

void setup(){
  pinMode(ledPin, OUTPUT);
  for (int i=0; i<4; i++){            // blinks LED 4 times before initilization
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);
  }
  Serial.begin(115200);
  timer2.setup();             // setup for timer2 counter
  #if FASTADC
    // set prescale to 16 i.e. 1 MHz ADC clock and theoretical 76.9 kHz sample rate (1 Mhz / 13)
    sbi(ADCSRA,ADPS2);
    cbi(ADCSRA,ADPS1);
    cbi(ADCSRA,ADPS0);
  #endif
  delay(1000);
  mem.initPinMode();          // initialization required pin modes to OUTPUT
  mem.initContentAddress();   // initial initialization (all float ready for probing)
  mem.establishContact('A');  // establish contact
}


/*
Main function
   Reads in serial port data from python script
   Runs required function
   Sends establish contact byte 'Z'
*/

void loop(){
  if(Serial.available() >= 8){      // checks if all data is on the serial port
    inByte = Serial.read();         // reads serial data from python
    for (int i=0; i<7; i++){
      inBuffer[i] = Serial.read();  // reads serial data from python into buffer
    }
    switch (inByte){
      case '1':
        // Content addressable read function
        mem.camread(inBuffer[0], inBuffer[2], inBuffer[3], inBuffer[4], inBuffer[6]);
        break;
      case '2':
        // Forming all bits function
        mem.formarray(inBuffer[4], inBuffer[5], inBuffer[6]);
        break;
      case '3':
        // Write a ZERO state function
        mem.writeZERO(inBuffer[0], inBuffer[1], inBuffer[3], inBuffer[5], inBuffer[6]);
        break;
      case '4':
        // Write a ONE state function
        mem.writeONE(inBuffer[0], inBuffer[1], inBuffer[3], inBuffer[5], inBuffer[6]);
        break;
      case '5':
        // Standard read function (currently not functioning)
        mem.stdread_rewrite(inBuffer[0], inBuffer[1], inBuffer[3], inBuffer[3], inBuffer[5], inBuffer[6]);
        break;
    }
    delay(1000);
    mem.establishContact('Z');
  }
}

