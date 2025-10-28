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
  SensorData snapshot = {0.0f, 0.0f};
  if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    snapshot = latestData;
    xSemaphoreGive(dataMutex);
  }

  float temperature = snapshot.temperature;
  float humidity = snapshot.humidity;

  String neoMode = "";
  switch(currentNeoMode) {
    case HUMIDITY_MODE: neoMode = "Humidity"; break;
    case POLICE_MODE: neoMode = "Police"; break;
    case TRAFFIC_MODE: neoMode = "Traffic Light"; break;
  }

  return R"rawliteral(
    <!DOCTYPE html><html><head>
      <meta charset='UTF-8'>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <title>ESP32 Dashboard</title>
      <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        
        body { 
          font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
          background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
          min-height: 100vh;
          padding: 20px;
          position: relative;
        }
        
        .container { 
          margin: 0 auto; 
          max-width: 480px; 
          animation: fadeIn 0.5s ease;
        }
        
        @keyframes fadeIn {
          from { opacity: 0; transform: translateY(20px); }
          to { opacity: 1; transform: translateY(0); }
        }
        
        .card {
          background: rgba(255, 255, 255, 0.95);
          backdrop-filter: blur(10px);
          border-radius: 20px;
          box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
          padding: 25px;
          margin-bottom: 20px;
          transition: transform 0.3s ease;
        }
        
        .card:hover {
          transform: translateY(-5px);
          box-shadow: 0 12px 40px rgba(0, 0, 0, 0.15);
        }
        
        .header {
          text-align: center;
          color: white;
          margin-bottom: 25px;
        }
        
        .header h1 {
          font-size: 28px;
          font-weight: 700;
          text-shadow: 2px 2px 4px rgba(0,0,0,0.2);
        }
        
        .sensor-grid {
          display: grid;
          grid-template-columns: repeat(2, 1fr);
          gap: 15px;
          margin-bottom: 20px;
        }
        
        .sensor-box {
          padding: 20px;
          border-radius: 15px;
          text-align: center;
          color: white;
          position: relative;
          overflow: hidden;
          transition: all 0.3s ease;
        }
        
        .sensor-box.temp-cold {
          background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
        }
        
        .sensor-box.temp-normal {
          background: linear-gradient(135deg, #43e97b 0%, #38f9d7 100%);
        }
        
        .sensor-box.temp-warm {
          background: linear-gradient(135deg, #fa709a 0%, #fee140 100%);
        }
        
        .sensor-box.temp-hot {
          background: linear-gradient(135deg, #ff6b6b 0%, #ee5a6f 100%);
        }
        
        .sensor-box.hum-dry {
          background: linear-gradient(135deg, #ffa751 0%, #ffe259 100%);
        }
        
        .sensor-box.hum-normal {
          background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        }
        
        .sensor-box.hum-humid {
          background: linear-gradient(135deg, #2193b0 0%, #6dd5ed 100%);
        }
        
        .sensor-box::before {
          content: '';
          position: absolute;
          top: -50%;
          left: -50%;
          width: 200%;
          height: 200%;
          background: rgba(255,255,255,0.1);
          transform: rotate(45deg);
          transition: 0.5s;
        }
        
        .sensor-box:hover::before {
          left: 100%;
        }
        
        .sensor-icon {
          margin-bottom: 10px;
          position: relative;
          z-index: 1;
        }
        
        .sensor-icon svg {
          filter: drop-shadow(0 2px 4px rgba(0,0,0,0.2));
        }
        
        .sensor-label {
          font-size: 14px;
          opacity: 0.95;
          margin-bottom: 5px;
          position: relative;
          z-index: 1;
          font-weight: 500;
        }
        
        .sensor-value {
          font-size: 32px;
          font-weight: bold;
          margin-bottom: 8px;
          position: relative;
          z-index: 1;
        }
        
        .sensor-status {
          font-size: 12px;
          padding: 4px 12px;
          background: rgba(255,255,255,0.3);
          border-radius: 12px;
          display: inline-block;
          position: relative;
          z-index: 1;
          font-weight: 600;
          letter-spacing: 0.5px;
        }
        
        .history-card {
          background: rgba(255, 255, 255, 0.95);
          backdrop-filter: blur(10px);
          border-radius: 20px;
          box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
          padding: 20px;
          margin-bottom: 20px;
        }
        
        .history-title {
          font-size: 18px;
          font-weight: 600;
          color: #333;
          margin-bottom: 15px;
          display: flex;
          align-items: center;
          gap: 10px;
        }
        
        .history-list {
          max-height: 200px;
          overflow-y: auto;
        }
        
        .history-item {
          display: flex;
          justify-content: space-between;
          align-items: center;
          padding: 12px;
          background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
          border-radius: 10px;
          margin-bottom: 8px;
          font-size: 14px;
          transition: 0.2s;
          border-left: 4px solid #667eea;
        }
        
        .history-item:hover {
          transform: translateX(5px);
          box-shadow: 0 2px 8px rgba(0,0,0,0.1);
        }
        
        .history-time {
          color: #666;
          font-size: 12px;
          min-width: 70px;
          font-weight: 500;
        }
        
        .history-values {
          display: flex;
          gap: 15px;
          font-weight: 600;
          color: #333;
        }
        
        .prediction-card {
          text-align: center;
        }
        
        .prediction-header {
          font-size: 20px;
          font-weight: 600;
          color: #333;
          margin-bottom: 15px;
          display: flex;
          align-items: center;
          justify-content: center;
          gap: 8px;
        }
        
        .season-box {
          background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
          color: white;
          padding: 25px 20px;
          border-radius: 15px;
          margin-bottom: 15px;
          box-shadow: 0 4px 15px rgba(240, 147, 251, 0.3);
          position: relative;
          overflow: hidden;
        }
        
        .season-box::before {
          content: '';
          position: absolute;
          top: -50%;
          right: -50%;
          width: 200%;
          height: 200%;
          background: radial-gradient(circle, rgba(255,255,255,0.1) 0%, transparent 70%);
          animation: rotate 10s linear infinite;
        }
        
        @keyframes rotate {
          from { transform: rotate(0deg); }
          to { transform: rotate(360deg); }
        }
        
        .season-name {
          font-size: 36px;
          font-weight: bold;
          margin-bottom: 10px;
          position: relative;
          z-index: 1;
          text-shadow: 2px 2px 4px rgba(0,0,0,0.2);
        }
        
        .confidence-text {
          font-size: 14px;
          opacity: 0.9;
          margin-bottom: 15px;
          position: relative;
          z-index: 1;
        }
        
        .confidence-text span {
          font-weight: 600;
          font-size: 16px;
        }
        
        .confidence-bar-container {
          height: 8px;
          background: rgba(255,255,255,0.3);
          border-radius: 4px;
          overflow: hidden;
          position: relative;
          z-index: 1;
        }
        
        .confidence-bar {
          height: 100%;
          background: white;
          width: 0%;
          transition: width 0.8s cubic-bezier(0.4, 0, 0.2, 1);
          border-radius: 4px;
          box-shadow: 0 0 10px rgba(255,255,255,0.5);
        }
        
        .prediction-footer {
          font-size: 13px;
          color: #666;
          margin-top: 10px;
          font-style: italic;
        }
        
        .neo-control {
          text-align: center;
        }
        
        .neo-title {
          font-size: 20px;
          font-weight: 600;
          color: #333;
          margin-bottom: 10px;
        }
        
        .neo-status {
          display: inline-block;
          padding: 8px 20px;
          background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
          color: white;
          border-radius: 20px;
          font-size: 14px;
          margin-bottom: 20px;
          animation: pulse 2s infinite;
          font-weight: 500;
        }
        
        @keyframes pulse {
          0%, 100% { opacity: 1; }
          50% { opacity: 0.8; }
        }
        
        .neo-buttons {
          display: grid;
          gap: 12px;
        }
        
        .neo-btn {
          background: white;
          color: #333;
          padding: 15px 20px;
          border: 2px solid #e0e0e0;
          border-radius: 12px;
          cursor: pointer;
          transition: all 0.3s ease;
          font-size: 16px;
          font-weight: 500;
          display: flex;
          align-items: center;
          justify-content: center;
          gap: 10px;
        }
        
        .neo-btn:hover {
          transform: translateY(-3px);
          box-shadow: 0 6px 20px rgba(0,0,0,0.15);
          border-color: #667eea;
        }
        
        .neo-btn.active {
          background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
          color: white;
          border-color: transparent;
          transform: scale(1.05);
          box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
        }
        
        .settings-btn {
          position: fixed;
          top: 20px;
          right: 20px;
          background: rgba(255, 255, 255, 0.95);
          backdrop-filter: blur(10px);
          border: none;
          border-radius: 50%;
          width: 50px;
          height: 50px;
          cursor: pointer;
          box-shadow: 0 4px 15px rgba(0,0,0,0.2);
          transition: all 0.3s ease;
          z-index: 1000;
          display: flex;
          align-items: center;
          justify-content: center;
        }
        
        .settings-btn svg {
          width: 24px;
          height: 24px;
          stroke: #667eea;
          transition: transform 0.3s ease;
        }
        
        .settings-btn:hover {
          box-shadow: 0 6px 25px rgba(0,0,0,0.3);
          transform: scale(1.1);
        }
        
        .settings-btn:hover svg {
          transform: rotate(90deg);
        }
        
        /* Responsive */
        @media (max-width: 480px) {
          body { padding: 15px; }
          .header h1 { font-size: 24px; }
          .sensor-value { font-size: 28px; }
          .card { padding: 20px; }
          .settings-btn {
            width: 45px;
            height: 45px;
          }
          .settings-btn svg {
            width: 20px;
            height: 20px;
          }
          .season-name {
            font-size: 30px;
          }
        }
        
        /* Scrollbar */
        .history-list::-webkit-scrollbar {
          width: 6px;
        }
        
        .history-list::-webkit-scrollbar-track {
          background: #f1f1f1;
          border-radius: 10px;
        }
        
        .history-list::-webkit-scrollbar-thumb {
          background: #667eea;
          border-radius: 10px;
        }
      </style>
    </head>
    <body>
      <button class="settings-btn" onclick="window.location='/settings'">
        <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <circle cx="12" cy="12" r="3"></circle>
          <path d="M12 1v6m0 6v6m8.66-15l-3 5.2M6.34 17.8l-3 5.2m15.32-5.2l-3-5.2M6.34 6.2l-3-5.2"></path>
        </svg>
      </button>
      
      <div class='container'>
        <div class='header'>
          <h1>ESP32 Environmental Monitor</h1>
        </div>

        <div class='card'>
          <div class='sensor-grid'>
            <div class='sensor-box temp-normal' id='tempBox'>
              <div class='sensor-icon'>
                <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                  <path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"></path>
                </svg>
              </div>
              <div class='sensor-label'>Temperature</div>
              <div class='sensor-value'><span id='temp'>)rawliteral" + String(temperature, 1) + R"rawliteral(</span>&deg;C</div>
              <div class='sensor-status' id='tempStatus'>Normal</div>
            </div>
            <div class='sensor-box hum-normal' id='humBox'>
              <div class='sensor-icon'>
                <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                  <path d="M12 2.69l5.66 5.66a8 8 0 1 1-11.31 0z"></path>
                </svg>
              </div>
              <div class='sensor-label'>Humidity</div>
              <div class='sensor-value'><span id='hum'>)rawliteral" + String(humidity, 1) + R"rawliteral(</span>%</div>
              <div class='sensor-status' id='humStatus'>Normal</div>
            </div>
          </div>
        </div>
        
        <div class='history-card'>
          <div class='history-title'>
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <line x1="12" y1="20" x2="12" y2="10"></line>
              <line x1="18" y1="20" x2="18" y2="4"></line>
              <line x1="6" y1="20" x2="6" y2="16"></line>
            </svg>
            <span>Recent History (10s)</span>
          </div>
          <div class='history-list' id='historyList'>
            <div style='text-align:center; color:#999; padding:20px;'>Loading...</div>
          </div>
        </div>
        
        <div class='card prediction-card'>
          <div class='prediction-header'>
            <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <circle cx="12" cy="12" r="5"></circle>
              <line x1="12" y1="1" x2="12" y2="3"></line>
              <line x1="12" y1="21" x2="12" y2="23"></line>
              <line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line>
              <line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line>
              <line x1="1" y1="12" x2="3" y2="12"></line>
              <line x1="21" y1="12" x2="23" y2="12"></line>
              <line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line>
              <line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line>
            </svg>
            Season Prediction
          </div>
          <div class='season-box'>
            <div class='season-name' id='seasonName'>Loading...</div>
            <div class='confidence-text'>
              Confidence: <span id='confidence'>--%</span>
            </div>
            <div class='confidence-bar-container'>
              <div class='confidence-bar' id='confidenceBar'></div>
            </div>
          </div>
          <div class='prediction-footer'>
            Based on temperature and humidity patterns
          </div>
        </div>
        
        <div class='card neo-control'>
          <div class='neo-title'>RGB LED Control</div>
          <div class='neo-status'>
            Active: <span id='neoMode'>)rawliteral" + neoMode + R"rawliteral(</span>
          </div>
          <div class='neo-buttons'>
            <button class='neo-btn' onclick='setNeoMode("humidity")'>
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M12 2.69l5.66 5.66a8 8 0 1 1-11.31 0z"></path>
              </svg>
              <span>Humidity Indicator</span>
            </button>
            <button class='neo-btn' onclick='setNeoMode("police")'>
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <circle cx="12" cy="12" r="10"></circle>
                <circle cx="12" cy="12" r="3"></circle>
              </svg>
              <span>Police Light</span>
            </button>
            <button class='neo-btn' onclick='setNeoMode("traffic")'>
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <rect x="5" y="2" width="14" height="20" rx="2" ry="2"></rect>
                <circle cx="12" cy="8" r="2"></circle>
                <circle cx="12" cy="14" r="2"></circle>
              </svg>
              <span>Traffic Signal</span>
            </button>
          </div>
        </div>
      </div>
      
      <script>
        let history = [];
        const MAX_HISTORY = 5;
        
        function getStatus(temp, hum) {
          let tempStatus, tempClass, humStatus, humClass;
          
          // Temperature status and class
          if (temp < 15) {
            tempStatus = 'VERY COLD';
            tempClass = 'temp-cold';
          } else if (temp < 20) {
            tempStatus = 'COLD';
            tempClass = 'temp-cold';
          } else if (temp < 26) {
            tempStatus = 'NORMAL';
            tempClass = 'temp-normal';
          } else if (temp < 32) {
            tempStatus = 'WARM';
            tempClass = 'temp-warm';
          } else {
            tempStatus = 'HOT';
            tempClass = 'temp-hot';
          }
          
          // Humidity status and class
          if (hum < 30) {
            humStatus = 'VERY DRY';
            humClass = 'hum-dry';
          } else if (hum < 40) {
            humStatus = 'DRY';
            humClass = 'hum-dry';
          } else if (hum < 60) {
            humStatus = 'NORMAL';
            humClass = 'hum-normal';
          } else if (hum < 70) {
            humStatus = 'HUMID';
            humClass = 'hum-humid';
          } else {
            humStatus = 'VERY HUMID';
            humClass = 'hum-humid';
          }
          
          return { tempStatus, tempClass, humStatus, humClass };
        }
        
        function updateHistory(temp, hum) {
          const now = new Date();
          const timeStr = now.toLocaleTimeString('en-US', { hour12: false });
          
          history.unshift({ time: timeStr, temp, hum });
          if (history.length > MAX_HISTORY) history.pop();
          
          const listHTML = history.map(item => `
            <div class='history-item'>
              <span class='history-time'>${item.time}</span>
              <div class='history-values'>
                <span>T: ${item.temp}&deg;C</span>
                <span>H: ${item.hum}%</span>
              </div>
            </div>
          `).join('');
          
          document.getElementById('historyList').innerHTML = listHTML || '<div style="text-align:center; color:#999;">No data yet</div>';
        }
        
        function updatePrediction() {
          fetch('/predict')
            .then(res => res.json())
            .then(data => {
              const season = data.season || 'Unknown';
              const confidence = (data.confidence * 100).toFixed(1);
              
              document.getElementById('seasonName').innerText = season.toUpperCase();
              document.getElementById('confidence').innerText = confidence + '%';
              document.getElementById('confidenceBar').style.width = confidence + '%';
            })
            .catch(err => {
              console.log('Prediction error:', err);
              document.getElementById('seasonName').innerText = 'N/A';
              document.getElementById('confidence').innerText = '0%';
              document.getElementById('confidenceBar').style.width = '0%';
            });
        }
        
        function setNeoMode(mode) {
          fetch('/neo?mode='+mode)
          .then(response=>response.json())
          .then(json=>{
            document.getElementById('neoMode').innerText=json.mode;
            document.querySelectorAll('.neo-btn').forEach(btn => {
              btn.classList.remove('active');
            });
            event.target.closest('.neo-btn').classList.add('active');
          })
          .catch(err=>console.log(err));
        }
        
        function updateUI() {
          fetch('/sensors')
           .then(res=>res.json())
           .then(d=>{
             const temp = parseFloat(d.temp);
             const hum = parseFloat(d.hum);
             
             document.getElementById('temp').innerText = temp.toFixed(1);
             document.getElementById('hum').innerText = hum.toFixed(1);
             
             const status = getStatus(temp, hum);
             
             // Update temperature
             document.getElementById('tempStatus').innerText = status.tempStatus;
             const tempBox = document.getElementById('tempBox');
             tempBox.className = 'sensor-box ' + status.tempClass;
             
             // Update humidity
             document.getElementById('humStatus').innerText = status.humStatus;
             const humBox = document.getElementById('humBox');
             humBox.className = 'sensor-box ' + status.humClass;
             
             updateHistory(temp.toFixed(1), hum.toFixed(1));
           })
           .catch(err=>console.log(err));
          
          // Update prediction
          updatePrediction();
        }
        
        function initActiveButton() {
          let currentMode = document.getElementById('neoMode').innerText.toLowerCase();
          document.querySelectorAll('.neo-btn').forEach(btn => {
            if(btn.innerText.toLowerCase().includes(currentMode)) {
              btn.classList.add('active');
            }
          });
        }
        
        initActiveButton();
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

void handlePredict() {
  String seasonCopy;
  float confCopy;

  if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    seasonCopy = currentSeason;
    confCopy = currentConfidence;
    xSemaphoreGive(dataMutex);
  } else {
    seasonCopy = "Unknown";
    confCopy = 0.0f;
  }

  String json = "{\"season\":\"" + seasonCopy + "\",\"confidence\":" + String(confCopy/100, 2) + "}";
  server.send(200, "application/json", json);
}


void handleToggle() {
  int led = server.arg("led").toInt();
  if (led == 1) led1_state = !led1_state;
  else if (led == 2) led2_state = !led2_state;
  server.send(200, "application/json",
    "{\"led1\":\"" + String(led1_state ? "ON":"OFF") +
    "\",\"led2\":\"" + String(led2_state ? "ON":"OFF") + "\"}");
}

void handleSensors() {
  SensorData snapshot = {0.0f, 0.0f};

  if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    snapshot = latestData;
    xSemaphoreGive(dataMutex);
  } else {
    xSemaphoreGive(dataMutex);
  }

  String json = "{\"temp\":" + String(snapshot.temperature, 1) + 
               ",\"hum\":" + String(snapshot.humidity, 1) + "}";
  server.send(200, "application/json", json);
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
  server.on("/predict", HTTP_GET, handlePredict);

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