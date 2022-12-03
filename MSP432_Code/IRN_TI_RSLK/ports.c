// Ports file for IRN, did not use for project


#include "msp.h"

void Init_Our_Custom_Ports(void) {
    P2->SEL0 &= ~0x01;
     P2->SEL1 &= ~0x01;
     P2->DIR  |=  0x01;
     P2->OUT  |=  0x01;
}
