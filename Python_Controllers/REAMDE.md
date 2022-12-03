# Python Controllers

This folder contains two scripts. The first is `robot_controller.py` and it is the main script that controlls the robots to move in the shapes and move to their initial startingi positions. The second is `uwb_position_display.py` and it is used to visualize the position of single tag in the operating theatre.


## Requirements
- Python 3 Installed on your computer
- Sudo access to the computer (User has system root access and password entry may be required)
- WiFi Connection
- IP Address


## Robot Controller Python Script

### Run the Script:
1. The first step is to go into the file and change two configuration secitons:

    a. The first is that you find your IP address (Network Settings on your computer > Find IP Address), and edit the variable `IP_ADDRESS` at the top of the file with your IP address. Otherwise, you will get an error "OSError: [Errno 49] Can't assign requested address" when you run the script

    b. Change the variable named `circleOnlyOrRegular` depending on what you want the script to do. If you want to have the script control the robots for the entire setup and shape sequence, change the variable to "2", but if you only want the robots to circle around in the circle only, change the variable to "4"

2. Now, open up your favorite terminal command and run your Python executor on the script with sudo (root system) permissions. On Mac, this looks like `sudo python3 robot_control.py`, but on Windows it can vary by Python version and terminal used.


### Using the script: 
- The script will walk you through how to use it
- The first step is to enter your password if prompted
- Make sure the script says "About to accept connections...", as that means it is correclty waiting for incoming connection requests from the robots
- Turn on each of the robots, one at a time if the order matters. The order the robots connect is the order they are received.
- Make sure the script shows "connected to first", "second", "third", robots in order
- Then follow instructions on Python script for how to control robots.


### Enter X and Y coordinates:
- Enter the numbers with one units place and two decimal places ALWAYS, so the format has to be: 1.23 or 2.25, etc. Always make sure this is exact

### Enter Turning angle:
- The turning angle is relative to polar coordinates with 0 degrees facing "north" aka towards the tags
- Enter this number as a three digit number, so 000 -> 360 degrees, with a leading zero (or two) always. (e.g. 045 or 005 for 45 degrees and 2 degrees respectively)

### Enter Circle Ticks:
- The circle ticks is how many circle rotations the robots will make after you press enter (all will go at the same time)
- 40 ticks is one circle rotation, so enter "040" for a full circle rotation
- Always enter this number as a three digit number with leading digits as necessary





## UWB Position Display

### Run the Script:
1. The first is that you find your IP address (Network Settings on your computer > Find IP Address), and edit the variable `IP_ADDRESS` at the top of the file with your IP address. Otherwise, you will get an error "OSError: [Errno 49] Can't assign requested address" when you run the script

2. Now, open up your favorite terminal command and run your Python executor on the script with sudo (root system) permissions. On Mac, this looks like `sudo python3 uwb_position_display.py`, but on Windows it can vary by Python version and terminal used.


### Using the script
- If the tag is able to get readings from the anchors and the IP address on the tag is correct for your laptop, you should see a 2-D map of where the tag is in relation to the anchors
- Move the tag around to see the location update in real-time on the turtle graphics display


## After Scripts Execution
- Press Control-C (^C) to stop execution of the python script at any time
- This is also how you reboot and restart the python script if anything went wrong
