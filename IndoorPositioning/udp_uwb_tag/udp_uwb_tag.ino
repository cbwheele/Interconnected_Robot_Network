#include <SPI.h>
#include <DW1000Ranging.h>
#include <WiFi.h>
#include "link.h"
#include <HardwareSerial.h>



#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4
#define PIN_RST 27
#define PIN_IRQ 34

#define RED_LED 33
#define GRN_LED 32

const char *ssid = "ncsu";
const char *password = "";
const char *host = "10.154.27.73";
WiFiClient client;

struct MyLink *uwb_data;
int index_num = 0;
long runtime = 0;
String all_json = "";

HardwareSerial SerialPort(2); // use UART2

void setup()
{
    delay(500);
    pinMode(RED_LED, OUTPUT);
    digitalWrite(RED_LED, LOW);
    pinMode(GRN_LED, OUTPUT);
    digitalWrite(GRN_LED, LOW);
  
    Serial.begin(115200);
    SerialPort.begin(115200, SERIAL_8N1, 17, 16);

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
    digitalWrite(RED_LED, LOW);
    digitalWrite(GRN_LED, HIGH);
    Serial.println("Connected");
    
    Serial.print("IP Address:");
    Serial.println(WiFi.localIP());

    delay(1000);

    if (client.connect(host, 80))
    {
        Serial.println("Success");
        client.print(String("GET /") + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Connection: close\r\n" +
                     "\r\n");
    }

    delay(1000);

    //init the configuration
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, DW_CS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);

    //we start the module as a tag
    DW1000Ranging.startAsTag("7D:00:22:EA:82:60:3B:9C", DW1000.MODE_LONGDATA_RANGE_LOWPOWER);

    uwb_data = init_link();

    
    
}

void loop()
{
    DW1000Ranging.loop();
    if ((millis() - runtime) > 1000)
    {
        Coordinates currentCoordinates = getCoordinates(uwb_data);
        make_link_json(uwb_data, &all_json);
        send_udp(&all_json);
        runtime = millis();

        Serial.print("From Current Coordinates: ");
        Serial.print(currentCoordinates.x);
        Serial.print(", ");
        Serial.println(currentCoordinates.y);

        if (!(isnan(currentCoordinates.x) || isnan(currentCoordinates.y))) {
          // Only transmit to the MSP432 if the data is good
          SerialPort.print(currentCoordinates.x,4);
          SerialPort.print(" ");
          SerialPort.print(currentCoordinates.y,4);
          SerialPort.print('\r');
        }

        
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
        Serial.println("UDP send");
    }
}
