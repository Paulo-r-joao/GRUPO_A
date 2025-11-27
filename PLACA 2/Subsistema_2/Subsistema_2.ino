//Nome: Bruno Davi Dutka
//Turma: TDESI-V1
//Grupo: Bruno, Paulo e Vinicius Angelo

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DHT.h"
#include <Adafruit_NeoPixel.h>

#define SCREEN_WIDTH 128  //Largura da tela em pixels
#define SCREEN_HEIGHT 64  //Altura da tela em pixels

#define I2C_SCK 6 //ou SLC
#define I2C_SDA 5 

#define PIN 8
#define NUMPIXELS 1

#define DHTPIN 19     
#define DHTTYPE DHT11   
DHT dht(DHTPIN, DHTTYPE);

Adafruit_SSD1306 tela(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

const String SSID = "Paulo";
const String PSW = "cxos1234";

const String brokerURL = "114fab7ddbdc46f4a5582af51a52f53f.s1.eu.hivemq.cloud"; //URL do broker    (servidor)
const int porta = 8883;                        //Porta do broker  (servidor)
const char* brokerUser = "BrunoPaulo1";
const char* brokerPass = "BrunoPaulo1";

const char* placaonoff = "placas/on/off/bruno";
const char* brunoRecebeStatus =   "bruno/topic/placa2/recebe/status";
const char* brunoRecebeOcupacao = "bruno/topic/placa2/recebe/ocupacao";
const char* brunoEnvia =    "bruno/topic/placa2/envia";
const char* Message_LWT = "Offline";
const int QoS_LWT = 1;
const bool Retain_LWT = true;

String status = "------";
String ocupacao = "--/--";

//Carrinhos/Cheios/1

JsonDocument doc;

WiFiUDP ntpUDP;
WiFiClientSecure espClient;           //Criando Cliente WiFi
PubSubClient mqttClient(espClient);   //Criando Cliente MQTT

// timeClient.getFormattedTime(); -- Para salvar um horário em uma variável String

NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);

void conexaoWifi();
void conexaoBroker();
void reconexaoWifi();
void reconexaoBroker();
void mandarMensagem();
void printTelaRespostaOn();
void printTelaRespostaOff();


void setup() {
  espClient.setInsecure();
  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
  Wire.begin(I2C_SDA, I2C_SCK);  //Inicia comunicação I2C
  tela.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  tela.clearDisplay();
  tela.setTextSize(1);
  tela.setTextColor(SSD1306_WHITE);
  tela.display();
  delay(2000);
  Serial.begin(115200);
  conexaoWifi();
  conexaoBroker();
  tela.clearDisplay();
  tela.display();
  timeClient.begin();
  timeClient.setTimeOffset(13500);
  dht.begin();
}

void loop() {
  int red = 0;
  mqttClient.loop();
  reconexaoWifi();
  reconexaoBroker();
  enviarDados();
  printTelaRespostaOn(ocupacao, status);
  led_rgb();
}

void conexaoWifi(){
  Serial.println("Iniciando conexao com rede WiFi");
  WiFi.begin(SSID, PSW);
  while(WiFi.status() != WL_CONNECTED){
    printTelaRespostaOff();
    Serial.println("...");
    delay(10000);
  }
  Serial.println("Rede de Wi-Fi conectada");
}

void reconexaoWifi(){
  if(WiFi.status() != WL_CONNECTED){
    printTelaRespostaOff();
    Serial.println("Conexão de rede perdida!!!");
    conexaoWifi();
  }
}

void conexaoBroker(){
  Serial.println("Conectando ao broker...");
  printTelaRespostaOff();
  mqttClient.setServer(brokerURL.c_str(), porta);
  String userId = "ESP_BRUNO";
  userId += String(random(0xffff), HEX);

  StaticJsonDocument<200> doc;
  
  doc["status"] = Message_LWT;
  char buffer[200];
  serializeJson(doc, buffer);

  while(!mqttClient.connected()){
    mqttClient.connect(userId.c_str(), brokerUser, brokerPass, placaonoff, QoS_LWT, Retain_LWT, buffer);
  }

  doc["status"] = "Online";
  serializeJson(doc, buffer);

  mqttClient.publish(placaonoff, buffer, Retain_LWT);
  mqttClient.subscribe(brunoRecebeStatus, QoS_LWT);
  mqttClient.subscribe(brunoRecebeOcupacao, QoS_LWT);
  mqttClient.setCallback(callback);
  Serial.println("Conectado com sucesso!");
  delay(1500);
}

void callback(char* topic, byte* payload, unsigned long length){
  String resposta = "";
  for(int i = 0;  i < length; i++){
    resposta += (char) payload[i];
  }

  DeserializationError error = deserializeJson(doc, resposta);
  
  if(!error){
    if(String(topic) == brunoRecebeStatus){
      status = doc["status"].as<String>();
      Serial.println(status);
    }
    else if(String(topic) == brunoRecebeOcupacao){
      ocupacao = doc["ocupacao"].as<String>();
    }
  }
  else if(error){
    Serial.println("Deu ruim");
  }
}

void printTelaRespostaOn(String ocupa, String status){
  tela.clearDisplay();
  tela.setCursor(0, 0);
  tela.println("---------------------");
  tela.println("Ocupacao atual");
  tela.println(ocupa);
  tela.println("---------------------");
  tela.println("Status da sala");
  tela.println(status);
  tela.println("---------------------");
  tela.display();
  // Serial.println(ocupa);
  // Serial.println(status);
}

void printTelaRespostaOff(){
  tela.clearDisplay();
  tela.setCursor(0, 0);
  tela.println("---------------------");
  tela.println("Ocupacao atual");
  tela.println("--/--");
  tela.println("---------------------");
  tela.println("Status da sala");
  tela.println("------");
  tela.println("---------------------");
  tela.display();
}

void reconexaoBroker(){
  if(!mqttClient.connected()){
    printTelaRespostaOff();
    Serial.println("Conexão com o broker perdida!!!");
    conexaoBroker();
  }
}

void enviarDados(){
  int temperatura = dht.readTemperature();
  int humidade = dht.readHumidity();

  StaticJsonDocument<200> doc;
  doc["temp"] = temperatura; 
  doc["humi"] = humidade; 

  char men[200];
  serializeJson(doc, men);
  mqttClient.publish(brunoEnvia, men, Retain_LWT);
}

void led_rgb(){
  int r = 255;
  int g1 = 255;
  int g2 = 107;
  if(status == "Livre"){
    pixels.setPixelColor(0, pixels.Color(0, g1, 0));   //código rgb(0, 255, 0) para verde 
    pixels.show();
  }
  else if(status == "Enchendo"){
    pixels.setPixelColor(0, pixels.Color(r, g1, 0));   //código rgb(255, 255, 0) para amarelo
    pixels.show();
  }
  else if(status == "Meio Cheio"){
    pixels.setPixelColor(0, pixels.Color(r, g2, 0));   //código rgb(255, 140, 0) para laranja
    pixels.show();
  }
  else if(status == "Quase Cheio"){
    pixels.setPixelColor(0, pixels.Color(r, 0, 0));   //código rgb(255, 0, 0) para vermelho
    pixels.show();
  }
  else if(status == "Lotado"){
    pixels.setPixelColor(0, pixels.Color(139, 0, 0));
    pixels.show();
    delay(250);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    delay(250);
  }
  else{
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
  }
}