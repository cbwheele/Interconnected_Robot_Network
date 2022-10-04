// GPIOmain.c
// Runs on MSP432
// Initialize four GPIO pins as outputs.  Continually generate output to
// drive simulated stepper motor.
// Daniel Valvano
// September 23, 2017

/* This example accompanies the book
   "Embedded Systems: Introduction to Robotics,
   Jonathan W. Valvano, ISBN: 9781074544300, copyright (c) 2019
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/

Simplified BSD License (FreeBSD License)
Copyright (c) 2019, Jonathan Valvano, All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are
those of the authors and should not be interpreted as representing official
policies, either expressed or implied, of the FreeBSD Project.
*/

// P4.3 is an output to LED3, negative logic
// P4.2 is an output to LED2, negative logic
// P4.1 is an output to LED1, negative logic
// P4.0 is an output to LED0, negative logic

#include <stdint.h>
#include "msp.h"
#include "Clock.h"
#include "ports.h"
#include "./inc/TExaS.h"
#include "./inc/Motor.h"
#include "./inc/PWM.h"

//#include "./inc/TimerA2.h"


volatile unsigned char timerDone = 0;

#include "./inc/UART1.h"



void GPIO_Init(void){
  // initialize P4.3-P4.0 and make them outputs
  P4->SEL0 &= ~0x0F;
  P4->SEL1 &= ~0x0F;            // configure stepper motor/LED pins as GPIO
  P4->DIR |= 0x0F;              // make stepper motor/LED pins out
}

int main(void){
  Clock_Init48MHz();
  GPIO_Init();
  TExaS_Init(LOGICANALYZER_P4);

  Init_Our_Custom_Ports(); // This will turn on the Red LED

  PWM_Init34(15000, 0, 0);
  Motor_Init();


  // First, we need to wait until coordinates start being sent over UART
  
  // Then, we will want to delay for maybe 10 seconds to allow those coordinates to settle

  // Then, read in those coordinates for five seconds and save the average of those as "starting location"

  // Drive forward for two seconds

  // Wait five seconds for coordinates to stabilize

  // Read in the coordinates over five seconds (average) and save that as "secondary location"

  // Calculate based on starting location and secondary location

  // Turn in direction of the starting point

  // Drive for distance until the starting point


  Motor_Forward(5000,5000);
  Clock_Delay1ms(1000);

  Motor_Stop();

  

  while(1){



  }
}

// Program 2.13 from Volume 2
#define STEPPER  (*((volatile uint8_t *)0x40004C23))  /* Port 4 Output, bits 3-0 are stepper motor */
static void step(uint8_t n){
  STEPPER = (STEPPER&~0x0F)|n;  // output to stepper causing it to step once
  Clock_Delay1ms(100);          // 100ms delay 10 steps/sec
}
int main2(void){ // reset clears P4REN, P4DS, P4SEL0, P4SEL1
  Clock_Init48MHz();
  TExaS_Init(LOGICANALYZER_P4);
  GPIO_Init();
  while(1){
    step(5);  // motor is 0101
    step(6);  // motor is 0110
    step(10); // motor is 1010
    step(9);  // motor is 1001
  }
}

int main3(void){        //UART code
  char ch;
  char string[20];
  //uint32_t n;
  Clock_Init48MHz();  // makes SMCLK=12 MHz
  UART1_Initprintf(); // initialize UART and printf
  UART1_OutString("\nTest program for UART driver\n\rUART0_OutChar examples\n");
  for(ch='A'; ch<='Z'; ch=ch+1){// print the uppercase alphabet
    UART1_OutChar(ch);
  }
  UART1_OutChar(LF);
  for(ch='a'; ch<='z'; ch=ch+1){// print the lowercase alphabet
    UART1_OutChar(ch);
  }
  //BookExamples();
  while(1){
    UART1_OutString("InString: ");
    UART1_InString(string,19); // user enters a string
    UART1_OutString(" OutString="); UART1_OutString(string); UART1_OutChar(LF);

    /*UART1_OutString("InUDec: ");   n=UART0_InUDec();
    UART1_OutString(" OutUDec=");  UART0_OutUDec(n); UART0_OutChar(LF);
    UART1_OutString(" OutUFix1="); UART0_OutUFix1(n); UART0_OutChar(LF);
    UART1_OutString(" OutUFix2="); UART0_OutUFix2(n); UART0_OutChar(LF);
    printf(" Using printf= %d, %2d.%.1d,\n",n,n/10,n%10);

    UART1_OutString("InUHex: ");   n=UART0_InUHex();
    UART1_OutString(" OutUHex=");  UART0_OutUHex(n); UART0_OutChar(LF);
    printf(" Using printf= %#x\n",n);*/
    /*UART1_InString(string,19);
    UART1_OutString(string);
    UART1_OutChar(LF);*/

  }
}

