/***************************************************************************************
 * GROUP: BrainFlow
 * SUB: CPE122 
 * PROJECT: SmartPlug project
 ***************************************************************************************/
 
#include "secrets.h"  // Contains all the credentials

// Important Libraries
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <WiFi.h>

#include "Alex_connect.h"
#include "Current.h"
#include "SinricPro.h"
#include "SinricProSwitch.h"

// Topics of MQTT
#define AWS_IOT_PUBLISH_TOPIC1 "esp32/pub1"
#define AWS_IOT_PUBLISH_TOPIC2 "esp32/pub2"

#define AWS_IOT_SUBSCRIBE_TOPIC1 "esp32/relay1"
#define AWS_IOT_SUBSCRIBE_TOPIC2 "esp32/relay2"


// call Wifi client
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);


// set up Button & bool function default
bool myPowerStateN1 = false;
unsigned long lastBtnPressN1 = 0;

bool myPowerStateN2 = false;
unsigned long lastBtnPressN2 = 0;

// general set up
bool status;
int i;

// online button function --> sinric (on/off)
bool onPowerStateN1(const String &deviceId, bool &state)
{
  Serial.printf("Device %s turned %s (via SinricPro) \r\n", deviceId.c_str(), state ? "on" : "off");
  myPowerStateN1 = state;
  digitalWrite(NODE1, myPowerStateN1 ? LOW : HIGH);
  
  return true; // request handled properly
}

bool onPowerStateN2(const String &deviceId, bool &state)
{
  Serial.printf("Device %s turned %s (via SinricPro) \r\n", deviceId.c_str(), state ? "on" : "off");
  myPowerStateN2 = state;
  digitalWrite(NODE2, myPowerStateN2 ? LOW : HIGH);
  
  return true; // request handled properly
}

// manual button
void handleButtonPress() {
  unsigned long actualMillis = millis(); 
  if (digitalRead(BUTTON_PIN) == LOW && actualMillis - lastBtnPressN1 > 1000)  { 
    if (myPowerStateN1) {
      myPowerStateN1 = false;
    } else {
      myPowerStateN1 = true;
    }
    digitalWrite(NODE1, myPowerStateN1?LOW:HIGH);

    
    SinricProSwitch& mySwitch = SinricPro[SWITCH_ID_NODE1];
    
    mySwitch.sendPowerStateEvent(myPowerStateN1);
    Serial.printf("Device %s turned %s (manually via flashbutton)\r\n", mySwitch.getDeviceId().c_str(), myPowerStateN1?"on":"off");

    lastBtnPressN1 = actualMillis;  // update last button press variable
  } 

  if (digitalRead(BUTTON_PIN) == LOW && actualMillis - lastBtnPressN2 > 1000)  { 
    if (myPowerStateN2) {     
      myPowerStateN2 = false;
    } else {
      myPowerStateN2 = true;
    }
    digitalWrite(NODE2, myPowerStateN1?LOW:HIGH);

    // get Switch device back
    SinricProSwitch& mySwitch = SinricPro[SWITCH_ID_NODE2];
    // send powerstate event
    mySwitch.sendPowerStateEvent(myPowerStateN2); // send the new powerState to SinricPro server
    Serial.printf("Device %s turned %s (manually via flashbutton)\r\n", mySwitch.getDeviceId().c_str(), myPowerStateN2?"on":"off");

    lastBtnPressN2 = actualMillis;  // update last button press variable
  }   
}

// setup function for WiFi connection
void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

void connectAWS() {

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA); // Get AWS CA
  net.setCertificate(AWS_CERT_CRT); // Get AWS Certificate
  net.setPrivateKey(AWS_CERT_PRIVATE);// Get Private key

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  // Create a message handler
  client.setCallback(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC1);
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC2);
  Serial.println("AWS IoT Connected!");
}

void Current(int Fate_pin,char *St) {
  StaticJsonDocument<200> doc; // set up doc to get data

  // call function calculate current form library(Custom)
  int reading = analogRead(Fate_pin);
  float Crr = Get_Ampare(reading);
  float Power = Get_Watt(Crr);
  float Crcy = Currency_THB(Get_WKH(Power));
  
  // transfer data to doc
  doc["Current"] = Crr;
  doc["Power"] = Power;
  doc["Currency"] = Crcy;
  
  char payload[512]; // set payload
  // Up data to Json --> MQTT to cloud AWS
  serializeJson(doc, payload);
  client.publish(St, payload); 
  
  Serial.println("\nmessage Publish");
  
  // Display
  if(Fate_pin == Curr_Fate_1){
    Serial.println("Massage from : Device 1");
  } else {
    Serial.println("Massage from : Device 2");
  }
  
  Serial.print("Current (in amperes): ");
  Serial.println(Crr);
  Serial.print("Power (in watts): ");
  Serial.println(Power);
  Serial.print("Cost (in THB): ");
  Serial.println(Crcy);
  Serial.print("\n");
}

void messageHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("incoming: ");
  Serial.println(topic);
  
  // topic input to close or open relay
  if (strstr(topic, "esp32/relay1")) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, payload);
    String Relay_data = doc["status"];
    int r = Relay_data.toInt();
    digitalWrite(NODE1, !r);
    Serial.print("NODE1 - ");
    Serial.println(NODE1);
    Serial.print("\n");
  }
  
  if (strstr(topic, "esp32/relay2")) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, payload);
    String Relay_data = doc["status"];
    int r = Relay_data.toInt();
    digitalWrite(NODE2, !r);
    Serial.print("NODE2 - ");
    Serial.println(NODE2);
    Serial.print("\n");
  }
}

// function detected distance
void distance(){
  digitalWrite(Tr, 1);
  delayMicroseconds(2);
  digitalWrite(Tr, 0);

  long duration = pulseIn(Ech, 1);

  float distanceCm = duration * SOUND_SPEED / 2;

  Serial.print("\nDistance (cm): "); Serial.println(distanceCm); Serial.print("\n");
  
  delay(2);
  
  // toggle status transform Data
  if(distanceCm < 8 && distanceCm != 0){
    if(status == false){
        status = true;
        return;                       
      }

    if(status == true){
      status = false;
      return;                       
    }    
  }
}

// setup function for SinricPro
void setupSinricPro() {
  // add device to SinricPro
  SinricProSwitch& mySwitchN1 = SinricPro[SWITCH_ID_NODE1];
  SinricProSwitch& mySwitchN2 = SinricPro[SWITCH_ID_NODE2];

  // set callback function to device
  mySwitchN1.onPowerState(onPowerStateN1);
  mySwitchN2.onPowerState(onPowerStateN2);
  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  //SinricPro.restoreDeviceStates(true); // Uncomment to restore the last known state from the server.
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void setup() {
  Serial.begin(115200);
  pinMode(Red, OUTPUT);
  pinMode(Green, OUTPUT);
  pinMode(Blue, OUTPUT);

  digitalWrite(Red, 1);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(NODE1, OUTPUT);
  pinMode(NODE2, OUTPUT);

  pinMode(Tr, OUTPUT);
  pinMode(Ech, INPUT);

  digitalWrite(NODE1, 1);
  digitalWrite(NODE2, 1);

  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");
  
  Serial.println("\nStarting Switch on " + String(ARDUINO_BOARD));
  Serial.println("Version : " + String(SINRICPRO_VERSION_STR));

  setupWiFi();
  digitalWrite(Red, 0);
  
  setupSinricPro();
  connectAWS();
  digitalWrite(Green, 1);
}

void loop() {

  // Detected distance
  distance();
  
  handleButtonPress();
  SinricPro.handle();
  // access status to send data
  if(status == true){
    client.loop();

    digitalWrite(Blue, 1);
    // Transform data & Display
    Current(Curr_Fate_1, AWS_IOT_PUBLISH_TOPIC1);
    Current(Curr_Fate_2, AWS_IOT_PUBLISH_TOPIC2);
    delay(40); 
    
    digitalWrite(Blue, 0);
    digitalWrite(Red, 0);
    digitalWrite(Green, 0);
    delay(500);
  } else {
    // one time Green light
    if(i != 1){
      delay(500);
      digitalWrite(Green, 0);
      i++;
    }
    digitalWrite(Red, 1);
    digitalWrite(Blue, 1);    
  }
}