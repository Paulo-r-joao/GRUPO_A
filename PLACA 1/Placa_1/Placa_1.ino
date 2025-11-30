#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

const String SSID = "Vini@@";
const String PSWD = "viniciusA";
const String brokerUrl = "114fab7ddbdc46f4a5582af51a52f53f.s1.eu.hivemq.cloud";             
const char* brokerUser = "Vinicius_Alves9" ;
const char* brokerPass = "Vinicius_Alves9" ;
const int port = 8883;                     
const char* topico_1 = "placas/on/off/vini";
const char* topico_2 = "vini/topic/placa1/envia";

// MQTT

const String LWTMessage = "Offline";
const int  LWTQoS = 1;
const bool Retain_LWT = true;
//bool estado_UL1_passou = false;
//bool estado_UL2_passou = false;
//LWT

  const byte trigg_pin_1 = 22;
  const byte echo_pin_1 = 18;
  // Ultra Sonico 1

  const byte trigg_pin_2 = 15;
  const byte echo_pin_2 = 23;
  // Ultra Sonico 2

unsigned long tempo_UL1 = 0;
unsigned long tempo_UL2 = 0;
//Ultra Sonicos
    const int N = 10;

    float bufferUL1[N];
    float bufferUL2[N];
    int idx1 = 0;
    int idx2 = 0;
    
    bool bufferInicializado = false;

WiFiClientSecure espClient;
//WiFi

PubSubClient mqttClient(espClient);
//MQTT

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org",  -10800, 60000);
//NTPClient

void connectLocalworks();
void connectBroker();
void PublishOnNodeRED();

void setup() {

Serial.begin(115200);
espClient.setInsecure();
connectLocalworks();
connectBroker();

Serial.println("Teste Serial OK!");

delay(100);

pinMode(trigg_pin_1, OUTPUT);
pinMode(echo_pin_1, INPUT);
//Ultra Sonico 1

Serial.println("Ultra Sonico 1 - OK");
delay(100);
pinMode(trigg_pin_2, OUTPUT);
pinMode(echo_pin_2, INPUT);
Serial.println("Ultra Sonico 2 - OK");
//Ultra Sonico 2

timeClient.begin();
// NPTClient

}

void loop() {


  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Conexão Perdida\n");
    connectLocalworks();
    
  }
  // Reconexão do WiFi
  if (!mqttClient.connected()) {
    Serial.println("Erro de Conexão no Broker");
    connectBroker();
  }
  // Reconexão do Broker
    float distancia_UL1 = 0;
    for(int i  = 0; i < 5; i++){                  
      digitalWrite(trigg_pin_1, LOW); 
      delayMicroseconds(10);
      digitalWrite(trigg_pin_1, HIGH); 
      delayMicroseconds(10);
      digitalWrite(trigg_pin_1, LOW); 
      delayMicroseconds(10);

      unsigned long duracao_UL1 = pulseIn(echo_pin_1, HIGH,30000);  
      distancia_UL1 += ((duracao_UL1 * 340)/2)/10000;
    }
    distancia_UL1 /= 5;
    // Ultra Sonico 1
    float distancia_UL2 = 0;      
    for(int i  = 0; i < 5; i++){                 
      digitalWrite(trigg_pin_2, LOW); 
      delayMicroseconds(10);
      digitalWrite(trigg_pin_2, HIGH); 
      delayMicroseconds(10);
      digitalWrite(trigg_pin_2, LOW); 
      delayMicroseconds(10);
      unsigned long duracao_UL2 = pulseIn(echo_pin_2, HIGH,30000);  
      distancia_UL2 += ((duracao_UL2 * 340)/2)/10000;
    }
    distancia_UL2 /= 5;
    // Ultra Sonico 2


    
    bufferUL1[idx1] = distancia_UL1;
    idx1 = (idx1 + 1) % N;
    float media1 = 0;
    for(int i = 0; i <N; i++) media1 += bufferUL1[i];
    media1 /= N;

    bufferUL2[idx2] = distancia_UL2;
    idx2 = (idx2 +1) % N;

    float media2 = 0;
    for(int i = 0; i < N; i++) media2 += bufferUL2[i];
    media2 /= N;

    if(!bufferInicializado && idx1 == 0 && idx2 == 0){
      bufferInicializado = true;
      return;
    }

    static float ultimaMedia1 = media1;
    static float ultimaMedia2 = media2;
    float desvio_UL1 = media1 - ultimaMedia1;
    float desvio_UL2 = media2 - ultimaMedia2;

    ultimaMedia1 = media1;
    ultimaMedia2 = media2;

    int movimento_UL1 = desvio_UL1 < -8;
    int movimento_UL2 = desvio_UL2 < -8;

    if(movimento_UL1 == true && movimento_UL2 == false ){
    tempo_UL1 = millis();
    //estado_UL1_passou = true;

    }else if(movimento_UL1 == false && movimento_UL2 == true ){
    tempo_UL2 = millis();
    //estado_UL2_passou = true;
    }
    
  
    
    if(movimento_UL1 && movimento_UL2){

      long resultado = (long) tempo_UL2 - (long) tempo_UL1;

      if(resultado > 0){
        Serial.println("  Entrada");
            timeClient.update();
            String evento = "Entrando";
           String timestamp = timeClient.getFormattedTime();
           Serial.println(timestamp);
           PublishOnNodeRED(evento,timestamp);
         estado_UL1_passou = false;
         estado_UL2_passou = false;
        delay(1000);
      }else{
        Serial.println("  Saída");
        timeClient.update();
        String evento = "Saindo";
        String timestamp = timeClient.getFormattedTime();
        Serial.println(timestamp);
        PublishOnNodeRED(evento,timestamp);
        estado_UL1_passou = false;
        estado_UL2_passou = false;
        delay(1000);
      }
    }

    delay(5);
    mqttClient.loop();
}  
// Fechar do Loop
void connectLocalworks() {
  Serial.println("Iniciando conexão com rede WiFi");
    WiFi.begin(SSID, PSWD);
    
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConectado!");
}
void PublishOnNodeRED(String teste1, String teste2) {
  mqttClient.setServer(brokerUrl.c_str(), port);
  String userId = "ESP-alves9";
  userId += String(random(0xffff), HEX);
  // json
  StaticJsonDocument<200>doc;
  
  char nodeRED[200];
  
  while (!mqttClient.connected()) {
    mqttClient.connect(
      userId.c_str(),
      "",
      "",
      topico_2,
      LWTQoS,
      Retain_LWT,
      nodeRED
    );
    Serial.println(".");
    
  delay(5000);
  }

  doc["evento"] = teste1;
  doc["timestamp"] = teste2;
  serializeJson(doc,nodeRED);

  mqttClient.publish(topico_2, nodeRED, Retain_LWT);
  Serial.print("Enviado ao Node-RED!");
}

void connectBroker() {
  Serial.println("Conectando ao broker");
  mqttClient.setServer(brokerUrl.c_str(), port);
  String userId = "ESP-alves9";
  userId += String(random(0xffff), HEX);

  // json
  StaticJsonDocument<200>doc;
    doc["status"] =LWTMessage;
    char buffer[200];
    serializeJson(doc,buffer);

    
  while (!mqttClient.connected()) {
    mqttClient.connect(
      userId.c_str(),
      brokerUser,
      brokerPass,
      topico_1,
      LWTQoS,
      Retain_LWT,
      buffer
    );
    Serial.println(".");
    
  delay(5000);
  }

  doc["status"] = "Online";

  serializeJson(doc, buffer);

  mqttClient.publish(topico_1, buffer, Retain_LWT);
  Serial.print("Conectado com sucesso!");
}
