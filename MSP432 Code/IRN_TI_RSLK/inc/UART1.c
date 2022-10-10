// UART1.c
// Runs on MSP432
// Use UCA2 to implement bidirectional data transfer to and from a
// CC2650 BLE module, uses interrupts for receive and busy-wait for transmit

// Daniel Valvano
// May 24, 2016

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


// UCA2RXD (VCP receive) connected to P3.2
// UCA2TXD (VCP transmit) connected to P3.3
// J1.3  from Bluetooth (DIO3_TXD) to LaunchPad (UART RxD){MSP432 P3.2}
// J1.4  from LaunchPad to Bluetooth (DIO2_RXD) (UART TxD){MSP432 P3.3}

#include <stdint.h>
#include <stdio.h> // added line to get Initprintf running
#include "UART1.h"
#include "msp.h"

#define FIFOSIZE   256       // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS 1        // return value on success
#define FIFOFAIL    0        // return value on failure
uint32_t RxPutI;      // should be 0 to SIZE-1
uint32_t RxGetI;      // should be 0 to SIZE-1 
uint32_t RxFifoLost;  // should be 0 
uint8_t RxFIFO[FIFOSIZE];
void RxFifo_Init(void){
  RxPutI = RxGetI = 0;                      // empty
  RxFifoLost = 0; // occurs on overflow
}
int RxFifo_Put(uint8_t data){
  if(((RxPutI+1)&(FIFOSIZE-1)) == RxGetI){
    RxFifoLost++;
    return FIFOFAIL; // fail if full  
  }    
  RxFIFO[RxPutI] = data;                    // save in FIFO
  RxPutI = (RxPutI+1)&(FIFOSIZE-1);         // next place to put
  return FIFOSUCCESS;
}
int RxFifo_Get(uint8_t *datapt){ 
  if(RxPutI == RxGetI) return 0;            // fail if empty
  *datapt = RxFIFO[RxGetI];                 // retrieve data
  RxGetI = (RxGetI+1)&(FIFOSIZE-1);         // next place to get
  return FIFOSUCCESS; 
}
                    
//------------UART1_InStatus------------
// Returns how much data available for reading
// Input: none
// Output: number of bytes in receive FIFO
uint32_t UART1_InStatus(void){  
 return ((RxPutI - RxGetI)&(FIFOSIZE-1));  
}
//------------UART1_Init------------
// Initialize the UART for 115,200 baud rate (assuming 12 MHz SMCLK clock),
// 8 bit word length, no parity bits, one stop bit
// Input: none
// Output: none
void UART1_Init(void){
  RxFifo_Init();              // initialize FIFOs
  EUSCI_A2->CTLW0 = 0x0001;         // hold the USCI module in reset mode
  // bit15=0,      no parity bits
  // bit14=x,      not used when parity is disabled
  // bit13=0,      LSB first
  // bit12=0,      8-bit data length
  // bit11=0,      1 stop bit
  // bits10-8=000, asynchronous UART mode
  // bits7-6=11,   clock source to SMCLK
  // bit5=0,       reject erroneous characters and do not set flag
  // bit4=0,       do not set flag for break characters
  // bit3=0,       not dormant
  // bit2=0,       transmit data, not address (not used here)
  // bit1=0,       do not transmit break (not used here)
  // bit0=1,       hold logic in reset state while configuring
  EUSCI_A2->CTLW0 = 0x00C1;
                              // set the baud rate
                              // N = clock/baud rate = 12,000,000/115,200 = 104.1667
  EUSCI_A2->BRW = 104;        // UCBR = baud rate = int(N) = 104

  EUSCI_A2->MCTLW = 0x0000;   // clear first and second modulation stage bit fields
// since TxFifo is empty, we initially disarm interrupts on UCTXIFG, but arm it on OutChar
  P3->SEL0 |= 0x0C;
  P3->SEL1 &= ~0x0C;          // configure P3.3 and P3.2 as primary module function
  NVIC->IP[4] = (NVIC->IP[4]&0xFF00FFFF)|0x00400000; // priority 2
  NVIC->ISER[0] = 0x00040000; // enable interrupt 18 in NVIC
  EUSCI_A2->CTLW0 &= ~0x0001; // enable the USCI module
                              // enable interrupts on receive full
  EUSCI_A2->IE = 0x0001;      // disable interrupts on transmit empty, start, complete
}


//------------UART1_InChar------------
// Wait for new serial port input, interrupt synchronization
// Input: none
// Output: an 8-bit byte received
// spin if RxFifo is empty
uint8_t UART1_InChar(void){
  uint8_t letter;
  while(RxFifo_Get(&letter) == FIFOFAIL){};
  return(letter);
}

///------------UART1_OutChar------------
// Output 8-bit to serial port, busy-wait
// Input: letter is an 8-bit data to be transferred
// Output: none
void UART1_OutChar(char letter){
  while((EUSCI_A2->IFG&0x02) == 0);
  EUSCI_A2->TXBUF = letter;
}
// interrupt 18 occurs on :
// UCRXIFG RX data register is full
// vector at 0x00000088 in startup_msp432.s
void EUSCIA2_IRQHandler(void){
  if(EUSCI_A2->IFG&0x01){             // RX data register full
    RxFifo_Put((uint8_t)EUSCI_A2->RXBUF);// clears UCRXIFG
  } 
}

//------------UART1_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART1_OutString(char *pt){
  while(*pt){
    UART1_OutChar(*pt);
    pt++;
  }
}
//------------UART1_FinishOutput------------
// Wait for all transmission to finish
// Input: none
// Output: none
void UART1_FinishOutput(void){
  // Wait for entire tx message to be sent
  while((EUSCI_A2->IFG&0x02) == 0);
}

void UART1_InString(char *bufPt, uint16_t max) {
int length=0;
char character;
  character = UART1_InChar();
  while(character != CR){
    if(character == BS){
      if(length){
        bufPt--;
        length--;
        UART1_OutChar(BS);
      }
    }
    else if(length < max){
      *bufPt = character;
      bufPt++;
      length++;
      UART1_OutChar(character);
    }
    character = UART1_InChar();
  }
  *bufPt = 0;
}

//-------------------------------------------------Added the below to test
//-----CCS implementation
#include "file.h"
int uart1_open(const char *path, unsigned flags, int llv_fd){
  UART1_Init();
  return 0;
}
int uart1_close( int dev_fd){
    return 0;
}
int uart1_read(int dev_fd, char *buf, unsigned count){char ch;
  ch = UART1_InChar();    // receive from keyboard
  ch = *buf;         // return by reference
  UART1_OutChar(ch);  // echo
  return 1;
}
int uart1_write(int dev_fd, const char *buf, unsigned count){ unsigned int num=count;
    while(num){
        if(*buf == 10){
           UART1_OutChar(13);
        }
        UART1_OutChar(*buf);
        buf++;
        num--;
    }
    return count;
}
off_t uart1_lseek(int dev_fd, off_t ioffset, int origin){
    return 0;
}
int uart1_unlink(const char * path){
    return 0;
}
int uart1_rename(const char *old_name, const char *new_name){
    return 0;
}

//------------UART1_Initprintf------------
// Initialize the UART for 115,200 baud rate (assuming 48 MHz bus clock),
// 8 bit word length, no parity bits, one stop bit
// Input: none
// Output: none
void UART1_Initprintf(void){int ret_val; FILE *fptr;
  UART1_Init();
  ret_val = add_device("uart", _SSA, uart1_open, uart1_close, uart1_read, uart1_write, uart1_lseek, uart1_unlink, uart1_rename);
  if(ret_val) return; // error
  fptr = fopen("uart","w");
  if(fptr == 0) return; // error
  freopen("uart:", "w", stdout); // redirect stdout to uart
  setvbuf(stdout, NULL, _IONBF, 0); // turn off buffering for stdout
}
