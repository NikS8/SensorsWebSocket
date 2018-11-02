/*******************************************************************\
 * Сервер ArduinoWebsocketServer 
 * https://github.com/ejeklint/ArduinoWebsocketServer
   Сервер ArduinoJson выдает значения: 
          аналоговый: 
              от датчика дыма и газа MQ9
          цифровые: 
              от датчика дыма и газа MQ9
              от датчика движения HC-SR501
              от датчика температуры и вдажности DHT22
/*******************************************************************/

#include <SPI.h>

// Enabe debug tracing to Serial port.
#define DEBUG

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

#include <WebSocket.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 0 , 102 };
EthernetServer server(3003);


// Create a Websocket server
WebSocketServer wsServer;

#include <ArduinoJson.h>

#include <DHT.h>        // You have to download DHT22  library
#define DHT1PIND 7       // PIN подключения датчика DTH22
#define DHT1TYPE DHT22 
DHT dht1(DHT1PIND, DHT1TYPE);
int sensorDhtTemp;      // температура от датчика DHT22 в прихожей 
int sensorDhtHum;       // влажность от датчика DHT22

#define MQ1PINA 1   //  аналоговыйPIN подключения датчика дыма MQ9
#define MQ1PIND 6   //  цифровойPIN подключения датчика дыма MQ9
int sensorMQa;  //  аналоговый сигнал датчика дыма
int sensorMQd;  //  цифровой сигнал датчика дыма

#define HCSRPIND 5 //  цифровойPIN подключения датчика движения
int sensorSRd;  //  цифровой сигнал датчика движения

void onConnect(WebSocket &socket) {
  Serial.println(F("onConnect called"));
}

// You must have at least one function with the following signature.
// It will be called by the server when a data frame is received.
// У вас должна быть хотя бы одна функция со следующей подписью.
// Он будет вызываться сервером при получении фрейма данных.
void onData(WebSocket &socket, char* dataString, byte frameLength) {
  
#ifdef DEBUG
  Serial.print("Got data: ");
  Serial.write((unsigned char*)dataString, frameLength);
  Serial.println();
#endif
  
  // Just echo back data for fun.
  socket.send(dataString, strlen(dataString));
}

void onDisconnect(WebSocket &socket) {
  Serial.println(F("onDisconnect called"));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void setup() {

#ifdef DEBUG  
  Serial.begin(57600);
#endif
  Ethernet.begin(mac, ip);
  
  
  Serial.println(F("Initializing Ethernet.begin... "));
 
  Serial.print(F("Server IP: "));
  Serial.println(Ethernet.localIP());

  // ---

  wsServer.registerConnectCallback(&onConnect);
  wsServer.registerDataCallback(&onData);
  wsServer.registerDisconnectCallback(&onDisconnect);  

  wsServer.begin();
    Serial.println(F("Server is ready."));
  Serial.print(F("Please connect to http://"));
  Serial.println(Ethernet.localIP());

  pinMode(MQ1PINA, INPUT);
  dht1.begin();

  server.begin();
    delay(100); // Give Ethernet time to get ready
    Serial.println(Ethernet.localIP());
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void loop() {

  sensorDhtHum = dht1.readHumidity();
  sensorDhtTemp = dht1.readTemperature();

  sensorMQa = analogRead(MQ1PINA);
  sensorMQd = digitalRead(MQ1PIND);

  sensorSRd = digitalRead(HCSRPIND);

  // Should be called for each loop.
  wsServer.listen();
  
  // Do other stuff here, but don't hang or cause long delays.
  delay(100);
  if (wsServer.connectionCount() > 0) {
    wsServer.send("abc123", 6);
  }
  // Wait for an incomming connection
  EthernetClient client = server.available();

  // Do we have a client?
  if (!client) return;

  Serial.println(F("New client"));

  // Read the request (we ignore the content in this example)
  while (client.available()) client.read();

  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonBuffer <512> jsonBuffer;

  // Create the root object
  JsonObject& root = jsonBuffer.createObject();

  // Create the "analog" array
  JsonArray& analogValues = root.createNestedArray("analog");
     
    root["sensorMQa"] = sensorMQa;
   
  // Create the "digital" array
  JsonArray& digitalValues = root.createNestedArray("digital");
  /*for (int pin = 0; pin < 14; pin++) {
    // Read the digital input
    int value = digitalRead(pin);

    // Add the value at the end of the array
    digitalValues.add(value);
   }
   */
    root["sensorMQd"] = sensorMQd;   
    root["sensorSRd"] = sensorSRd; 
    root["sensorDhtHum %"] = sensorDhtHum;
    root["sensorDhtTemp °C"] = sensorDhtTemp;

  Serial.print(F("Sending: "));
  root.printTo(Serial);
  Serial.println();

  // Write response headers
  client.println("HTTP/1.0 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  // Write JSON document
  root.prettyPrintTo(client);

  // Disconnect
  client.stop();
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/