#include <Arduino.h>        
#include <ESP8266WiFi.h>      //WiFi connection
#include <PubSubClient.h>     //mqtt broker connection
#include "DHT.h"              //temp Sensor


//#####################################################################
//insert your environment here
//#####################################################################

//define WLAN connection
#define SID "GonzosNet"
#define PW "J+F16102010"

//define mqtt broker
# define mqtt_broker "192.168.178.100"
# define mqtt_port 1883 
# define mqtt_user "wohnen"
# define mqtt_password "d3HV7BM_G-Qo*@AR!Brg"
# define mqtt_client "VentilationUnit" 
# define mqtt_main_pub_topic "home/VentilationUnit"
# define mqtt_main_control_sub_topic "home/VentilationUnit/control/#"


//PIN for relais
#define RELAIS4 5         //Pin on board 5 is D1
#define RELAIS3 4         //Pin on board 4 is D2
#define RELAIS2 0         //Pin on board 0 is D3
#define RELAIS1 2         //Pin on board 2 is D4


//#####################################################################
//#####################################################################
//#####################################################################

//define payload var
char msgBuffer[20];

//default metric delay, how often send the getted metrics to the backend, im milliseconds
int metric_send_interval = 300000;
int metric_send_next_time = 0;

//
int ventilation_value;

//set
WiFiClient espClient;
PubSubClient client(espClient);


void connect_to_wlan() {
  WiFi.begin(SID, PW);
  //wait for success wlan connection
  Serial.print("Connecting to WLAN");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void connect_to_mqtt_broker() {
  client.setServer(mqtt_broker, mqtt_port);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqtt_client, mqtt_user, mqtt_password )) {
      Serial.println("connected to mqtt broker");

      Serial.println("subscribe to topics");
      subscribe_control_topics();

    } else {
      Serial.print("failed connect to mqtt broker with state ");
      Serial.print(client.state());
      Serial.println("try to reconnect");
      delay(2000);
    }
  }
}

void subscribe_control_topics() {
  Serial.println("subscribe mqtt topic");
  client.subscribe(mqtt_main_control_sub_topic, 1);
  client.setCallback(callback);
}

void send_data_to_mqtt_broker(const char* subtopic, const char* payload ,bool retain = false) {

  char* concate_topic = new char[strlen(mqtt_main_pub_topic)+strlen(subtopic)+1];
  strcpy(concate_topic,mqtt_main_pub_topic);
  strcat(concate_topic,subtopic);


  //check if connection is established
  if (!client.connected()){
    connect_to_mqtt_broker();
    Serial.print("send data over MQTT after reconnect");
    client.publish(concate_topic, payload, retain);
  }
  else {
    Serial.print("send data over MQTT");   
    client.publish(concate_topic, payload, retain);
  }
  
}

void send_metrics(){
  
  float runtime = millis();

  if (runtime >= metric_send_next_time) {
    
    metric_send_next_time = runtime + metric_send_interval;
   }
}

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived from topic '");
  Serial.print(topic);
  Serial.println("'");

  if (strcmp(topic,"home/greenhouse/control/ventilation_value") == 0) {
        
    Serial.println("new tempthreshold: ");
    ventilation_value = atoi((char*)payload);
    Serial.println(ventilation_value);
    Serial.println("\r\n");
    set_ventilation_value(ventilation_value);    
    send_data_to_mqtt_broker("/settings/ventilation_value", dtostrf(ventilation_value,2,2,msgBuffer),true);

  }

}

void set_ventilation_value(int vent_level){

  if (ventilation_value == 0){
    //vetilation off
    digitalWrite(RELAIS4,HIGH);
    digitalWrite(RELAIS3,HIGH);
    digitalWrite(RELAIS2,HIGH);
    digitalWrite(RELAIS1,LOW);
  }
  if (ventilation_value == 1){
    //vetilation 90qm3
    digitalWrite(RELAIS4,HIGH);
    digitalWrite(RELAIS3,HIGH);
    digitalWrite(RELAIS1,HIGH);
    digitalWrite(RELAIS2,LOW);
  }
  if (ventilation_value == 2){
    //120qm3
    digitalWrite(RELAIS4,HIGH);
    digitalWrite(RELAIS2,HIGH);
    digitalWrite(RELAIS1,HIGH);
    digitalWrite(RELAIS3,LOW);
  }
  if (ventilation_value == 3){
    //180qm3
    digitalWrite(RELAIS2,HIGH);
    digitalWrite(RELAIS3,HIGH);
    digitalWrite(RELAIS1,HIGH);
    digitalWrite(RELAIS4,LOW);
  } 

}

void send_all_default_values(){

  send_data_to_mqtt_broker("/settings/metric_send_interval", dtostrf(metric_send_interval,2,2,msgBuffer),true);

}

void setup() {
  // baud rate
  Serial.begin(115200);
  
  connect_to_wlan();
  
  connect_to_mqtt_broker();
  
  //Onboard RELAISPIN port Direction output
  pinMode(RELAIS4,OUTPUT); 
  pinMode(RELAIS3,OUTPUT); 
  pinMode(RELAIS2,OUTPUT); 
  pinMode(RELAIS1,OUTPUT); 
  
  //Powerstate off
  digitalWrite(RELAIS4,HIGH);
  digitalWrite(RELAIS3,HIGH);
  digitalWrite(RELAIS2,HIGH);
  digitalWrite(RELAIS1,HIGH);

  //send all init/default values to brker
  send_all_default_values();

}

void loop() {
    
  //send all defined metrics
  send_metrics();
    
  //MQTT subscribed client loop
  client.loop();  
}
