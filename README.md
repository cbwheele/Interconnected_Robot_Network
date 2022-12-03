# Interconnected_Robot_Network
The Interconnected Robot Network (abbreviated IRN) is a Senior Design project for four students in Electrical and Computer Engineering at NC State University that was completed in the Spring and Fall semesters of 2022.

Team Members: Brandon O, Caleb Wheeler, Clay Scarboro, and Tanner Boylan

This repo contains all of the code that relates in any way to the project.

We had three main sections of code:
- Code on the MSP432 written in C to control the robot
- Code on the ESP32 written in C++ to talk via UART to the MSP432, via WiFi to the computer, and via SPI to the DWM1000 module to get coordinates
- Code on the computer terminal written in Python to control the entire operation and communicate via WiFi with the ESP32s

There are three different folders inside this repo for the code for each of these sections, and each folder contains a README file that outlines what the code does.
