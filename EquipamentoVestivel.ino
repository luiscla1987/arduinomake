//Incluir Bibliotecas
#include "TinyGPS++.h" 
#include "SoftwareSerial.h"
#include <ESP8266WiFi.h>
#include "ThingSpeak.h"

//define pinos sensor ultrassonico
const int trigPin = 2;  //D4
const int echoPin = 0;  //D3
long duration;
int distance;
int verificagps = 0;


//salvar dados
const char* host = "192.168.0.190";  

unsigned long myChannelNumber = ;
const char * myWriteAPIKey = "";

WiFiClient  client;



const char* ssid = "SUA_REDE";
const char* password =  "SUA_SENHA@";

//Configura posição casa
const double HOME_LAT = "LATITUDE";                          // Enter Your Latitude and Longitude here
const double HOME_LNG = "LONGITUDE";                         // to track how far away the "Dog" is away from Home 

SoftwareSerial serial_connection(12, 13); //TX=pin 12-D6, RX=pin 13-D7 - RX/TX da placa GPS
TinyGPSPlus gps;//This is the GPS object that will pretty much do all the grunt work with the NMEA data

void ConectaWifi()
{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.println("Connecting to WiFi..");
    }
     Serial.println("Connected to the WiFi network");
     Serial.println(WiFi.localIP());
}

void setup()  {
   
   Serial.begin(9600);//This opens up communications to the Serial monitor in the Arduino IDE
   serial_connection.begin(9600);//This opens up communications to the GPS
   Serial.println("GPS Start");//Just show to the monitor that the sketch has started
   pinMode(trigPin, OUTPUT); // Sets o trigPin as an Output
   pinMode(echoPin, INPUT); // Sets o echoPin as an Input
   ThingSpeak.begin(client);
   ConectaWifi();
}

void displayInfo()
{
  if (gps.location.isValid())
  {

    double latitude = (gps.location.lat());
    double longitude = (gps.location.lng());
    
    String latbuf;
    latbuf += (String(latitude, 6));
    Serial.println(latbuf);

    String lonbuf;
    lonbuf += (String(longitude, 6));
    Serial.println(lonbuf);

    ThingSpeak.setField(1, latbuf);
    ThingSpeak.setField(2, lonbuf);
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);  
    //delay(20000);
    if(verificagps == 1)
    {
        tone(14, 31); //Pino 14-D5, Frequencia
        delay(1000);
        noTone(14);
        verificagps = 2;        
    }
  }
  else
  {
    Serial.print(F("INVALID"));
    if(verificagps == 0)
    {
        tone(14, 31); //Pino 14-D5, Frequencia
        delay(1000);
        noTone(14);
        verificagps = 1;
    }
    //tone(14, 31); //Pino 14-D5, Frequencia
   // delay(150);
  }

 
  Serial.println();
    
}

void loop()  {
 
while (serial_connection.available() > 0)
    if (gps.encode(serial_connection.read()))
        displayInfo();
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
  loop2();
   //smartDelay(500);  
}

void loop2()
{
   // Clears the trigPin
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    
    // Calculating the distance
    distance= duration*0.034/2;
    // Prints the distance on the Serial Monitor
    Serial.print("Distancia: ");
    Serial.println(distance);
    VerificaDistancia();
    //delay(500);
}
void VerificaDistancia()
{
  if(distance < 80 and distance != 0)
  {
    AvisoObs();
    VerificaGPS();
  }
  
}
 
void VerificaGPS()
{
    float Distance_To_Home;
    Serial.println("Satellite Count Gravados:");
    Serial.println(gps.satellites.value());
    Serial.println("Latitude Gravada:");
    Serial.println(gps.location.lat(), 6);
    Serial.println("Longitude Gravada:");
    Serial.println(gps.location.lng(), 6);
    Serial.println("Speed KM/h Gravda:");
    Serial.println(gps.speed.kmph());
    Serial.println("Altitude Feet Gravada:");
    Serial.println(gps.altitude.feet());
    Distance_To_Home = (unsigned long)TinyGPSPlus::distanceBetween(gps.location.lat(),gps.location.lng(),HOME_LAT, HOME_LNG);  //Query Tiny GPS to Calculate Distance to Home
    Serial.println("Distancia de casa:");
    Serial.println(Distance_To_Home );
    Serial.println("");
    GravaDados();
         
}

void GravaDados()
{
  if(verificagps == 2)
  { 
      Serial.print("Conectando com: ");
      Serial.println(host);
      
      // Use WiFiClient class to create TCP connections
      WiFiClient client;
      const int httpPort = 80;
      if (!client.connect(host, httpPort)) {
        Serial.println("Falha na conexão");
        return;
      }
      
      // We now create a URI for the request
      String url = "/gps/salva.php?";
            url += "latitude=";
            url += String(gps.location.lat(), 8);
            url += "&longitude=";
            url += String(gps.location.lng(), 8);
            
      
      Serial.print("Requesitando URL: ");
      Serial.println(url);
      
      // This will send the request to the server
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" + 
                   "Connection: close\r\n\r\n");
      unsigned long timeout = millis();
      while (client.available() == 0) {
        if (millis() - timeout > 5000) {
          Serial.println(">>> Client Timeout !");
          client.stop();
          return;
        }
      }
      
      // Read all the lines of the reply from server and print them to Serial
      while(client.available()){
        String line = client.readStringUntil('\r');
        //Serial.print(line);
        Serial.println();
        if(line.indexOf("salvo_com_sucesso") != -1)
        {
          Serial.println("Salvo com sucesso.");  
        } else if(line.indexOf("erro_ao_salvar") != -1)
          {
             Serial.println("Erro ao salvar."); 
          }
        
      }
      
      Serial.println();
      Serial.println("Conexão fechada");
  }
}

void AvisoObs()
{
   for (int i=0; i <= 2; i++)
      {
        Serial.println("Beep");
        tone(14, 31); //Pino 14-D5, Frequencia
        delay(250);
        noTone(14);  //Pino 14-D5
        delay(125);
      }
}
