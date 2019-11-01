
#include <CayenneMQTTESP8266.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include<SoftwareSerial.h>
SoftwareSerial SUART(4, 5); //SRX=Dpin-D2; STX-DPin-D1
ESP8266WebServer server1(80);

const char* apiKey = "YO36SD2CFLQY7FNZ";

const char* resource = "/update?api_key=";

const char* server = "api.thingspeak.com";

//Variable where serial from Arduino will be save on the NodeMCU
short int temperatura = 0;

//Limits for serial strings
const char startOfNumberDelimiter = '<';
const char endOfNumberDelimiter   = '>';

const short int ENA = 14;
const short int IN1 = 0;
const short int IN2 = 2;

short int flag1 = 0;
unsigned short int slider = 0;

const unsigned short int pot = A0;
const unsigned short int b = 12;
float datos;
int modo = 0;

//Cayenne login data for sending information
char username[] = "274413b0-fa63-11e9-84bb-8f71124cfdfb";
char mqtt_password[] = "777dd444a16b510b1633a2d3db46419a6b6ea264";
char client_id[] = "b29c46c0-fa73-11e9-b49d-5f4b6757b1bf";


// Wifi credentials
const char* ssid     = "rasp";
const char* password = "s3rv1c10$";

// Temporary variables
static char temperatureTemp[7];
    
void setup() {
  
  //Initializing Cayenne for sending data...
  Cayenne.begin(username, mqtt_password, client_id, ssid, password);

  //Initializing serial ports with the MCU and Arduino
  Serial.begin(115200);
  SUART.begin(115200); 

  //Pin modes for motor
  pinMode(IN1,OUTPUT);
  pinMode(IN2,OUTPUT);
  pinMode(ENA,OUTPUT);

  //Turn off motors first
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  //Initializing the motors
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);  

  //Settings pin for input
  pinMode(pot, INPUT);
  pinMode(b, INPUT);

  //Initialize connection with the internet
  initWifi();
  initWEBSERVER();

}

void loop() {
  
if (SUART.available())
 {
   processInput();
 }

  if (digitalRead(b)){
          modo = modo + 1;
        }
        
        if((modo%2) != 0){
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          datos = analogRead(pot);
          analogWrite(ENA, datos);
          delay(1000);
        }
        
        else if ((modo%2) ==0){
        
            if (flag1 == 1){
              
              remoteMode1();
            }
            else{
              remoteMode2();
            }

           makeHTTPRequest();
           server1.handleClient();
           Cayenne.loop();
           
           delay(2000); 
        }         
}

CAYENNE_OUT(0)
{
  Cayenne.virtualWrite(V0, temperatura);
}

CAYENNE_IN(1)
{
   flag1 = getValue.asInt();   
}


CAYENNE_IN(2)
{
  slider = getValue.asInt();
}

void remoteMode2()
{ 
    Serial.println(temperatura);
    
      if (20 <= temperatura and temperatura  <= 22)
      {
              analogWrite(ENA,320);    
               delay(1000);        
      }
      else if (23 <= temperatura and temperatura  <= 24)
      {
              analogWrite(ENA,370);   
               delay(1000);
      }
      else if (23 <= temperatura and temperatura  <= 24)
      {
             analogWrite(ENA,400); 
              delay(1000);
      }
      else if (temperatura > 25)
      {
            analogWrite(ENA,500); 
            delay(1000);
      }
}

void remoteMode1()
{    
     analogWrite(ENA,slider);
}

void processNumber (const short int n)
 {
    temperatura = n;
 } 

void processInput ()
 {
 static long receivedNumber = 0;
 static boolean negative = false;
 
 byte c = SUART.read();
 
 switch (c)
   { 
   case endOfNumberDelimiter:  
     if (negative)
       processNumber (- receivedNumber);
     else
       processNumber (receivedNumber);
       

   // fall through to start a new number
   case startOfNumberDelimiter:
     receivedNumber = 0;
     negative = false;
     break;
     
   case '0' ... '9':
     receivedNumber *= 10;
     receivedNumber += c - '0';
     break;
     
   case '-':
     negative = true;
     break;
     
   }      
 }  

void makeHTTPRequest() {

  // Check if any reads failed and exit early (to try again).
  if (isnan(temperatura)) {
    Serial.println("Failed to read from DHT sensor!");
    strcpy(temperatureTemp,"Failed");
    return;    
  }
  else {
                if(temperatura >= 15 and temperatura <= 35)
              {
                  dtostrf(temperatura, 6, 2, temperatureTemp);
                  Serial.print("Connecting to "); 
                  Serial.print(server);
              
              WiFiClient client;
              int retries = 5;
              while(!!!client.connect(server, 80) && (retries-- > 0)) {
                Serial.print(".");
              }
              Serial.println();
              if(!!!client.connected()) {
                 Serial.println("Failed to connect, going back to sleep");
              }
              
              Serial.println("Request resource: "); 
              Serial.println(resource);
              client.print(String("GET ") + resource + apiKey + "&field1=" + temperatureTemp + " HTTP/1.1\r\n" + "Host: " + server + "\r\n" + "Connection: close\r\n\r\n");
                              
              int timeout = 5 * 10; // 5 seconds             
              while(!!!client.available() && (timeout-- > 0)){
                delay(100);
              }
              if(!!!client.available()) {
                 Serial.println("No response, going back to sleep");
              }
              while(client.available()){
                Serial.write(client.read());
              }
              
              client.stop();
              }
                
     }
  
}

void initWifi() {
  Serial.print("Connecting to: "); 
  Serial.print(ssid);
  WiFi.begin(ssid, password);  

  int timeout = 10 * 4; // 10 seconds
  while(WiFi.status() != WL_CONNECTED  && (timeout-- > 0)) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");

  if(WiFi.status() != WL_CONNECTED) {
     Serial.println("Failed to connect, going back to sleep");
  }

  Serial.print("WiFi connected in: "); 
  Serial.print(millis());
  Serial.print(", IP address: "); 
  Serial.println(WiFi.localIP());
}

void handle_OnConnect() {
  Serial.println("Bienvenido a la obtencion local de temperatura");
  server1.send(200, "text/html", SendHTML(temperatura)); 
}

void handle_NotFound(){
  server1.send(404, "text/plain", "Not found");
}

void initWEBSERVER()
{
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  server1.on("/", handle_OnConnect);
  server1.onNotFound(handle_NotFound);

  server1.begin();
  Serial.println("HTTP server started");
}

String SendHTML(unsigned short int prueba){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #1abc9c;}\n";
  ptr +=".button-on:active {background-color: #16a085;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Web Server para ESP8266</h1>\n";
  ptr +="<h3>Semana i</h3>\n";

  if(prueba == 20)
  {
     ptr +="<h3>La temperatura es de 20 C grados</h3>\n";
  }
  else if(prueba == 21)
  {
    ptr +="<h3>La temperatura es de 21 C grados</h3>\n";
  }
  else if(prueba == 22)
  {
    ptr +="<h3>La temperatura es de 22 C grados</h3>\n";
  }
  else if(prueba == 23)
  {
    ptr +="<h3>La temperatura es de 23 C grados</h3>\n";
  }
  else if(prueba == 24)
  {
    ptr +="<h3>La temperatura es de 24 C grados</h3>\n";
  }
  else if(prueba == 25)
  {
    ptr +="<p>La temperatura es de 25 C grados</p>\n";
  }
  else if(prueba == 26)
  {
    ptr +="<h3>La temperatura es de 26 C grados</h3>\n";
  }
  else if(prueba == 27)
  {
    ptr +="<h3>La temperatura es de 27 C grados</h3>\n";
  }
  else if(prueba == 28)
  {
    ptr +="<h3>La temperatura es de 28 C grados</h3>\n";
  }

  else if(prueba == 29)
  {
    ptr +="<h3>La temperatura es de 29 C grados</h3>\n";
  }
   else if(prueba == 30)
  {
    ptr +="<h3>La temperatura es de 30 C grados</h3>\n";
  }
  else
  {
    ptr +="<h3>No hay temperatura que mostrar</h3>\n";
  }
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
