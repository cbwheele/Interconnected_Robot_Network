// UARTmain.c
// Runs on MSP432
// Alpha code that drives the robot to center of 2 anchors

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

  float second_x_val[8];
  float second_y_val[8];



  float initial_x_cor;
  float initial_y_cor;
  float second_x_cor;
  float second_y_cor;

  float north_angle_atan;

  Clock_Init48MHz();  // makes SMCLK=12 MHz
  UART1_Initprintf(); // initialize UART and printf
  PWM_Init34(15000, 0, 0);
  Motor_Init();
  Tachometer_Init();

  while(1){
      switch(stage){
          case 0:                 //Start up, does nothing and wait for 30s
          {
              if(!timer_set){
                  TimerA2_Init(&Timer_Done, 512*30);  //wait for 30s to start up
                  timer_set = 1;
                  timerDone = 0;
              }
              num_of_cor = 0;
              if(timerDone){
                  stage ++;
                  timer_set = 0;
                  num_of_cor = 0;
              }
              break;
          }
          case 1:              //Gather data
          {
              //===============
              RxPutI = 0;
              RxGetI = 0;
              UART1_InString(string,19);         //IMPORTANT: message separate with space " ", end with CR "\r"
              /*printf( " %s\n", token );
      token = strtok(NULL, " ");
      printf( " %s\n", token );
      token = strtok(NULL, " ");
      printf( " %s\n", token ); */
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
              second_x_val[num_of_cor] = x1;
              second_y_val[num_of_cor] = y1;
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
                  x_sum += second_x_val[i];
                  y_sum += second_y_val[i];
              }
              second_x_cor = x_sum / 8;
              second_y_cor = y_sum / 8;

                            // Now, initial_x_cor and initial_y_cor have the initial coordinate.
              stage++;                        //go back and move motors

              break;
          }

          case 7:       // Calculate the angle needed to turn to center of anchors
          {
              float change_x = second_x_cor - initial_x_cor;
              float change_y = second_y_cor - initial_y_cor;
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
              //=========================Calculate
              if(second_x_cor > 1.5){
                  x_leg_of_tri = second_x_cor - 1.5;
                  extra_left_turn = atan(x_leg_of_tri/second_y_cor);
                  north_angle_atan = (int)(north_angle_atan + extra_left_turn)  % 360;
              }
                  else{
                      x_leg_of_tri = second_x_cor - 1.5;
                      extra_left_turn = atan(x_leg_of_tri/second_y_cor);
                      north_angle_atan = (int)(north_angle_atan - extra_left_turn) % 360;
                  }

              stage++;
              timer_set = 0;
              break;
          }
          case 8:   // face the center of anchor
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
                        stage++;
                    }
                    break;
          }
          case 9:   // Move towards to center
          {
              distance_to_travel = sqrt(pow(x_leg_of_tri, 2)+pow(second_x_cor, 2));         //10sec 1 unit
              if(!timer_set){
                  TimerA2_Init(&Timer_Done, distance_to_travel*20*512);  //wait for 30s to start up
                  timer_set = 1;
                  timerDone = 0;
              }
              Motor_Forward(2000,2000);
              if(timerDone){
                  timer_set = 0;
                  Motor_Stop();
                  stage++;
              }
              break;
          }
          case 10:
          {
              break;
          }
          case 11:
          {
              int stophere;
              break;
          }
          default:
          {
              break;
          }


      }


  }
}


