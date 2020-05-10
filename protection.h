/* 
 * File:   protectionSetup.h
 * Author: raffs
 *
 * Created on 12 februari 2020, 14:56
 */

#ifndef PROTECTION_H
#define	PROTECTION_H

void D1ADCConfig(void);
void ShADCConfig(void);
void thermoInterrupt(void);

#define OCLED LATEbits.LATE10;      //RE10      over current LED
#define POWERLED LATEbits.LATE11;   //RE11      power LED
#define CLIPPINGLED LATBbits.LATB6; //RB6       clipping LED
#define THERMALLED LATBbits.LATB7;  //RB7       thermal LED

#endif	/* PROTECTION_H */

