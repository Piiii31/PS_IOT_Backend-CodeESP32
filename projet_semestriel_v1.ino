#include <WiFi.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <EEPROM.h>
#include <ESP.h>
#include <AsyncWebSocket.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>

const int EEPROM_SIZE = 512; // Define the size of EEPROM

char ssid[32]; // Define variables to store parameters
char password[64];
char serverAddress[32];
int serverPort;
char username[32];
char userPassword[32];

const char *loginEndpoint = "/";
const char *apiEndpoint = "/api/data/";

String token; // Variable to store the token

#define DHTPIN 26      // Pin where the DHT11 sensor is connected
#define DHTTYPE DHT11  // Type of DHT11 sensor

DHT dht(DHTPIN, DHTTYPE);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
WiFiClient client;

bool newDataWritten = false;

struct SensorSample
{
  char node_id[100];
  char time_and_date[100];
  char sensor_type[100];
  float value;
  char gps_location[100];
};

bool loginSuccessful = false; // Flag to indicate if login was successful

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
BLEServer *pServer;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    pServer->startAdvertising();
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    Serial.print(value.c_str());
    if (value.length() > 0) {
      Serial.println("*********");
      Serial.print("New value: ");
      for (int i = 0; i < value.length(); i++)
        Serial.print(value[i]);
      Serial.println();
      Serial.println("*********");

      // Parse the received JSON data and store in parameters
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, value.c_str());
      
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return; // Exit if JSON parsing fails
      }

      // Extract parameters from JSON
      String newSsid = doc["ssid"].as<String>();
      String newPassword = doc["password"].as<String>();
      String newServerAddress = doc["serverAddress"].as<String>();
      int newServerPort = doc["serverPort"].as<int>();
      String newUsername = doc["username"].as<String>();
      String newUserPassword = doc["userPassword"].as<String>();

      // Print received parameters for debugging
      Serial.println("Received Parameters:");
      Serial.println("SSID: " + newSsid);
      Serial.println("Password: " + newPassword);
      Serial.println("Server Address: " + newServerAddress);
      Serial.println("Server Port: " + String(newServerPort));
      Serial.println("Username: " + newUsername);
      Serial.println("User Password: " + newUserPassword);

      // Update parameters in EEPROM
      newSsid.toCharArray(ssid, 32);
      newPassword.toCharArray(password, 64);
      newServerAddress.toCharArray(serverAddress, 32);
      serverPort = newServerPort;
      newUsername.toCharArray(username, 32);
      newUserPassword.toCharArray(userPassword, 32);

      EEPROM.put(0, ssid);
      EEPROM.put(32, password);
      EEPROM.put(96, serverAddress);
      EEPROM.put(128, serverPort);
      EEPROM.put(132, username);
      EEPROM.put(164, userPassword);
      EEPROM.commit();
      newDataWritten = true;
      Serial.println("Parameters saved to EEPROM");

      // Print the parameters
    Serial.println("SSID: " + String(ssid));
    Serial.println("Password: " + String(password));
    Serial.println("Server Address: " + String(serverAddress));
    Serial.println("Server Port: " + String(serverPort));
    Serial.println("Username: " + String(username));
    Serial.println("User Password: " + String(userPassword));
    
    
    
    }


    
  }
  
};

void loginToServer() {
  if (client.connect(serverAddress, serverPort)) {
    Serial.println("Logged In");

    String data = "username=" + String(username) + "&password=" + String(userPassword);

    client.println("POST " + String(loginEndpoint) + " HTTP/1.1");
    client.println("Host: " + String(serverAddress) + ":" + String(serverPort));
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(data.length()));
    client.println();
    client.println(data);
    delay(3000);

    // Skip HTTP headers
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }

    // Read the Samlite response
    String response = "";
    while (client.available()) {
      response += client.readStringUntil('\n');
    }
    Serial.println(response);  // Print out the entire server response

    // Extract the token from the Samlite response
    int startIndex = response.indexOf("{\"token\": \"") + 11;
    int endIndex = response.indexOf("\"}", startIndex);
    if (startIndex != -1 && endIndex != -1) {
      token = response.substring(startIndex, endIndex);
      Serial.println("Token: " + token);
      loginSuccessful = true;  // Set login flag to true
    } else {
      Serial.println("Failed to extract token from the Samlite response");
    }

    client.stop();
  } else {
    Serial.println("Connection to server failed");
  }
}

void collectSensorData(SensorSample &sample) {
  // Code to read temperature data from the sensor
  float temperature = dht.readTemperature();

  // Assigning data to the structure
  strcpy(sample.node_id, "A1");
  sprintf(sample.time_and_date, "%04d-%02d-%02d %02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
  strcpy(sample.sensor_type, "T");
  sample.value = temperature;
  strcpy(sample.gps_location, "your_gps_location");
}

void collectHumidityData(SensorSample &sample) {
  // Code to read humidity data from the sensor
  float humidity = dht.readHumidity();

  // Assigning data to the structure
  strcpy(sample.node_id, "A1");
  sprintf(sample.time_and_date, "%04d-%02d-%02d %02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
  strcpy(sample.sensor_type, "H");
  sample.value = humidity;
  strcpy(sample.gps_location, "your_gps_location");
}

void sendDataToServer(SensorSample &sample) {
  if (loginSuccessful) {  // Check if login was successful
    if (client.connect(serverAddress, serverPort)) {
      Serial.println("Connected to server");

      String data = "node_id=" + String(sample.node_id) + "&time_and_date=" + String(sample.time_and_date)
                    + "&sensor_type=" + String(sample.sensor_type) + "&value=" + String(sample.value)
                    + "&gps_location=" + String(sample.gps_location);

      client.println("POST " + String(apiEndpoint) + " HTTP/1.1");
      client.println("Host: " + String(serverAddress) + ":" + String(serverPort));
      client.println("Content-Type: application/x-www-form-urlencoded");
      Serial.println("token" + token);
      client.println("Authorization: Token " + token);  // Include the token in the request
      client.println("Content-Length: " + String(data.length()));
      client.println();
      client.println(data);
      delay(10);

      // Read and display the server response
      while (client.available()) {
        Serial.write(client.read());
      }
      sendWebSocketMessage("Success");
      client.stop();
      
    } else {
      Serial.println("Connection to server failed");
      sendWebSocketMessage("Failed");
      
    }
  } else {
    Serial.println("Not logged in. Data not sent.");
    sendWebSocketMessage("Failed");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  // Initialize EEPROM with EEPROM_SIZE
  EEPROM.begin(EEPROM_SIZE);
  
  // Read parameters from EEPROM
  EEPROM.get(0, ssid);
  EEPROM.get(32, password);
  EEPROM.get(96, serverAddress);
  EEPROM.get(128, serverPort);
  EEPROM.get(132, username);
  EEPROM.get(164, userPassword);

  // Print the parameters
  Serial.println("SSID: " + String(ssid));
  Serial.println("Password: " + String(password));
  Serial.println("Server Address: " + String(serverAddress));
  Serial.println("Server Port: " + String(serverPort));
  Serial.println("Username: " + String(username));
  Serial.println("User Password: " + String(userPassword));

  // Check if parameters are empty
  
    BLEDevice::init("A1");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
   pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  
    Serial.println("Waiting for a BLE connection...");
    
    
   

 
  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

   // Initialize mDNS
  if (!MDNS.begin("esp32A1")) {   // Set the hostname to "esp32.local"
    Serial.println("Error setting up MDNS responder!");
    while(1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");  
  loginToServer();

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
  
}




void loop() {
  SensorSample temperatureSample;
  SensorSample humiditySample;

  collectSensorData(temperatureSample);
  collectHumidityData(humiditySample);

  sendDataToServer(temperatureSample);
  sendDataToServer(humiditySample);

  if (newDataWritten) {
     // Delay the restart to ensure the write operation is complete
    delay(1000);  // Delay for 1 second
    ESP.restart();
    
    
  }
  
  delay(10000);  // Wait for 10 seconds before collecting and sending the next data
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->opcode == WS_TEXT) {
      // Handle incoming messages from the WebSocket client
    }
  }
}

void sendWebSocketMessage(const char *message) {
  ws.textAll(message);
}
