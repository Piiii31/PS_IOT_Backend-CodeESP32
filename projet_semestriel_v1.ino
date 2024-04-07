#include <WiFi.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <TimeLib.h>

const char *ssid = "Orange-FC80";
const char *password = "dRA2G4gneFN";
const char *serverAddress = "192.168.1.121"; // Replace with the IP address of your Django server
const int serverPort = 8000; // Replace with the port of your Django server
const char *apiEndpoint = "/api/data/"; // Replace with the API endpoint of your Django server

#define DHTPIN 26      // Pin where the DHT11 sensor is connected
#define DHTTYPE DHT11  // Type of DHT11 sensor

DHT dht(DHTPIN, DHTTYPE);

WiFiClient client;

struct SensorSample {
  char node_id[100];
  char time_and_date[100];
  char sensor_type[100];
  float value;
  char gps_location[100];
};

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
  if (client.connect(serverAddress, serverPort)) {
    Serial.println("Connected to server");

    String data = "node_id=" + String(sample.node_id) + "&time_and_date=" + String(sample.time_and_date)
                  + "&sensor_type=" + String(sample.sensor_type) + "&value=" + String(sample.value)
                  + "&gps_location=" + String(sample.gps_location);

    client.println("POST " + String(apiEndpoint) + " HTTP/1.1");
    client.println("Host: " + String(serverAddress));
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(data.length()));
    client.println();
    client.println(data);
    delay(10);
    client.stop();
  } else {
    Serial.println("Connection to server failed");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  SensorSample temperatureSample;
  SensorSample humiditySample;

  collectSensorData(temperatureSample);
  collectHumidityData(humiditySample);

  sendDataToServer(temperatureSample);
  sendDataToServer(humiditySample);

  delay(10000); // Wait for 10 seconds before collecting and sending the next data
}
