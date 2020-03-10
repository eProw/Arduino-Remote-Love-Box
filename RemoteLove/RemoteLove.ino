#define IO_USERNAME  "username"                         //adafruit.io username
#define IO_KEY       "aio_key"                          //adafruit.io api key
#define STATE_PIN D3                                    //Indicates whether the sd card is available or not and the initial connection status
#define SEND_PIN D1                                     //Indicates the signal has been sent
#define RECEIVE_PIN D8                                  //Indicates the signal has been received
#define BUTTON_PIN D0                                   //Checks for the state of the button

#include <SPI.h>
#include <SD.h>

#include "AdafruitIO_WiFi.h"
#include "ESP8266WiFi.h"

File wifiList;

AdafruitIO_WiFi* io;
AdafruitIO_Feed* sender;
AdafruitIO_Feed* receiver;

char ssid[32];
char password[64];

int fadeValue = 0;
int sign = 1;

unsigned long nextSend = 0;

bool canSend = true;

void fadeLed(int spd, int pin){

  for(; fadeValue < 1023; fadeValue+=spd){ 
    // increasing the LED brightness with PWM
    analogWrite(pin, fadeValue);
    delay(1);
  }
  
  for(; fadeValue > 0; fadeValue-=spd){ 
    // decreasing the LED brightness with PWM
    analogWrite(pin, fadeValue);
    delay(1);
  }
  
}

void stabilizeLed(int level, int pin){
  if(fadeValue < level){
    for(; fadeValue < level; fadeValue+=2){ 
      // changing the LED brightness with PWM
      analogWrite(pin, fadeValue);
      delay(1);
    }
  }else{
    for(; fadeValue > level; fadeValue-=2){ 
      // changing the LED brightness with PWM
      analogWrite(pin, fadeValue);
      delay(1);
    }
  }
}

void matchWifi(){
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();

  // Initialize SD module
  
  while (!SD.begin(D4)){
    fadeLed(3,STATE_PIN);
  }

  stabilizeLed(1000,STATE_PIN);

  // Open the file
  wifiList = SD.open("wifi.txt");

  if (wifiList) {
    bool found = false;
    
    while (wifiList.available() && !found) {
      String ssid_line, password_line;
      
      ssid_line = wifiList.readStringUntil('\n');
      password_line = wifiList.readStringUntil('\n');

      ssid_line.remove(ssid_line.length() - 1);
      password_line.remove(password_line.length() - 1);
      
      for (int i = 0; i < n && !found; ++i) {
        // Get SSID from each network found, compare it with the stored ssids
        
        if(ssid_line == WiFi.SSID(i)){
          Serial.println("Match Found:");
          // If both ssids match, keep the value in memory
          ssid_line.toCharArray(ssid, ssid_line.length() + 1);
          password_line.toCharArray(password, password_line.length() + 1);
          
          Serial.println(ssid_line);
          Serial.println(password_line);
          
          found = true;       
        }
      }

      Serial.println("\n");
    }    
  }

  wifiList.close();
}

void connectToServer(){
  io = new AdafruitIO_WiFi (IO_USERNAME, IO_KEY, ssid, password);
  sender = io->feed("remotelove_s");
  receiver = io->feed("remotelove_r");
  
  io->connect();
  
  while(io->status() < AIO_CONNECTED) {
    Serial.println(".");
    fadeLed(10,STATE_PIN);
    delay(200);
  }

  // we are connected
  Serial.println();
  Serial.println(io->statusText());

  // set up a message handler for the 'receive' feed.
  // the handleMessage function (defined below)
  // will be called whenever a message is
  // received from adafruit io.
  receiver->onMessage(handleMessage);

  stabilizeLed(500,STATE_PIN);
}

void setup() {

  pinMode(STATE_PIN,OUTPUT);
  pinMode(SEND_PIN,OUTPUT);
  pinMode(RECEIVE_PIN,OUTPUT);
  pinMode(BUTTON_PIN,INPUT);

  //Indicates the power is on
  fadeValue = 1000;
  analogWrite(STATE_PIN, fadeValue);
  
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  // Scan the available networks and store in the memory the network that matches one of those saved on the SD card
  matchWifi();

  //Connect to io.adafruit.com
  connectToServer();
}

void loop() {
  // keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io->run();
  
  if(digitalRead(BUTTON_PIN) == LOW && millis() > nextSend && canSend){
    nextSend = millis() + 5000;
    canSend = false;
    sender->save(1);

    fadeLed(1,SEND_PIN);
    digitalWrite(SEND_PIN, LOW);
  }else if(digitalRead(BUTTON_PIN) == HIGH && !canSend){
    canSend = true;
  }

}

void handleMessage(AdafruitIO_Data *data){
  if(data->toPinLevel() == HIGH){
    fadeLed(1,RECEIVE_PIN);
    sender->save(0);
  }
}
