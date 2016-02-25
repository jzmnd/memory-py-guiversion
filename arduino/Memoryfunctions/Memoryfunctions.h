/*
Memoryfunctions.h - Library for memory testing functions
Jeremy Smith
EECS, University of California Berkeley
Version 1.6
*/

#ifndef Memoryfunctions_h
#define Memoryfunctions_h

#include <Arduino.h>

class Memoryfunctions {
  public:
    // declare class constructor method
    Memoryfunctions();
    // declare basic functions
    void precharge(int, int);
    void applypattern(int, int);
    void wordlineread(int);
    void forming(int);
    int stdread(int, int, int);
    void stdwriteZERO(int, int, int);
    void stdwriteONE(int, int, int);
    void gndall(int);
    // declare initialization functions
    void initPinMode();
    void initContentAddress();
    void initOneThirdTwoThirdONE();
    void initOneThirdTwoThirdZERO();
    // declare other functions
    void establishContact(char);
    // declare high level functions
    void camread(int, int, int, int, int);
    void formarray(int, int, int);
    void writeZERO(int, int, int, int, int);
    void writeONE(int, int, int, int, int);
    int stdread_rewrite(int, int, int, int, int, int);
  private:
    int _analogPinARD[3];       // Analog reads for WLs
    int _digitalPinWL[3];       // Controls for WLs
    int _digitalPinBL[3];       // Controls for BLs
    int _digitalPinReadWL[3];   // Digital reads for WLs

    int _digitalPinWLSELA;      // V<->2/3V select for WLs
    int _digitalPinWLSELB;      // float<->SELC select for WLs
    int _digitalPinWLSELC;      // GND<->1/3V select for WL SELB

    int _digitalPinBLSELA;      // V<->2/3V select for BLs
    int _digitalPinBLSELB;      // float<->SELC select for BLs
    int _digitalPinBLSELC;      // GND<->1/3V select for BL SELB

    int _digitalPinINHWL;       // Inhibit all WLs
    int _digitalPinINHBL;       // Inhibit all BLs

    int _ledPin;

    unsigned int _vwordline[500]; // Stores analogue voltage read on WL
    unsigned long _time[500];     // Stores time in ms during read
};

// external existence of object mem so it can be used in sketch
extern Memoryfunctions mem;

#endif