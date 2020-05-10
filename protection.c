/* /////////////////////////////
 * Design of a Class-D Amplifier
 * /////////////////////////////
 * Simon Cools
 * Raff Schlesser
 * 
 * Promotor: Prof. G. Van Loock
 * Co-promotor: Prof. R. Vanhoof
 * 
 * 2019-2020
 */

/* protection.c
 * 
 * in protectyion.c, the decicade core 1 and shared core are set up to sample the necesary channels that are needed for the protections.
 * secondly the extarnal interupt wich is used for the thermostat is configered. 
 * last the interrupt functions if the external inerterupt is processed here.
 */

#include <xc.h>
#include <p33CK256MP508.h>

//Constant
#define thermal_LED LATBbits.LATB7

//set up for dedicated core ADC channels 
void D1ADCConfig(void){
    // dedicated core 0 ADC INITIALIZATION
    // Configure the I/O pins to be used as analog inputs.
    ANSELAbits.ANSELA1 = 1; 
    TRISBbits.TRISB2 = 1; // AN1/RB2 
    // Configure the common ADC clock.
    ADCON3Hbits.CLKSEL = 3; // clock from FVCO/4    140MHz
    ADCON3Hbits.CLKDIV = 1; // no clock divider (1:2)   70MHz 
    // Configure the cores? ADC clock.
    ADCORE1Hbits.ADCS = 0; // dedicated core clock divider (1:1) (max is 70MHz)
    // Configure sample time for shared core.
    ADCON4Lbits.SAMC1EN = 1; //Dedicated ADC Core 1 Conversion Delay Enabled
    ADCORE1Lbits.SAMC = 100; // 102 TAD sample time
    // Configure the ADC reference sources.
    ADCON3Lbits.REFSEL = 0; // AVdd and AVss as voltage reference
    //sellect resolution
    ADCORE1Hbits.RES = 3; // 12 bit resolution
    // Configure the integer of fractional output format.
    ADCON1Hbits.FORM = 0; // integer format
    // Select single-ended input configuration and unsigned output format.
    //defuald is Channel is single-ended and Channel output data are unsigned
    ADMOD0Lbits.SIGN1 = 0; // AN1/RB2
    ADMOD0Lbits.DIFF1 = 0; // AN1/RB2

    // Set initialization time to maximum
    ADCON5Hbits.WARMTIME = 15;
    // Turn on ADC module
    ADCON1Lbits.ADON = 1;
    // Turn on analog power for dedicated core 0
    ADCON5Lbits.C1PWR = 1;
    // Wait when the core 0 is ready for operation
    while(ADCON5Lbits.C1RDY == 0);
    // Turn on digital power to enable triggers to the core 0
    ADCON3Hbits.C1EN = 1;
   
    // Set same trigger source for all inputs to sample signals simultaneously.
    ADTRIG0Lbits.TRGSRC1 = 0b1000; // PWM3 trig1 for AN1
    ADLVLTRGLbits.LVLEN1 = 0; // Input trigger is edge-sensitive
    //see PWM for PWM and trigger config
}

//set up for shared core ADC channels 
void ShADCConfig(void){
    // shared core ADC INITIALIZATION
    // Configure the I/O pins to be used as analog inputs.
    ANSELCbits.ANSELC7 = 1; 
    ANSELDbits.ANSELD10 = 1;
   // ANSELDbits.ANSELD11 = 1;
    TRISCbits.TRISC7 = 1;
    TRISDbits.TRISD10 = 1;
   // TRISDbits.TRISD11 = 1;
    // Configure the common ADC clock.
    ADCON3Hbits.CLKSEL = 3; // clock from FVCO/4    140MHz
    ADCON3Hbits.CLKDIV = 1; // no clock divider (1:2)   70MHz
    // Configure the cores? ADC clock.
    ADCON2Lbits.SHRADCS = 0; // shared core clock divider (1:2) (max is 70MHz)
    // Configure sample time for shared core.
    ADCON2Hbits.SHRSAMC = 100;  // 102 TAD sample time
    // Configure the ADC reference sources.
    ADCON3Lbits.REFSEL = 0; // AVdd and AVss as voltage reference
    //sellect resolution
    ADCON1Hbits.SHRRES = 3; // 12 bit resolution
    // Configure the integer of fractional output format.
    ADCON1Hbits.FORM = 0; // integer format
    // Select single-ended input configuration and unsigned output format.
    //defuald is Channel is single-ended and Channel output data are unsigned
    ADMOD1Lbits.SIGN16 = 0; // AN16
    ADMOD1Lbits.DIFF16 = 0; // AN16
    ADMOD1Lbits.SIGN18 = 0; // AN18
    ADMOD1Lbits.DIFF18 = 0; // AN18
    
    // Set initialization time to maximum
    ADCON5Hbits.WARMTIME = 15;
    // Turn on ADC module
    ADCON1Lbits.ADON = 1;
    // Turn on analog power for shared core
    ADCON5Lbits.SHRPWR = 1;
    // Wait when the shared core is ready for operation
    while(ADCON5Lbits.SHRRDY == 0);
    // Turn on digital power to enable triggers to the shared core
    ADCON3Hbits.SHREN = 1;

    // Set same trigger source for all inputs to sample signals simultaneously.
    ADTRIG4Lbits.TRGSRC16 = 0b1010; // PWM4 trig1 for AN16
    ADLVLTRGHbits.LVLEN16 = 0; // Input trigger is edge-sensitive
    ADTRIG4Hbits.TRGSRC18 = 0b1000; // PWM3 trig1 for AN18
    ADLVLTRGHbits.LVLEN18 = 0; // Input trigger is edge-sensitive
}

//set up of the external interrupt for the thermostat
void thermoInterrupt(void){
    INTCON2bits.INT1EP = 1;     //Interrupt on negative edge
    RPINR0bits.INT1R = 45;      //RP45 is external interupt 1
    //IPC3bits.INT1IP = 
    _INT1IF = 0;    //clear flag
    //_INT1IP = 0;
    _INT1IE = 1;    //enable interrupt (see _INT1Interrupt below)
}

// Extrenal interrupt 1 for thermostat interrupt
void __attribute__((interrupt, no_auto_psv)) _INT1Interrupt(void)
{
    PG4CONLbits.ON = 0; //PWM module is disabled
    PG3CONLbits.ON = 0; //PWM module is disabled
    thermal_LED = 1;
    _INT1IF = 0; // clear interrupt flag
}