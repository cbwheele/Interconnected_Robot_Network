// motor_value_testing.c
// Runs on MSP432
// Used to test what motor speed can let the robot drive in straight line
// Novmeber 5, 2022


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

#define TIME2DIST 3072
#define TIME2DEG 6
volatile unsigned char timerDone = 0;
uint32_t RxPutI;      // should be 0 to SIZE-1
uint32_t RxGetI;      // should be 0 to SIZE-1
float distance_to_travel = 0;
float x_leg_of_tri = 0;

void GPIO_Init(void)
{
    // initialize P4.3-P4.0 and make them outputs
    P4->SEL0 &= ~0x0F;
    P4->SEL1 &= ~0x0F;            // configure stepper motor/LED pins as GPIO
    P4->DIR |= 0x0F;              // make stepper motor/LED pins out
}

void TA1_0_IRQHandler(void)
{
    TIMER_A1->CCTL[0] &= ~0x0001; // ack
// body
    P2->OUT ^= 2;
}

void Timer_Done(void)
{
    TIMER_A2->CTL &= ~0x0030;       // halt Timer A2
    timerDone = 1;
}

int main(void)
{        // Main State Machine
    char string[20];
    char *pend;           // identify the space between numbers
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
    char motor_on = 0;

    while (1)
    {
        if (!timer_set)
        {
            TimerA2_Init(&Timer_Done, 1*TIME2DIST);  //move 8s
            timer_set = 1;
            motor_on = 1;
            timerDone = 0;

        }
        if (motor_on){
            Motor_Forward(2000, 2000); //Left Right ratio 2100:2000, now use tachometer to measure =========================== Editing
        }
        if (timerDone)
        {
            stage++;                        //get data again
            motor_on = 0;
            Motor_Stop();
        }
        WaitForInterrupt();
    }
}

