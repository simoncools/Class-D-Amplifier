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

/* amplifier.c
 * 
 * First, the PWM is configured, the PWM frequnecy and modulation type can be set in the settings below.
 * Second, the dedicated ADC core 0 is set up to sample the imput signal.
 * Oversampling filter of the ADC can be enabled in the settings .
 * Last, the interupts of the ADC or oversampling filter are processed here. 
 */


#include <xc.h>
#include <p33CK256MP508.h>

//SETTINGS
#define PWMfrequncy 465// valid values in kHz are 226, 465 and 930
#define oversampling 2  //enable oversampling (2 and 8 times are valid oversampoling sample ratios)
                        // 2 times -> 13 bit result
                        // 8 times -> 14 bit result
//#define ADmodulation    //if defined AD modulation is used else BD modulation is used


//Constants
#if PWMfrequncy == 226
#define PWMperiod 0x451D; 
#elif PWMfrequncy == 465
#define PWMperiod 0x2198;
#elif PWMfrequncy == 930
#define PWMperiod 0x10CB;
#endif
#define PWMdeadtime 50;
#define PWMphase 0x10;
#define PWMduty 0x0;
#define PWM3TRG1 0x0010;
#define PWM4TRG1 0x800;
#define power_LED LATEbits.LATE11

//variables
static int16_t dataADC0 = 2048;
char disablePWM = 0;
int16_t lowpass[4] = {0,0,0,0};

void PWMConfig(void){
    //PWM control register configuration
    PCLKCONbits.MCLKSEL = 1; //  AFVCO/2 ? Auxiliary VCO/2 500MHz
    PCLKCONbits.DIVSEL = 0 ; // 1:1
    
    PG3CONLbits.CLKSEL = 1; // PWM Generator uses Master clock selected by the MCLKSEL[1:0] (PCLKCON[1:0]) control bits)
    PG3CONLbits.HREN = 1; // PWM Generator x operates in High-Resolution mode
    PG3CONLbits.MODSEL = 0b100; //Center aligned triggered mode 
    PG3CONH = 0x0000;
    //PWM uses PG3DC, PG3PER, PG3PHASE registers
    //PWM Generator does not broadcast UPDATE status bit state or EOC signal
    //Update the data registers at start of next PWM cycle (SOC) 
    //PWM generator is self-triggered
    PG3IOCONH = 0x000c;
    //PWM Generator outputs operate Independent in mode
    //PWM Generator controls the PWM3H and not PWM3L output pins
    //PWM3H & PWM3L output pins are active high
    
    PG3EVTLbits.UPDTRG = 1; // A write of the PGxDC register automatically sets the UPDATE bit
    PG3EVTLbits.ADTR1PS = 0; //1:1 postscaler Trigger 1
    PG3EVTLbits.ADTR1EN1 = 1; //PGxTRIGA for triggering trigger 1
    
    //Write to DATA REGISTERS
    PG3PER = PWMperiod;////PWM frequency
    PG3DC = PWMduty; // 0% duty
    PG3PHASE = PWMphase; //Phase offset in rising edge of PWM
    PG3DTH = PWMdeadtime; //Dead time on PWMH   250ps * 400 = 100 ns
    PG3DTL = PWMdeadtime; //Dead time on PWML   250ps * 400 = 100 ns
    PG3TRIGA = PWM3TRG1;  //trigger 
    
    
    PG4CONLbits.CLKSEL = 1; // PWM Generator uses Master clock selected by the MCLKSEL[1:0] (PCLKCON[1:0]) control bits)
    PG4CONLbits.HREN = 1; // PWM Generator x operates in High-Resolution mode
    PG4CONLbits.MODSEL = 0b100; //Center aligned triggered mode 
    PG4CONH = 0x0003;
    //PWM uses PG2DC, PG2PER, PG2PHASE registers
    //PWM Generator does not broadcast UPDATE status bit state or EOC signal
    //Update the data registers at start of next PWM cycle (SOC) 
    //Trigger output selected by PG3 Start of cycle (SOC) = local EOC
    PG4IOCONH = 0x000c; //0x000F
    //PWM Generator outputs operate Independent in mode
    //PWM Generator controls the PWM2H and not PWM2L output pins
    //PWM2H & PWM2L output pins are active high
    PG4EVTLbits.UPDTRG = 1; // A write of the PGxDC register automatically sets the UPDATE bit
    PG4EVTLbits.ADTR1PS = 0; //1:1 postscaler Trigger 1
    PG4EVTLbits.ADTR1EN1 = 1; //PGxTRIGA for triggering trigger 1
    //Write to DATA REGISTERS
    PG4PER = PWMperiod;//PWM frequency
    PG4DC = PWMduty; // DC/PER = ...% duty
    PG4PHASE = PWMphase; //Phase offset in rising edge of PWM
    PG4DTH = PWMdeadtime; //Dead time on PWMH   250ps * DT = ... ns
    PG4DTL = PWMdeadtime; //Dead time on PWML   250ps * DT = ... ns
    PG3TRIGA = PWM4TRG1;  //trigger 
    
    //Enable PWM
    PG4CONLbits.ON = 1; //PWM module is enabled
    PG3CONLbits.ON = 1; //PWM module is enabled
    
}

void D0ADCConfig(void){
    // dedicated core 0 ADC INITIALIZATION
    ANSELAbits.ANSELA0 = 1; // Configure the I/O pins to be used as analog inputs.
    TRISAbits.TRISA0 = 1; // AN23/RE3 connected the shared core for potentiometer devboard
    // Configure the common ADC clock.
    ADCON3Hbits.CLKSEL = 3; // clock from FVCO/4    140MHz
    ADCON3Hbits.CLKDIV = 1; // no clock divider (1:2)   70MHz   TAD = 14.3 ns
    // Configure the cores? ADC clock.
    ADCORE0Hbits.ADCS = 0; // dedicated core clock divider (1:2) (max is 70MHz)
    // Configure sample time for shared core.

#if oversampling == 8
    ADFL0CONbits.OVRSAM=0b101;  // oversampling x8 14bit result
    // Configure sample time for shared core.
    ADCON4Lbits.SAMC0EN = 1; //Dedicated ADC Core 0 Conversion Delay Enabled
    ADCORE0Lbits.SAMC = 0; // 2 TAD sample time     2*14.3 = 28.6 ns
    ADFL0CONbits.FLCHSEL=0; // AN0 for filter 0
    ADFL0CONbits.MODE=0;    // oversampling filter
    ADFL0CONbits.IE=1;      // filter will give interupt when data is ready
    _ADFLTR0IF =0;          // clear interupt flag
    _ADFLTR0IE =1;          // enable interupt 
    ADFL0CONbits.FLEN=1;    // enalble filter
#elif oversampling == 2
    ADFL0CONbits.OVRSAM=0b100;  // oversampling x2 13bit result
    // Configure sample time for shared core.
    ADCON4Lbits.SAMC0EN = 1; //Dedicated ADC Core 0 Conversion Delay Enabled
    ADCORE0Lbits.SAMC = 0; // 2 TAD sample time     2*14.3 = 28.6 ns
    ADFL0CONbits.FLCHSEL=0; // AN0 for filter 0
    ADFL0CONbits.MODE=0;    // oversampling filter
    ADFL0CONbits.IE=1;      // filter will give interupt when data is ready
    _ADFLTR0IF =0;          // clear interupt flag
    _ADFLTR0IE =1;          // enable interupt 
    ADFL0CONbits.FLEN=1;    // enalble filter
#else 
    ADCON4Lbits.SAMC0EN = 0; //Dedicated ADC Core 0 Conversion Delay disabled
    // Configure and enable early ADC interrupts.
    ADCORE0Hbits.EISEL = 0; // early interrupt is generated 1 TADCORE clock prior to when the data is ready
    ADCON2Lbits.EIEN = 1; // enable early interrupts for ALL inputs
    ADEIELbits.EIEN0 = 1; // enable early interrupt for AN0 
    _ADCAN0IF = 0; // clear interrupt flag for AN23
    _ADCAN0IE = 1; // enable interrupt for AN23
#endif
    
    // Configure the ADC reference sources.
    ADCON3Lbits.REFSEL = 0; // AVdd and AVss as voltage reference
    //sellect resolution
    ADCORE0Hbits.RES = 3; // 12 bit resolution
    // Configure the integer of fractional output format.
    ADCON1Hbits.FORM = 0; // integer format
    // Select single-ended input configuration and unsigned output format.
    //defuald is Channel is single-ended and Channel output data are unsigned
    ADMOD0Lbits.SIGN0 = 0; // AN0/RA0
    ADMOD0Lbits.DIFF0 = 0; // AN0/RA0

    // Set initialization time to maximum
    ADCON5Hbits.WARMTIME = 15;
    // Turn on ADC module
    ADCON1Lbits.ADON = 1;
    // Turn on analog power for dedicated core 0
    ADCON5Lbits.C0PWR = 1;
    // Wait when the core 0 is ready for operation
    while(ADCON5Lbits.C0RDY == 0);
    // Turn on digital power to enable triggers to the core 0
    ADCON3Hbits.C0EN = 1;
    ADCON3L = (ADCON3L & 0xFE00) | 0; //select adc channel for conversion
    
    // Set same trigger source for all inputs to sample signals simultaneously.
    ADTRIG0Lbits.TRGSRC0 = 0b1000; // PWM3 trig1 for AN0
    ADLVLTRGLbits.LVLEN0 = 0; // Input trigger is edge-sensitive
    //see PWM for PWM and trigger config
}

// ADC AN0 ISR (CORE 0)
void __attribute__((interrupt, no_auto_psv)) _ADCAN0Interrupt(void)
{
    dataADC0 = (ADCBUF0); // read conversion resul 
    if(dataADC0>0x3ff0||dataADC0<0x00f){
        power_LED = 0; 
    }else{power_LED = 1;}
  /*   lowpass[7]=lowpass[6];
    lowpass[6]=lowpass[5];
    lowpass[5]=lowpass[4];
    lowpass[4]=lowpass[3];
    lowpass[3]=lowpass[2];
    lowpass[2]=lowpass[1];
    lowpass[1]=lowpass[0];
    lowpass[0]=dataADC0;
    dataADC0 = ((lowpass[7]+lowpass[6])/2)+((lowpass[5]+lowpass[4])/2)+((lowpass[3]+lowpass[2])/2)+((lowpass[1]+lowpass[0])/2);*/
    if(disablePWM==0){
    #if PWMfrequncy == 226
    PG3DC = dataADC0<<2;
    PG4DC = (dataADC0^0x0fff)<<2; 
    #elif PWMfrequncy == 465
    PG3DC = dataADC0<<1;
    PG4DC = (dataADC0^0x0fff)<<1;
    #elif PWMfrequncy == 930
    PG3DC = dataADC0;
    PG4DC = (dataADC0^0x0fff);
    #endif
    }else{
        PG3DC = 0;
        PG4DC = 0;
    }
    _ADCAN0IF = 0; // clear interrupt flag
}

// ADC interupt for oversampling filter
void __attribute__((interrupt, no_auto_psv)) _ADFLTR0Interrupt(void){
    dataADC0 = ADFL0DAT; // read conversion resul
 
    lowpass[3]=lowpass[2];
    lowpass[2]=lowpass[1];
    lowpass[1]=lowpass[0];
    lowpass[0]=dataADC0;
    dataADC0 =(lowpass[3]+lowpass[2]+lowpass[1]+lowpass[0])/4;
    if(dataADC0>0x3ff0||dataADC0<0x00f){
        power_LED = 0; 
    }else{power_LED = 1;}
    //dataADC0=PWMduty; //DC VALUE
    
    if(disablePWM==0){
    #if oversampling == 8
    #if PWMfrequncy == 226
    PG3DC = dataADC0;
    PG4DC = (dataADC0^0x3fff); 
    #else
    #error "8 times oversampling can only be used with a PWMfrequncy of 226 kHz"
    #endif
    #elif oversampling == 2
    #if PWMfrequncy == 226
    PG3DC = dataADC0<<1;
    PG4DC = (dataADC0^0x1fff)<<1; 
    #elif PWMfrequncy == 465
    PG3DC = dataADC0;
    PG4DC = (dataADC0^0x1fff);
    #elif PWMfrequncy == 930
    #error "2 times oversampling can only be used with a PWMfrequncy of 226 and 465 kHz"
    #endif
    #endif
}else{
        PG3DC = 0;
        PG4DC = 0;
    }
    _ADFLTR0IF =0;
}