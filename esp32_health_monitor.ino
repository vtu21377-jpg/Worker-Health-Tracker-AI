#include <WiFi.h>
#include <DHT.h>
#include <ESPAsyncWebServer.h>

// ----- WiFi Settings -----
const char* ssid = "band";      // 🔹 Replace with your WiFi name
const char* password = "12345678"; // 🔹 Replace with your WiFi password

// ----- DHT Settings -----
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ----- Heart Rate Sensor -----
#define HEART_PIN 34   // ✅ Use GPIO 34 (safe analog/digital input)
int lastState = 0;
unsigned long lastBeatTime = 0;
int beatCount = 0;
int bpm = 0;

// ----- Web Server -----
AsyncWebServer server(80);

// ----- HTML Page -----
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Sensor Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align:center; background: linear-gradient(to right, #00c6ff, #0072ff); color: white; }
    h1 { margin: 20px; }
    .card { background: rgba(255,255,255,0.15); border-radius: 15px; padding: 20px; margin: 20px auto; width: 200px; box-shadow: 0 8px 16px rgba(0,0,0,0.3);}
    .value { font-size: 2em; font-weight: bold; }
  </style>
</head>
<body>
  <h1>ESP32 Sensor Dashboard</h1>
  <div class="card">
    <div> Temperature</div>
    <div class="value" id="temp">--</div>
  </div>
  <div class="card">
    <div> Humidity</div>
    <div class="value" id="hum">--</div>
  </div>
  <div class="card">
    <div> Heart Rate</div>
    <div class="value" id="hr">--</div>
  </div>

<script>
function fetchData(){
  fetch("/data").then(r=>r.json()).then(data=>{
    document.getElementById("temp").innerHTML = data.temp + " &deg;C";
    document.getElementById("hum").innerHTML = data.hum + " %";
    document.getElementById("hr").innerHTML = data.hr + " BPM";
  });
}
setInterval(fetchData, 1000); // update every second
</script>
</body>
</html>
)rawliteral";

// ----- Setup -----
void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(HEART_PIN, INPUT);

  // WiFi Client
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n Connected to WiFi!");
  Serial.print(" Webpage: http://");
  Serial.println(WiFi.localIP());

  // Serve webpage
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Serve JSON data
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    // Prepare JSON data
    String json = "{";
    json += "\"temp\":" + String(temp) + ",";
    json += "\"hum\":" + String(hum) + ",";
    json += "\"hr\":" + String(bpm);
    json += "}";
    request->send(200, "application/json", json);
  });

  server.begin();
  Serial.println("HTTP server started");
}

// ----- Loop -----
void loop() {
  // Read pulse input
  int sensorValue = digitalRead(HEART_PIN);
  unsigned long currentTime = millis();

  // Detect rising edge (heartbeat pulse)
  if (sensorValue == HIGH && lastState == LOW) {
    unsigned long timeDiff = currentTime - lastBeatTime;
    if (timeDiff > 300 && timeDiff < 2000) { // filter noise (30–200 BPM)
      bpm = 60000 / timeDiff;
      Serial.print(" Beat detected, BPM = ");
      Serial.println(bpm);
    }
    lastBeatTime = currentTime;
  }
  lastState = sensorValue;
}
