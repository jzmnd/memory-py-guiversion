/*
Memory testing functions sketch 3
For testing high level functions
FOR DEBUGGING ONLY
Jeremy Smith
EECS, University of California Berkeley
Version 1.0
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

const int ledPin = 13;            // LED pin number

void setup(){
  pinMode(ledPin, OUTPUT);
  for (int i=0; i<4; i++){        // blinks LED 4 times before initilization
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);
  }
  Serial.begin(115200);
  timer2.setup();       // setup for timer2 counter
  #if FASTADC
    // set prescale to 16 i.e. 1 MHz ADC clock and theoretical 76.9 kHz sample rate (1 Mhz / 13)
    sbi(ADCSRA,ADPS2);
    cbi(ADCSRA,ADPS1);
    cbi(ADCSRA,ADPS0);
  #endif
  delay(1000);
  mem.initPinMode();                  // initialization required pin modes to OUTPUT
  mem.initContentAddress();           // initial initialization (all float ready for probing)
  //mem.initOneThirdTwoThirdZERO();   // 1/3 2/3 initialization ZERO write
  //mem.initOneThirdTwoThirdONE();    // 1/3 2/3 initialization ONE write
}

void loop(){
  //mem.camread(0, B111, 0, 100, 100);      // CAM read function (line, pattern, t_pat, t_pre, t_gnd)

  mem.writeZERO(0, 0, 20, 1, 40);    // Write ONE function (w, b, t_write, loop, t_gnd)

  //mem.writeONE(0, 0, 20, 1, 40);     // Write ONE function (w, b, t_write, loop, t_gnd)
  
  delay(20);
}

