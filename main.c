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

/* main.c
 * 
 * First all neccesary functions are called to set up the amplifier fully.
 * The protections can be disabled in the settings below.
 * later in the main loop the protecftions get polled and processed.
 */

/* potections LED info
 * 
 * power led always on and starts blicking when clipping
 * DC led on when dc is detected
 * overcurrent led on when overcurrent but starts blinking when shortcucuit on mosfets
 * thermal led on when thermal triggerd
 */

//includes
#include <xc.h>
#include <p33CK256MP508.h>
#include "system.h"
#include "amplifier.h"
#include "protection.h"

//Settings
#define protections     //define to enable the protections

//constants
#define DC_LED LATBbits.LATB6
#define overcurrent_LED LATEbits.LATE10
#define power_LED LATEbits.LATE11

//Variables
#ifdef protections
static int16_t dataADC1 = 0;
static int16_t dataADC16 = 0;
static int16_t dataADC18 = 0;

static int16_t DCthreshold = 300;    //the max DC allowed
static signed long DCvalue = 0;          //the current DC
static long DCcounter = 0;

static int16_t OCthreshold = 1200;    //the max current allowed  
//static int16_t OCvalue[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static signed long OCvalue = 0;          //the current DC
static long OCcounter = 0;
static int16_t savedData = 0;
extern char disablePWM;
#endif



int main(void) {
    INTCON2bits.GIE = 0;    //disable global interupts 
    
    //setting led outputs
    TRISBbits.TRISB6 = 0;   //clipping LED set as output
    TRISBbits.TRISB7 = 0;   //thermal LED output
    TRISEbits.TRISE10 = 0;  //overcurrent LED output
    TRISEbits.TRISE11 = 0;  //power LED output
    
    //startup the necessary clocks
    ClockShift();           //shift systhem clock to a higher frequency
    AuxiliaryPLL();         //auxiliary clock used for PWM
    
#ifdef protections
    //startup thermal protection
    thermoInterrupt();  
    
    //startup current and dc protection
    D1ADCConfig();
    ShADCConfig();
#endif
    
    //startup the amplifier
    PWMConfig();
    D0ADCConfig();
    
    INTCON2bits.GIE = 1;    //enable global interrupts
    
    power_LED = 1; //turn on power LED

    while(1){
        
#ifdef protections
        if(ADSTATHbits.AN18RDY && ADSTATLbits.AN1RDY){  // DC detection
            dataADC1 = ADCBUF1;
            dataADC18 = ADCBUF18;
            DCvalue +=(dataADC1-dataADC18);
            
            if(DCcounter>45200){
                DCvalue = DCvalue/45200;
                if(DCvalue>DCthreshold||DCvalue<-DCthreshold){
                disablePWM = 1;
                PG3DC = 0;
                PG4DC = 0;
                DC_LED = 1;
            }else{
                //DC_LED = 0;
            }
            DCcounter=0;
            DCvalue = 0;
            }else{DCcounter++;}       
        }//end of DC detection 
        
        
        if(ADSTATHbits.AN16RDY){    //Over Current protection 
            dataADC16 = ADCBUF16;
            /*
            OCvalue[15]=OCvalue[14];
            OCvalue[14]=OCvalue[13];
            OCvalue[13]=OCvalue[12];
            OCvalue[12]=OCvalue[11];
            OCvalue[11]=OCvalue[10];
            OCvalue[10]=OCvalue[9];
            OCvalue[9]=OCvalue[8];
            OCvalue[8]=OCvalue[7];
            OCvalue[7]=OCvalue[6];
            OCvalue[6]=OCvalue[5];
            OCvalue[5]=OCvalue[4];
            OCvalue[4]=OCvalue[3];
            OCvalue[3]=OCvalue[2];
            OCvalue[2]=OCvalue[1];
            OCvalue[1]=OCvalue[0];
            OCvalue[0]=dataADC16;
            dataADC16 =(OCvalue[15]+ OCvalue[14]+ OCvalue[13]+ OCvalue[12]+OCvalue[11]+ OCvalue[10]+ OCvalue[9]+ OCvalue[8]+OCvalue[7]+ OCvalue[6]+ OCvalue[5]+ OCvalue[4]+OCvalue[3]+ OCvalue[2]+ OCvalue[1]+ OCvalue[0])/16;
            */

            OCvalue += dataADC16;
            if(OCcounter>45200){
                OCvalue=OCvalue/45200;
                savedData = OCvalue;
                if(OCvalue>OCthreshold){
                    disablePWM = 1; 
                    PG3DC = 0;
                    PG4DC = 0;
                    overcurrent_LED = 1;           
                }
                OCvalue=0;
                OCcounter=0;
            }else{OCcounter++;}
        }//end of overcurrent protection 
#endif //protections 
    }//end of main while
    return 0;
}
