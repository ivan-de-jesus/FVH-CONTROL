#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Notificaciones

const char* apiKey = "o.4XYofIx4kyLQrzvIrOQFP7Yt0DbMeJ3P";//Cuenta de hidroalimentos
int pinBoton = 0;

//Sensor de humedad relativa
#include "DHT.h"
#define DHTPIN 5   
#define DHTTYPE DHT11   
DHT dht(DHTPIN, DHTTYPE);

//millis
unsigned long TiempoInicio = 0;
int periodo = 1000;

//sensor de nivel de agua
int Trig = 4; //D2
int Echo = 2; //D4

//bomba de agua
int bomba = 16;
int contador= 0;
//monitoreo de bomba
int estado_bomba;

int estadoBombaNoti = 0;             
int estadoAnteriorBomba = 0;

//humedad en charola
int Valor;

//***************************
//****configuracion de red***
//***************************
const char* ssid     = "";
const char* password = "";

//****************************
//**configuracion Sky Things**
//****************************
const char *mqtt_server = "ieiot.com.mx";
const int mqtt_port = 1883;
const char *mqtt_user = "web_client";
const char *mqtt_pass = "111";

WiFiClient espClient;
PubSubClient client(espClient);

//Los utilizo para cnvertir lo que se va enviar a toCharArray
long lastMsg = 0;
char msg[25];
char msg2[25];
char msg3[25];
char msgEstadoBomba[25];
char msgCharola[25];
char msgAlerta[25];
char msgAlertaOk[25];
char msgAlertaMedio[25];

//***************************
//***Declaracion Funciones***
//***************************
void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();





void setup() {
  Serial.begin(115200); 
  Serial.println("Iniciando...");
  dht.begin();
  
  //bomba de agua
  pinMode(bomba, OUTPUT);
  
  //Sensor ultrasonico
  pinMode(Trig,OUTPUT);
  pinMode (Echo,INPUT);
  
  //necesario para el envio de datos
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); 
  pinMode(pinBoton,INPUT_PULLUP);

}

void loop() {
//reconecion mqtt
  if (!client.connected()){
         reconnect();
}
  client.loop();
  HumedadAmbiente();
  estadoBomba();
}

//******************************
//****Estado bomba****
//******************************  
void estadoBomba(){
  String msgConcatenado;
    
    estado_bomba = digitalRead(bomba);
    if (estado_bomba == 1){
    msgConcatenado = " Bomba encendida";
    estado_bomba + msgConcatenado;
    
    }
   else if (estado_bomba == 0){
    msgConcatenado = " Bomba apagada";
    estado_bomba + msgConcatenado;
    } 
    
   String estadoReal = String(estado_bomba)+ msgConcatenado;
    estadoReal.toCharArray(msgEstadoBomba,25);
    client.publish("Ambiente/estado-bomba",msgEstadoBomba); 
    notificaciones();
       
  delay(1000);
  }
 
void setup_wifi(){
  delay(10);
  Serial.println();
  Serial.print("Conectado a: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
      }
  Serial.println("");
  Serial.println("Conectado a red Wifi");
  Serial.println("Dirección IP: "); 
  Serial.println(WiFi.localIP());  
  }

//******************************
//****Humedad relativa & tem****
//******************************  
void HumedadAmbiente() {
   if (millis()-TiempoInicio >= periodo){
  float h = dht.readHumidity(); //Leemos la Humedad
  float t = dht.readTemperature(); //Leemos la temperatura en grados Celsius
  
  //--------Enviamos las lecturas por el puerto serial-------------
  Serial.print("Humedad Relativa: ");
  Serial.print(h);
  Serial.print(" %");
  Serial.print(" Temperatura: ");
  Serial.print(t);
  Serial.println(" °C ");
  TiempoInicio=millis();
  TiempoInicio = 0;
    if(h <=20  && t <=18){
      distancia();
      }
   String humedad_relativa = String(h) + " %";
   String temperatura = String(t) + " °C";
    humedad_relativa.toCharArray(msg,25);
    temperatura.toCharArray(msg2,25);
    //Serial.print("Publicamos mensaje: ");
    //Serial.print(msg);
    client.publish("Ambiente /TOPIC2",msg);  
    client.publish("Ambiente/Temp",msg2); 
    
      HumedadCharola();
      estadoBomba(); 
      distancia(); 
      
 }
}

//***********************
//***Contenedor de agua**
//***********************
void distancia(){
  if (millis()-TiempoInicio >= periodo){
  //TiempoInicio = 0;
  long duracion;
  long distancia;
  char* mensajeAlerta;
  char* mensajeOk;
  
  digitalWrite(Trig,LOW);
  delayMicroseconds(4);
  digitalWrite(Trig,HIGH);
  delayMicroseconds(10);
  digitalWrite(Trig,LOW);

  duracion = pulseIn(Echo,HIGH);
  duracion = duracion/2;

  distancia = duracion/29;
  Serial.print("Contenedor: ");
  Serial.print(distancia);
  Serial.println("CM");
  delay(50);
       if (distancia <=8){
           contador ++;
           Serial.println(contador);             
           digitalWrite(bomba, HIGH);
           Serial.println("Nivel de agua optimo");
           TiempoInicio=millis();
           HumedadCharola();
             if(contador >= 100){
                digitalWrite(bomba, LOW);
                Serial.println("Bomba apagada por tiempo");
                contador= 0;
                }

        mensajeOk= "No hay alertas ✓";
           String alerta_medioOk = String(mensajeOk);
           alerta_medioOk.toCharArray(msgAlertaOk,25);
           client.publish("Ambiente/alertasmsg",msgAlertaOk);
      }
      
     if (distancia <=20){
      contador ++;
      Serial.println(contador);        
      digitalWrite(bomba, HIGH);
      Serial.println("Nivel de agua medio");
      
      TiempoInicio=millis();
      HumedadCharola();
      
         if(contador >= 100){
          digitalWrite(bomba, LOW);
          Serial.println("Bomba apagada por tiempo");
          contador= 0;      
         }
         
      }
      
     if (distancia >=21){
      contador= 0;
      digitalWrite(bomba, LOW);
      Serial.println("Agua insuficiente (Bomba apagada por seguridad)");
      HumedadCharola();
      
          mensajeAlerta = "Agua insuficiente X";
          String alerta_medio = String(mensajeAlerta);
          alerta_medio.toCharArray(msgAlerta,25);
          client.publish("Ambiente/alertasmsg",msgAlerta);
          TiempoInicio=millis();
       }   
       String distancia_contenedor = String(distancia) + " CM";
        distancia_contenedor.toCharArray(msg3,25);
        client.publish("Ambiente/contenedor",msg3);        
    }
  }
//***********************
//****Humedad charola****
//***********************
void HumedadCharola(){
  TiempoInicio = 0;
  if (millis()-TiempoInicio >= periodo){
    Valor = analogRead(0);
    Serial.print(Valor);
  int lecturaPorcentaje = map (Valor, 1024,360,0,100);
    Serial.print(" Humedad charola: ");
    Serial.print(lecturaPorcentaje);
    Serial.println(" %");
    TiempoInicio=millis();
   
    if (lecturaPorcentaje >= 29){
      digitalWrite(bomba, LOW);
      Serial.println("Bomba apagada por humeda en charola");
      contador =0;    
     }
     String humedadCharola = String(lecturaPorcentaje)+ " %";
    humedadCharola.toCharArray(msgCharola,25);
    client.publish("Ambiente/humedadcharola",msgCharola);
  } 
}

//***********************
//*****notificaciones********
//***********************
void notificaciones() {
 estadoBombaNoti = digitalRead(bomba); 
      
  if (estadoBombaNoti != estadoAnteriorBomba) {                                            
    if (estadoBombaNoti == HIGH) {                                                                 
         Serial.println("Notificacion enviada");
          enviarMensaje("Invernadero","Bomba encendida");
          delay(2000);                     
    }
    else if (estadoBombaNoti == LOW){
          Serial.println("Notificacion enviada");
          enviarMensaje("Invernadero","Bomba apagada");
          delay(2000);
      }
        
  }                                                
  estadoAnteriorBomba = estadoBombaNoti; 
  delay(100);
}

const char* host = "api.pushbullet.com"; 
void enviarMensaje(String titulo,String mensaje) {
  
  WiFiClientSecure client;
  client.setInsecure();
  if(!client.connect(host,443)) {
    Serial.println("No se pudo conectar con el servidor de notificaciones");
    return;
  }

String url = "/v2/pushes";
  String message = "{\"type\": \"note\", \"title\": \""+titulo+"\", \"body\": \""+mensaje+"\"}\r\n";
  Serial.print("requesting URL: ");
  Serial.println(url);
  //send a simple note
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Bearer " + apiKey + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length: " +
               String(message.length()) + "\r\n\r\n");
  client.print(message);

  delay(2000);
  while (client.available() == 0);

  while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
  }  
}


//***********************
//*****Callback**********
//*********************** 
 void callback(char* topic, byte* payload, unsigned int length){
    String incoming = "";
    Serial.print("Mensaje recibido desde: ");
    Serial.print(topic);
    Serial.println("");
      for(int i = 0; i < length; i++){ //conversion de byts a string
          incoming += (char)payload[i];
        }
     incoming.trim(); //Elimina espacios en blanco
     Serial.println("Mensaje: "+incoming);
  }

//***********************
//*****Reconexion********
//***********************
  void reconnect(){
     while (!client.connected()) {
    Serial.print("Intentando conexión Mqtt...");
    // Creamos un cliente ID
    String clientId = "esp32_";
    clientId += String(random(0xffff), HEX);
    // Intentamos conectar
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) {
      Serial.println("Conectado!");
      // Nos suscribimos
      //client.subscribe("led1");
    } else {
      Serial.print("falló :( con error -> ");
      Serial.print(client.state());
      Serial.println(" Intentamos de nuevo en 5 segundos");

      delay(5000);
    }
  }
 }


 
 
