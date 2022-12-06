# Interconnected_Robot_Network
The Interconnected Robot Network (abbreviated IRN) is a Senior Design project for four students in Electrical and Computer Engineering at NC State University that was completed in the Spring and Fall semesters of 2022.

Team Members: Brandon O, Caleb Wheeler, Clay Scarboro, and Tanner Boylan

This repo contains all of the code that relates in any way to the project.

We had three main sections of code:
- Code on the MSP432 written in C to control the robot
- Code on the ESP32 written in C++ to talk via UART to the MSP432, via WiFi to the computer, and via SPI to the DWM1000 module to get coordinates
- Code on the computer terminal written in Python to control the entire operation and communicate via WiFi with the ESP32s

There are three different folders inside this repo for the code for each of these sections, and each folder contains a README file that outlines what the code does.

High-level instructions for how to download/use the code (refer to individual folders for more specific instructions):
- Change IP address on ESP32 Robot Code and Flash the code to all ESP32s put on robots using an Arduino IDE
- Flash two ESP32s at the "anchors" with one IP addres starting with 82 and the other starting with 83
- Flash all MSP432s with the code contained in the MSP432 folder using Code Composer Studio IDE
- Place anchors 81" apart at the top of the operating theatre and turn on
- Place all three tag robots in the center of the operating theatre
- Start python script
- Turn on tag robots
- Control using python script
- Turn off all robots and put everything away

Please refer to the User Guide PDF in this repo for more information about how to use this project.

We hope you enjoy using the Interconnected Robot Network!
