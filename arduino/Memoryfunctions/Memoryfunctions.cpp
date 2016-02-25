/*
Memoryfunctions.cpp - Library for memory testing functions
Jeremy Smith
EECS, University of California Berkeley
Version 1.6
*/

#include <Arduino.h>
#include "Memoryfunctions.h"
#include <eRCaGuy_Timer2_Counter.h>

#define MEASURETYPE 1      // Change for measure while applying pattern (1) or apply pattern then measure (0)
#define CAMTYPE 0          // Change for CAM apply pattern with 2/3V (1) or V (0)

// Pre-instantiate an object of this library class
Memoryfunctions mem;

Memoryfunctions::Memoryfunctions(){
  _analogPinARD[0] = A0;    // Analogue read WL0
  _analogPinARD[1] = A1;    // Analogue read WL1
  _analogPinARD[2] = A2;    // Analogue read WL2
  _digitalPinWL[0] = 24;    // Control WL0
  _digitalPinWL[1] = 26;    // Control WL1
  _digitalPinWL[2] = 28;    // Control WL2
  _digitalPinBL[0] = 25;    // Control BL0
  _digitalPinBL[1] = 27;    // Control BL1
  _digitalPinBL[2] = 29;    // Control BL2

  _digitalPinWLSELA = 30;    // Voltage select WLs V - 2/3V
  _digitalPinWLSELB = 32;    // Voltage select WLs float - SELC
  _digitalPinWLSELC = 34;    // Voltage select WLs GND - 1/3V
  _digitalPinBLSELA = 31;    // Voltage select BLs V - 2/3V
  _digitalPinBLSELB = 33;    // Voltage select BLs float - SELC
  _digitalPinBLSELC = 35;    // Voltage select BLs GND - 1/3V

  _digitalPinINHWL = 22;     // Inhibit all WLs
  _digitalPinINHBL = 23;     // Inhibit all BLs

  _digitalPinReadWL[0] = 36;    // Digital read WL0
  _digitalPinReadWL[1] = 38;    // Digital read WL1
  _digitalPinReadWL[2] = 40;    // Digital read WL2

  _ledPin = 13;     // LED pin
}

/*
Basic functions for:
1. Precharging
2. Applying a pattern
3. Reading the word line voltage
4. Forming array
5. Reading from a single cell
6. Writing a 0 to a single cell
7. Writing a 1 to a single cell
*/

void Memoryfunctions::precharge(int t, int line){
  Serial.println(F("PREC..."));
  digitalWrite(_digitalPinWL[line], HIGH); // WL[line]: V
  delay(t);
  digitalWrite(_digitalPinINHWL, HIGH);    // Inhibit WLs (keeps each WL floating and isolated)
}

void Memoryfunctions::applypattern(int pattern, int t){
  digitalWrite(_digitalPinINHBL, HIGH);  // Inhibit BLs
  digitalWrite(_digitalPinBLSELB, LOW);  // BLs: GND mode
  // apply pattern to the BLs ("1" = V, "0" = GND for CAMTYPE=0; "1" = 2/3V, "0" = GND for CAMTYPE=1)
  #if CAMTYPE
    digitalWrite(_digitalPinBLSELA, LOW);  // BLs: 2/3V mode
  #endif
  for (int y=0; y<3; y++){
    if (pattern & (1<<y)){
      digitalWrite(_digitalPinBL[y], HIGH); // BL[y] to V
    } else{
      digitalWrite(_digitalPinBL[y], LOW);  // BL[y] to GND
    }
  }
  digitalWrite(_digitalPinINHBL, LOW);  // Enable BLs
  #if MEASURETYPE
  #else
    delay(t);
    digitalWrite(_digitalPinINHBL, HIGH); // Inhibit BLs
  #endif
}

void Memoryfunctions::wordlineread(int line){
  // read voltage as function of time
  timer2.reset();
  for (int i=0; i<500; i++){
    _time[i] = timer2.get_count();                    // counts every 0.5 us
    _vwordline[i] = analogRead(_analogPinARD[line]);  // voltage read on WL[line]
  }
  #if MEASURETYPE
    digitalWrite(_digitalPinINHBL, HIGH); // Inhibit BLs
  #endif
  // writes out data to serial port
  for (int j=0; j<500; j++){
    Serial.print(_time[j]);
    Serial.print(',');
    Serial.print(_vwordline[j]);
    Serial.print('\n');
  }
  delay(100);
  // restore normal mode (all lines floating)
  #if CAMTYPE
    digitalWrite(_digitalPinBLSELA, HIGH);  // BLs: Vread mode
  #endif
  digitalWrite(_digitalPinBLSELB, HIGH); // BLs: float mode
  for (int i=0; i<3; i++){
    digitalWrite(_digitalPinBL[i], LOW); // BL[i]: float
  }
  digitalWrite(_digitalPinINHBL, LOW);   // Enable BLs
  digitalWrite(_digitalPinINHWL, LOW);   // Enable WLs
}

void Memoryfunctions::forming(int t){
  Serial.println(F("FORM..."));
  digitalWrite(_digitalPinINHWL, HIGH); // Inhibit WLs
  digitalWrite(_digitalPinINHBL, HIGH); // Inhibit BLs
  for (int i=0; i<3; i++){
    digitalWrite(_digitalPinWL[i], HIGH); // WL[i]: V
    digitalWrite(_digitalPinBL[i], LOW);  // BL[i]: GND
  }
  digitalWrite(_digitalPinINHWL, LOW); // Enable WLs
  digitalWrite(_digitalPinINHBL, LOW); // Enable BLs
  delay(t);
  digitalWrite(_digitalPinINHWL, HIGH); // Inhibit WLs
  digitalWrite(_digitalPinINHBL, HIGH); // Inhibit BLs
  for (int i=0; i<3; i++){
    digitalWrite(_digitalPinWL[i], LOW);  // WL[i]: 1/3V
    digitalWrite(_digitalPinBL[i], HIGH); // BL[i]: 2/3V
  }
  digitalWrite(_digitalPinINHWL, LOW); // Enable WLs
  digitalWrite(_digitalPinINHBL, LOW); // Enable BLs
}

int Memoryfunctions::stdread(int w, int b, int t){
  Serial.println(F("READ..."));
  digitalWrite(_digitalPinWLSELA, LOW); // WLs: 2/3V mode

  digitalWrite(_digitalPinWL[w], HIGH); // WL[w]: Read at 2/3V
  digitalWrite(_digitalPinBL[b], LOW);  // BL[b]: GND
  delay(t);
  digitalWrite(_digitalPinINHWL, HIGH); // Inhibit WLs (float)
  pinMode(_digitalPinReadWL[w], INPUT_PULLUP);  // Pullup input
  // wait for line to stabilize before read
  delay(1000);
  int state = digitalRead(_digitalPinReadWL[w]);   // Reads state
  pinMode(_digitalPinReadWL[w], INPUT);  // Return input to high-Z
  
  digitalWrite(_digitalPinWL[w], LOW);  // WL[w]: 1/3V
  digitalWrite(_digitalPinBL[b], HIGH); // BL[b]: 2/3V
  
  digitalWrite(_digitalPinINHWL, LOW);     // Enable WLs
  digitalWrite(_digitalPinWLSELA, HIGH);   // WLs: Return to Vread mode

  return !state;     // note pullup means HIGH when open i.e. 0 state
}

void Memoryfunctions::stdwriteZERO(int w, int b, int t){
  Serial.println(F("WRT0..."));
  digitalWrite(_digitalPinINHWL, HIGH);  // Inhibit WLs
  digitalWrite(_digitalPinINHBL, HIGH);  // Inhibit BLs

  digitalWrite(_digitalPinWL[w], HIGH);  // WL[w]: V
  digitalWrite(_digitalPinBL[b], LOW);   // BL[b]: GND

  digitalWrite(_digitalPinINHWL, LOW);   // Enable WLs
  digitalWrite(_digitalPinINHBL, LOW);   // Enable BLs
  delay(t);
  digitalWrite(_digitalPinINHWL, HIGH);  // Inhibit WLs
  digitalWrite(_digitalPinINHBL, HIGH);  // Inhibit BLs

  digitalWrite(_digitalPinWL[w], LOW);   // WL[w]: 1/3V
  digitalWrite(_digitalPinBL[b], HIGH);  // BL[b]: 2/3V

  digitalWrite(_digitalPinINHWL, LOW);   // Enable WLs
  digitalWrite(_digitalPinINHBL, LOW);   // Enable BLs
}

void Memoryfunctions::stdwriteONE(int w, int b, int t){
  Serial.println(F("WRT1..."));
  digitalWrite(_digitalPinINHWL, HIGH);  // Inhibit WLs
  digitalWrite(_digitalPinINHBL, HIGH);  // Inhibit BLs

  digitalWrite(_digitalPinWL[w], LOW);   // WL[w]: GND
  digitalWrite(_digitalPinBL[b], HIGH);  // BL[b]: V

  digitalWrite(_digitalPinINHWL, LOW);   // Enable WLs
  digitalWrite(_digitalPinINHBL, LOW);   // Enable BLs
  delay(t);
  digitalWrite(_digitalPinINHWL, HIGH);  // Inhibit WLs
  digitalWrite(_digitalPinINHBL, HIGH);  // Inhibit BLs

  digitalWrite(_digitalPinWL[w], HIGH);  // WL[w]: 2/3V
  digitalWrite(_digitalPinBL[b], LOW);   // BL[b]: 1/3V

  digitalWrite(_digitalPinINHWL, LOW);   // Enable WLs
  digitalWrite(_digitalPinINHBL, LOW);   // Enable BLs
}

void Memoryfunctions::gndall(int t){
  Serial.println(F("GNDS..."));
  digitalWrite(_digitalPinINHWL, HIGH);  // Inhibit WLs
  digitalWrite(_digitalPinINHBL, HIGH);  // Inhibit BLs
  digitalWrite(_digitalPinWLSELB, LOW);  // WLs: WLSELC mode (GND)
  digitalWrite(_digitalPinBLSELB, LOW);  // BLs: BLSELC mode (GND)
  digitalWrite(_digitalPinWLSELC, HIGH);  // WLs: GND
  digitalWrite(_digitalPinBLSELC, HIGH);  // BLs: GND
  for (int i=0; i<3; i++){
    digitalWrite(_digitalPinWL[i], LOW);   // WL[i]: GND
    digitalWrite(_digitalPinBL[i], LOW);   // BL[i]: GND
  }
  digitalWrite(_digitalPinINHWL, LOW);  // Enable WLs
  digitalWrite(_digitalPinINHBL, LOW);  // Enable BLs
  delay(t);
  // REMEMBER TO RE-INITIALIZE AFTER CALLING GNDALL
}

/*
Initialization functions
*/

void Memoryfunctions::initPinMode(){
  /*
  Pin mode initialization
  */
  pinMode(_digitalPinINHWL, OUTPUT);
  pinMode(_digitalPinINHBL, OUTPUT);
  
  pinMode(_digitalPinWLSELA, OUTPUT);
  pinMode(_digitalPinWLSELB, OUTPUT);
  pinMode(_digitalPinWLSELC, OUTPUT);
  
  pinMode(_digitalPinBLSELA, OUTPUT);
  pinMode(_digitalPinBLSELB, OUTPUT);
  pinMode(_digitalPinBLSELC, OUTPUT);
  
  for (int i=0; i<3; i++){
    pinMode(_digitalPinWL[i], OUTPUT);
    pinMode(_digitalPinBL[i], OUTPUT);
    pinMode(_digitalPinReadWL[i], INPUT);
  }
  
  pinMode(_digitalPinINHWL, OUTPUT);
  pinMode(_digitalPinINHBL, OUTPUT);
}

void Memoryfunctions::initContentAddress(){
  /*
  Pin initialization for content addressable voltage scheme (default)
  WLs either at Vread or floating
  BLs either at Vread or floating
  */
  digitalWrite(_digitalPinINHWL, HIGH); // Inhibit WLs
  digitalWrite(_digitalPinINHBL, HIGH); // Inhibit BLs
  
  digitalWrite(_digitalPinWLSELA, HIGH);   // WLs: Vread mode
  digitalWrite(_digitalPinWLSELB, HIGH);   // WLs: float mode
  digitalWrite(_digitalPinWLSELC, HIGH);   // WLs: GND mode
  
  digitalWrite(_digitalPinBLSELA, HIGH);   // BLs: Vread mode
  digitalWrite(_digitalPinBLSELB, HIGH);   // BLs: float mode
  digitalWrite(_digitalPinBLSELC, HIGH);   // BLs: GND mode
  
  for (int i=0; i<3; i++){
    digitalWrite(_digitalPinWL[i], LOW); // WL[i]: float
    digitalWrite(_digitalPinBL[i], LOW); // BL[i]: float
  }
  
  digitalWrite(_digitalPinINHWL, LOW); // Enable WLs
  digitalWrite(_digitalPinINHBL, LOW); // Enable BLs 
}

void Memoryfunctions::initOneThirdTwoThirdZERO(){
  /*
  Pin initialization for 1/3-2/3 voltage scheme
  WLs either at V or 1/3V
  BLs either at 2/3V or GND
  */
  digitalWrite(_digitalPinINHWL, HIGH); // Inhibit WLs
  digitalWrite(_digitalPinINHBL, HIGH); // Inhibit BLs
  
  digitalWrite(_digitalPinWLSELA, HIGH);  // WLs: Vread mode
  digitalWrite(_digitalPinWLSELB, LOW);   // WLs: WLSELC mode
  digitalWrite(_digitalPinWLSELC, LOW);   // WLs: 1/3V mode
  
  digitalWrite(_digitalPinBLSELA, LOW);   // BLs: 2/3V mode
  digitalWrite(_digitalPinBLSELB, LOW);   // BLs: WLSELC mode
  digitalWrite(_digitalPinBLSELC, HIGH);  // BLs: GND mode
  
  for (int i=0; i<3; i++){
    digitalWrite(_digitalPinWL[i], LOW);  // WL[i]: 1/3V
    digitalWrite(_digitalPinBL[i], HIGH); // BL[i]: 2/3V
  }
  
  digitalWrite(_digitalPinINHWL, LOW); // Enable WLs
  digitalWrite(_digitalPinINHBL, LOW); // Enable BLs
}

void Memoryfunctions::initOneThirdTwoThirdONE(){
  /*
  Pin initialization for 1/3-2/3 voltage scheme
  WLs either at 2/3V or GND
  BLs either at V or 1/3V
  */
  digitalWrite(_digitalPinINHWL, HIGH); // Inhibit WLs
  digitalWrite(_digitalPinINHBL, HIGH); // Inhibit BLs
  
  digitalWrite(_digitalPinWLSELA, LOW);   // WLs: 2/3V mode
  digitalWrite(_digitalPinWLSELB, LOW);   // WLs: BLSELC mode
  digitalWrite(_digitalPinWLSELC, HIGH);  // WLs: GND mode
  
  digitalWrite(_digitalPinBLSELA, HIGH);  // BLs: Vread mode
  digitalWrite(_digitalPinBLSELB, LOW);   // BLs: BLSELC mode
  digitalWrite(_digitalPinBLSELC, LOW);   // BLs: 1/3V mode
  
  for (int i=0; i<3; i++){
    digitalWrite(_digitalPinWL[i], HIGH); // WL[i]: 2/3V
    digitalWrite(_digitalPinBL[i], LOW);  // BL[i]: 1/3V
  }
  
  digitalWrite(_digitalPinINHWL, LOW); // Enable WLs
  digitalWrite(_digitalPinINHBL, LOW); // Enable BLs
}

/*
Other basic functions
*/

void Memoryfunctions::establishContact(char contact){
  while (Serial.available() == 0) {
    Serial.print(contact);   // send a byte every second
    delay(1000);
  }
}

/*
High level functions for:
1. Content addressable read
2. Forming all bits
3. Writing a ZERO state
4. Writing a ONE state
5. Standard read function
*/

void Memoryfunctions::camread(int line, int pattern, int t_pat, int t_pre, int t_gnd){
  // Content addressable read function
  digitalWrite(_ledPin, HIGH);
  initContentAddress();                 // reinitialize
  precharge(t_pre, line);               // precharge time, WL number
  applypattern(pattern, t_pat);         // pattern (binary), time for applying pattern in ms
  wordlineread(line);                   // WL number
  gndall(t_gnd);                        // grounds all lines for time in ms
  digitalWrite(_ledPin, LOW);
}

void Memoryfunctions::formarray(int t_form, int loop, int t_gnd){
  // Forming all bits function
  digitalWrite(_ledPin, HIGH);
  initOneThirdTwoThirdZERO();           // 1/3-2/3 initialize ZERO write
  for (int i=0; i<loop; i++){
    forming(t_form);                    // forming time in ms (set all bits to 0-state)
  }
  gndall(t_gnd);                        // grounds all lines for time in ms
  digitalWrite(_ledPin, LOW);
}

void Memoryfunctions::writeZERO(int w, int b, int t_write, int loop, int t_gnd){
  // Write a ZERO state function
  digitalWrite(_ledPin, HIGH);
  initOneThirdTwoThirdZERO();           // 1/3-2/3 initialize ZERO write
  delay(100);
  for (int i=0; i<loop; i++){
    stdwriteZERO(w, b, t_write);        // write 0 to bit w, b, for time in ms
  }
  delay(100);
  gndall(t_gnd);                        // grounds all lines for time in ms
  digitalWrite(_ledPin, LOW);
}

void Memoryfunctions::writeONE(int w, int b, int t_write, int loop, int t_gnd){
  // Write a ONE state function
  digitalWrite(_ledPin, HIGH);
  initOneThirdTwoThirdONE();            // 1/3-2/3 initialize ONE write
  delay(100);
  for (int i=0; i<loop; i++){
    stdwriteONE(w, b, t_write);         // write 1 to bit w, b, for time in ms
  }
  delay(100);
  gndall(t_gnd);                        // grounds all lines for time in ms
  digitalWrite(_ledPin, LOW);
}

int Memoryfunctions::stdread_rewrite(int w, int b, int t_read, int t_write, int loop, int t_gnd){
  // Standard read functon and rewrite
  // STILL IN TESTING
  digitalWrite(_ledPin, HIGH);
  initOneThirdTwoThirdZERO();           // 1/3-2/3 initialize ZERO write
  int state = stdread(w, b, t_read);    // read at bit w, b, for time in ms
  Serial.println(state);
  if (state == 1){                      // rewrite bit if it was a 1 state
    initOneThirdTwoThirdONE();
    for (int i=0; i<loop; i++){
      stdwriteONE(w, b, t_write);       // write 1 to bit w, b, for time in ms
    }
    gndall(t_gnd);                      // grounds all lines for time in ms
  }
  digitalWrite(_ledPin, LOW);
  return state;
}
