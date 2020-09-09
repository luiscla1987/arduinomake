//Adiciona bibliotecas
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <UniversalTelegramBot.h>
#define BOTtoken ""//Define o Token do *seu* BOT

//motor
#include <Stepper.h> 

//temperatura
#include <OneWire.h>
OneWire  ds(2);  // on pin 10 (a 4.7K resistor is necessary)

//Colocar a API Key para escrita neste campo
//Ela é fornecida no canal que foi criado na aba API Keys

unsigned long lastConnectionTime = 0; 
const unsigned long postingInterval = 20L * 1000L; // Post data every 20 seconds.

const char* server = "mqtt.thingspeak.com"; 
char mqttUserName[] = "";  // Can be any name.
char mqttPass[] = "";  // Change this your MQTT API Key from Account > MyProfile.
char writeAPIKey[] = "";    // Change to your channel Write API Key.
long channelID = ;

static const char alphanum[] ="0123456789"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz";  // For random generation of client ID.

WiFiClient client;

PubSubClient mqttClient(client); // Initialize the PuBSubClient library.

WiFiClientSecure client2;
UniversalTelegramBot bot(BOTtoken, client2);

String id, text;//Váriaveis para armazenamento do ID e TEXTO gerado pelo Usuario
unsigned long tempo;

void readTel()//Funçao que faz a leitura do Telegram.

{
   int newmsg = bot.getUpdates(bot.last_message_received + 1);

   for (int i = 0; i < newmsg; i++)//Caso haja X mensagens novas, fara este loop X Vezes.
   {
      id = bot.messages[i].chat_id;//Armazenara o ID do Usuario à Váriavel.
      text = bot.messages[i].text;//Armazenara o TEXTO do Usuario à Váriavel.
      text.toUpperCase();//Converte a STRING_TEXT inteiramente em Maiuscúla.

    if (text == "/START")                                  // comando comeca
    {
      String welcome = "Bem-vindo ao IoTank, aqui poderá interagir com seu aquário!\n";
      welcome += "Esses são os controles que poderão ser utilizados:.\n\n";
      welcome += "/temp : para saber a temperatura atual do aquário\n";
      welcome += "/ph : para saber o nível de pH atual do aquário\n";
      welcome += "/Status : para informações gerais.\n";
      bot.sendMessage(id, welcome, "Markdown");
    }
     if (text == "/TEMP")                                  // comando comeca
    {
      String welcome = "Temperatura lida: \n";
      String tmp = String(VerificarTemperatura());
      welcome += tmp + " Celsius.\n\n";      
      bot.sendMessage(id, welcome, "Markdown");
   }
  if (text == "/PH")                                  // comando comeca
    {
      String welcome = "pH lido: \n";
      String pH = String (VerificarPH());
      welcome += pH;
      bot.sendMessage(id, welcome, "Markdown");
  }
  if (text == "/STATUS")                                  // comando comeca
    {
      String welcome = "Informações Gerais do aquário:\n\n";
      String tmp = String(VerificarTemperatura());
      String pH = String (VerificarPH());
      welcome += tmp + " Celsius.\n\n" + pH + " pH\n\n";
      bot.sendMessage(id, welcome, "Markdown");
    }
   }
}


//PH
// Constants:-
const byte pHpin = A0;// Connect the sensor's Po output to analogue pin 0.
// Variables:-
float Po;

//Configura rede SSID e SENHA
const char* ssid = "SUA_REDE";
const char* password =  "SUA_SENHA";

//adiciona váriaveis globais
const int stepsPerRevolution = 500; 
//ligacao ao motor 
Stepper myStepper(stepsPerRevolution, D1,D2,D5,D6); 

WiFiUDP ntpUDP;
 
int16_t utc = -3; //UTC -3:00 Brazil
uint32_t currentMillis = 0;
uint32_t previousMillis = 0;
 
NTPClient timeClient(ntpUDP, "a.st1.ntp.br", utc*3600, 60000);

//método para conectar ao Wi-Fi
void ConectaWifi()
{
    Serial.println("Método para conectar ao Wi-Fi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.println("Connecting to WiFi..");
    }
     Serial.println("Connected to the WiFi network");
     Serial.println(WiFi.localIP());
}

float VerificarTemperatura()
{
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    float celsius;

    //Serial.println("Método para temperatura");
   if ( !ds.search(addr)) {
      Serial.println("No more addresses.");
     // Serial.println();
      //ds.reset_search();
      //delay(250);
      return celsius;
    }
    
    Serial.print("ROM =");
    for( i = 0; i < 8; i++) {
      //Serial.write(' ');
      //Serial.print(addr[i], HEX);
    }
  
    if (OneWire::crc8(addr, 7) != addr[7]) {
        //Serial.println("CRC is not valid!");
        return celsius;
    }
    //Serial.println();
   
    // the first ROM byte indicates which chip
    switch (addr[0]) {
      case 0x10:
        //Serial.println("  Chip = DS18S20");  // or old DS1820
        type_s = 1;
        break;
      case 0x28:
        //Serial.println("  Chip = DS18B20");
        type_s = 0;
        break;
      case 0x22:
        //Serial.println("  Chip = DS1822");
        type_s = 0;
        break;
      default:
        //Serial.println("Device is not a DS18x20 family device.");
        break;
    } 
  
    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);        // start conversion, with parasite power on at the end
    
    delay(1000);     // maybe 750ms is enough, maybe not
    // we might do a ds.depower() here, but the reset will take care of it.
    
    present = ds.reset();
    ds.select(addr);    
    ds.write(0xBE);         // Read Scratchpad
  
    
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
      
    }
    
 
    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    } else {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    celsius = (float)raw / 16.0;
    Serial.print("Temperatura = ");
    Serial.print(celsius);
    Serial.print(" Celsius");

    if(celsius > 0.99){
      
    if (celsius > 25.00){
      Serial.print("\n#####ATENÇÃO! Temperatura da água ACIMA dos padrões!#####");
      bot.sendMessage(id, "ATENÇÃO! Temperatura da água do seu aquário ACIMA dos padrões. Recomenda-se ajuste imediato!", "");//Envia uma Mensagem para a pessoa que enviou o Comando.
      }
    if (celsius < 18.00){
      Serial.print("\n#####ATENÇÃO! Temperatura da água ABAIXO dos padrões!#####");
      bot.sendMessage(id, "ATENÇÃO! Temperatura da água do seu aquário ABAIXO dos padrões. Recomenda-se ajuste imediato!", "");//Envia uma Mensagem para a pessoa que enviou o Comando.
    }
    if (celsius < 25.00 && celsius > 18.00){
      Serial.print("\nTemperatura da água DENTRO dos padrões.");
    }
    }
  return celsius;
}

float VerificarPH()
{
  Serial.println("\nMétodo para pH: ");
  Po = (analogRead(pHpin)/59)-10;
  
  Serial.println(Po, 2);// Print the result in the serial monitor.
  if (Po > 8.50){
      Serial.print("\n*****ATENÇÃO! pH da água ACIMA dos padrões!*****");
      bot.sendMessage(id, "ATENÇÃO!(pH Alcalino) pH da água do seu aquário está ACIMA dos padrões. Recomenda-se ajuste imediato!", "");//Envia uma Mensagem para a pessoa que enviou o Comando.
    }
    if (Po < 6.50){
      Serial.print("\n*****ATENÇÃO! pH da água ABAIXO dos padrões!*****");
      bot.sendMessage(id, "ATENÇÃO!(pH Ácido) pH da água do seu aquário está ABAIXO dos padrões. Recomenda-se ajuste imediato! ", "");//Envia uma Mensagem para a pessoa que enviou o Comando.
    }
      if(Po < 8.50 && Po > 6.50){
      Serial.print("\npH DENTRO dos padrões.");
      }
  return Po;
}


//método onde verifica o tempo
void VerificarTempo()
{
  //criar if quando chegar a hora de alimentar o peixe chame o método alimentar
  forceUpdate();
  String atual;
  String hora="22:09:00";
  String fim = "22:11:30";
   
  atual=timeClient.getFormattedTime();
  if (hora <= atual){
    while (atual<=fim){
      AlimentarPeixe();
      forceUpdate();
      atual=timeClient.getFormattedTime();
    }
  }
}

//método onde faz o motor de passo funcionar e alimentar os peixes
void AlimentarPeixe()
{
  //Gira o motor
   Serial.println("método para STEPPER");
   Serial.println("Contando...");
   Serial.println(timeClient.getFormattedTime());
   myStepper.step(360); 
   delay(0);
}

//configura os módulos
void setup() {
    
  //Configura monitor serial*
  Serial.begin(9600);
  myStepper.setSpeed(50); 
  timeClient.begin();
  timeClient.update();
  //chama método que conecta Wi-Fi*
  ConectaWifi();
  mqttClient.setServer(server, 1883);   // Set the MQTT broker details.
}
void forceUpdate(void) {
  timeClient.forceUpdate();
}

void checkOST(void) {
  timeClient.update();
  currentMillis = millis();//Tempo atual em ms
  //Lógica de verificação do tempo
  if (currentMillis - previousMillis > 1000) {
    previousMillis = currentMillis;    // Salva o tempo atual
    //printf("Time Epoch: %d: ", timeClient.getEpochTime());
    Serial.println("\nHora atual: ");
    Serial.println(timeClient.getFormattedTime());
  }
}

void reconnect() 
{
  char clientID[10];

  // Loop until we're reconnected
  while (!mqttClient.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Generate ClientID
    for (int i = 0; i < 8; i++) {
        clientID[i] = alphanum[random(51)];
    }
    clientID[8]='\0';

    // Connect to the MQTT broker
    if (mqttClient.connect(clientID,mqttUserName,mqttPass)) 
    {
      Serial.println("connected");
    } else 
    {
      Serial.print("failed, rc=");
      // Print to know why the connection failed.
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqttpublish(float tmp, float pH) {
  
  // Create data string to send to ThingSpeak
  String data = String("field1=" + String(tmp) + "&field2=" + String(pH));
  int length = data.length();
  char msgBuffer[length];
  data.toCharArray(msgBuffer,length+1);
  Serial.println(msgBuffer);
  
  // Create a topic string and publish data to ThingSpeak channel feed. 
  String topicString ="channels/" + String( channelID ) + "/publish/"+String(writeAPIKey);
  length=topicString.length();
  char topicBuffer[length];
  topicString.toCharArray(topicBuffer,length+1);
 
  mqttClient.publish( topicBuffer, msgBuffer );

  lastConnectionTime = millis();
}
void loop() {
  // chama método verifica tempo
  //Leitura de umidade
  float tmp = VerificarTemperatura();
  //Leitura de temperatura
  float pH = VerificarPH();
  //Se não for um numero retorna erro de leitura
  if (isnan(tmp) || isnan(pH)) {
    Serial.println("Erro ao ler o sensor!");
    //return;
  }
  checkOST();
  VerificarTempo();
  VerificarTemperatura();
  readTel();
   // Reconnect if MQTT client is not connected.
  if (!mqttClient.connected()) 
  {
    reconnect();
  }

  mqttClient.loop();   // Call the loop continuously to establish connection to the server.

  // If interval time has passed since the last connection, Publish data to ThingSpeak
  if (millis() - lastConnectionTime > postingInterval) 
  {
    if (tmp != 0.0){
    mqttpublish(tmp, pH);
  }  
}
}
