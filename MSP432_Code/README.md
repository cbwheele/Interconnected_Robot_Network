# MSP432 Code

This folder contains two sub-folders.
- The important folder is the [IRN_TI_RSLK](./IRN_TI_RSLK/) which contains all of the code necessary to flash the MSP432 devices to control the robots.
- The other is a folder that holds our specific configutation data for the Code Composer Studio IDE, but it is not necessary to merge all of that before flashing the robots, as all of the necessary code dependency files are contained within the original folder.



## Requirements to Flash Code
- [Code Composer Studio (CCS) IDE](https://www.ti.com/tool/CCSTUDIO) Installed on your computer 
- Add development capabilities for MSP432 Module:
   - You may have to through the process of setting up development for the MSP432 microcontroller on your system
   - If the type of MSP432 is slightly off, we also had this problem and it didn't seem to give any isues with our development
- Open the folder in Code Composer Studio
- Make sure that IRN_TI_RSLK is selected in the menu on the left so that it says it is "Active Debug" (it is posible to have multiple projects in the same folder. Since there is only one in this folder it should be easy to tell if it is selected, but be ware when multiple projects are in the same folder that the correct one is selected)
- Plug in the MSP432 via micro-USB to your computer
- Click the "Debug" (the button that looks like a green beatle) to download the code
- If you are asked to update the firmware of the MSP432, then click to proceed and wait for that process to finish
- After the code has downoaded, either debug in the window or click the red square to exit the debugger
- Then unplug the MSP432 and connect external power for it to run


## Code Dependency Files
- The entire [inc](./IRN_TI_RSLK/inc/) folder contains dependencies for the correct function of the code
- These include motor, PWM, timer, and encoder (tachometer) code
- Also the main folder has dependencies for clocks and ports
- All of these files are imported into the centerl .c folder named [Final_Robot_Code.c](./IRN_TI_RSLK/Final_Robot_Code.c)


## Main Final Code
- All of the code for the final operation of the MSP432 is contained within the file [Final_Robot_Code.c](./IRN_TI_RSLK/Final_Robot_Code.c)
- The code has lots of initialization functions at the top
- And then it has the main state machine
- This state machine can do different things depending on what is received over UART into the MSP432
- A "G" command will cause the script to try to go to the specified initial starting position specified by the coordinates passed in
- A "M" command will cause the robot to start being controlled by manual WASD commands (with P meaning to stop manual control)
- A "C" command will cause the script to turn in a circle after an "S" for "start" is specified
- All of these are independent of one another, so it provides lots of functionality that can be triggered at different times by the ESP32
  
## Move to Cerain Coordinates State Function:
This is the most complicated operation of the MSP432 script, so it will be summarized here
- The code reads in the coordinates that it was sent over UART
- The code reads in eight averages of the robots current location and averages those as "first positoin"
- Then it moves forward by a certain amount
- It then averages eight readings for "second location"
- Does math on those to see which way it is facing
- Moves to face "north" (facing towards the anchors)
- Then moves however far (forwards or backwards) it needs to go in the "y" axis
- Then turns 90 degrees
- Then moves however far (forwards or backwards) it needs to go in the "x" axis
- Then it sends "A" to the ESP32 over UART


## Extra Files
- The folder also has lots of other files
- Some of which are not included in the project (so they are excluded to not have conflicts of the main() function)
- They provide insight into the progress of the project as the code went along throughout the project
- Only use them as references or other starting points for your own project
- But know that the main code is the code that is contained within the [Final_Robot_Code.c](./IRN_TI_RSLK/Final_Robot_Code.c) file

