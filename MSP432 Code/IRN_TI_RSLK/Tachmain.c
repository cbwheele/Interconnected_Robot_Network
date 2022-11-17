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
#include <stdlib.h> //added for reading in coordinates
#include <stdio.h>  //added for reading in coordinates
#include <math.h>   //added for reading in coordinates
#include "msp.h"
#include "Clock.h"
#include "ports.h"
#include "./inc/TExaS.h"
#include "./inc/Motor.h"
#include "./inc/PWM.h"
#include "./inc/TimerA2.h"
#include "./inc/Tachometer.h"

#include "./inc/UART1.h"    //serial communication
extern uint16_t left_counter ;
extern uint16_t right_counter ;
uint16_t R_ADJ_VAL = 100;
uint16_t L_ADJ_VAL = 100;

  uint16_t leftSpeed, rightSpeed, leftSpeed_reading, rightSpeed_reading ;
  enum TachDirection *leftDirection_reading, *rightDirection_reading;
  //enum TachDirection leftDirection, rightDirection,
  //int32_t leftStep, rightStep;
  int32_t *leftStep_reading, *rightStep_reading;

volatile unsigned char timerDone = 0;
 uint32_t RxPutI;      // should be 0 to SIZE-1
 uint32_t RxGetI;      // should be 0 to SIZE-1
float distance_to_travel = 0;
float x_leg_of_tri = 0;

void EnableInterrupts(void);
void DisableInterrupts(void);
void WaitForInterrupt(void);
void TimerA3Capture_Init01(void(*task0)(uint16_t time), void(*task1)(uint16_t time));

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
    TIMER_A2->CTL &= ~0x0030;       // halt Timer A2
    timerDone = 1;
}
////////////////////////////////////////////////////////////////////
#define P2_4 (*((volatile uint8_t *)(0x42098070)))
#define P2_3 (*((volatile uint8_t *)(0x4209806C)))
#define P2_2 (*((volatile uint8_t *)(0x42098068)))
#define P2_1 (*((volatile uint8_t *)(0x42098064)))
#define P2_0 (*((volatile uint8_t *)(0x42098060)))
#define P1_0 (*((volatile uint8_t *)(0x42098040)))

uint16_t Period0;              // (1/SMCLK) units = 83.3 ns units
uint16_t First0;               // Timer A3 first edge, P10.4
int Done0;                     // set each rising
// max period is (2^16-1)*83.3 ns = 5.4612 ms
// min period determined by time to run ISR, which is about 1 us
void PeriodMeasure0(uint16_t time){
  P2_0 = P2_0^0x01;            // thread profile, P2.0
  Period0 = (time - First0)&0xFFFF; // 16 bits, 83.3 ns resolution
  First0 = time;               // setup for next
  Done0 = 1;
}
uint16_t Period1;              // (1/SMCLK) units = 83.3 ns units
uint16_t First1;               // Timer A3 first edge, P10.5
int Done1;                     // set each rising
// max period is (2^16-1)*83.3 ns = 5.4612 ms
// min period determined by time to run ISR, which is about 1 us
void PeriodMeasure1(uint16_t time){
  P1_0 = P1_0^0x01;            // thread profile, P1.0
  Period1 = (time - First1)&0xFFFF; // 16 bits, 83.3 ns resolution
  First1 = time;               // setup for next
  Done1 = 1;
}
/////////////////////////////////////////////////////////////////////////

int main(void){        // Main State Machine

  volatile unsigned char timer_set = 0; //identify if timer is set or not





  DisableInterrupts(); //==============================added to test interrupt
  Clock_Init48MHz();  // makes SMCLK=12 MHz
  //UART1_Initprintf(); // initialize UART and printf
  P1->SEL0 &= ~0x01;
  P1->SEL1 &= ~0x01;   // configure P1.0 as GPIO
  P1->DIR |= 0x01;     // P1.0 output
  P2->SEL0 &= ~0x01;
  P2->SEL1 &= ~0x01;   // configure P2.0 as GPIO
  P2->DIR |= 0x01;     // P2.0 output
  int counter = 0;
  leftSpeed = 1500;
  rightSpeed = 1500;
  First0 = First1 = 0; // first will be wrong
  Done0 = Done1 = 0;   // set on subsequent
  PWM_Init34(15000, 0, 0);
  Motor_Init();
  Tachometer_Init();
  Motor_Forward(1500, 1500); // 10%
  EnableInterrupts();
  char revolve_on = 0;

  while(1){
      WaitForInterrupt();
      //Tachometer_Get(leftSpeed_reading, leftDirection_reading, leftStep_reading,
      //               rightSpeed_reading, rightDirection_reading, rightStep_reading);
      if (right_counter == R_ADJ_VAL){
                rightSpeed = 0;
                //if (left_counter < L_ADJ_VAL)
                L_ADJ_VAL ++;
                right_counter++;

      }
      if (left_counter == L_ADJ_VAL){
                leftSpeed = 0;
                //if (right_counter < R_ADJ_VAL)
                R_ADJ_VAL ++;
                left_counter++;
            }
      if (right_counter >= R_ADJ_VAL && left_counter >= L_ADJ_VAL) {
          left_counter = 0;
          right_counter = 0;
          rightSpeed = 1500;
          leftSpeed = 1500;
      }
      counter++;
      if(counter >= 2000){
          int checkpoint;
          checkpoint = 0;
      }
      if(counter >= 10000){
          Motor_Stop();
      }
      else{
      Motor_Forward(leftSpeed, rightSpeed);
      }
  }
}


