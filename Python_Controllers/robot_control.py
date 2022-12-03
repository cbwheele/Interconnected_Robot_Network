# To run this script:
# Mac: sudo python3 robot_control.py
# Windows: Find how to run a script in sudo, and find out what Python version you have installed



# Modify this IP address variable to be the IP address of your computer
IP_ADDRESS = "10.137.95.115"  # YOU MUST MANUALLY SET YOUR IP ADDRESS IN THIS STRING (no extra spaces on either end, and include periods as shown here)
# Otherwise, you will get the error "OSError: [Errno 49] Can't assign requested address" and the script will fail



# Change this variable to control if the script runs the entire shape sequence, or only causes the robots to circle around in the circle:
circleOnlyOrRegular = 2                
# For regular control, change the variable above to "2"
# For circling only control, change the variable to "4"



# Imports for UDP connections
import socket

# Variable to see if all robots are ready to move
readyCheck = [0,0,0]



# This function reads in the data over udp and if it contains "Ready" it will mark which connection said it was ready and store it in the variable above
#   Only once all three devices have said they are ready will the functio return 1
def check_if_ready():   
    
    # These are wrapped in try-catch blocks so that if there is no data to read, it won't error out because the port is non-blocking
    try:
        msg1 = data1.recv(4096).decode()    # Receive the data over the first connection
        print("Message on 1: ", msg1)       # Print that connection's data it sent so the user can see it
        
        # Set the variable inside ready for the first tag
        if "Ready" in msg1:
            readyCheck[0] = 1
            
    except BlockingIOError as e:
        # Do nothing if there was a Blocking error, as a blocking error means there was nothing to read in at that time
        pass
    
    # Repeat the same process for the other two connectiosn msg2 and msg3
    try:
        msg2 = data2.recv(4096).decode()
        print("Message on 2: ", msg2)
        if "Ready" in msg2:
            readyCheck[1] = 1
    except BlockingIOError as e:
        pass
    try:
        msg3 = data3.recv(4096).decode()
        print("Message on 3: ", msg3)
        if "Ready" in msg3:
            readyCheck[2] = 1
    except BlockingIOError as e:
        pass
    
    # Check to see if all of the tags (all three) are initialized and ready
    allReady = 1
    for x in readyCheck:
        if x == 0: 
            allReady = 0
            break
            
    return allReady # Return whether the tags are ready yet

    
    
    
    
# Function read_data needs to return a special command to see what it was that was received from any of the 
#   It will read in the data from all three connections adn return 2 if the data was "In position at" and return 3 if the data was "Drive me manually"
def read_data():
    returnCommand = 0
    
    lineIsValid = False # Variable to see if any data was received and should therefore be processed and printed
    
    # Nested try-catch blocks to read in the data from the non-blocking port where there will be three data connectios: msg1, msg2, and msg3
    try:
        line = data1.recv(4096).decode()    # Receive and decode the data
        lineIsValid = True                  # If there was data, set this flag high to process
    except BlockingIOError as e:
        # Only if no data was read in from previous try, look for data on next socket and read in the same
        try:
            line = data2.recv(4096).decode()
            lineIsValid = True
        except BlockingIOError as e:
                try:
                    line = data3.recv(4096).decode()
                    lineIsValid = True
                except BlockingIOError as e:
                    pass
    
    # Process the data to see if it was a special command
    if lineIsValid:
        print("Data inside read_data:", line) # Print out the data regardless
        if "In position at" in line:
            returnCommand = 2
        
        if "Drive me manually" in line:
            returnCommand = 3
    
    # If the line was not valid, then no data was received, so return 0 to say nothing happened
    else:
        returnCommand = 0 # This line is uncessary but shows that should happen
        
    return returnCommand


UDP_PORT = 80
print("***Local ip:" + str(IP_ADDRESS) + "***")

# Set up the socket that should be /received/ from the ESP32
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind((IP_ADDRESS, UDP_PORT))
print("After bind")  

sock.listen(3)  # Listen for three connectiosns into the UDP port

# Connect to all three of these connections and store the info in dataX and addrX respectively
print("About to accept connections...")
data1, addr1 = sock.accept()
print("Just accepted first connection.")
data2, addr2 = sock.accept()
print("Just accepted second connection.")
data3, addr3 = sock.accept()
print("Just accepted third connection.")

# Set the socket and all data inputs to nonblocking. This allows us to always watch for data on any of the three of them
#    But it does require the BlockingError try-catch blocks to be serviced if no data was received
sock.setblocking(False) 
data1.setblocking(False)
data2.setblocking(False)
data3.setblocking(False)


# Print out the information of the IP addresses just recieved connections from 
print("Data1", data1) # This is the data object that will receive the input of the UDP port connection just made
print("Addr1", addr1[0]) # This is the IP address you connected with
print("Data2", data2) 
print("Addr2", addr2[0]) 
print("Data3", data3) 
print("Addr3", addr3[0]) 



outgoingSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # This creates an outgoing UDP socket so that you can send data to any of these IP addresses




# State 0:
#   Wait for ready on all three
# State 1:
# .  Special logic to set addr to each of addr1, addr2, addr3, in sequence, and then on it's exectuion, go to 8
# State 2-4
#   Same, and they loop to seven just the same
#   Then loop back to 1
# State 8;
# .   On enter, send "Start" to all three addresses (addr1, addr2, addr3) at the same time!




state = 0           # Set up state machine
stateOneCheck = 0   # This variable is essentially the number of times that the state machine has been in state one

# Loop forever
while True:
    if state == 0: # Wait for "Ready" from all of the esp32s
        allReady = check_if_ready()
        if allReady == 1:
            state = 1
            
    elif state == 1:   # Figure out which ESP you're communicating with, do them all in order, and then go to "Start all of them" movements state
        if stateOneCheck == 0:
            addr = addr1[0]                 # The first time through, set addr to addr1 because communicating with first robot
            stateOneCheck = 1               # Set this variable so that next step time to this case goes to next case
            state = circleOnlyOrRegular     # This is 2 if regular (entire sequence) control, and 4 if circle data only
        elif stateOneCheck == 1:
            addr = addr2[0]
            stateOneCheck = 2
            state = circleOnlyOrRegular
        elif stateOneCheck == 2:
            addr = addr3[0]
            stateOneCheck = 3
            state = circleOnlyOrRegular
        elif stateOneCheck == 3:
            state = 5                           # Go to the state that is waiting to send "Start" to all robots at the same time
            
        
    elif state == 2: # Get input coordinates and send the message "GX.xx:Y.yy" to ESP32 
        xCoord = input("Input X coordinate (X.XX): ")
        yCoord = input("Input Y coordinate (Y.YY): ")
        
        # Send "go to location" back to ESP32 in form "M1.23:2.34" - has to be this form exactly
        stringToESP32 = "M" + xCoord + ":" + yCoord
        print("About to send this to ESP32: ", stringToESP32)
        outgoingSock.sendto(bytes(stringToESP32, "utf-8"), (addr, UDP_PORT))
        print("Just sent message over UDP out that says to go to the location")
        state = 3
        
    elif state == 3: # Wait for "In position" message from ESP32 or "drive me manually"
        specialCommand = read_data()
        if (specialCommand == 2):
            print("The robot is now in the correct position!")
            state = 4 # Go get information from the user about how to turn in preparation for the circle
        elif (specialCommand == 3):
            print("Should now control the robot manually")
            state = 7   # Go to control the robot manually using WASD keys
            
    elif state == 4: # Get input from user for where to turn and how far to turn in a circle and send that info ("C090:040") to the ESP32
        circle_angle = input("Input circle angle (3 digits): ")
        num_ticks = input("Input num ticks (3 digits): ")
        
        # Send "info for circle" back to ESP32 - has to be in form "C270:040"
        stringToESP32 = "C" + circle_angle + ":" + num_ticks
        print("About to send this to ESP32: ", stringToESP32)
        outgoingSock.sendto(bytes(stringToESP32, "utf-8"), (addr, UDP_PORT))
        print("Just sent message over UDP out about cricle information")
        state = 1 # Start back over with next robot
        pass
    
    elif state == 5: # Wait for user to click enter (or a message and click enter) and then send "S" to the ESP32
       input("Click enter for all of the robots to start the shape: ") # This input command will wait for "Enter" or "retun" on the keyboard
       stringToESP32 = "S"
       print("About to send this to ESP32: ", stringToESP32)
       # Send the "S" (aka start) string to all ESP32s at almost the same time
       outgoingSock.sendto(bytes(stringToESP32, "utf-8"), (addr1[0], UDP_PORT))
       outgoingSock.sendto(bytes(stringToESP32, "utf-8"), (addr2[0], UDP_PORT))
       outgoingSock.sendto(bytes(stringToESP32, "utf-8"), (addr3[0], UDP_PORT))
       print("Sent S to all ESP32s")
       print("\nThank you for using the IRN Swarm demo!!")
       state = 6
       pass
   
    elif state == 6: 
        
        # Loop forever because we don't want to do anything else.
        pass
    
    elif state == 7: # Drive the robot manually
        drive_char = input("w, a, s, d, ' ', or p: ")
        # Send the inputted character on to the ESP32
        outgoingSock.sendto(bytes(drive_char, "utf-8"), (addr, UDP_PORT))
        if (drive_char == 'p'):
            state = 4 # Go back to get circle information
        pass
        


# These are never called because of infinite loop and control-C automatically closes them, but is good practice to always close sockets created
sock.close() 
outgoingSock.close() 