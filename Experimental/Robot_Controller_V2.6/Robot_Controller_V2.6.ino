// Adafruit MQTT Library - Version: Latest 
// PubSubClient - Version: Latest
// WiFi - Version: Latest 
// SPI - Version: Latest 

//VERSION 2.6
//This version has no delays and has the converging to the hall marker stuff!

/*Welcome to the Arduino code for the Hibotics Project! All code here is attributed to both 
 * Tony Mauro and Nathaniel Shalev, but may be edited for whatever purpose you need. 
 * Please credit us for any project you use this code for. Enjoy!
 * 
 * The goal of this code is to connect the Arduino to Wi-Fi and control it through a 
 * phone app. We decided to use MQTT to connect the phone app MQTT Dashboard to Arduino.
 * We use an Arduino Mega board with a Wi-Fi shield to connect to Wi-Fi,
 * in addition to various libraries. 
 * 
 * Your first step here is to download all the correct libraries. Some of them 
 * are already downloaded as a preset and others are from GitHub. 
 * Here is where you can find each library:
 * 
 * Note: All of the libraries below are already installed in Arduino. All you have to do is go to the library manager and 
 * search for them. Install them. The only one you have to install from GitHub is AFMotor.
 * 
 * Adafruit MQTT (both libraries are in here): https://github.com/adafruit/Adafruit_MQTT_Library
 * WiFi stuff: https://github.com/arduino/wifishield/tree/master/libraries/WiFi
 * SPI: Built-in. Go to Library Manager under Sketch.
 * PubSubClient: https://github.com/arduino/pubsubclient
 * Adafruit Motor Shield Library (AFMotor): https://github.com/adafruit/Adafruit-Motor-Shield-library
 * 
 * Before running the code, please be sure to select the right board and import all
 * correct libraries.
 * Then, make a Cloud MQTT account and create a server. Go to the details of the
 * server and have it handy for future steps.
 * 
 * From there, download MQTT Dashboard from the Play Store. It is free and takes very little time to set up.
 * 
 * For MQTT stuff, here is the API (methods and whatnot) documentation: 
 * https://pubsubclient.knolleary.net/api.html
*/

//LIBRARIES:

#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <AFMotor.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

//Define all lights, motors, and other devices:

#define CHASSIS5P0V1

#ifdef CHASSIS5P0V1
AF_DCMotor driveMotor(3, MOTOR12_64KHZ); //For chassis version 5.0V1
#endif

#define LED_BLUE 13  

#define HALL_MARKER_SAMPLE_INTERVAL 10  //ms
#define MOTOR_MOVE_TIME HALL_MARKER_SAMPLE_INTERVAL*200 //ms - MUST BE AN MULTIPLE OF HALL_MARKER_SAMPLE_INTERVAL

// TO DO: ******** NEED TO HAVE A LOWER THRESHOLD FOR OTHER POLARITY OF MAGNET!!!!!!!! (will be lower)  **********
#define HALL_DETECT_THRESHOLD 500 

//Configure drive motor for 4.0V1 chassis or 5.0V1 chassis

#ifdef CHASSIS4P0V1
AF_DCMotor driveMotor(1, MOTOR12_64KHZ);//For chassis version 4.0V1
#endif

//Variables here are the actual topic and payload, but are defined as soon as callback gets a message.
String globalPayload; 
String globalTopic;

//Timer variables!
unsigned long previousMillis = 0;
unsigned long currentMillis, globalTime; 
bool expired; //1 = yes, 0 = no

//Timer function (Why? Because delay() stops all code, but this will let things run simultaneously):
//I used the BlinkWithoutDelay Example to make this function.

void startTimer(unsigned long tim){ //Run this in place of delay() basically
     globalTime = tim;
     previousMillis = currentMillis;
     expired = 0; //Start timer 
}

bool moving = 0; //1 = yes, 0 = no ; This is used for the timer and the motor to see if the motor is moving
String moveComm;
void stopTimer(){ //Done in void loop() so that the timer stops on the right time stamp
if ((expired == 0)&&(currentMillis- previousMillis >= globalTime)){ //Once the timer goes off
       //Reset timer
       previousMillis = currentMillis;
       globalTime = 0;
       expired = 1; 
       if (moving ==1){
        globalPayload = "stop";
       }
}
}  

//DEFINE WI-FI INFORMATION (insert different information, if that applies to you):

char ssid[] = "guest-SDUHSD"; //  your network SSID (name)
char pass[] = "guest-SDUHSD";    // your network password (use for WPA, or use as key for WEP)  
int keyIndex = 0;            // your network key Index number (needed only for WEP)

//This part is important. Remember the Cloud MQTT server? Paste the given URL below:

String mqtt_server = "m15.cloudmqtt.com:14269";

//The stuff below sets up the Wi-Fi connection. Leave it alone unless you absolutely know what you are doing!

int status = WL_IDLE_STATUS;
long lastMsg = 0;
char msg[50];
int value = 0;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.

WiFiClient wifiClient;
PubSubClient client(wifiClient);

//This method sets the Wi-Fi connection up. Have the Serial open when it runs!

void setup_wifi(){ 
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
//Actually connects to Wi-Fi:
  WiFi.begin(ssid, pass);
  startTimer(500);
  while (WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (expired ==1){
      Serial.print(".");
      startTimer(500);
    }
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//Reconnect to Wi-Fi if you disconnect:

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Arduino-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect("Arduino","tycbgiep","ReFQCY-W0gtG")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("Output", "hello Arduino");
      // ... and resubscribe
      client.subscribe("Commands");
      Serial.println("Subscribed to commands");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      startTimer(5000);
    }
  }
}

// Method to check a string to see if it is numeric

bool isNumeric(String str){
for(byte i=0;i<str.length();i++)
{
if(isDigit(str.charAt(i))) return true;
}
return false;
} 

//Hall effect sensor variables
int  hallSensorPin  =  A7;    // Hall effect analog input pin
int  hallSensorValue =  0;    // hall effect variable
///Setup bools so know what direction last traveled when detected Hall marker
//TODO:  make this persistent over power cycles
bool foundHallMarkerOnFwdMove = false;
bool foundHallMarkerOnRevMove = false;

//MESSAGE RECEPTION:

void callback(char* topic, byte* payload, unsigned int length) {
  globalPayload = "";
  globalTopic = "";
  //Tells you information about the message:
  Serial.print("Received message length: "); 
  Serial.println(length);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");
  //The for loop below builds the message into a list of characters:
  for (int i = 0; i < length; i++) {
    globalPayload += (char)payload[i];
  }
  globalTopic = topic; //Saves the topic globally
  Serial.println(globalPayload);
  } //End the callback function here!


void execute(String function){
 if(function.substring(0)=="forward"||function.substring(0)=="backward" ||
 isNumeric(function.substring(0))|| function.substring(0)=="go" || function.substring(0)=="stop"){
 moveComm = function;
 moveMotor(moveComm);
 moveComm= "";
 }
 if(globalTopic.substring(0)=="Commands"){
      if (function != ""){ //I may need to change the conditions
      LED(function);
      //toMarker(function);
      readHall(function);
      }else { 
      }  
 }
}
void LED(String msg) {
  if(msg.substring(0)=="off"){
      digitalWrite(LED_BUILTIN, LOW);
     Serial.println("Built-in LED off!");
    } else if(msg.substring(0)=="on"){
      digitalWrite(LED_BUILTIN, HIGH);
     Serial.println("Built-in LED on!");
    }
    globalPayload = "";
}

//Setup variables for motor

String direct;
//The moving variable was declared earlier to be used for stopTimer()

//Move the motor!

void moveMotor(String msg)
{
  driveMotor.setSpeed(255);
  if(msg.substring(0)=="forward"){
      direct = "forward";
      Serial.println("Motor will move forward!");
      driveMotor.run(RELEASE);
      moving = 0;
    }else if(msg.substring(0)=="backward"){
      direct = "backward";
      Serial.println("Motor will move backward!");
      driveMotor.run(RELEASE);
      moving = 0;
    }
    else if (isNumeric(msg.substring(0)))
    {
      globalTime = msg.toFloat() * 1000; 
      Serial.println("Time set to "+ msg + " seconds!");
      driveMotor.run(RELEASE);
      moving = 0;
    }
    else if (msg.substring(0)=="go"){
      if (direct == "forward")
      {
        driveMotor.run(FORWARD);
        Serial.println("Moving forward!");
      }else if (direct == "backward"){
        driveMotor.run(BACKWARD);
        Serial.println("Moving backward!");
      }
      startTimer(globalTime);
      moving = 1;
      moveComm = "";
      globalPayload = "";
  } else if (msg.substring(0)=="stop"){
     if (moving == 1){
      driveMotor.run(RELEASE); 
     Serial.println("Motor stopped!");
     moving = 0;
     client.publish("Commands", "stop");
     } else {
      client.publish("Commands", "");
     }
  }
    globalPayload = "";
}
//Converge to the Hall marker (magnet)
void findTheHallMarker(String directionComingFrom){
  driveMotor.setSpeed(255);
  if(directionComingFrom.equals("forward")){
    driveMotor.run(BACKWARD);
    moving = 1;
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      startTimer(10);  
              
    driveMotor.run(FORWARD);
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      startTimer(10);    

    driveMotor.run(BACKWARD);
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      startTimer(10);                          
  }
  else{
    driveMotor.run(FORWARD);
    moving = 1;
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      startTimer(10);  
              
    driveMotor.run(BACKWARD);
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      startTimer(10);    

    driveMotor.run(FORWARD);
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      startTimer(10);                          
  }
  driveMotor.run(RELEASE);
  moving = 0;
  Serial.println("Converged to the Hall marker");
}

//Move the robot to the nearest Hall Marker.

void toMarker (String msg){

     if(msg.substring(0)=="right"){  //right is forward
        Serial.println("*****Received right/forward command*****");
        //Stop motor if already running, move forward
        driveMotor.run(RELEASE);
        moving = 0;
        foundHallMarkerOnRevMove = false;  //Reset the reverse marker detect flag
  
        //First check if trying to move past a hall marker in the same direction 
        //as last detected the marker (can't do this!).
        if(!foundHallMarkerOnFwdMove){
          Serial.println("Motor forward");
          driveMotor.setSpeed(255);
          driveMotor.run(FORWARD);
          moving = 1;
          startTimer(400);  //move once in case positioned on top of the magnet
  
          for (int i = 0; i<MOTOR_MOVE_TIME; i+=HALL_MARKER_SAMPLE_INTERVAL) {
            hallSensorValue =  analogRead(hallSensorPin);
            Serial.print("Hall value: ");  Serial.println(hallSensorValue);
            if (hallSensorValue > HALL_DETECT_THRESHOLD){
              startTimer(HALL_MARKER_SAMPLE_INTERVAL);
              foundHallMarkerOnFwdMove = false;
            }
            else {
              foundHallMarkerOnFwdMove = true;
              driveMotor.run(RELEASE);
              moving = 0;
              Serial.println("Found a Hall marker!!!");
              startTimer(500);
              findTheHallMarker("forward"); //converge to the marker 
              break;
            }
          }        
        }
        driveMotor.run(RELEASE);
        moving = 0;
        Serial.println("Motor stopped");     
      } //end payload==right
      
      else if(msg.substring(0)=="left"){  //left is reverse
        Serial.println("*****Received left/reverse command*****");
        //Stop motor if already running, move reverse
        driveMotor.run(RELEASE);
        moving = 0;
        foundHallMarkerOnFwdMove = false;  //Reset the forward marker detect flag
  
        //First check if trying to move past a hall marker in the same direction 
        //as last detected the marker (can't do this!).
        if(!foundHallMarkerOnRevMove){
          Serial.println("Motor reverse");
          driveMotor.setSpeed(255);
          driveMotor.run(BACKWARD);
          moving = 1;
          startTimer(400);  //move once in case positioned on top of the magnet
          for (int i = 0; i<MOTOR_MOVE_TIME; i+=HALL_MARKER_SAMPLE_INTERVAL) {
            hallSensorValue =  analogRead(hallSensorPin);
            Serial.print("Hall value: ");  Serial.println(hallSensorValue);
            if (hallSensorValue > HALL_DETECT_THRESHOLD){
              startTimer(HALL_MARKER_SAMPLE_INTERVAL);
              foundHallMarkerOnRevMove = false;
            }
            else {
              foundHallMarkerOnRevMove = true;
              driveMotor.run(RELEASE);
              moving = 0;
              Serial.println("Found a Hall marker!!!");
              startTimer(500);
              findTheHallMarker("reverse"); //connverge to the marker
              break;
            }
          }     
        }
        driveMotor.run(RELEASE);
        moving = 0;
        Serial.println("Motor stopped");
      } //end payload==left
      //Stop the motor after any command
      driveMotor.run(RELEASE);
      moving = 0;
}
*/
//Just reads the Hall marker value

void readHall(String msg){
if (msg.substring(0)=="sense"){
hallSensorValue = analogRead(hallSensorPin);
Serial.print("Hall value (loop): ");  Serial.println(hallSensorValue);
}
}

//Sets stuff up when you run the code:

void setup() {
  Serial.begin(115200);
  //Define all pins HERE!!!
  pinMode(LED_BUILTIN, OUTPUT);
  setup_wifi();
  //Write in your URL here as shown, and put in the PIN thing in the second argument:
  client.setServer("m15.cloudmqtt.com", 14269);
  //Runs the callback stuff:
  client.setCallback(callback);
}

void loop() {
currentMillis = millis();
//Reconnects to Wi-Fi if you lose connection:
  if (!client.connected()) {
    reconnect();
  }
  hallSensorValue =  analogRead(hallSensorPin);
  execute(globalPayload);
  stopTimer();
  //Serial.print("Hall value (loop): ");  Serial.println(hallSensorValue);
//Keeps scanning for new information:
client.loop();

}

