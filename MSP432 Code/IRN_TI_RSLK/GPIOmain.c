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
#include "./inc/TimerA2.h"


void GPIO_Init(void){
  // initialize P4.3-P4.0 and make them outputs
  P4->SEL0 &= ~0x0F;
  P4->SEL1 &= ~0x0F;            // configure stepper motor/LED pins as GPIO
  P4->DIR |= 0x0F;              // make stepper motor/LED pins out
}


void TA1_0_IRQHandler(void){
TIMER_A1->CCTL[0] &= ~0x0001; // ack
// body
P2->OUT ^= 2;
}


void Timer_Done(void) {
    P2->OUT ^= 2;
}


// This main version is for timers
int main(void) {
    /* initialize P2.1 for green LED */
    P2->SEL1 &= ~2;         /* configure P2.1 as simple I/O */
    P2->SEL0 &= ~2;
    P2->DIR |= 2;           /* P2.1 set as output */
    P2->OUT &= ~2;
//
//    TIMER_A1->CTL = 0x01D1;     /* ACLK, ID = /8, up mode, TA clear */
//    TIMER_A1->CCR[0] = 512 - 1; /* for 1 sec */
//    TIMER_A1->EX0 = 7;          /* IDEX = /8 */
//
//    NVIC->IP[2]= (NVIC->IP[2] & 0xFFFFFF00) | 0x00400000;
//    NVIC->ISER[0] = 0x00000400;
//
//
//

    TimerA2_Init(&Timer_Done, 512);// initialize 1000 Hz sine wave output

    while (1) {
            //while((TIMER_A1->CCTL[0] & 1) == 0); /* wait until the CCIFG flag is set */
            //TIMER_A1->CCTL[0] &= ~1;            /* clear interrupt flag */
                                   /* toggle green LED */
        }
    }


int main1(void){
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
