// Lab16_Tachmain.c
// Runs on MSP432
// Test the operation of the tachometer by implementing
// a simple DC motor speed controller.
// Daniel Valvano
// July 11, 2019

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

// See Bump.c for bumper connections (Port 8 or Port 4)

// Debug heartbeat connected to P2.0 (built-in red LED)
// Debug heartbeat connected to P2.4

// Pololu kit v1.1 connections:
// Left Encoder A connected to P10.5 (J5)
// Left Encoder B connected to P5.2 (J2.12)
// Right Encoder A connected to P10.4 (J5)
// Right Encoder B connected to P5.0 (J2.13)

// Left motor direction connected to P5.4 (J3.29)
// Left motor PWM connected to P2.7/TA0CCP4 (J4.40)
// Left motor enable connected to P3.7 (J4.31)
// Right motor direction connected to P5.5 (J3.30)
// Right motor PWM connected to P2.6/TA0CCP3 (J4.39)
// Right motor enable connected to P3.6 (J2.11)

// Negative logic bump sensors defined in Bump.c (use Port 4)
// P4.7 Bump5, left side of robot
// P4.6 Bump4
// P4.5 Bump3
// P4.3 Bump2
// P4.2 Bump1
// P4.0 Bump0, right side of robot

// Debug heartbeat connected to P2.0 (built-in red LED)
// Debug heartbeat connected to P1.0 (built-in LED1)

//===================================================
// This is the Lab code and how we tried to solve it
// This is not used for the swarm robot project
// But you may learn from it
// Senior Design Team, December 2022
//===================================================
#include <stdint.h>
#include "msp.h"
#include "Clock.h"


#include "../inc/Motor.h"

#include "../inc/Tachometer.h"
#include "../inc/TA3InputCapture.h"

uint32_t RxPutI;      // should be 0 to SIZE-1
uint32_t RxGetI;      // should be 0 to SIZE-1
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
int main(void){ //Program16_1(void){
  DisableInterrupts();
  Clock_Init48MHz();   // 48 MHz clock; 12 MHz Timer A clock
  P1->SEL0 &= ~0x01;
  P1->SEL1 &= ~0x01;   // configure P1.0 as GPIO
  P1->DIR |= 0x01;     // P1.0 output
  P2->SEL0 &= ~0x01;
  P2->SEL1 &= ~0x01;   // configure P2.0 as GPIO
  P2->DIR |= 0x01;     // P2.0 output
  First0 = First1 = 0; // first will be wrong
  Done0 = Done1 = 0;   // set on subsequent
  PWM_Init34(15000, 0, 0);
  Motor_Init();        // activate Lab 12 software
  TimerA3Capture_Init01(&PeriodMeasure0, &PeriodMeasure1);
  Motor_Forward(1500, 1500); // 50%
  EnableInterrupts();
  while(1){
    WaitForInterrupt();
  }
}
uint16_t SpeedBuffer[500];      // RPM
uint32_t PeriodBuffer[500];     // 1/12MHz = 0.083 usec
uint32_t Duty,DutyBuffer[500];  // 0 to 15000
uint32_t Time; // in 0.01 sec
/*void Collect(void){
  P2_1 = P2_1^0x01;    // thread profile, P2.1
  if(Done0==0) Period0 = 65534; // stopped
  if(Done1==0) Period1 = 65534; // stopped
  Done0 = Done1 = 0;   // set on subsequent
  if(Time==100){       // 1 sec
    Duty = 7500;
    Motor_Forward(7500, 7500);  // 50%
  }
  if(Time==200){       // 2 sec
    Duty = 11250;
    Motor_Forward(11250, 11250);// 75%
  }
  if(Time==300){       // 3 sec
    Duty = 7500;
    Motor_Forward(7500, 7500);  // 50%
  }
  if(Time==400){       // 4 sec
    Duty = 3750;
    Motor_Forward(3750, 3750);  // 25%
  }
  if(Time<500){        // 5 sec
    SpeedBuffer[Time] = 2000000/Period0;
    PeriodBuffer[Time] = Period0;
    DutyBuffer[Time] = Duty;
    Time = Time + 1;
  }
  if((Time==500)||Bump_Read()){
    Duty = 0;
    Motor_Stop();      // 0%
    TimerA1_Stop();
  }
}*/




