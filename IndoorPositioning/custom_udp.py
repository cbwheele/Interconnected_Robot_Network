# You have to run this script as sudo


import socket
import json

readyCheck = [0,0,0]

# How to change for circle only or regular
circleOnlyOrRegular = 2                
# For regular: 2
# For circle only: 4


UDP_IP = "172.20.10.3" # You have to manually set your IP address here unfortunately


# This function reads in the data over udp and if it starts with the "Read" or "In position at" it returns a special flag isRea
#     Otherwise, it will parse the json and return the list of the info of the tags it received
def check_if_ready():   
    
    #print("At top of read_data function")
    try:
        msg1 = data1.recv(4096).decode()
        print("Message on 1: ", msg1)
        if "Ready" in msg1:
            readyCheck[0] = 1
    except BlockingIOError as e:
        #print("Blocking error on message 1: ", e)
        pass
    try:
        msg2 = data2.recv(4096).decode()
        print("Message on 2: ", msg2)
        if "Ready" in msg2:
            readyCheck[1] = 1
    except BlockingIOError as e:
        #print("Blocking error on message 2: ", e)
        pass
    try:
        msg3 = data3.recv(4096).decode()
        print("Message on 3: ", msg3)
        if "Ready" in msg3:
            readyCheck[2] = 1
    except BlockingIOError as e:
        #print("Blocking error on message 3: ", e)
        pass
    
    allReady = 1
    for x in readyCheck:
        if x == 0: 
            allReady = 0
            break
            
    return allReady

    
    
    
    
# Function read_data needs to return a special command to see what it was that was received from any of the 


def read_data():
    returnCommand = 0
    
    lineIsValid = False
    try:
        line = data1.recv(4096).decode()
        lineIsValid = True
    except BlockingIOError as e:
        try:
            line = data2.recv(4096).decode()
            lineIsValid = True
        except BlockingIOError as e:
                try:
                    line = data3.recv(4096).decode()
                    lineIsValid = True
                except BlockingIOError as e:
                    pass
    
    if lineIsValid:
        print("Data inside read_data:", line)
        if "In position at" in line:
            returnCommand = 2
        
        if "Drive me manually" in line:
            returnCommand = 3
        
    else:
        returnCommand = 0 # This line is uncessary but shows that should happen
        
    return returnCommand


UDP_PORT = 80
print("***Local ip:" + str(UDP_IP) + "***")

# Set up the socket that should be /received/ from the ESP32
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind((UDP_IP, UDP_PORT))
print("After bind")  

sock.listen(3)  # The number of connections received
print("About to accept connection")
data1, addr1 = sock.accept()
print("Just accepted one")
data2, addr2 = sock.accept()
print("Just accepted two")
data3, addr3 = sock.accept()
print("Just accepted three")

sock.setblocking(False) # Set the socket to nonblocking!
data1.setblocking(False)
data2.setblocking(False)
data3.setblocking(False)


print("Data1", data1) # This is the data object that will receive the input of the UDP port connection just made
print("Addr1", addr1[0]) # This is the IP address you connected with

print("Data2", data2) # This is the data object that will receive the input of the UDP port connection just made
print("Addr2", addr2[0]) # This is the IP address you connected with

print("Data3", data3) # This is the data object that will receive the input of the UDP port connection just made
print("Addr3", addr3[0]) # This is the IP address you connected with




outgoingSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # This creates an outgoing UDP connection to that IP address so that you can send info back




# State 0:
#   Wait for ready on all three
# State 1:
# .  Special logic to set addr to each of addr1, addr2, addr3, in sequence, and then on it's exectuion, go to 8
# State 2-4
#   Same, and they loop to seven just the same
#   Then loop back to 1
# State 8;
# .   On enter, send "Start" to all three addresses (addr1, addr2, addr3) at the same time!




state = 0
stateOneCheck = 0

while True:
    if state == 0: # Wait for "Ready" from all of the esp32s
        allReady = check_if_ready()
        if allReady == 1:
            state = 1
            
    elif state == 1:   # Figure out which ESP you're communicating, do them all in order, and then go to "Start all of them" movements
        if stateOneCheck == 0:
            addr = addr1[0]
            stateOneCheck = 1
            state = circleOnlyOrRegular
        elif stateOneCheck == 1:
            addr = addr2[0]
            stateOneCheck = 2
            state = circleOnlyOrRegular
        elif stateOneCheck == 2:
            addr = addr3[0]
            stateOneCheck = 3
            state = circleOnlyOrRegular
        elif stateOneCheck == 3:
            state = 5                           # Change this to be the correct version
            
        
    elif state == 2: # Send the message "GX.xx:Y.yy" to ESP32 and get input coordinates
        xCoord = input("Input X coordinate (X.xx): ")
        yCoord = input("Input Y coordinate (Y.yy): ")
        
        # Send "go to location" back to ESP32
        stringToESP32 = "M" + xCoord + ":" + yCoord
        print("About to send this to ESP32: ", stringToESP32)
        outgoingSock.sendto(bytes(stringToESP32, "utf-8"), (addr, UDP_PORT))
        print("Just sent message over UDP out that says to go to the location")
        state = 3
        
    elif state == 3: # Wait for "In position" message from ESP32 or "drive me manually"
        specialCommand = read_data()
        if (specialCommand == 2):
            print("The robot is now in the correct position!!")
            state = 4 # Start back over at the beginning to read in the coordinates
        elif (specialCommand == 3):
            print("Should now control the robot manually")
            state = 7
            
    elif state == 4: # Get input from user for where to turn and how far to turn in a circle and send that info ("C090:040") to the ESP32
        circle_angle = input("Input circle angle (3 digits): ")
        num_ticks = input("Input num ticks (3 digits): ")
        
        # Send "info for circle" back to ESP32
        stringToESP32 = "C" + circle_angle + ":" + num_ticks
        print("About to send this to ESP32: ", stringToESP32)
        outgoingSock.sendto(bytes(stringToESP32, "utf-8"), (addr, UDP_PORT))
        print("Just sent message over UDP out about cricle information")
        state = 1
        pass
    
    elif state == 5: # Wait for user to click enter (or a message and click enter) and then send "S" to the ESP32
       input("Click enter for all of the robots to start the shape: ")
       stringToESP32 = "S"
       print("About to send this to ESP32: ", stringToESP32)
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
        outgoingSock.sendto(bytes(drive_char, "utf-8"), (addr, UDP_PORT))
        if (drive_char == 'p'):
            state = 4
        pass
        



sock.close()