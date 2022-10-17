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

#include "./inc/UART1.h"    //serial communication

volatile unsigned char timerDone = 0;

 // added for test =========================================================================
void reverse(char* str, int len)
{
    int i = 0, j = len - 1, temp;
    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

// Converts a given integer x to string str[].
// d is the number of digits required in the output.
// If d is more than the number of digits in x,
// then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x) {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';

    reverse(str, i);
    str[i] = '\0';
    return i;
}

// Converts a floating-point/double number to a string.
void ftoa(float n, char* res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;

    // Extract floating part
    float fpart = n - (float)ipart;

    // convert integer part to string
    int i = intToStr(ipart, res, 0);

    // check for display option after point
    if (afterpoint != 0) {
        res[i] = '.'; // add dot

        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter
        // is needed to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);

        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}

//==================================================================================

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

// This main version is for timers
int main4(void) {
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

    TimerA2_Init(&Timer_Done, 512); // 512 = 1 second, and then the variable timerDone will go to 1 at the end of the timer!

    while (1) {
        if (timerDone) {
            timerDone = 0;
            P2->OUT ^= 2;
            TimerA2_Init(&Timer_Done, 1014);
        }
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

int main3(void){        //=======================================UART code
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
    //UART1_OutString("InString: ");
    //UART1_InString(string,19); // user enters a string
    //UART1_OutString(" OutString="); UART1_OutString(string); UART1_OutChar(LF);

    /*UART1_OutString("InUDec: ");   n=UART0_InUDec();
    UART1_OutString(" OutUDec=");  UART0_OutUDec(n); UART0_OutChar(LF);
    UART1_OutString(" OutUFix1="); UART0_OutUFix1(n); UART0_OutChar(LF);
    UART1_OutString(" OutUFix2="); UART0_OutUFix2(n); UART0_OutChar(LF);
    printf(" Using printf= %d, %2d.%.1d,\n",n,n/10,n%10);

    UART1_OutString("InUHex: ");   n=UART0_InUHex();
    UART1_OutString(" OutUHex=");  UART0_OutUHex(n); UART0_OutChar(LF);
    printf(" Using printf= %#x\n",n);*/
    UART1_InString(string,19);
    UART1_OutString(string);
    UART1_OutChar(LF);

  }
}

int main5(void){        //=======================================UART code
  char ch;
  char string[20];
  char xcor[8];
  char ycor[8];
  char* pend;           // test
  float x1;
  float y1;

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
    UART1_InString(string,19);         //IMPORTANT: message separate with space " ", end with CR "\r"
    x1 = strtof(string, &pend);
    y1 = strtof(pend, NULL);
    ftoa(x1, xcor, 4);
    ftoa(y1, ycor, 4);
    UART1_OutString("x is: ");
    UART1_OutString(xcor);
    UART1_OutChar(LF);
    UART1_OutString("y is: ");
    UART1_OutString(ycor);
    UART1_OutChar(LF);
  }
}

int main7(void){        //=======================================UART + Motor code

  char string[20];
  char motor_on = 0;
  char* pend;           // test if strtof can separate numbers
  char timer_set = 0;   // test if motor can be controlled
  float x1;
  float y1;
  float dis = 0;
  Clock_Init48MHz();  // makes SMCLK=12 MHz
  UART1_Initprintf(); // initialize UART and printf
  PWM_Init34(15000, 0, 0);
  Motor_Init();

  while(1){
    if(!timer_set){
        UART1_OutString("Waiting for input");
        UART1_InString(string,19);         //IMPORTANT: message separate with space " ", end with CR "\r"
        x1 = strtof(string, &pend);
        y1 = strtof(pend, NULL);
        UART1_OutString("Got data");
        TimerA2_Init(&Timer_Done, 2048);  //wait for 4s to get data
        dis = sqrt(pow(x1,2)+pow(y1,2));

        motor_on = 1;
        timer_set = 1;
        timerDone = 0;
    }
    if(motor_on){
        UART1_OutString("Motor is on");
        if(dis>=2)
            Motor_Forward(2000,2000);
        else
            Motor_Backward(2000,2000);
        }
    else{
        UART1_OutString("Motor not on");
        Motor_Stop();
    }
    if(timerDone){
        UART1_OutString("Reset timer");
        Motor_Stop();
        timer_set = 0;
        motor_on = 0;
    }
  }
}

int main6(void){        //=======================================UART code
  char string[20];
  char* pend;           // test
  float x[5];
  float y[5];
  char timer_set = 0;   // test if motor can be controlled
  char i = 0;           // counter
  char f[0]=0;
  //uint32_t n;
  Clock_Init48MHz();  // makes SMCLK=12 MHz
  UART1_Initprintf(); // initialize UART and printf
  UART1_OutString("\nTest program for UART driver\n\rUART0_OutChar examples\n");

  while(1){
      if(!timer_set){
          TimerA2_Init(&Timer_Done, 2048);  //wait for 4s to get data
          timer_set = 1;
          timerDone = 0;
          }
      UART1_OutString("Waiting for input ");
      UART1_InString(string,19);         //IMPORTANT: message separate with space " ", end with CR "\r"
      x[i] = strtof(string, &pend);
      y[i] = strtof(pend, NULL);
      i++;
      f[0]++;
      UART1_OutString("Got ");

      UART1_OutString(f);
      UART1_OutString(" Coordinates\r");
      if(timerDone){
          timer_set = 0;
          UART1_OutString("reset timer\r");
          i=0;
      }

  }
}

int main(void){        //=======================================UART code
  char timer_set = 0;   // test if motor can be controlled
  char motor_on = 0;
  Clock_Init48MHz();  // makes SMCLK=12 MHz
  PWM_Init34(15000, 0, 0);
  Motor_Init();
  char mode = 0;
  while(1){
      switch(mode){
      case 0:
          if(!timer_set){
              TimerA2_Init(&Timer_Done, 512*4);  //wait for 30s to start up
              timer_set = 1;
              timerDone = 0;
          }
          if(timerDone){
              timer_set = 0;
              mode ++;
          }
          break;
      case 1:
      if(!timer_set){
          TimerA2_Init(&Timer_Done, 512*8);  //wait for 30s to start up
          timer_set = 1;
          timerDone = 0;
          motor_on = 1;
      }
      if(motor_on)
      Motor_Forward(2100,2000);         //Left Right ratio 2100:2000
      if(timerDone){
          //timer_set = 0;
          Motor_Stop();
          motor_on =0;
      }
      break;

      }
  }
}