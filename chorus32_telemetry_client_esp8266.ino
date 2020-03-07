//#include <WiFi.h>
//#include <esp_wifi.h>
#include <ESP8266WiFi.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#define WIFI_AP_NAME "Chorus32 LapTimer"

#define UART_TX 15
#define UART_RX 4


#define PROXY_PREFIX 'P' // used to configure any proxy (i.e. this device ;))
#define PROXY_CONNECTION_STATUS 'c'
#define PROXY_WIFI_STATUS 'w'

WiFiClient client;

#include <SPort.h>                  //Include the SPort library

SPortHub hub(0x12, 4);              //Hardware ID 0x12, Software serial pin 3
SimpleSPortSensor lapsensor0(0x1AEB);   //Sensor with ID 0x5900
SimpleSPortSensor lapsensor1(0x1AEC);   //Sensor with ID 0x5900
SimpleSPortSensor lapsensor2(0x1AED);   //Sensor with ID 0x5900
SimpleSPortSensor lapsensor3(0x1AEE);   //Sensor with ID 0x5900
SimpleSPortSensor sensor(0x5900);    


#define MAX_BUF 1500
char buf[MAX_BUF];
uint32_t buf_pos = 0;
int intlap = 0;
void chorus_connect() {
        if (WiFi.status() == WL_CONNECTED) {
    if(client.connect("192.168.4.1", 9000)) {
      Serial.println("Connected to chorus via tcp");
    }
  }
}

void setup() {
  //hub.commandId = 0x1B;                               //Listen to data send to thist physical ID
  //hub.commandReceived = handleCommand;   
  hub.registerSensor(lapsensor0);       //Add sensor to the hub
  hub.registerSensor(lapsensor1);       //Add sensor to the hub
  hub.registerSensor(lapsensor2);       //Add sensor to the hub
  hub.registerSensor(lapsensor3);       //Add sensor to the hub
  hub.registerSensor(sensor);       //Add sensor to the hub
  
  hub.begin();  
  Serial.begin(115200);
//  Serial1.begin(115200, SERIAL_8N1, UART_RX, UART_TX, true);
  WiFi.begin(WIFI_AP_NAME);
  delay(500);

  memset(buf, 0, 1500 * sizeof(char));
}

void loop() {
    sensor.value = 4444;              //Set the sensor value
  hub.handle();    
 
  while(client.connected() && client.available()) {
    // Doesn't include the \n itself
    String line = client.readStringUntil('\n');

    // Only forward specific messages for now
    if(line[2] == 'L') {
      Serial1.println(line);
      String hextime = line.substring(4, 13);
      intlap = hexToDec(hextime);
      lapsensor0.value=intlap;
      //Serial.printf("Forwarding to taranis: %s %s \n", line.c_str(), intlap.c_str());
      Serial.println(intlap);
    }
  }

  while(Serial1.available()) {
    if(buf_pos >= MAX_BUF -1 ) {
      buf_pos = 0; // clear buffer when full
      break;
    }
    buf[buf_pos++] = Serial1.read();
    if(buf[buf_pos - 1] == '\n') {
      // catch proxy command
      if(buf[0] == 'P') {
        switch(buf[3]) {
          case PROXY_WIFI_STATUS:
            Serial1.printf("%cS*%c%1x\n", PROXY_PREFIX, PROXY_WIFI_STATUS, WiFi.status());
            break;
          case PROXY_CONNECTION_STATUS:
            Serial1.printf("%cS*%c%1x\n", PROXY_PREFIX, PROXY_CONNECTION_STATUS, client.connected());
            break;
        }
      }
      else if((buf[0] == 'E' && (buf[1] == 'S' || buf[1] == 'R')) || buf[0] == 'S' || buf[0] == 'R') {
        Serial.print("Forwarding to chorus: ");
        Serial.write((uint8_t*)buf, buf_pos);
        client.write(buf, buf_pos);
      }
      buf_pos = 0;
    }
  }

  if(!client.connected()) {
    delay(1000);
    Serial.println("Lost tcp connection! Trying to reconnect");
    chorus_connect();
  }
}

void handleCommand(int prim, int applicationId, int value) {
  hub.sendCommand(0x19, applicationId, value + 1);    //Send a command back to lua, with 0x32 as reply and the value + 1
  Serial.print(applicationId);
  Serial.print("------");
  Serial.println(value);
}

unsigned int hexToDec(String hexString) {

  unsigned int decValue = 0;
  int nextInt;

  for (int i = 0; i < hexString.length(); i++) {

    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);

    decValue = (decValue * 16) + nextInt;
  }

  return decValue;
}
