#include <WiFi.h>
#include <PubSubClient.h>

#define WIFISSID ""                       // This is where the Wi-Fi address can be stored, so a user inputted address cna change this definition
#define PASSWORD ""                    // This is where the corrosponding password will be stored
#define TOKEN ""// This is where the unique token for Ubidots needs to be inputed to allow the MQTT server to connect
#define MQTT_CLIENT_NAME ""            // This can be any random 12 digit ASCII code                                           
#define VARIABLE_LABEL "Sensor"                    // This is the label for the sensor 
#define DEVICE_LABEL "ECG"                         // This is the label for the device
#define SENSOR A0

const int numReadings = 3;      //This will keep a buffer of 2 samples at one time

int readings[numReadings];      // The readings from the analog input
int readIndex = 0;              // The index of the current reading
int total = 0;                  // The running total

float average;                  // The average

char mqttBroker[]  = "industrial.api.ubidots.com";  //This will be the broker to which the MQTT will communicate
char payload[100];                                  //The payload is the information which is being sent to the client 
char info[150];                                     //This will relay the information on the device  
char str_ecg[10];                                   //This string is the heart rate signal which will link to the information read from the sensor

WiFiClient ubidots;                                              //This is assigning the client the code is connecting to, in this case it is ubidots
PubSubClient client(ubidots);

void callback(char* info, byte* payload, unsigned int length)    //This function will send the payload which will relay the information on the device 
             
{                           
  char i[length + 1];                                            //This will assign variable "i" to the index of the required length which can change on each itteration  
  memcpy(i, payload, length);                                    //This is a memory copy variable which will copy "i" to "payload" with "length" determining how much to copy
  i[length] = NULL;                                              //This will reset "i" allowing an empty variable for the next itteration 
  Serial.write(payload, length);                                 //This will write the payload to the length of variable "length"
  Serial.println(info);                                          //This will relay the info back to the user 
}

void reconnect()                                                     //The function will make sure the ECG is always connected to the MQTT Server
{
  while (!client.connected())                                        //This means that the client is not connected leading to two different outcomes
  {
    Serial.println("Attempting to connect to the MQTT Server...");   
    
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, ""))                 //This will attempt to connect to the MQTT Server matching the client name against the token provided by Ubidots
    {
      Serial.println("Connected");
    }
    else 
    {
      Serial.print("Failed connecting to the MQTT Server");
      Serial.print(client.state());                                  //This will show the error message showing the problem with the client 
      Serial.println("Please try again in 2 seconds");
    }
  }
}

void setup() 
{
            
  Serial.begin(9600);                                           // This will initialize the serial communication:
  pinMode(SENSOR,INPUT);
  pinMode(37, INPUT);                                           // Setup for leads off detection LO +
  pinMode(36, INPUT);                                           // Setup for leads off detection LO -
  WiFi.begin(WIFISSID,PASSWORD);                                // This will connect to the Wi-Fi address using the wifissid and password provided
  
  Serial.println();
  Serial.print("Waiting for WiFi...");
  
    for (int thisReading = 0; thisReading < numReadings; thisReading++) 
    {
    readings[thisReading] = 0;                                  //This will clear the buffer and make sure that data doesn't pile up
    }
    while (WiFi.status() != WL_CONNECTED)                       //As Wi-Fi can take some time to connect depending on internet speed, this will give it a 0.5 second delay before trying again
    {
    Serial.print(".");
    delay(1);
    }
  
  Serial.println("");
  Serial.println("WiFi Connected");                             //This will following on from wifi.begin and inform the user that the Wi-Fi has succesfully connected
  client.setServer(mqttBroker, 1883);                           //This will connect to the chosen server and the port 
  client.setCallback(callback);                                 //This will implement the callback function

}
void loop() {
  if (!client.connected())                                      //This will make sure the client is connected before the readings are taken
  {
    reconnect();
  }

  sprintf(info, "%s%s", "/v1.6/devices/", DEVICE_LABEL);        //This will add the device label to the graph 
  sprintf(payload, "%s", "");                                   //This will empty the payload and not allow the data to build up
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL);                 //This will end the variable label to the graph 
  
  if((digitalRead(37) == 1)||(digitalRead(36) == 1))
  {
    Serial.println('!'); //This will detect if the leads are taken off during the reading
  }
  else
  {

    total = total - readings[readIndex];     // This will subtract the last reading

    readings[readIndex] = analogRead(SENSOR);    // This will read from the sensor

    total = total + readings[readIndex];     // This wil add the reading to the total

    readIndex = readIndex + 1;               // This will move to the next position in the array


  if (readIndex >= numReadings) 
  {

    readIndex = 0;                           // This will loop the smoothing filter around again
  }

  average = total / numReadings;             // This will calculate the average as a mean

  
  dtostrf(average, 4, 2, str_ecg);           // This will convert a float to a character array as this is the format required for Ubidots

  sprintf(payload, "%s {\"value\": %s}}", payload, str_ecg);            // This will format all the data together 
  Serial.println("Sending data to Ubidots ");                   
  client.publish(info, payload);                                          // Once the payload has all the data along with the information variable it will publish the data to Ubidots
  client.loop();                                                             // Wait for a bit to keep serial data from saturating
  }                        
}
