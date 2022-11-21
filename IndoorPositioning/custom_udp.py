# You have to run this script as sudo


import socket
import json


UDP_IP = "10.154.40.99" # You have to manually set your IP address here unfortunately


# This function reads in the data over udp and if it starts with the "Read" or "In position at" it returns a special flag isRea
#     Otherwise, it will parse the json and return the list of the info of the tags it received
def read_data():   
    isReady = False
    line = data.recv(1024).decode('UTF-8')

    uwb_list = []

    if line.startswith('Ready'):
        isReady = True
    
    if line.startswith('In position at'):
        isReady = True
        
    try:
        uwb_data = json.loads(line)
        print(uwb_data)
        

        uwb_list = uwb_data["links"]
        for uwb_archor in uwb_list:
            print(uwb_archor)
        
    except:
        print(line)
    print("")

    return uwb_list, isReady


UDP_PORT = 80
print("***Local ip:" + str(UDP_IP) + "***")

# Set up the socket that should be /received/ from the ESP32
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind((UDP_IP, UDP_PORT))
print("After bind")
sock.listen(1)  # The number of connections received
print("About to accept connection")
data, addr = sock.accept()


print("Data", data) # This is the data object that will receive the input of the UDP port connection just made
print("Addr", addr[0]) # This is the IP address you connected with

outgoingSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # This creates an outgoing UDP connection to that IP address so that you can send info back


state = 0

while True:
    if state == 0: # Wait for "Ready" from the ESP32
        _unused, isReady = read_data()
        if (isReady):
            print("Just received ready from the device!!")
            state = 1
            
    elif state == 1: # Get the input for the x and y coordinates from user
        xCoord = input("Input X coordinate (X.xx): ")
        yCoord = input("Input Y coordinate (Y.yy): ")
        state = 2
        
    elif state == 2: # Send the message "GX.xx:Y.yy" to ESP32
        # Send "go to location" back to ESP32
        stringToESP32 = "M" + xCoord + ":" + yCoord
        print("About to send this to ESP32: ", stringToESP32)
        outgoingSock.sendto(bytes(stringToESP32, "utf-8"), (addr[0], UDP_PORT))
        print("Just sent message over UDP out that says to go to the location")
        state = 3
        
    elif state == 3: # Wait for "In position" message from ESP32
        _unused, isReady = read_data()
        if (isReady):
            print("The robot is now in the correct position!!")
            state = 4 # Start back over at the beginning to read in the coordinates
            
    elif state == 4: # Get input from user for where to turn and how far to turn in a circle and send that info ("C090:040") to the ESP32
        circle_angle = input("Input circle angle (3 digits): ")
        num_ticks = input("Input num ticks (3 digits): ")
        
        # Send "info for circle" back to ESP32
        stringToESP32 = "C" + circle_angle + ":" + num_ticks
        print("About to send this to ESP32: ", stringToESP32)
        outgoingSock.sendto(bytes(stringToESP32, "utf-8"), (addr[0], UDP_PORT))
        print("Just sent message over UDP out about cricle information")
        state = 5
        pass
    
    elif state == 5: # Wait for user to click enter (or a message and click enter) and then send "S" to the ESP32
        input("Click enter to go forward: ")
        print("Will send S")
        stringToESP32 = "S"
        print("About to send this to ESP32: ", stringToESP32)
        outgoingSock.sendto(bytes(stringToESP32, "utf-8"), (addr[0], UDP_PORT))
        print("Send S to ESP32")
        print("\nThank you for using the IRN Swarm demo!!")
        state = 6
        pass
    elif state == 6:
        
        # Loop forever because we don't want to do anything else.
        pass



sock.close()