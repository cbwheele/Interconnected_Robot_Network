#!/usr/bin/env python


#########################
# Code for receiving input over terminal

# These imports are necessary for receiving keyboard input via the terminal
import sys
import time
import os
# Windows
if os.name == 'nt':
    import msvcrt

# Posix (Linux, OS X)
else:
    import termios
    import atexit
    from select import select


# This class is necessary for receiving keyboard input via the terminal
class KBHit:

    def __init__(self):
        '''Creates a KBHit object that you can call to do various keyboard things.
        '''

        if os.name == 'nt':
            pass

        else:

            # Save the terminal settings
            self.fd = sys.stdin.fileno()
            self.new_term = termios.tcgetattr(self.fd)
            self.old_term = termios.tcgetattr(self.fd)

            # New terminal setting unbuffered
            self.new_term[3] = (self.new_term[3] & ~termios.ICANON & ~termios.ECHO)
            termios.tcsetattr(self.fd, termios.TCSAFLUSH, self.new_term)

            # Support normal-terminal reset at exit
            atexit.register(self.set_normal_term)


    def set_normal_term(self):
        ''' Resets to normal terminal.  On Windows this is a no-op.
        '''

        if os.name == 'nt':
            pass

        else:
            termios.tcsetattr(self.fd, termios.TCSAFLUSH, self.old_term)


    def getch(self):
        ''' Returns a keyboard character after kbhit() has been called.
            Should not be called in the same program as getarrow().
        '''

        s = ''

        if os.name == 'nt':
            return msvcrt.getch().decode('utf-8')

        else:
            return sys.stdin.read(1)


    def getarrow(self):
        ''' Returns an arrow-key code after kbhit() has been called. Codes are
        0 : up
        1 : right
        2 : down
        3 : left
        Should not be called in the same program as getch().
        '''

        if os.name == 'nt':
            msvcrt.getch() # skip 0xE0
            c = msvcrt.getch()
            vals = [72, 77, 80, 75]

        else:
            c = sys.stdin.read(3)[2]
            vals = [65, 67, 66, 68]

        return vals.index(ord(c.decode('utf-8')))


    def kbhit(self):
        ''' Returns True if keyboard character was hit, False otherwise.
        '''
        if os.name == 'nt':
            return msvcrt.kbhit()

        else:
            dr,dw,de = select([sys.stdin], [], [], 0)
            return dr != []
kb = KBHit() # This initialization is required to read in terminal input

# End of code required to read in input from the terminal
######################################

def flush():
    sys.stdout.flush()
    
def receive_input():
    while (1):
        if (kb.kbhit()):
            c = kb.getch()
            print(c, end = '')
            flush()
            if (c == '\n'):
                return
                
                

print("\nWelcome to the Interconnected Robot Network!")
print("Please press 's' to setup WiFi Networks on your robots or 'c' to control your robots: ", end = '')
sys.stdout.flush();

setupMode = False

while (1):
    if (kb.kbhit()):
        c = kb.getch()
        print(c, end = '')
        flush()
        if (c == "s" or c == "S"):
            setupMode = True
            break
        elif (c == "c" or c == "C"):
            break
    
if (setupMode):
    print("\n\nPlease enter your WiFi Network name: ", end = '')
    flush()
    receive_input()
    print("\n\nPlease enter your WiFi password: ", end = '')
    flush()
    receive_input()
    while (1):
        print("\nPlug in one of the robots to your computer via USB")
        print("Press enter when connected to continue: ", end = '')
        flush()
        receive_input()
        print("Transfering network internet credentials", end = '')
        flush()
        for x in range(0, 10):
            print(" .", end = '')
            flush()
            time.sleep(0.3)
        print(" Successfully transfered credentials!")
        print("\nPress 'n' to setup WiFi on another robot, press 'd' to finish setup: ", end = '')
        flush()
        done = False
        while (1):
            if (kb.kbhit()):
                c = kb.getch()
                print(c, end = '')
                flush()
                if (c == "n" or c == "N"):
                    break
                elif (c == "d" or c == "D"):
                    done = True
                    break
        if (done):
            break
        print("\n") # New lines fors tarting over
                
            
    
else:
    while(1):
        print("\n\nPlease power up all of the robots and make sure the green LEDs are illuminated on all.")
        print("Press enter when all green LEDs are illuminated: ", end = '')
        flush()
        num = 0
        while (1):
            if (kb.kbhit()):
                c = kb.getch()
                
                if (c >= '1' and c <= '9'):
                    num = c
                    break
                if (c == '\n'):
                    break
        
        
        print(f"\n{num} robots were detected, is this the correct number of robots? [y/n]: ", end = '')
        flush()
        done = False
        while (1):
            if (kb.kbhit()):
                c = kb.getch()
                print(c, end = '')
                flush()
                if (c == "n" or c == "N"):
                    break
                elif (c == "y" or c == "Y"):
                    done = True
                    break
        if (done):
            break
            
    print("\n\nAll robots are set up!")
    
    print("Which shape would you like the robots to complete?")
    print("Press 's' for square and 't' for triangle: ", end = '')
    flush()
    square = False
    while (1):
        if (kb.kbhit()):
            c = kb.getch()
            print(c, end = '')
            flush()
            if (c == "t" or c == "T"):
                break
            elif (c == "s" or c == "S"):
                square = True
                break
    if (square):
        print("\n\nRobots will now move in a square.")
    else:
        print("\n\nRobots will now move in a triangle.")
        
    flush()
    print("\nPress enter to abort!!: ", end = '')
    flush()
    receive_input()
    print("\nRobots are now stopping!")
    
    
    
print("\n\nThank you for using the IRN!\n\n")
