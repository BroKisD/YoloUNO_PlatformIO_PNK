#include "mainserver.h"
#include <WiFi.h>
#include <WebServer.h>

bool led1_state = false;
bool led2_state = false;
bool isAPMode = true;
NeoMode currentNeoMode = HUMIDITY_MODE;
bool neoControlEnabled = false;

Adafruit_NeoPixel neoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

WebServer server(80);

String ssid = "ESP32-PNK";
String password = "12345678";
String wifi_ssid = "KP_5G";
String wifi_password = "26012004";

unsigned long connect_start_ms = 0;
bool connecting = false;

String mainPage() {
  static SensorData data;
  float temperature = 0;
  float humidity = 0;
  
  if (xQueuePeek(sensorQueue, &data, 0) == pdPASS) {
    temperature = data.temperature;
    humidity = data.humidity;
  }
  
  String led1 = led1_state ? "ON" : "OFF";
  String led2 = led2_state ? "ON" : "OFF";
  String neoMode = "";
  switch(currentNeoMode) {
    case HUMIDITY_MODE: neoMode = "Humidity"; break;
    case POLICE_MODE: neoMode = "Police"; break;
    case TRAFFIC_MODE: neoMode = "Traffic Light"; break;
  }

  return R"rawliteral(
    <!DOCTYPE html><html><head>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <title>ESP32 Dashboard</title>
      <style>
        body { font-family: Arial, sans-serif; text-align: center; margin: 0; background: #f5f5f5; }
        .container { 
          margin: 20px auto; 
          max-width: 400px; 
          background: #ffffff; 
          border-radius: 15px; 
          box-shadow: 0 4px 6px rgba(0,0,0,0.1);
          padding: 25px;
        }
        .header {
          margin-bottom: 25px;
          color: #2c3e50;
        }
        .sensor-data {
          background: #f8f9fa;
          padding: 20px;
          border-radius: 12px;
          margin: 15px 0;
          display: grid;
          grid-template-columns: repeat(2, 1fr);
          gap: 15px;
        }
        .sensor-box {
          padding: 15px;
          border-radius: 8px;
          background: white;
          box-shadow: 0 2px 4px rgba(0,0,0,0.05);
        }
        .sensor-label {
          color: #666;
          font-size: 0.9em;
          margin-bottom: 5px;
        }
        .sensor-value {
          font-size: 1.8em;
          font-weight: bold;
          color: #2c3e50;
        }
        .neo-control {
          background: #ffffff;
          padding: 20px;
          border-radius: 12px;
          margin: 20px 0;
          box-shadow: 0 2px 4px rgba(0,0,0,0.05);
        }
        .neo-control h3 {
          color: #2c3e50;
          margin-bottom: 15px;
        }
        .neo-btn {
          background: #4CAF50;
          color: white;
          padding: 12px 20px;
          margin: 8px;
          border: none;
          border-radius: 8px;
          cursor: pointer;
          transition: all 0.3s ease;
          font-size: 16px;
          width: calc(100% - 16px);
          max-width: 200px;
        }
        .neo-btn:hover {
          transform: translateY(-2px);
          box-shadow: 0 4px 8px rgba(0,0,0,0.1);
        }
        .neo-btn.active {
          background: #2196F3;
          transform: translateY(1px);
        }
        .status {
          font-weight: 500;
          color: #666;
          margin: 15px 0;
          padding: 10px;
          background: #f8f9fa;
          border-radius: 6px;
          display: inline-block;
        }
        #settings {
          position: absolute;
          top: 20px;
          right: 20px;
          background: #007bff;
          color: white;
          border: none;
          border-radius: 50%;
          width: 40px;
          height: 40px;
          font-size: 20px;
          cursor: pointer;
          transition: 0.3s;
        }
        #settings:hover {
          background: #0056b3;
          transform: rotate(90deg);
        }
      </style>
    </head>
    <body>
      <div class='container'>
        <div class='header'>
          <h2>ESP32 Environmental Monitor</h2>
        </div>

        <div class='sensor-data'>
          <div class='sensor-box'>
            <div class='sensor-label'>Temperature</div>
            <div class='sensor-value'>
              <span id='temp'>)rawliteral" + String(temperature) + R"rawliteral(</span>&deg;C
            </div>
          </div>
          <div class='sensor-box'>
            <div class='sensor-label'>Humidity</div>
            <div class='sensor-value'>
              <span id='hum'>)rawliteral" + String(humidity) + R"rawliteral(</span>%
            </div>
          </div>
        </div>
        
        <div class='neo-control'>
          <h3>RGB LED Control</h3>
          <p class='status'>Active Mode: <span id='neoMode'>)rawliteral" + neoMode + R"rawliteral(</span></p>
          <div>
            <button class='neo-btn' onclick='setNeoMode("humidity")'>
              <i class="fas fa-tint"></i> Humidity Indicator
            </button>
            <button class='neo-btn' onclick='setNeoMode("police")'>
              <i class="fas fa-lightbulb"></i> Police Light
            </button>
            <button class='neo-btn' onclick='setNeoMode("traffic")'>
              <i class="fas fa-traffic-light"></i> Traffic Signal
            </button>
          </div>
        </div>
      </div>
      <button id="settings" onclick="window.location='/settings'">&#9881;</button>
      <script>
        function toggleLED(id) {
          fetch('/toggle?led='+id)
          .then(response=>response.json())
          .then(json=>{
            document.getElementById('l1').innerText=json.led1;
            document.getElementById('l2').innerText=json.led2;
          });
        }
        
        function setNeoMode(mode) {
          fetch('/neo?mode='+mode)
          .then(response=>response.json())
          .then(json=>{
            document.getElementById('neoMode').innerText=json.mode;
            // Update active button state
            document.querySelectorAll('.neo-btn').forEach(btn => {
              btn.classList.remove('active');
              if(btn.innerText.toLowerCase().includes(json.mode.toLowerCase())) {
                btn.classList.add('active');
              }
            });
          });
        }
        
        // Update sensors and highlight current mode button on load
        function updateUI() {
          fetch('/sensors')
           .then(res=>res.json())
           .then(d=>{
             document.getElementById('temp').innerText=d.temp;
             document.getElementById('hum').innerText=d.hum;
           });
          
          let currentMode = document.getElementById('neoMode').innerText.toLowerCase();
          document.querySelectorAll('.neo-btn').forEach(btn => {
            if(btn.innerText.toLowerCase().includes(currentMode)) {
              btn.classList.add('active');
            }
          });
        }
        
        // Initial UI update and start interval
        updateUI();
        setInterval(updateUI, 3000);
      </script>
    </body></html>
  )rawliteral";
}

String settingsPage() {
  return R"rawliteral(
    <!DOCTYPE html><html><head>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <title>Settings</title>
      <style>
        body { font-family: Arial; text-align:center; margin:0;}
        .container { margin:20px auto; max-width:350px;background:#f9f9f9;border-radius:10px;box-shadow:0 2px 10px #ccc;padding:20px;}
        input[type=text], input[type=password]{width:90%;padding:10px;}
        button { padding:10px 15px; margin:10px; font-size:18px;}
      </style>
    </head>
    <body>
      <div class='container'>
        <h2>Wi-Fi Settings</h2>
        <form id="wifiForm">
          <input name="ssid" id="ssid" placeholder="SSID" required><br>
          <input name="password" id="pass" type="password" placeholder="Password" required><br><br>
          <button type="submit">Connect</button>
          <button type="button" onclick="window.location='/'">Back</button>
        </form>
        <div id="msg"></div>
      </div>
      <script>
        document.getElementById('wifiForm').onsubmit = function(e){
          e.preventDefault();
          let ssid = document.getElementById('ssid').value;
          let pass = document.getElementById('pass').value;
          fetch('/connect?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass))
            .then(r=>r.text())
            .then(msg=>{
              document.getElementById('msg').innerText=msg;
            });
        };
      </script>
    </body></html>
  )rawliteral";
}

// ========== Handlers ==========
void handleRoot() { server.send(200, "text/html", mainPage()); }

void handleToggle() {
  int led = server.arg("led").toInt();
  if (led == 1) led1_state = !led1_state;
  else if (led == 2) led2_state = !led2_state;
  server.send(200, "application/json",
    "{\"led1\":\"" + String(led1_state ? "ON":"OFF") +
    "\",\"led2\":\"" + String(led2_state ? "ON":"OFF") + "\"}");
}

void handleSensors() {
  static SensorData data;
  // Wait for the semaphore with a timeout
  if (xSemaphoreTake(ledSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (xQueuePeek(sensorQueue, &data, 0) == pdPASS) {
      String json = "{\"temp\":" + String(data.temperature, 1) + 
                   ",\"hum\":" + String(data.humidity, 1) + "}";
      server.send(200, "application/json", json);
    } else {
      // If queue is empty, send last known values
      String json = "{\"temp\":" + String(data.temperature, 1) + 
                   ",\"hum\":" + String(data.humidity, 1) + 
                   ",\"status\":\"no new data\"}";
      server.send(200, "application/json", json);
    }
  } else {
    // If semaphore timeout, send last known values
    String json = "{\"temp\":" + String(data.temperature, 1) + 
                 ",\"hum\":" + String(data.humidity, 1) + 
                 ",\"status\":\"waiting for sensor\"}";
    server.send(200, "application/json", json);
  }
}

void handleNeoMode() {
  String mode = server.arg("mode");
  String modeName = "Unknown";
  
  if (mode == "humidity") {
    currentNeoMode = HUMIDITY_MODE;
    neoControlEnabled = false;
    modeName = "Humidity";
  } 
  else if (mode == "police") {
    currentNeoMode = POLICE_MODE;
    neoControlEnabled = true;
    modeName = "Police";
  }
  else if (mode == "traffic") {
    currentNeoMode = TRAFFIC_MODE;
    neoControlEnabled = true;
    modeName = "Traffic Light";
  }
  
  String json = "{\"mode\":\"" + modeName + "\"}";
  server.send(200, "application/json", json);
}

void handleSettings() { server.send(200, "text/html", settingsPage()); }

void handleConnect() {
  wifi_ssid = server.arg("ssid");
  wifi_password = server.arg("pass");
  server.send(200, "text/plain", "Connecting....");
  isAPMode = false;
  connecting = true;
  connect_start_ms = millis();
  connectToWiFi();
}

// ========== WiFi ==========
void setupServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/sensors", HTTP_GET, handleSensors);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/connect", HTTP_GET, handleConnect);
  server.on("/neo", HTTP_GET, handleNeoMode);
  server.begin();
}

void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str(), password.c_str());
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  isAPMode = true;
  connecting = false;
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  Serial.print("Connecting to ");
  Serial.print(wifi_ssid.c_str());
  Serial.print(wifi_password.c_str());
  
  Serial.println(wifi_ssid);
}

// ========== Main task ==========
void main_server_task(void *pvParameters){
  pinMode(BOOT_PIN, INPUT_PULLUP);
  
  // Initialize Neo LED
  neoPixel.begin();
  neoPixel.setBrightness(50);
  neoPixel.clear();
  neoPixel.show();

  startAP();
  setupServer();

  while(1){
    server.handleClient();

    // Nếu nhấn BOOT thì về AP mode
    if (digitalRead(BOOT_PIN) == LOW) {
      vTaskDelay(100);
      if (digitalRead(BOOT_PIN) == LOW) {
        if (!isAPMode) {
          startAP();
          setupServer();
        }
      }
    }

    // Nếu đang connect STA
    if (connecting) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("STA IP address: ");
        Serial.println(WiFi.localIP());
        isAPMode = false;
        connecting = false;
      } else if (millis() - connect_start_ms > 10000) { // timeout 10s
        Serial.println("WiFi connect failed! Back to AP.");
        startAP();
        setupServer();
        connecting = false;
      }
    }

    // Update Neo LED patterns if in control mode
    updateNeoLED();

    vTaskDelay(20); // avoid watchdog reset
  }
}