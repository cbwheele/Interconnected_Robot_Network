import socket
import time
import json



def read_data():
    isReady = False
    line = data.recv(1024).decode('UTF-8')

    uwb_list = []

    if line.startswith('Ready'):
        print("Just checked if it was ready and was correctly Ready")
        isReady = True
        
    try:
        uwb_data = json.loads(line)
        print("About to print out data")
        print(uwb_data)
        print("About to check if it is ready")
        

        uwb_list = uwb_data["links"]
        for uwb_archor in uwb_list:
            print(uwb_archor)
        
    except:
        print(line)
    print("")

    return uwb_list, isReady



UDP_IP = "10.154.51.2"
print("***Local ip:" + str(UDP_IP) + "***")
UDP_PORT = 80
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print("After socket")
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind((UDP_IP, UDP_PORT))

print("After bind")
sock.listen(1)  # The number of connections received
print("About to accept connection")
data, addr = sock.accept()


print("Data", data)
print("Addr", addr[0])

outgoingSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP



state = 0
timeLeftStateZero = time.time()

while True:
    if state == 0:
        _unused, isReady = read_data()
        if (isReady):
            print("Just received ready from the device!!")
            timeLeftStateZero = time.time()
            state = 1
    elif state == 1:
        if time.time() > timeLeftStateZero + 1:
            state = 2
    elif state == 2:
        # Send "go to location" back to ESP32
        print("One second has passed")
        outgoingSock.sendto(bytes("GO TO THE LOCATION NOW", "utf-8"), (addr[0], UDP_PORT))
        print("Just sent message over UDP out that says to go to the location")
        state = 3
        pass
    elif state == 3:
        pass


sock.close()