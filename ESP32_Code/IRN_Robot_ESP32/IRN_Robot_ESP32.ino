#include <SPI.h>
#include <DW1000Ranging.h>
#include <WiFi.h>
#include "link.h"
#include <HardwareSerial.h>
#include <cstring>

using namespace std;

// State machine states
#define WAIT_FOR_DWM_TO_BOOTUP      (0)
#define WAIT_FOR_NON_NULL_READINGS  (1)
#define READ_TARGET_COORDINATES     (2)
#define SEND_TARGET_COORD_TO_MSP    (3)
#define WAIT_FOR_ARRIVED_FROM_MSP   (4)
#define WAIT_NEW_POS_READ_SETTLE    (5)
#define CHECK_IF_COORDS_ARE_GOOD    (6)
#define SEND_ARRIVED_TO_COMPUTER    (7)
#define WAIT_TO_START_SENDING_COORDINATES (8)
#define CONTROL_VIA_WASD            (9)
#define RECEIVE_CIRCLE_DIRECTION_AND_DURATION (15)



/* Beginning of variables that need changing for proper script execution */


// Change this variable to control if the ESP32 runs the entire shape sequence (aka gets to initial location as well), or only causes the robot to circle around in the circle:
int circleOnlyOrRegular = READ_TARGET_COORDINATES;      // Modify this value to see what should be done with this script
// For full processing and data control: READ_TARGET_COORDINATES.  
// For moving in a circle only: RECEIVE_CIRCLE_DIRECTION_AND_DURATION


// Modify this macro to see how many times the robot should try to get to the correct starting location autonomously before switching over to manual control
// The reccomended number of times is 2 or 3, as it is not uncommon for the robot to converge to the correct location after 2 or 3 attempts, but usually after three if it hasn't made it yet it's unlikely to
#define NUM_OF_TIMES_TO_AUTO_LOC (2)
// The script has not been tested with this set to a value less than one
// The value must also be a non-negative integer, so 1, 2, 3, etc.  


// Modify these WiFi credentials for how it should connect to the WiFi network
// This must be the same network as the controlling computer is connected to
// Make sure to add the MAC address to the network preferences if that's needed for (especially school) WiFi networks
// If there is no WiFi password necessary, then leave it as a blank string such as ""
const char *ssid = "WiFi Network Name"; // Example "ncsu"
const char *password = "wifipassword";  // Example "" because school WiFi and MAC address required but no password required


// Modify this IP address variable to be the IP address of the controlling computer:
const char *host = "172.20.10.3";  // . YOU MUST MANUALLY SET YOUR IP ADDRESS IN THIS STRING (no extra spaces on either end, and include periods as shown here)
// Otherwise, the robot will not be able to connect up to the computer to be able to receive commands


/* End of variables that need changing for proper script execution */




// Defines for the connections to the DWM100. These are constant on the Makerfabs board
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4
#define PIN_RST 27
#define PIN_IRQ 34

// Defines for the connections to the LEDS. These are constant on our custom PCB
#define RED_LED 33
#define GRN_LED 32


// Other defines
#define MAX_ERROR_FROM_STARTING_COORDINATES 0.3 // This number specifies that the robots must be within 0.2 units in both the x and y directions of the initial shape coordinates to say "ready" to the ground control station


// WiFi variables for sending and receiving data
WiFiClient client;
WiFiUDP Udp;

// Serial veriables:
HardwareSerial SerialPort(2); // This is the serial port to the MSP432. It is defined as serial port 2








char packetBuffer[255]; // Buffer to hold incoming packets over UDP connection

// Variables for interactions with the DWM1000
struct MyLink *uwb_data;
int index_num = 0;
long runtime = 0;
String all_json = "";

// Flags for where coordinates should be sent/printed to
bool shouldSendCoordinatesToMSP432 = false;
bool shouldSendCoordinatesToSerial = false;
bool shouldSendCoordinatesToComputer = false;

// Global variables
Coordinates currentCoordinates; // The current location of the ESP32. This is updated by calling the get_coordinates function
int state = 0; // The state of the large state machine
int timeWhenDWM100Initialized;
int timeWhenReceivedArrivedFromMSP432;

int circle_angle;
int num_ticks;



// Setup function:
//   Initialization
//   
void setup()
{
    delay(500); // Wait half a second so that the modules can boot up

    init_LEDs();// Initialize both LEDs low
    
    // Initialize both the USB-Computer serial port and the MSP432 serial connection
    Serial.begin(115200);
    SerialPort.begin(115200, SERIAL_8N1, 17, 16);

    // Connect to WiFi (code provided by Arduino references)
    Serial.println("WiFi Mac Address:");
    Serial.println(WiFi.macAddress());
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        digitalWrite(RED_LED, !digitalRead(RED_LED));
    }

    // Put the green LED high because the WiFi connection was successful! Also share WiFi connection information through computer serial link
    digitalWrite(RED_LED, LOW);
    digitalWrite(GRN_LED, HIGH);
    Serial.println("Connected");
    Serial.print("IP Address:");
    Serial.println(WiFi.localIP());



    delay(1000); // Delay 1 second to make sure that the WiFi connection is stabilized

    if (client.connect(host, 80))
    {
        Serial.println("Success");
        client.print(String("GET /") + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Connection: close\r\n" +
                     "\r\n");
    }

    Udp.begin(80); // Start a listening client for UDP messages on port 80

    delay(1000); // Delay 1 second to make sure configurations are all good to go before starting DWM1000

    // Initialization for DWM1000 modules
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, DW_CS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);

    // Start the module as a tag
    DW1000Ranging.startAsTag("7D:00:22:EA:82:60:3B:9C", DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
    uwb_data = init_link(); // Create linked lists that are necessary for keeping track of the data from the DWM1000 modules

    timeWhenDWM100Initialized = millis();
}



// The loop function contains all of the code that will be run continually on the ESP32
void loop()
{
    DW1000Ranging.loop(); // This function must be called so that the DWM module is kept up-to-date on it's localization

    // Every 1000ms (every second), send the coordinates on
    if ((millis() - runtime) > 1000) {
        handleCoordinatesEverySecond();
        runtime = millis(); // Reset the runtime so that this function will be caled again in 
    }

    // Run the state machine as well
    loopStateMachine();
}


String readyAtStr;
String thinksAtLocation = "MSP432 thinks it has arrived at the location";
Coordinates firstReadingCoordinates = Coordinates();
int len = 0;
int packetSize = 0;
Coordinates shapeStartingLocationCoordinates = Coordinates();
int timeSentGo;

int numOfTimesAuto = 0;









// State machine
void loopStateMachine() {
    switch (state) {
        // Wait for DWM1000 module to boot up:
        case WAIT_FOR_DWM_TO_BOOTUP:
            if (millis() > timeWhenDWM100Initialized + 10000) {
                state = WAIT_FOR_NON_NULL_READINGS; // After ten seconds, move on to the next state
            }
            break;

        // Wait until reading is not null and then send it to computer
        case WAIT_FOR_NON_NULL_READINGS: 
            firstReadingCoordinates = getCoordinates(uwb_data); // Read in the current location

            // Check if the coordinates are not zero or NAN
            if (!(isnan(firstReadingCoordinates.x) || isnan(firstReadingCoordinates.y) || firstReadingCoordinates.x == 0 || firstReadingCoordinates.y == 0)) {
                
                // If the coordinates are good readings, then go ahead and send "Ready: 1.23:4.23" on to the ground control station
                readyAtStr = "Ready: ";
                readyAtStr += firstReadingCoordinates.x;
                readyAtStr += ":";
                readyAtStr += firstReadingCoordinates.y;
                send_udp(&readyAtStr); // Send it on to the ground control station

                state = circleOnlyOrRegular;
            }
            break;

        // Read in the target coordinates
        case READ_TARGET_COORDINATES:
            // Read incoming message
            packetSize = Udp.parsePacket();

            if (packetSize) {
                Serial.print("Received packet of size ");
                Serial.println(packetSize);
                Serial.print("From ");
                IPAddress remoteIp = Udp.remoteIP();
                Serial.print(remoteIp);
                Serial.print(", port ");
                Serial.println(Udp.remotePort());

                // read the packet into packetBufffer
                len = Udp.read(packetBuffer, 255);
                if (len > 0) {
                    packetBuffer[len] = 0;
                }
                Serial.println("Contents:");
                Serial.println(packetBuffer);

                // packetBuffer should be of the form "M1.23:4.56" where the first (1.23) is the x coordinate and the second (4.56) is the y coordinates
                if (packetBuffer[0] == 'M') {

                    // Convert the received string into the coordinates:
                    shapeStartingLocationCoordinates.x = (packetBuffer[1] - 0x30) + ((float)(packetBuffer[3] - 0x30))/10 + ((float)(packetBuffer[4] - 0x30))/100;
                    shapeStartingLocationCoordinates.y = (packetBuffer[6] - 0x30) + ((float)(packetBuffer[8] - 0x30))/10 + ((float)(packetBuffer[9] - 0x30))/100;

                    Serial.print("Target coordinates converted: ");
                    Serial.print(shapeStartingLocationCoordinates.x);
                    Serial.print("x, and ");
                    Serial.print(shapeStartingLocationCoordinates.y);
                    Serial.println("y! Did it convert correctly?");
                    state = SEND_TARGET_COORD_TO_MSP;
                }
            }
            break;

        // Send these desired coordinates on to the MSP432
        case SEND_TARGET_COORD_TO_MSP:
            {
                // Send "Go to shapeStartingLocationCoordinates to the MSP432"
                SerialPort.print("G\r");
                SerialPort.print(shapeStartingLocationCoordinates.x,2);
                SerialPort.print(" ");
                SerialPort.print(shapeStartingLocationCoordinates.y,2);
                SerialPort.print('\r');

                Serial.println("Just sent target coordinates to MSP432");
                timeSentGo = millis();
                state = WAIT_TO_START_SENDING_COORDINATES;
            }
            break;
        
        case WAIT_TO_START_SENDING_COORDINATES:
            {
                if (millis() > timeSentGo + 100) { // Delay by .1 seconds to start sending current coordinates
                    shouldSendCoordinatesToMSP432 = true; // Start continually sending "current location" coordinates on to the MSP432, which will continue every second
                    state = WAIT_FOR_ARRIVED_FROM_MSP;
                }
                break;
            }

        // Wait for the message from the MSP432 "A" which means it thinks it has arrived
        case WAIT_FOR_ARRIVED_FROM_MSP:
            {
                // Wait for "A" from the MSP432
                if (SerialPort.available() > 0) {
                    if (SerialPort.read() == 'A') {
                        send_udp(&thinksAtLocation);
                        Serial.println("Received 'A' from the MSP432!");
                        shouldSendCoordinatesToMSP432 = false;
                        timeWhenReceivedArrivedFromMSP432 = millis();
                        state = WAIT_NEW_POS_READ_SETTLE;
                    }
                }
            }
            break;
        
        case WAIT_NEW_POS_READ_SETTLE:
            digitalWrite(GRN_LED, LOW);
            digitalWrite(RED_LED, LOW);
            if (millis() > timeWhenReceivedArrivedFromMSP432 + 5000) {
                state = CHECK_IF_COORDS_ARE_GOOD; // After five seconds, move on to the next state
                Serial.print("Has let coordinates settle and is moving on now.");
            }
            break;

        // Check to see if the coordinates are good
        case CHECK_IF_COORDS_ARE_GOOD:
            {
                // Check if the coordinates are good or not. This means if their errors are within the margin set in the macros at the top of the file
                currentCoordinates = getCoordinates(uwb_data);
                bool xCoordGood = (currentCoordinates.x - shapeStartingLocationCoordinates.x) <= MAX_ERROR_FROM_STARTING_COORDINATES && (currentCoordinates.x - shapeStartingLocationCoordinates.x) > -MAX_ERROR_FROM_STARTING_COORDINATES;
                bool yCoordGood = (currentCoordinates.y - shapeStartingLocationCoordinates.y) <= MAX_ERROR_FROM_STARTING_COORDINATES && (currentCoordinates.y - shapeStartingLocationCoordinates.y) > -MAX_ERROR_FROM_STARTING_COORDINATES;
                Serial.println("About to check if x and y coordinates are good");

                // Send what the coordinates are to the computer
                // Say good to go to host computer!
                readyAtStr = "About to check if these coordinates are good: ";
                readyAtStr += currentCoordinates.x;
                readyAtStr += ":";
                readyAtStr += currentCoordinates.y;                
                send_udp(&readyAtStr);
                

                
                if (xCoordGood && yCoordGood) {
                    // Move on to the next state because the coordinates are good
                    state = SEND_ARRIVED_TO_COMPUTER;
                    Serial.println("Coordinates were good!");
                    digitalWrite(GRN_LED, HIGH);
                    numOfTimesAuto = 0; // Reset back to zero if we started another trial over again
                } else {
                    if (++numOfTimesAuto >= NUM_OF_TIMES_TO_AUTO_LOC) {
                        state = CONTROL_VIA_WASD;
                        digitalWrite(RED_LED, HIGH);
                        digitalWrite(GRN_LED, HIGH);
                        // Say not arrived to host computer
                        readyAtStr = "Drive me manually";             
                        send_udp(&readyAtStr);
                        SerialPort.print("M\r");
                    } else {
                        state = SEND_TARGET_COORD_TO_MSP;
                        Serial.println("Coordinates were bad, so re-sending new coordinates");
                        digitalWrite(RED_LED, HIGH);
                    }
                }
            }
            break;
        case SEND_ARRIVED_TO_COMPUTER:
            {
                // Say good to go to host computer!
                readyAtStr = "In position at: ";
                readyAtStr += currentCoordinates.x;
                readyAtStr += ":";
                readyAtStr += currentCoordinates.y;                
                send_udp(&readyAtStr);
                state = RECEIVE_CIRCLE_DIRECTION_AND_DURATION;
            }
            break;
        case CONTROL_VIA_WASD:
            {
                // Control via the WASD received, and then go on to state RECEIVE_CIRCLE_DIRECTION_AND_DURATION
                packetSize = Udp.parsePacket();

                if (packetSize) {
                    // read the packet into packetBufffer
                    len = Udp.read(packetBuffer, 255);
                    if (len > 0) {
                        packetBuffer[len] = 0;
                    }
                    Serial.println("Contents:");
                    Serial.println(packetBuffer);

                    // packetBuffer should be of the form "w" "a" "s" "d" or " ". If it is "p" for "in position" then tell MSP432 and skip states
                    if (packetBuffer[0] == 'p') {
                        state = RECEIVE_CIRCLE_DIRECTION_AND_DURATION;
                        digitalWrite(RED_LED, LOW);
                    }

                    // Send packetBuffer[0] over UART
                    Serial.print("Just received the character: ");
                    Serial.println(packetBuffer[0]);
                    SerialPort.print(packetBuffer[0]);
                    SerialPort.print('\r');
                }
            }
            break;
        case 1000:
            {
                Serial.println("ESP32 code is now about to loop back to get another set of coordinates");
                state = WAIT_FOR_NON_NULL_READINGS;
            }
            break;
        case RECEIVE_CIRCLE_DIRECTION_AND_DURATION:
            // Read incoming message
            packetSize = Udp.parsePacket();

            if (packetSize) {

                // read the packet into packetBufffer
                len = Udp.read(packetBuffer, 255);
                if (len > 0) {
                    packetBuffer[len] = 0;
                }
                Serial.println("Contents:");
                Serial.println(packetBuffer);

                // packetBuffer should be of the form "C270:050"
                if (packetBuffer[0] == 'C') {

                    // Convert the received string into the coordinates:
                    circle_angle = (packetBuffer[1] - 0x30)*100 + ((packetBuffer[2] - 0x30))*10 + ((packetBuffer[3] - 0x30));
                    num_ticks = (packetBuffer[5] - 0x30)*100 + ((packetBuffer[6] - 0x30))*10 + ((packetBuffer[7] - 0x30));

                    Serial.print("Information converted: ");
                    Serial.print(circle_angle);
                    Serial.print("for degrees to turn, and ");
                    Serial.print(num_ticks);
                    Serial.println("for how far to go around circle.");
                    state = 16;
                }
            }
            break;
        case 16:
            {
                // Send "Go to shapeStartingLocationCoordinates to the MSP432"
                SerialPort.print("C\r");
                SerialPort.print(circle_angle);
                SerialPort.print(" ");
                SerialPort.print(num_ticks);
                SerialPort.print('\r');

                Serial.println("Just sent target coordinates to MSP432");
                timeSentGo = millis();
                state = 17;
            }
            break;
        case 17:
            // Read incoming message
            packetSize = Udp.parsePacket();

            if (packetSize) {

                // read the packet into packetBufffer
                len = Udp.read(packetBuffer, 255);
                if (len > 0) {
                    packetBuffer[len] = 0;
                }
                Serial.println("Contents:");
                Serial.println(packetBuffer);

                // packetBuffer should be of the form "C270:050"
                if (packetBuffer[0] == 'S') {

                    SerialPort.print("S\r");
                    state = 18;
                }
            }
            break;
        case 18:
            break;


        default:
            {}
            break;
    }

}

void newRange()
{
    char c[30];

    //Serial.print("from: ");
    //Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
    //Serial.print("\t Range: ");
    //Serial.print(DW1000Ranging.getDistantDevice()->getRange());
    //Serial.print(" m");
    //Serial.print("\t RX power: ");
    //Serial.print(DW1000Ranging.getDistantDevice()->getRXPower());
    //Serial.println(" dBm");
    fresh_link(uwb_data, DW1000Ranging.getDistantDevice()->getShortAddress(), DW1000Ranging.getDistantDevice()->getRange(), DW1000Ranging.getDistantDevice()->getRXPower());
}

void newDevice(DW1000Device *device)
{
    Serial.print("ranging init; 1 device added ! -> ");
    Serial.print(" short:");
    Serial.println(device->getShortAddress(), HEX);

    add_link(uwb_data, device->getShortAddress());
}

void inactiveDevice(DW1000Device *device)
{
    Serial.print("delete inactive device: ");
    Serial.println(device->getShortAddress(), HEX);

    delete_link(uwb_data, device->getShortAddress());
}

void send_udp(String *msg_json)
{
    if (client.connected())
    {
        client.print(*msg_json);
        Serial.println("Message sent to UDP host");
    }
}

void init_LEDs(void) {
    pinMode(RED_LED, OUTPUT);
    digitalWrite(RED_LED, LOW);
    pinMode(GRN_LED, OUTPUT);
    digitalWrite(GRN_LED, LOW);
}

void handleCoordinatesEverySecond() {
    currentCoordinates = getCoordinates(uwb_data); // This is a custom function created to get the 2D coordinates based on the anchors being a specified distance apart
        
    if (shouldSendCoordinatesToComputer) {
        // This is code that is used to make sure that the json is made based on the uwb_data (aka only distances from each anchor), and sent on to the computer
        make_link_json(uwb_data, &all_json);
        send_udp(&all_json);
    }

    if (shouldSendCoordinatesToSerial) {
        Serial.print("From Current Coordinates: ");
        Serial.print(currentCoordinates.x);
        Serial.print(", ");
        Serial.println(currentCoordinates.y);
    }

    if (shouldSendCoordinatesToMSP432) {
        if (!(isnan(currentCoordinates.x) || isnan(currentCoordinates.y))) {
        // Only transmit to the MSP432 if the data is good
        SerialPort.print(currentCoordinates.x,4);
        SerialPort.print(" ");
        SerialPort.print(currentCoordinates.y,4);
        SerialPort.print('\r');
        }
    }
}
