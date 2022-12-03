# ESP32 Code

This folder contains three sub-folders, which contain code for:
- [`anchor`](./anchor/): Setting up an ESP32 to be an "anchor" which are the robots on the edge of the operating theatre that act as grounding points for localization
- [`IRN_Robot_ESP32`](./IRN_Robot_ESP32/): Code necessary to put on tag robots to have them be controlled via Python script, move to initial positions, and move in coordinated shapes
- [`tag`](./tag/): Only useful for testing distances to anchor DWM1000s as it only provides raw distances over USB connection into Arduino IDE terminal output window


## Requirements
- [Arduino IDE](https://www.arduino.cc/en/software) Installed on your computer 
- Add development capabilities for ESP32 Module:
   - Add "https://dl.espressif.com/dl/package_esp32_index.json" to "Additional Board Manager URLs". Tutorial [here](https://support.arduino.cc/hc/en-us/articles/360016466340-Add-or-remove-third-party-boards-in-Boards-Manager)
   - Install "esp32" by Espressif Systems in the Aruidno Board manager. Tutorial [here](https://support.arduino.cc/hc/en-us/articles/360016119519-Add-boards-to-Arduino-IDE)
- Select "ESP32 Dev Module" from the Tools > Board drop-down menu
- Micro-USB cable connected between computer and ESP32 
- Make sure you correct the correct serial port from the Tools > Port drop-down menu *every single time* before flashing code 
- If the code doesn't flash and just times out while "Connecting...___...___" then press and hold the "Flash" button on the ESP32 while downloading the code


## Anchor Code

### Modify "Anchor Address"
- Go to the top of the file and change the ANCHOR_ADD macro depending on which anchor you are programing
- For the anchor at the top left, start the anchor address with an 82
- For the anchor at the top right, start the anchor address with an 83
- More detailed instructions are in the comments around the comments around the macro

### Flashing the code
- Compile and flash the code onto two anchors
- Configure one anchor to have address 82 and another to have address 83
- Put the 82 address anchor to the top left of the operating theatre
- Put the 83 address anchor to the top right of the operating theatre
- Place them approximately 81 inches apart (3.0 units is appriximately 27*3=81 inches)
- Turn them on so they activate as anchors
- At first to debug, leave plugged in via USB to Arduino IDE terminal emulator and ensure serial debug output says "Initialized as anchor with address 8X:17:5B:D5:A9:9A:E2:9C"


## IRN_Robot_ESP32 Code


### Modify code to make it be fully sequence control or circling only
- Change the `circleOnlyOrRegular` variable to control if the ESP32 runs the entire shape sequence (aka gets to initial location as well), or only causes the robot to circle around in the circle:
- For full processing and data control, change the value to `READ_TARGET_COORDINATES`
- For moving in a circle only, change the value to `RECEIVE_CIRCLE_DIRECTION_AND_DURATION`
- These details are outined in the code as well in comments surrounding the variable
- This works by changing which state the state machine loops to after the modules boot up

### Modify NUM_OF_TIMES_TO_AUTO_LOC
- Modify the `NUM_OF_TIMES_TO_AUTO_LOC` with now many times you want the robot to try to get to the initial starting location autonomously
- This is the number of failed attempts the robot will tolerate before moving into manual control mode
- This must be a non-zero, positive integer, so 1, 2, 3, 4, etc.
- Reccomended values are 2 or 3

### Modify WiFi Credentials
- You must modify these WiFi credentials (`ssid` and `password`) for how the tag should connect to the WiFi network
- This must be the same network as the controlling computer is connected to
- Make sure to add the MAC address to the network preferences if that's needed for (especially school) WiFi networks
- If there is no WiFi password necessary, then leave it as a blank string such as ""
- If you do not add the WiFi credentials, then the tag won't be able to connect, and the red light will blink forever
- The red light stops blinking and the green light is illuminated when the tag successfully connects to the WiFi network

### Modify IP Address
- You must modify the `host` variable to be the string of the IP address of the controlling computer. 
- Make sure to leave it without leading or trailing spaces, and with the periods as shown in the example
- The tag must be on the same network as the controlling computer.



### Flashing the code and using the tags
- Put the ESP32s on top of the custom PCBs on top of the MSP432s on top of the robots 
- After the variables above have been changed for the specific test, compile and flash the code onto three ESP32s 
- Then put the three robots in the middle of the operating theatre (make sure to put two anchors at the top left and right if doing a full sequence test)
- Start the python script on the controlling computer
- Turn on each of the robots. Make sure to turn them on in order if order matters (see Python script README file instructions)
- Then control the robots using the python script
- When finished, power off all of the robots and anchors and put everything away




## Tag Code
- This code is incredibly simple
- It only boots up an ESP32/DWM1000 module into tag mode, which means it reads in the distances from as many anchors as it can "see"
- It is printed out to the serial communication port
- To run this code, compile and flash the code onto an ESP32
- Make sure at least one anchor (with distinct anchor addresses) are turned on nearby
- Open the Aruidno serial debug output
- Leave the tag plugged into the computer
- Read out the values that are coming over the serial port on the Aruidno IDE output 