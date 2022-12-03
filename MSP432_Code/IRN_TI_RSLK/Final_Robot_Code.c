// Final_Robot_Code.c
// Runs on MSP432
// Decemeber 1, 2022
// This code allows the robot to drive to the coordinate input by user
// and then turn to a given orientation input by user
// and finally drive in a circle

#include <stdint.h>
#include <stdlib.h> //added for reading in coordinates
#include <stdio.h>  //added for reading in coordinates
#include <math.h>   //added for reading in coordinates
#include "msp.h"    
#include "Clock.h"  //added for timers
#include "ports.h"  //initialize ports
#include "./inc/TExaS.h"
#include "./inc/Motor.h"    //To control motors
#include "./inc/PWM.h"      //Use PWM functions
#include "./inc/TimerA2.h"  //Using Timers
#include "./inc/Tachometer.h"   //For Encoders
#include "./inc/UART1.h"    //serial communication

#define TIME2DIST 3072      //Factor of time required to travel 1 unit
#define TIME2DEG 6          //Factor of time required to turn 1 degree
#define MOTOR_L_VAL 2000    //Left Rigt motor ratio is 20:21, this values helps the motor to run fairly straight
#define MOTOR_R_VAL 2100

#define NINETY_TURN_ENCODERS 180    
#define ONE_DEGREE_ENCODERS 2

volatile unsigned char timerDone = 0;
uint32_t RxPutI;      // should be 0 to SIZE-1
uint32_t RxGetI;      // should be 0 to SIZE-1
float distance_to_travel = 0;
float x_leg_of_tri = 0;


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

/*
 // I could not get this to work on regular GPIO interrupts
 // For some reason it appears that Port 10 may not have an interrupt vector?!
 // Link to forum that says that: https://e2e.ti.com/support/microcontrollers/msp-low-power-microcontrollers-group/msp430/f/msp-low-power-microcontroller-forum/501845/msp432-gpio-interrupts
 //
 void Init_Tachometer(void) {
 P10->SEL0 &= ~0x30; // 10.4 and 10.5 GPIO
 P10->SEL1 &= ~0x30; // 10.4 and 10.5 GPIO
 P10->DIR  &= ~0x30; // Direction is input
 //P10->OUT |= 0x30;
 //P10->REN |= 0x30;
 P10->IFG &= ~0x30;
 P10->IES |= 0x30;
 P10->IE  |= 0x30;
 //NVIC->IP[4] = (NVIC->IP[4]&0xFFFFFF00)|0x00000040; // priority 2
 //NVIC->ISER[1] |= 0x00001000; // Enable port 10 interrupts which is 44?
 NVIC->IP[4] = (NVIC->IP[4]&0xFFFFFF00)|0x00000040; // priority 2
 NVIC->ISER[0] |= 0x0000C000;     // set pin 15 and 14
 //MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P10, GPIO_PIN3);
 //MAP_GPIO_clearInterruptFlag(GPIO_PORT_P10, GPIO_PIN3);
 }
 void PORT10_IRQHandler(void) {
 if (P10->IFG & 0x10) {
 // Right?
 }
 if (P10->IFG & 0x20) {
 // Left?
 }
 }
 */

volatile uint16_t left_counter = 0;
volatile uint16_t right_counter = 0;

void TimerA3Capture_Init()  // Timer used for encoder interrupt
{
    // wrote this for Lab 16
    P10->SEL0 |= 0x30;      //set SEL0 of P10.4 and P10.5 to 1      ==> SEL = 01
    P10->SEL1 &= ~0x30;     //set SEL1 of P10.4 and P10.5 to 0
    P10->DIR &= ~0x30;      // make P10.4, P10.5 in
    TIMER_A3->CTL &= ~0x0030;               // halt Timer A3
    TIMER_A3->CTL = 0x0200;
    TIMER_A3->CCTL[0] = 0x4910; //bit 0 and bit 4 set by 10.4
    TIMER_A3->CCTL[1] = 0x4910; //bit 0 and bit 4 set by 10.5
    TIMER_A3->EX0 &= ~0x0007;       // configure for input clock divider /1
    NVIC->IP[4] = (NVIC->IP[4] & 0xFFFFFF00) | 0x00000040; // priority 2
    NVIC->ISER[0] |= 0x0000C000;     // set pin 15 and 14
    TIMER_A3->CTL |= 0x0024;   // reset and start Timer A3 in continuous up mode
}

void TA3_0_IRQHandler(void)           // Set up Interrupt to count right motor encoder ticks
{
    TIMER_A3->CCTL[0] &= ~0x0001;     // acknowledge capture/compare interrupt 0
    right_counter++;

}

void TA3_N_IRQHandler(void)           // Set up Interrupt to count left motor encoder ticks
{
    TIMER_A3->CCTL[1] &= ~0x0001;     // acknowledge capture/compare interrupt 0
    left_counter++;
}

void Init_Bumper_Switches(void)       // Initialize bumper switches configuration
{

    P4->SEL0 &= ~0xED; // GPIO
    P4->SEL1 &= ~0xED; // GPIO
    P4->DIR &= ~0xED; // Direction is input
    P4->OUT |= 0xED;
    P4->REN |= 0xED;
    P4->IFG &= ~0xED;
    P4->IES |= 0xED;
    P4->IE |= 0xED;
    NVIC->IP[8] = (NVIC->IP[8] & 0x00FFFFFF) | 0x40000000; // priority 2
    NVIC->ISER[1] |= 0x00000040; // Enable 38
    // 35 was ISER[1] = 0x00000008;  // 00000000000001000
    // 40 was ISER[1] = 0x00000100;  // 00000000100000000
    // 38 is  ISER[1] = 0x00000040;  //       00001000000
    // 44 ??  ISER[1] = 0x00001000;  // 00001000000000000
}

volatile unsigned char stage = 0;

void PORT4_IRQHandler(void)
{
    // This is the IRQ for Port 4 which contains the bumper switches
    if (P4->IFG & 0x0ED)
    {
        P4->IFG &= ~0xED;
        // This means that a bumper switch was pressed
        stage = 100;
        Motor_Stop();
    }
}

#define MOVING_SPEED 2000
#define UNIT_PER_ROTATION 9.45

int main(void)
{        // Main State Machine
    char string[20];
    char *pend;           // identify the space between numbers
    float x1;
    float y1;

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

    float circle_angle;
    float num_of_circle_ticks;

    char facing_N = 0;
    char facing_E = 0;
    char na_turn_R = 0;
    int currentRightSpeed = 0;
    int currentLeftSpeed = 0;
    int rightEncoderCutoffVal = 100;
    int leftEncoderCutoffVal = 100;

    int movingForwardCounter = 0;
    int turnToNorthEncoderCount = 0;

    char movingForward = 0;
    int howManyToMoveStraight = 0;

    Clock_Init48MHz();  // makes SMCLK=12 MHz
    UART1_Initprintf(); // initialize UART and printf
    PWM_Init34(15000, 0, 0);    //Set PWM Cycle
    Motor_Init();       // Set GPIO ports for motor
    Tachometer_Init();  // Initialize encoder

    Init_Bumper_Switches(); 
    TimerA3Capture_Init();

    /// JUST FOR TESTING DELETE THIS!!!!!!!                             // BREAKING SCRIPT JUST FOR TESTING
    //stage = 20;
    //howManyToMoveStraight = 50;

    while (1)
    {
        switch (stage)
        {
        case 100:
        {
            Motor_Stop(); // Stay in this state forever
            // Turn the LED red to show that it hit something with the bumper switch
            break;
        }
        case 0:                 //Receive desired location to go to
        {
            Motor_Stop();
            RxGetI = 0;         // Clears RX ring buffer to filter unwanted data
            UART1_InString(string, 19); //IMPORTANT: message separate with space " ", end with CR "\r"
            if (string[0] == 'G')
            {
                UART1_InString(string, 19);
                desir_x_cor = strtof(string, &pend);
                desir_y_cor = strtof(pend, NULL);
                stage++;
            }
            else if (string[0] == 'C')
            {
                UART1_InString(string, 19);
                circle_angle = strtof(string, &pend);
                num_of_circle_ticks = strtof(pend, NULL);
                stage = 18;                          // Jump this straight to 18 which is acting upon values received just here
            }
            else if (string[0] == 'M') {
                stage = 17;                             // Go to the manual control state
            }
            num_of_cor = 0;
            break;
        }
        case 1:              //Get first 8 location
        {
            //===============
            RxPutI = 0;
            RxGetI = 0;
            UART1_InString(string, 19);
            x1 = strtof(string, &pend);
            y1 = strtof(pend, NULL);
            first_x_val[num_of_cor] = x1;
            first_y_val[num_of_cor] = y1;
            num_of_cor++;
            if (num_of_cor >= 8)
            {
                timer_set = 0;
                stage++;
            }
            break;
        }
        case 2:                     //Averaging
        {

            float x_sum = 0;
            float y_sum = 0;
            for (int i = 0; i < 8; i++)
            {
                x_sum += first_x_val[i];
                y_sum += first_y_val[i];
            }
            initial_x_cor = x_sum / 8;
            initial_y_cor = y_sum / 8;

            // Now, initial_x_cor and initial_y_cor have the initial coordinate.

            stage++;                        //Go ahead and move forward
            currentRightSpeed = MOVING_SPEED;
            currentLeftSpeed = MOVING_SPEED;
            right_counter = 0;
            left_counter = 0;
            rightEncoderCutoffVal = 100;    //Line up motors for every 100 encoder ticks
            leftEncoderCutoffVal = 100;
            movingForwardCounter = 0; // How many 1/4 turns it has gone forward.
            break;
        }

        case 3:               // Move forward
        {

            // Code for going straight
            if (right_counter == rightEncoderCutoffVal)
            {
                currentRightSpeed = 0;
                leftEncoderCutoffVal++;
                right_counter++;

            }
            if (left_counter == leftEncoderCutoffVal)
            {
                currentLeftSpeed = 0;
                rightEncoderCutoffVal++;
                left_counter++;
            }
            if (right_counter >= rightEncoderCutoffVal
                    && left_counter >= leftEncoderCutoffVal)
            {
                left_counter = 0;
                right_counter = 0;
                currentRightSpeed = MOVING_SPEED;
                currentLeftSpeed = MOVING_SPEED;

                if (movingForwardCounter++ > 6)
                {
                    stage++;
                    timer_set = 0;
                    Motor_Stop();
                }
            }
            // End of code to go straight

            Motor_Forward(currentLeftSpeed, currentRightSpeed);

            break;
        }
        case 4:               // Wait for getting location
        {
            if (!timer_set)
            {
                TimerA2_Init(&Timer_Done, 512 * 10);  //wait for 10s to start up
                timer_set = 1;
                timerDone = 0;
            }
            Motor_Stop();
            num_of_cor = 0;
            if (timerDone)
            {
                stage++;
                timer_set = 0;
            }
            break;
        }

        case 5:               // Get  second coordinates (Get 8 values for averaging)
        {

            RxPutI = 0;
            RxGetI = 0;
            UART1_InString(string, 19); //IMPORTANT: message separate with space " ", end with CR "\r"
            x1 = strtof(string, &pend);
            y1 = strtof(pend, NULL);
            current_x_val[num_of_cor] = x1;
            current_y_val[num_of_cor] = y1;
            num_of_cor++;
            if (num_of_cor >= 8)
            {
                timer_set = 0;
                stage++;
            }
            break;
        }

        case 6:               // Averaging
        {
            float x_sum = 0;
            float y_sum = 0;
            for (int i = 0; i < 8; i++)
            {
                x_sum += current_x_val[i];
                y_sum += current_y_val[i];
            }
            current_x_cor = x_sum / 8;
            current_y_cor = y_sum / 8;
            stage++;                        //go back and move motors

            break;
        }

        case 7:       //Calculate the angle to turn north (facing the anchors)
        {
            float change_x = current_x_cor - initial_x_cor;
            float change_y = current_y_cor - initial_y_cor;
            char going_up = (change_y < 0);
            char going_right = (change_x > 0);
            float extra_left_turn;

            if (going_up && going_right)
            {
                north_angle_atan = 90
                        + atan(change_y / change_x) * 180 / 3.14159;

            }
            else if (going_up && !going_right)
            {
                north_angle_atan = 270
                        + atan(change_y / change_x) * 180 / 3.14159;

            }
            else if (!going_up && going_right)
            {
                north_angle_atan = 90
                        + atan(change_y / change_x) * 180 / 3.14159;

            }
            else if (!going_up && !going_right)
            {
                north_angle_atan = 270
                        + atan(change_y / change_x) * 180 / 3.14159;

            }
            else
            {
                // This is really bad
                int n_a;
            }
            stage++;
            timer_set = 0;
            na_turn_R = 0;
            if (north_angle_atan > 180)
            {
                north_angle_atan -= 180;
                na_turn_R = 1;

            }
            break;
        }
        case 8:   //Turn to north
        {
            if (!timer_set)
            {
                timer_set = 1;
                right_counter = 0;
                left_counter = 0;
                currentRightSpeed = 2000;
                currentLeftSpeed = 2000;

                turnToNorthEncoderCount =
                        north_angle_atan * ONE_DEGREE_ENCODERS;
            }

            // Code to turn by the specific number of degrees
            if (right_counter == turnToNorthEncoderCount)
            {
                currentRightSpeed = 0;
            }
            if (left_counter == turnToNorthEncoderCount)
            {
                currentLeftSpeed = 0;
            }
            if (na_turn_R)
            {
                Motor_Right(currentLeftSpeed, currentRightSpeed);
            }
            else
            {
                Motor_Left(currentLeftSpeed, currentRightSpeed);
            }
            if (right_counter >= turnToNorthEncoderCount
                    || left_counter >= turnToNorthEncoderCount)
            {
                Motor_Stop();
                stage++;
                timer_set = 0;
                left_counter = 0;
                right_counter = 0;
            }
            // Code to turn by the specific number of degrees

            break;
        }
        case 9: // Delay for one second
        {
            if (!timer_set)
            {
                TimerA2_Init(&Timer_Done, 512); //wait for 30s to start up
                timer_set = 1;
                timerDone = 0;
            }
            if (timerDone)
            {
                timer_set = 0;
                stage++;
            }
            break;
        }

        case 10: // y_Decision: decide to go forward or backward on y axis
        {

            if (current_y_cor - desir_y_cor > 0.1)
            {
                //stage = MoveForward;                           // keep moving forward
                howManyToMoveStraight = (current_y_cor - desir_y_cor)
                        * UNIT_PER_ROTATION;
                movingForward = 1;
            }
            else if (desir_y_cor - current_y_cor > 0.1)
            {
                //stage = MoveBackward;
                movingForward = 0;
                howManyToMoveStraight = (desir_y_cor - current_y_cor)
                        * UNIT_PER_ROTATION;
            }
            else
            {
                stage++; // This basically means it will skip the "Move in Y axis" state
            }
            stage++;
            timer_set = 0;

            break;
        }

        case 11: // Move in Y axis
        {
            if (!timer_set)
            {
                timer_set = 1;

                // Initialize
                currentRightSpeed = MOVING_SPEED;
                currentLeftSpeed = MOVING_SPEED;
                right_counter = 0;
                left_counter = 0;
                rightEncoderCutoffVal = 100;
                leftEncoderCutoffVal = 100;
                movingForwardCounter = 0; // How many 1/4 turns it has gone forward.
            }

            // Code for going straight
            if (right_counter == rightEncoderCutoffVal)
            {
                currentRightSpeed = 0;
                leftEncoderCutoffVal++;
                right_counter++;
            }
            if (left_counter == leftEncoderCutoffVal)
            {
                currentLeftSpeed = 0;
                rightEncoderCutoffVal++;
                left_counter++;
            }
            if (right_counter >= rightEncoderCutoffVal
                    && left_counter >= leftEncoderCutoffVal)
            {
                left_counter = 0;
                right_counter = 0;
                currentRightSpeed = MOVING_SPEED;
                currentLeftSpeed = MOVING_SPEED;

                if (movingForwardCounter++ >= howManyToMoveStraight)
                {
                    stage++;
                    timer_set = 0;
                    Motor_Stop();
                }
            }
            // End of code to go straight

            if (movingForward)
            {
                Motor_Forward(currentLeftSpeed, currentRightSpeed);
            }
            else
            {
                Motor_Backward(currentLeftSpeed, currentRightSpeed);
            }

            break;
        }

        case 12: // Delay for one second
        {

            if (!timer_set)
            {
                TimerA2_Init(&Timer_Done, 512); //wait for 30s to start up
                timer_set = 1;
                timerDone = 0;
            }
            if (timerDone)
            {
                timer_set = 0;
                stage++;
            }
            break;
        }
        case 13: //FaceWest:
        {

            if (!timer_set)
            {
                timer_set = 1;
                right_counter = 0;
                left_counter = 0;
                currentRightSpeed = 2000;
                currentLeftSpeed = 2000;
            }

            // Code to turn ninety degrees
            if (right_counter == NINETY_TURN_ENCODERS)
            {
                currentRightSpeed = 0;
            }
            if (left_counter == NINETY_TURN_ENCODERS)
            {
                currentLeftSpeed = 0;
            }

            Motor_Left(currentLeftSpeed, currentRightSpeed);

            if (right_counter >= NINETY_TURN_ENCODERS
                    && left_counter >= NINETY_TURN_ENCODERS)
            {
                timer_set = 0;
                left_counter = 0;
                right_counter = 0;
                stage++;
                Motor_Stop();
            }
            // End of code to turn ninety degrees
            break;
        }

        case 14: // X decision , decide to go forward or backward
        {

            if (desir_x_cor - current_x_cor > 0.1)
            {    // need to move backwards
                movingForward = 0;
                howManyToMoveStraight = (desir_x_cor - current_x_cor)
                        * UNIT_PER_ROTATION;
            }
            else if (current_x_cor - desir_x_cor > 0.1)
            {
                // Need to move forwards
                movingForward = 1;
                howManyToMoveStraight = (current_x_cor - desir_x_cor)
                        * UNIT_PER_ROTATION;
            }
            else
            {
                stage++;
            }
            stage++;
            timer_set = 0;
            break;
        }

        case 15: // Move along x axis
        {

            if (!timer_set)
            {
                timer_set = 1;

                // Initialize
                currentRightSpeed = MOVING_SPEED;
                currentLeftSpeed = MOVING_SPEED;
                right_counter = 0;
                left_counter = 0;
                rightEncoderCutoffVal = 100;
                leftEncoderCutoffVal = 100;
                movingForwardCounter = 0; // How many 1/4 turns it has gone forward.
            }

            // Code for going straight
            if (right_counter == rightEncoderCutoffVal)
            {
                currentRightSpeed = 0;
                leftEncoderCutoffVal++;
                right_counter++;
            }
            if (left_counter == leftEncoderCutoffVal)
            {
                currentLeftSpeed = 0;
                rightEncoderCutoffVal++;
                left_counter++;
            }
            if (right_counter >= rightEncoderCutoffVal
                    && left_counter >= leftEncoderCutoffVal)
            {
                left_counter = 0;
                right_counter = 0;
                currentRightSpeed = MOVING_SPEED;
                currentLeftSpeed = MOVING_SPEED;

                if (movingForwardCounter++ >= howManyToMoveStraight)
                {
                    stage++;
                    timer_set = 0;
                    Motor_Stop();
                }
            }
            // End of code to go straight

            if (movingForward)
            {
                Motor_Forward(currentLeftSpeed, currentRightSpeed);
            }
            else
            {
                Motor_Backward(currentLeftSpeed, currentRightSpeed);
            }

            break;
        }

        case 16: //Send A:            // Where receives commands from esp32
        {
            Motor_Stop();
            UART1_OutChar('A');
            stage = 0;       // Go back to the beginning to decide what to do
            break;
        }
        case 17:    // Do manual control. Go to zero after manual control
        {

            // w, a, s, d, " ", p

            UART1_InString(string, 19); //IMPORTANT: message separate with space " ", end with CR "\r"
            // string[0] contains the character to act upon
            switch (string[0]) {
            case 'w':
                Motor_Forward(2000,2000);
                break;
            case 'a':
                Motor_Left(2000,2000);
                break;
            case 's':
                Motor_Backward(2000,2000);
                break;
            case 'd':
                Motor_Right(2000,2000);
                break;
            case ' ':
                Motor_Stop();
                break;
            case 'p':
                Motor_Stop();
                stage = 0;
                break;
            }


            break;
        }
        case 18:  
        {
            // Code for spinning to face orientation
            // Move to face in "circle_angle" direction
            // We are currently facing West

            if (circle_angle > 270) { // Need to turn right by less than 90 degrees
                na_turn_R = 1;
                north_angle_atan = circle_angle - 270;
            }
            else if (circle_angle < 90) { // Need to turn to the right by more than 90 degrees but less than 180 degrees
                na_turn_R = 1;
                north_angle_atan = circle_angle + 90;
            }
            else { // Need to turn to the left by 0-180 degrees
                na_turn_R = 0;
                north_angle_atan = 270 - circle_angle;
            }
            stage++;
            timer_set = 0;
            break;
        }
        case 19: // Turn to the direction where the robot is ready for driving in circle
        {
            if (!timer_set)
            {
                timer_set = 1;
                right_counter = 0;
                left_counter = 0;
                currentRightSpeed = 2000;
                currentLeftSpeed = 2000;

                turnToNorthEncoderCount = north_angle_atan * ONE_DEGREE_ENCODERS;
            }

            // Code to turn by the specific number of degrees
            if (right_counter == turnToNorthEncoderCount)
            {
                currentRightSpeed = 0;
            }
            if (left_counter == turnToNorthEncoderCount)
            {
                currentLeftSpeed = 0;
            }
            if (na_turn_R)
            {
                Motor_Right(currentLeftSpeed, currentRightSpeed);
            }
            else
            {
                Motor_Left(currentLeftSpeed, currentRightSpeed);
            }
            if (right_counter >= turnToNorthEncoderCount
                    || left_counter >= turnToNorthEncoderCount)
            {
                Motor_Stop();
                stage++;
                timer_set = 0;
                left_counter = 0;
                right_counter = 0;
            }
            // Code to turn by the specific number of degrees
            break;
        }
        case 20:    // Wait for "Go" from ground control station
        {
            // "S" for start and shape
            Motor_Stop();
            UART1_InString(string, 19); //IMPORTANT: message separate with space " ", end with CR "\r"
            if (string[0] == 'S')
            {
                timer_set = 0;
                stage++;
            }
            break;
        }
        case 21:    // Turn in a circle for num_of_circle_ticks steps
        {
            if (!timer_set)
            {
                timer_set = 1;

                // Initialize
                currentRightSpeed = 2000;
                currentLeftSpeed = 2680;
                right_counter = 0;
                left_counter = 0;
                rightEncoderCutoffVal = 100;
                leftEncoderCutoffVal = 132;
                movingForwardCounter = 0; // How many 1/4 turns it has gone forward.
            }

            // Code for going in a circle
            if (right_counter == rightEncoderCutoffVal)
            {
                currentRightSpeed = 0;
                leftEncoderCutoffVal++;
                right_counter++;
            }
            if (left_counter == leftEncoderCutoffVal)
            {
                currentLeftSpeed = 0;
                rightEncoderCutoffVal++;
                left_counter++;
            }
            if (right_counter >= rightEncoderCutoffVal
                    && left_counter >= leftEncoderCutoffVal)
            {
                left_counter = 0;
                right_counter = 0;
                currentRightSpeed = 2000;
                currentLeftSpeed = 2680;

                if (movingForwardCounter++ >= num_of_circle_ticks)
                {
                    stage++;
                    timer_set = 0;
                    Motor_Stop();
                }
            }
            // End of code to go straight


            Motor_Forward(currentLeftSpeed, currentRightSpeed);
            break;

        }
        case 22:
        {
            Motor_Stop();
            break;
        }

        default:
        {
            break;
        }

        }

    }
}

