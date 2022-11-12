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

#define TIME2DIST 10240

volatile unsigned char timerDone = 0;
 uint32_t RxPutI;      // should be 0 to SIZE-1
 uint32_t RxGetI;      // should be 0 to SIZE-1
float distance_to_travel = 0;
float x_leg_of_tri = 0;

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

int main(void){        // Main State Machine
  char string[20];
  char* pend;           // identify the space between numbers
  float x1;
  float y1;
  unsigned char stage = 0;
  volatile unsigned char timer_set = 0; //identify if timer is set or not

  char num_of_cor = 0;
  float first_x_val[8];
  float first_y_val[8];

  float current_x_val[8];
  float current_y_val[8];



  float initial_x_cor;
  float initial_y_cor;
  float current_x_cor;
  float current_y_cor;
  float desir_x_cor;
  float desir_y_cor;


  float north_angle_atan;

  char facing_N = 0;
  char facing_E = 0;

  Clock_Init48MHz();  // makes SMCLK=12 MHz
  UART1_Initprintf(); // initialize UART and printf
  PWM_Init34(15000, 0, 0);
  Motor_Init();
  Tachometer_Init();

  while(1){
      switch(stage){
          case 0:                 //Receive desired location to go to
          {
              UART1_InString(string,19);         //IMPORTANT: message separate with space " ", end with CR "\r"
              if(string[0] == 'G'){
                  UART1_InString(string,19);
                  desir_x_cor = strtof(string, &pend);
                  desir_y_cor = strtof(pend, NULL);
                  stage ++;
              }
              break;
          }
          case 1:              //Get first location
          {
              //===============
              RxPutI = 0;
              RxGetI = 0;
              UART1_InString(string,19);
              x1 = strtof(string, &pend);
              y1 = strtof(pend, NULL);
              first_x_val[num_of_cor] = x1;
              first_y_val[num_of_cor] = y1;
              num_of_cor++;
              if(num_of_cor>=8){
                  timer_set=0;
                  stage++;
              }
              break;
          }
          case 2:                     //Averaging
          {
              float x_sum = 0;
              float y_sum = 0;
              for (int i=0; i < 8; i++) {
                  x_sum += first_x_val[i];
                  y_sum += first_y_val[i];
              }
              initial_x_cor = x_sum / 8;
              initial_y_cor = y_sum / 8;

              // Now, initial_x_cor and initial_y_cor have the initial coordinate.

              stage++;                        //Go ahead and move forward
              break;
          }

          case 3:               // Move forward
          {
              if(!timer_set){
                  TimerA2_Init(&Timer_Done, 512*8);  //move 8s
                  timer_set = 1;
                  timerDone = 0;

              }
              Motor_Forward(2000,2000);         //Left Right ratio 2100:2000, now use tachometer to measure =========================== Editing

              if(timerDone){
                  stage ++;                        //get data again
                  timer_set = 0;
                  Motor_Stop();
              }
              break;
          }
          case 4:               // Wait for getting location
          {
              if(!timer_set){
                  TimerA2_Init(&Timer_Done, 512*10);  //wait for 10s to start up
                  timer_set = 1;
                  timerDone = 0;
              }
              Motor_Stop();
              num_of_cor = 0;
              if(timerDone){
                  stage ++;
                  timer_set = 0;
              }
              break;
          }

          case 5:               // Get  second coordinates
          {
              RxPutI = 0;
              RxGetI = 0;
              UART1_InString(string,19);         //IMPORTANT: message separate with space " ", end with CR "\r"
              x1 = strtof(string, &pend);
              y1 = strtof(pend, NULL);
              current_x_val[num_of_cor] = x1;
              current_y_val[num_of_cor] = y1;
              num_of_cor++;
              if(num_of_cor>=8){
                  timer_set=0;
                  stage++;
              }
              break;
          }

          case 6:               // Averaging
          {
              float x_sum = 0;
              float y_sum = 0;
              for (int i=0; i < 8; i++) {
                  x_sum += current_x_val[i];
                  y_sum += current_y_val[i];
              }
              current_x_cor = x_sum / 8;
              current_y_cor = y_sum / 8;
              stage++;                        //go back and move motors

              break;
          }

          case 7:       //
          {
              float change_x = current_x_cor - initial_x_cor;
              float change_y = current_y_cor - initial_y_cor;
              char going_up = (change_y < 0);
              char going_right = (change_x > 0);
              float extra_left_turn;

              if (going_up && going_right) {
                  north_angle_atan = 90 +atan(change_y/change_x)*180/3.14159;
                  //north_angle_atan2 = 90 +atan2(change_y,change_x)*180/3.14159;
              }
              else if (going_up && !going_right) {
                  north_angle_atan = 270 + atan(change_y/change_x)*180/3.14159;
                  //north_angle_atan2 = 270 + atan2(change_y,change_x)*180/3.14159;

              }
              else if (!going_up && going_right) {
                  north_angle_atan = 90 + atan(change_y/change_x)*180/3.14159;
                  //north_angle_atan2 =90 + atan2(change_y,change_x)*180/3.14159;

              }
              else if (!going_up && !going_right) {
                  north_angle_atan = 270  + atan(change_y/change_x)*180/3.14159;
                  //north_angle_atan2 =  270  + atan2(change_y,change_x)*180/3.14159;
              }
              else {
                  // This is really bad
                  int n_a;
              }

              stage++;
              timer_set = 0;
              break;
          }
          case 8:   //Turn to north
          {
              if(!timer_set){
                        TimerA2_Init(&Timer_Done, 9.9833*north_angle_atan);  //wait for 30s to start up
                        timer_set = 1;
                        timerDone = 0;
                    }
                    Motor_Left(2000,2000);
                    if(timerDone){
                        timer_set = 0;
                        Motor_Stop();
                        //facing_N = 1;
                        //stage = y_Decision;
                        stage ++;
                    }
                    break;
          }
          case 9: //y_Decision:
                    {
                            if(current_y_cor - desir_y_cor > 0.1)     // need to move upward | 0.2 threshold on y cor
                                //stage = MoveForward;                           // keep moving forward
                                stage ++;
                            else if( desir_y_cor - current_y_cor > 0.1) {
                                //stage = MoveBackward;
                                stage = stage +2;
                            }
                            else{
                                stage = 12;
                            }

                            break;
                        }

          case 10:      // MoveForward:  Move in y direction (Move forward)
          {
              distance_to_travel = current_y_cor - desir_y_cor;
              if(!timer_set){
                  TimerA2_Init(&Timer_Done, distance_to_travel*TIME2DIST);
                  timer_set = 1;
                  timerDone = 0;
              }
              Motor_Forward(2000,2000);
              if(timerDone){
                  timer_set = 0;
                  Motor_Stop();
                  /*if(facing_N)
                      stage = FaceEast;
                  else
                      stage = FaceNorth;*/
                  stage = stage + 2;
              }
              break;
          }
          case 11:          // MoveBackward in y direction backward
          {
              distance_to_travel =  desir_y_cor -current_y_cor ;
              if(!timer_set){
                  TimerA2_Init(&Timer_Done, distance_to_travel*TIME2DIST);  //wait for 30s to start up
                  timer_set = 1;
                  timerDone = 0;
              }
              Motor_Backward(2000,2000);
              if(timerDone){
                  timer_set = 0;
                  Motor_Stop();
                  stage ++;
              }
              break;
          }
          case 12: //FaceEast:
                    {
                        if(!timer_set){
                            TimerA2_Init(&Timer_Done, 900);  // Turn for 900 to turn 90 degrees
                            timer_set = 1;
                            timerDone = 0;
                        }
                        Motor_Right(2000,2000);
                        if(timerDone){
                            timer_set = 0;
                            Motor_Stop();
                            facing_N = 0;
                            facing_E = 1;
                            //stage = Decision;
                            stage ++;
                        }
                        break;
                    }


          case 13: //x_Decision:
          {
              if(current_x_cor - desir_x_cor > 0.1)     // need to move backwards
                  //stage = MoveBackward;
                  stage = stage + 2;
              else if( desir_x_cor - current_x_cor > 0.1){
                  //stage = MoveForward;
                  stage = stage ++;
              }
              else{
                  stage = stage + 3;
              }
              break;
          }

          case 14: // MoveForward:   // Move in x direction (Move forward)
                   {
                       distance_to_travel =  desir_x_cor - current_x_cor ;

                       if(!timer_set){
                           TimerA2_Init(&Timer_Done, distance_to_travel*TIME2DIST);  //wait for 30s to start up
                           timer_set = 1;
                           timerDone = 0;
                       }
                       Motor_Forward(2000,2000);
                       if(timerDone){
                           timer_set = 0;
                           Motor_Stop();
                           /*if(facing_N)
                               stage = FaceEast;
                           else
                               stage = FaceNorth;*/
                           stage = stage + 2;
                       }
                       break;
                   }
          case 15: // MoveBackward:  in y direction backward
                   {
                       distance_to_travel = current_x_cor - desir_x_cor;
                       if(!timer_set){
                           TimerA2_Init(&Timer_Done, distance_to_travel*TIME2DIST);  //wait for 30s to start up
                           timer_set = 1;
                           timerDone = 0;
                       }
                       Motor_Backward(2000,2000);
                       if(timerDone){
                           timer_set = 0;
                           Motor_Stop();
                           stage ++;
                       }
                       break;
                   }


          case 16: //SendA:            // Where receives commands from esp32
          {
              UART1_OutChar('A');
              stage = 0;
              break;
          }

          default:
          {
              break;
          }


      }


  }
}

