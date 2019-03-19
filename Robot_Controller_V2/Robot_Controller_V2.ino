// Adafruit MQTT Library - Version: Latest 
// PubSubClient - Version: Latest
// WiFi - Version: Latest 
// SPI - Version: Latest 

//VERSION 2.0

/*Welcome to the Arduino code for the Robot Project! All code here is attributed to both 
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
 * Adafruit MQTT (both libraries are in here): https://github.com/adafruit/Adafruit_MQTT_Library
 * WiFi stuff: https://github.com/arduino/wifishield/tree/master/libraries/WiFi
 * SPI: Built-in. Go to Library Manager under Sketch.
 * PubSubClient: https://github.com/arduino/pubsubclient
 * AFMotor: https://github.com/farukcankaya/AFMotor/tree/master/AFMotor
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

//DEFINE ALL LIGHTS, MOTORS, AND OTHER DEVICES:

#define CHASSIS5P0V1

#ifdef CHASSIS5P0V1
AF_DCMotor driveMotor(3, MOTOR12_64KHZ); //For chassis version 5.0V1
#endif

#define LED_BLUE 13  

#define HALL_MARKER_SAMPLE_INTERVAL 10  //ms
#define MOTOR_MOVE_TIME HALL_MARKER_SAMPLE_INTERVAL*200 //ms - MUST BE AN MULTIPLE OF HALL_MARKER_SAMPLE_INTERVAL

// TODO: ******** NEED TO HAVE A LOWER THRESHOLD FOR OTHER POLARITY OF MAGNET!!!!!!!! (will be lower)  **********
#define HALL_DETECT_THRESHOLD 500 

//Configure drive motor for 4.0V1 chassis or 5.0V1 chassis

#ifdef CHASSIS4P0V1
AF_DCMotor driveMotor(1, MOTOR12_64KHZ);//For chassis version 4.0V1
#endif


//DEFINE WI-FI INFORMATION (insert different information, if that applies to you):

char ssid[] = "guest-SDUHSD"; //  your network SSID (name)
char pass[] = "guest-SDUHSD";    // your network password (use for WPA, or use as key for WEP)  
int keyIndex = 0;            // your network key Index number (needed only for WEP)

//This part is important. Remember the Cloud MQTT server? Paste the given URL below:

char mqtt_server = "m15.cloudmqtt.com:14269";

//The stuff below sets up the Wi-Fi connection. Leave it alone unless you absolutely know what you are doing!

int status = WL_IDLE_STATUS;
long lastMsg = 0;
char msg[50];
int value = 0;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.

WiFiClient wifiClient;
PubSubClient client(wifiClient);


//Hall effect sensor variables
int  hallSensorPin  =  A7;    // Hall effect analog input pin
int  hallSensorValue =  0;    // hall effect variable
///Setup bools so know what direction last traveled when detected Hall marker
//TODO:  make this persistent over power cycles
bool foundHallMarkerOnFwdMove = false;
bool foundHallMarkerOnRevMove = false;

// Check a string to see if it is numeric

bool isNumeric(String str){
for(byte i=0;i<str.length();i++)
{
if(isDigit(str.charAt(i))) return true;
}
return false;
} 

//This method sets the Wi-Fi connection up. Have the Serial open when it runs!

void setup_wifi(){
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
//Actually connects to Wi-Fi:
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
/*
When the MQTT Dashboard pops up, 
set up the correct information and create consistent payloads.

//PUT ALL OF YOUR METHODS FOR INTERPRETING PAYLOADS HERE!

Use the following format:

void function_name(String msg){
if (msg.substring(0)=="whatever message you want to be interpreted"){
Put whatever you want here. We recommend the following command when you are done:
Serial.println("Here, write that the task you performed was completed");
}
Feel free to add elses or else ifs if you need to. And that's all!!!
}
*/

//Turn an LED on or off:

void LED(String msg) {
  if(msg.substring(0).equals("off")){
     digitalWrite(LED_BUILTIN, LOW);
     Serial.println("Built-in LED off!");
    } else if(msg.substring(0)=="on"){
     digitalWrite(LED_BUILTIN, HIGH);
     Serial.println("Built-in LED on!");
    }
}

//Setup variables for motor

String direct;
float tim;

//Move the motor!

void moveMotor(String msg)
{
  driveMotor.setSpeed(255);
  if(msg.substring(0)=="forward"){
      direct = "forward";
      Serial.println("Motor will move forward!");
      driveMotor.run(RELEASE);
    }else if(msg.substring(0)=="backward"){
      direct = "backward";
      Serial.println("Motor will move backward!");
      driveMotor.run(RELEASE);
    }
    else if (isNumeric(msg.substring(0)))
    {
      tim = msg.toFloat() * 1000; 
      Serial.println("Time set to "+ msg + " seconds!");
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
      delay(tim);
      driveMotor.run(RELEASE);  
      Serial.println("Motor stopped!");
  } else if (msg.substring(0)=="stop"){
     driveMotor.run(RELEASE); 
     Serial.println("Motor stopped!");
  }
}
//Converge to the Hall marker (magnet)

void findTheHallMarker(String directionComingFrom){
  driveMotor.setSpeed(255);
  if(directionComingFrom.equals("forward")){
    driveMotor.run(BACKWARD);
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      delay(10);  
              
    driveMotor.run(FORWARD);
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      delay(10);    

    driveMotor.run(BACKWARD);
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      delay(10);                          
  }
  else{
    driveMotor.run(FORWARD);
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      delay(10);  
              
    driveMotor.run(BACKWARD);
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      delay(10);    

    driveMotor.run(FORWARD);
    while(hallSensorValue =  analogRead(hallSensorPin) > HALL_DETECT_THRESHOLD)
      delay(10);                          
  }
  driveMotor.run(RELEASE);
  Serial.println("Converged to the Hall marker");
}

//Move the robot to the nearest Hall Marker.

void toMarker (String msg){

     if(msg.substring(0)=="right"){  //right is forward
        Serial.println("*****Received right/forward command*****");
        //Stop motor if already running, move forward
        driveMotor.run(RELEASE);
        foundHallMarkerOnRevMove = false;  //Reset the reverse marker detect flag
  
        //First check if trying to move past a hall marker in the same direction 
        //as last detected the marker (can't do this!).
        if(!foundHallMarkerOnFwdMove){
          Serial.println("Motor forward");
          driveMotor.setSpeed(255);
          driveMotor.run(FORWARD);
          delay(400);  //move once in case positioned on top of the magnet
  
          for (int i = 0; i<MOTOR_MOVE_TIME; i+=HALL_MARKER_SAMPLE_INTERVAL) {
            hallSensorValue =  analogRead(hallSensorPin);
            Serial.print("Hall value: ");  Serial.println(hallSensorValue);
            if (hallSensorValue > HALL_DETECT_THRESHOLD){
              delay(HALL_MARKER_SAMPLE_INTERVAL);
              foundHallMarkerOnFwdMove = false;
            }
            else {
              foundHallMarkerOnFwdMove = true;
              driveMotor.run(RELEASE);
              Serial.println("Found a Hall marker!!!");
              delay(500);
              findTheHallMarker("forward"); //converge to the marker 
              break;
            }
          }        
        }
        driveMotor.run(RELEASE);
        Serial.println("Motor stopped");     
      } //end payload==right
      
      else if(msg.substring(0)=="left"){  //left is reverse
        Serial.println("*****Received left/reverse command*****");
        //Stop motor if already running, move reverse
        driveMotor.run(RELEASE);
        foundHallMarkerOnFwdMove = false;  //Reset the forward marker detect flag
  
        //First check if trying to move past a hall marker in the same direction 
        //as last detected the marker (can't do this!).
        if(!foundHallMarkerOnRevMove){
          Serial.println("Motor reverse");
          driveMotor.setSpeed(255);
          driveMotor.run(BACKWARD);
          delay(400);  //move once in case positioned on top of the magnet
          for (int i = 0; i<MOTOR_MOVE_TIME; i+=HALL_MARKER_SAMPLE_INTERVAL) {
            hallSensorValue =  analogRead(hallSensorPin);
            Serial.print("Hall value: ");  Serial.println(hallSensorValue);
            if (hallSensorValue > HALL_DETECT_THRESHOLD){
              delay(HALL_MARKER_SAMPLE_INTERVAL);
              foundHallMarkerOnRevMove = false;
            }
            else {
              foundHallMarkerOnRevMove = true;
              driveMotor.run(RELEASE);
              Serial.println("Found a Hall marker!!!");
              delay(500);
              findTheHallMarker("reverse"); //connverge to the marker
              break;
            }
          }     
        }
        driveMotor.run(RELEASE);
        Serial.println("Motor stopped");
      } //end payload==left
      //Stop the motor after any command
      driveMotor.run(RELEASE);
}

//Just reads the Hall marker value

void readHall(String msg){
if (msg.substring(0)=="sense"){
hallSensorValue = analogRead(hallSensorPin);
Serial.print("Hall value (loop): ");  Serial.println(hallSensorValue);
}
}

//MESSAGE INTERPRETATION:

void callback(char* topic, byte* payload, unsigned int length) {
  char payloadChar[length];
  //Tells you information about the message:
  Serial.println("Received message length: "); 
  Serial.print(length);
  Serial.println("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  //The for loop below builds the message into a list of characters:
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    payloadChar[i] = (char)payload[i];
  }
  //The string variables are very important!!! They turn the list of characters into a usable String!
  String realPayload(payloadChar);
  String realTopic(topic);
  Serial.println(realPayload);
  
  /* CALL ALL FUNCTIONAL METHODS INSIDE THE IF STATEMENT! (e.g. turn LED on/off)
   Create an else if statement if there is another topic you want to subscribe to.
  */
  if(realTopic.substring(0)=="Commands"){
   LED(realPayload);
   moveMotor(realPayload);
   toMarker(realPayload);
   readHall(realPayload);
  }
    
     /*

  
   */   
  } //End the callback function here!
  
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
      delay(500);
    }
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
//Reconnects to Wi-Fi if you lose connection:
  if (!client.connected()) {
    reconnect();
  }

  hallSensorValue =  analogRead(hallSensorPin);
  //Serial.print("Hall value (loop): ");  Serial.println(hallSensorValue);
     
  //Keeps scanning for new information:
  client.loop();
}

