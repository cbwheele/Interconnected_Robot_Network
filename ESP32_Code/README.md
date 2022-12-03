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

### Modify IP Address


### Modify code to make it be fully sequence control or circling only


### Flashing the code and using the tags


## Tag Code