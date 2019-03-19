// Adafruit MQTT Library - Version: Latest 
// PubSubClient - Version: Latest
// WiFi - Version: Latest 
// SPI - Version: Latest 

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
String globalPayload; //This is a surrogate; it takes the message from the device into the buffer system.
String globalTopic;

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

// Method to check a string to see if it is numeric

bool isNumeric(String str){
for(byte i=0;i<str.length();i++)
{
if(isDigit(str.charAt(i))) return true;
}
return false;
} 
//Timer variables!
unsigned long previousMillis = 0;
unsigned long currentMillis; 
long interval;
bool expired; //1 = yes, 0 = no

//Timer function (Why? Because delay() stops all code, while this will let things run simultaneously):
//I used the BlinkWithoutDelay Example to make this function.

//COME BACK TO THIS!!!!!!!!!!!!!!

void realTimer(){
    currentMillis = millis();
    if (expired == 0){
      //if the timer goes off
      if (currentMillis- previousMillis >= interval) {
       //Reset timer
       previousMillis = currentMillis;
       expired = 1;
      }else{
        expired = 0;
      }
    }
    
}

//Hall effect sensor variables
int  hallSensorPin  =  A7;    // Hall effect analog input pin
int  hallSensorValue =  0;    // hall effect variable
///Setup bools so know what direction last traveled when detected Hall marker
//TODO:  make this persistent over power cycles
bool foundHallMarkerOnFwdMove = false;
bool foundHallMarkerOnRevMove = false;

//MESSAGE RECEPTION:
bool nuevo; //I can't use "new" because it is already a key word. This is to see if the inputted command is new or not

void callback(char* topic, byte* payload, unsigned int length) {
  //Tells you information about the message:
  Serial.println("Received message length: "); 
  Serial.print(length);
  Serial.println("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  //The for loop below builds the message into a list of characters:
  for (int i = 0; i < length; i++) {
    globalPayload= (char)payload[i];
  }
  //The for loop below builds the topic into a list of characters:
  for (int i = 0; i < length; i++) {
   globalTopic[i] = (char)topic[i];
  }
  Serial.println(globalPayload);
  
  nuevo = 1;
  } //End the callback function here!

//THE BUFFER SYSTEM!!!!!!!

//Important variables for buffering
bool b1busy; //If buffer 1 is busy
bool b2busy; //If buffer 2 is busy
bool ovrde; //If a command should override other commands (e.g. "stop")
bool sameTime; //If true, both b1 and b2 (see below) can be done at the same time.
//Here is the buffer (denoted by "b") system setup! I will explain as I go:
String b1 = "void";
String b2 = "void"; //"void" will be the key word for us to consider the buffers empty.

//Make ovrde = 0 when override NOT ACTIVE! IMPLEMENT THIS!

void bufferSystem(String msg){ //Call in void loop()
if (msg.substring(0) == "void"){
  nuevo = 0;
} else if(msg.substring(0) !="void"){
  nuevo = 1;
} //Input is new if the payload is void
if (b1 != "void" && b2 != "void"){ //Thus both buffers are full
  if (msg.substring(0)== "override"){
    ovrde = 1;
  } else if (b1busy == 1 && b2busy ==1){
   Serial.println("The following command was ignored due to both buffers being full: " + msg); 
  }
} else if (b1 == "void" && b2 == "void"){ //Both are empty
  if (msg.substring(0)== "override"){
    ovrde = 1;
  }else if (b1busy ==0 && b2busy ==0){
     b1 = msg;
     globalPayload = "void"; 
  }
} else if (b1 != "void" && b2 == "void"){ // 1 is full, 2 is empty
  if (msg.substring(0)== "override"){
    ovrde = 1;
  } else if (b1busy ==1 && b2busy ==0){
     b2 = msg;
     globalPayload = "void"; 
  }
} else if (b1 == "void" && b2 != "void"){ //1 is empty, 2 is full
  if (msg.substring(0)== "override"){
    ovrde = 1;
  } else if (b1busy == 0 && b2busy ==1){
    b1 = b2;
    b2 = "void";
    globalPayload = "void";
  }
}
execute(b1,b2,ovrde);
}
//Here is the override function. It immediately stops the motors, clears the buffers, and waits for new commands.

void overrideFunction(){
  nuevo = 0;
  b1 = "void";
  b2 = "void";
  driveMotor.run(RELEASE);
  b1busy = 0;
  b2busy = 0;
  ovrde = 0;
  sameTime = 0;
  expired = 1;
  delay(3000); //To be safe!
  
}

//Here is where all the functions are called!

void execute (String bA, String bB, bool overrid){
 if (overrid ==1){
  overrideFunction();
 }
 //LED function
 else if (bA != "void" && bB != "void"){                         //If both are filled
   if (bA.substring(0)=="on"||bA.substring(0)=="off"){      //Set up same time stuff
   sameTime = 1;
   } else if (bB.substring(0)=="on"||bB.substring(0)=="off"){
    sameTime= 1;
   } else if (bA.substring(0)=="sense"||bB.substring(0)=="sense"){
    sameTime=1;
   } else{
    sameTime =0;
   }
   if (sameTime == 1){
  
   //This starts up the same-time-enabled stuff
   if (bA.substring(0)=="on"||bA.substring(0)=="off"){
   b1busy = 1;
   LED(bA); 
   bA = "void";
   } else if (bB.substring(0)=="on"||bB.substring(0)=="off"){
    b2busy = 1;
    LED(bB);
    bB = "void";
   }
   if (bA.substring(0)=="sense"){
   b1busy = 1;
   readHall(bA); 
   bA = "void";
   } else if (bB.substring(0)=="sense"){
    b2busy = 1;
    readHall(bB);
    bB = "void";
   }
   if(globalTopic.substring(0)=="Commands"){
    //We can't clear the buffers for the moving the motor or moving to the marker because when the code loops through, it needs to 
    //retain the command until the command is completely done. Clearing it would terminate the action in the next loop.
      if (bA.substring(0)=="go"||bA.substring(0)=="stop"||bA.substring(0)=="forward"
      ||bA.substring(0)=="backward"||isNumeric(bA.substring(0))){
      b1busy = 1;
      moveMotor(bA); 
      }else if (bB.substring(0)=="go"||bB.substring(0)=="stop"||bB.substring(0)=="forward"
      ||bB.substring(0)=="backward"||isNumeric(bB.substring(0))){
        b2busy = 1;
        moveMotor(bB); 
      } else if (bA.substring(0)=="left"||bA.substring(0)=="right"){
       //toMarker(bA); 
      } else if (bB.substring(0)=="left"||bB.substring(0)=="right"){
       //toMarker(bB); 
      }
   }
   
    } else if (sameTime ==0){
      if(globalTopic.substring(0)=="Commands"){
      b1busy = 1;
      LED(bA);
      moveMotor(bA);
      //toMarker(bA);
      readHall(bA);
      if (bA.substring(0)=="on"||bA.substring(0)=="off"||bA.substring(0)=="sense"){
        bA = "void";
      } //Ends LED or Sense right after they are done
      }
    }
  
 } else if (bA != "void" && bB == "void"){
   if(globalTopic.substring(0)=="Commands"){
   LED(bA);
   moveMotor(bA);
   //toMarker(bA);
   readHall(bA);
   if (bA.substring(0)=="on"||bA.substring(0)=="off"||bA.substring(0)=="sense"){
      bA = "void";
   } //Ends LED or Sense right after they are done
  }
 }
 else if (bA == "void" && bB != "void"){
   if(globalTopic.substring(0)=="Commands"){
   LED(bB);
   moveMotor(bB);
   //toMarker(bB);
   readHall(bB);
   if (bB.substring(0)=="on"||bB.substring(0)=="off"||bB.substring(0)=="sense"){
      bB = "void";
   } //Ends LED or Sense right after they are done
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
}

//Setup variables for motor

String direct;
bool moving; //1 = yes, 0 = no

//Move the motor!

void moveMotor(String msg)
{
  if (1==1){ //????????????
  if (msg == b1){
        b1busy=1;
      }
      else if (msg == b2){
        b2busy=1;
      }
    
  }
  driveMotor.setSpeed(255);
  if(msg.substring(0)=="forward" && nuevo ==1){
      
      direct = "forward";
      Serial.println("Motor will move forward!");
      nuevo = 0;
      
    }else if(msg.substring(0)=="backward" && nuevo ==1){
      if (msg == b1){
        b1busy=1;
      }
      else if (msg == b2){
        b2busy=1;
      }
      direct = "backward";
      Serial.println("Motor will move backward!");
      nuevo = 0;
      if (b1 == "backward"){
        b1busy=0;
        b1 = "void";
      }
      else if (b2 == "backward"){
        b2busy=0;
        b2 = "void";
      }
    }
    else if (isNumeric(msg.substring(0))&& nuevo ==1)
    {
      interval = msg.toInt() * 1000; 
      Serial.println("Time set to "+ msg + " seconds!");
      nuevo = 0;
    }
    else if (msg.substring(0)=="go" && b1busy == 0){
      if (expired == 0 && moving == 0 && nuevo ==1){
        //If the timer is going, the robot isn't moving, and the command is new
         if (direct == "forward")
         {
           driveMotor.run(FORWARD);
           Serial.println("Moving forward!");
           moving = 1;
           nuevo =0;
         }else if (direct == "backward"){
           driveMotor.run(BACKWARD);
           Serial.println("Moving backward!");
           moving = 1;
           nuevo =0;
         }
      }
     else if (moving == 1 && expired == 0){
        b1busy = 1;
      }
     else if (msg.substring(0)=="stop" || expired ==1){
      //If the message is "stop" or if the timer expired
     driveMotor.run(RELEASE); 
     Serial.println("Motor stopped!");
     moving = 0;
     b1busy = 0;
     nuevo = 0;
  }
    }
    if (nuevo == 0 && moving == 0){
    if (msg == b1){
        b1busy=0;
        b1 = "void";
      }
      else if (msg == b2){
        b2busy=0;
        b2 = "void";
      }  
    }
    
}
//Converge to the Hall marker (magnet)
/*
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
*/
//Just reads the Hall marker value

void readHall(String msg){
if (msg.substring(0)=="sense"){
hallSensorValue = analogRead(hallSensorPin);
Serial.print("Hall value (loop): ");  Serial.println(hallSensorValue);
nuevo = 0;
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
  realTimer();
  hallSensorValue =  analogRead(hallSensorPin);
  //Serial.print("Hall value (loop): ");  Serial.println(hallSensorValue);
  bufferSystem(globalPayload);
  //Keeps scanning for new information:
  client.loop();

//Publish a message

//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    client.publish("Output", msg);
}

