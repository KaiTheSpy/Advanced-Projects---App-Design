#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>

//#include <AFMotor.h>
//#include <Servo.h>


#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

#define LED_BLUE 13  
//#define HALL_MARKER_SAMPLE_INTERVAL 10  //ms
//#define MOTOR_MOVE_TIME HALL_MARKER_SAMPLE_INTERVAL*200 //ms - MUST BE AN MULTIPLE OF HALL_MARKER_SAMPLE_INTERVAL

// TODO: ******** NEED TO HAVE A LOWER THRESHOLD FOR OTHER POLARITY OF MAGNET!!!!!!!! (will be lower)  **********
//#define HALL_DETECT_THRESHOLD 500 

//#define CHASSIS4P0V1
//#define CHASSIS5P0V1

//#ifdef CHASSIS4P0V1
//Configure drive motor for 4.0V1 chassis or 5.0V1 chassis
//AF_DCMotor driveMotor(1, MOTOR12_64KHZ); //For chassis version 4.0V1
//#endif

//#ifdef CHASSIS5P0V1
//AF_DCMotor driveMotor(3, MOTOR12_64KHZ);//For chassis version 5.0V1
//#endif

char ssid[] = "guest-SDUHSD"; //  your network SSID (name)
char pass[] = "guest-SDUHSD";    // your network password (use for WPA, or use as key for WEP)  
int keyIndex = 0;            // your network key Index number (needed only for WEP)

char mqtt_server = "m15.cloudmqtt.com:14269";


int status = WL_IDLE_STATUS;
long lastMsg = 0;
char msg[50];
int value = 0;
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.

WiFiClient wifiClient;
PubSubClient client(wifiClient);

//Servo variables
/*
Servo XYservo;
int XYservoPosition = 0;
Servo Zservo;
int ZservoPosition = 90;

//Hall effect sensor variables
int  hallSensorPin  =  A7;    // Hall effect analog input pin
int  hallSensorValue =  0;    // hall effect variable
///Setup bools so know what direction last traveled when detected Hall marker
//TODO:  make this persistent over power cycles
bool foundHallMarkerOnFwdMove = false;
bool foundHallMarkerOnRevMove = false;

*/
void setup_wifi(){
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

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

void callback(char* topic, byte* payload, unsigned int length) {
  char payloadChar[length];
  Serial.println("Received message length:"); Serial.print(length);
  Serial.println("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadChar[i] = (char)payload[i];
  }
  /*
  if(topic == "Commands"){
    if(payload == "left"){
      driveMotor.setSpeed(50);
      driveMotor.run(FORWARD);
    }else if(payload == "right"){
      driveMotor.setSpeed(50);
      driveMotor.run(BACKWARD);
    }else{
      driveMotor.run(RELEASE);
    }
  } 

  if(strcmp(topic, "Commands")==0){
      Serial.println(payloadChar);
      if(strcmp(payloadChar, "right")==0){  //right is forward
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
      
      if(strcmp(payloadChar, "left")==0){  //left is reverse
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
      
  } //End Commands topic
  
  Serial.println();
*/
}

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

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer("m15.cloudmqtt.com", 14269);
  client.setCallback(callback);

  //XYservo.attach(9);  //Corresponds to SERVO_2 on motor shield
  //XYservo.write(XYservoPosition);
/*  Zservo.attach(10);  //Corresponds to SER1 on motor shield
  Zservo.write(ZservoPosition);  */
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
/*
  hallSensorValue =  analogRead(hallSensorPin);
  Serial.print("Hall value (loop): ");  Serial.println(hallSensorValue);

  //Rotate the servos for the 3-axis phone holder
  
  Serial.println("Rotating servo postive");
  for (XYservoPosition = 0; XYservoPosition <=180; XYservoPosition++){
     XYservo.write(XYservoPosition);
     delay(10);
  }
  Serial.println("Rotating servo negative");
  for (XYservoPosition = 180; XYservoPosition >=0; XYservoPosition--){
     XYservo.write(XYservoPosition);
     delay(10);
     
  }
*/
  
  client.loop();

//  long now = millis();
//  if (now - lastMsg > 2000) {
//    lastMsg = now;
//    ++value;
//    snprintf (msg, 75, "hello world", value);
//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    client.publish("Output", msg);
//  }

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
*/

