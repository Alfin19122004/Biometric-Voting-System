#include <SPI.h>
#include <TFT_eSPI.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

using namespace fs;

// ============ CONFIGURATION ============
#define CS_LEFT   17
#define CS_RIGHT  5
#define RST_LEFT  21
#define RST_RIGHT 4

#define SCAN_TIMEOUT 10000
#define TOUCH_DEBOUNCE 300
#define CALIBRATION_FILE "/calibrationData"

// ============ COLOR SCHEME ============
#define COLOR_PRIMARY    0x1F63
#define COLOR_SECONDARY  0xF800
#define COLOR_SUCCESS    0x07E0
#define COLOR_WARNING    0xFD20
#define COLOR_DARK       0x0000
#define COLOR_LIGHT      0xFFFF
#define COLOR_ACCENT     0x00FD

TFT_eSPI tft = TFT_eSPI();

// ============ FINGERPRINT ============
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// ============ WIFI ============
// const char* ssid = "RH-2.4G-BE0A30";
// const char* password = "44953BBE0A30";
const char* ssid = "Alfin";
const char* password = ".........";
// ============ SERVER ============
String server = "https://biometric-voting.onrender.com";

String state = "Tamil Nadu";
String district = "Chennai";
String constituency = "ABC";

int voter_id = -1;

// ============ CANDIDATE DATA ============
struct Candidate {
  int id;
  String name;
} candidates[5];

int totalCandidates = 0;
bool alreadyVoted = false;

bool allowVote = false;
bool fingerprintScanning = false;
unsigned long fingerprintScanStartTime = 0;

// ============ TOUCH ZONES ============
struct CandidateZone {
  int x1, x2, y1, y2;
  int candidateIndex;
} candidateZones[5];

struct ButtonZone {
  int x1, x2, y1, y2;
  String action;
} currentButton;

// ============ SCREEN SELECT ============
void selectLeft() {
  digitalWrite(CS_RIGHT, HIGH);
  digitalWrite(CS_LEFT, LOW);
}

void selectRight() {
  digitalWrite(CS_LEFT, HIGH);
  digitalWrite(CS_RIGHT, LOW);
}

// ============ RESET ============
void resetLeft() {
  digitalWrite(RST_LEFT, LOW);
  delay(20);
  digitalWrite(RST_LEFT, HIGH);
  delay(150);
}

void resetRight() {
  digitalWrite(RST_RIGHT, LOW);
  delay(20);
  digitalWrite(RST_RIGHT, HIGH);
  delay(150);
}

// ============ TOUCH CALIBRATION (FORCE NEW CALIBRATION) ============
void touchCalibrate() {
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // FORCE CALIBRATION - Delete old file to recalibrate
  // Comment this line out after first successful calibration
  if (SPIFFS.exists(CALIBRATION_FILE)) {
   // SPIFFS.remove(CALIBRATION_FILE);----------------------------------------------------------------------------------
    Serial.println("Deleted old calibration file - Running new calibration...");
  }

  // Check if calibration file exists
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    File f = SPIFFS.open(CALIBRATION_FILE, "r");
    if (f) {
      if (f.readBytes((char *)calData, 14) == 14)
        calDataOK = 1;
      f.close();
    }
  }

  // If no calibration found, run calibration
  if (!calDataOK) {
    Serial.println("Running touch calibration...");
    selectRight();
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 20);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.println("Touch corners as indicated");

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
      Serial.println("Calibration data saved!");
    }
  } else {
    Serial.println("Loading existing calibration data...");
    tft.setTouch(calData);
  }
}

// ============ DRAWING UTILITIES ============
void drawCenteredText(const char* text, int y, uint16_t size, uint16_t color) {
  tft.setTextColor(color);
  tft.setTextSize(size);
  
  int charWidth = 6 * size;
  int textLength = strlen(text);
  int x = (320 - (textLength * charWidth)) / 2;
  
  tft.setCursor(x, y);
  tft.print(text);
}

void drawRoundButton(int x, int y, int w, int h, const char* text, uint16_t bgColor, uint16_t textColor) {
  tft.fillRect(x, y, w, h, bgColor);
  tft.drawRect(x, y, w, h, textColor);
  
  tft.setTextColor(textColor);
  tft.setTextSize(2);
  
  int charWidth = 12;
  int textLength = strlen(text);
  int textX = x + (w - (textLength * charWidth)) / 2;
  int textY = y + (h - 16) / 2;
  
  tft.setCursor(textX, textY);
  tft.print(text);
}

void drawHeader(const char* title, uint16_t bgColor) {
  tft.fillRect(0, 0, 320, 50, bgColor);
  tft.setTextColor(COLOR_LIGHT);
  tft.setTextSize(2);
  
  int charWidth = 12;
  int textLength = strlen(title);
  int x = (320 - (textLength * charWidth)) / 2;
  
  tft.setCursor(x, 15);
  tft.print(title);
  
  tft.drawLine(0, 50, 320, 50, COLOR_LIGHT);
}

// ============ UI - WELCOME SCREEN ============
void showWelcomeScreen() {
  selectLeft();
  tft.fillScreen(COLOR_PRIMARY);
  
  drawHeader("VOTING SYSTEM", COLOR_SECONDARY);
  
  tft.setTextColor(COLOR_LIGHT);
  tft.setTextSize(2);
  tft.setCursor(20, 100);
  tft.println("Welcome to");
  tft.setCursor(20, 140);
  tft.println("Secure Voting");
  tft.setCursor(20, 180);
  tft.println("System");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(20, 240);
  tft.println("Authorized Centers Only");

  selectRight();
  tft.fillScreen(COLOR_DARK);
  
  tft.fillCircle(160, 100, 40, COLOR_SUCCESS);
  tft.setTextColor(COLOR_DARK);
  tft.setTextSize(3);
  tft.setCursor(145, 85);
  tft.print("OK");
  
  tft.setTextColor(COLOR_LIGHT);
  tft.setTextSize(2);
  tft.setCursor(40, 160);
  tft.println("Ready to vote?");
  
  tft.setTextSize(1);
  tft.setCursor(30, 190);
  tft.println("Press button below");
  
  drawRoundButton(40, 240, 240, 50, "START VOTING", COLOR_SUCCESS, COLOR_LIGHT);
  
  currentButton.x1 = 40;
  currentButton.x2 = 280;
  currentButton.y1 = 240;
  currentButton.y2 = 290;
  currentButton.action = "start_voting";
}

// ============ UI - LOCATION SCREEN ============
void showLocationScreen() {
  selectLeft();
  tft.fillScreen(COLOR_PRIMARY);
  
  drawHeader("VOTING LOCATION", COLOR_ACCENT);
  
  tft.setTextColor(COLOR_LIGHT);
  tft.setTextSize(1);
  
  tft.setCursor(20, 70);
  tft.print("State:");
  tft.setTextSize(2);
  tft.setTextColor(COLOR_SUCCESS);
  tft.setCursor(20, 90);
  tft.println(state);
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_LIGHT);
  tft.setCursor(20, 130);
  tft.print("District:");
  tft.setTextSize(2);
  tft.setTextColor(COLOR_SUCCESS);
  tft.setCursor(20, 150);
  tft.println(district);
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_LIGHT);
  tft.setCursor(20, 190);
  tft.print("Constituency:");
  tft.setTextSize(2);
  tft.setTextColor(COLOR_SUCCESS);
  tft.setCursor(20, 210);
  tft.println(constituency);

  selectRight();
  tft.fillScreen(COLOR_DARK);
  
  tft.setTextColor(COLOR_LIGHT);
  tft.setTextSize(2);
  tft.setCursor(40, 80);
  tft.println("Location Info:");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(40, 120);
  tft.println("You are voting at:");
  tft.setCursor(40, 140);
  tft.println(state);
  tft.setCursor(40, 160);
  tft.println(district);
  
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(40, 200);
  tft.println("Confirm location?");
  
  drawRoundButton(40, 240, 240, 50, "CONTINUE", COLOR_SUCCESS, COLOR_LIGHT);
  
  currentButton.x1 = 40;
  currentButton.x2 = 280;
  currentButton.y1 = 240;
  currentButton.y2 = 290;
  currentButton.action = "next";
}

// ============ UI - FINGERPRINT SCREEN ============
void showPlaceFinger() {
  selectLeft();
  tft.fillScreen(COLOR_PRIMARY);
  
  drawHeader("FINGERPRINT", COLOR_SECONDARY);
  
  tft.setTextColor(COLOR_LIGHT);
  tft.setTextSize(2);
  tft.setCursor(20, 120);
  tft.println("Place your");
  tft.setCursor(20, 160);
  tft.println("finger on");
  tft.setCursor(20, 200);
  tft.println("the sensor");

  selectRight();
  tft.fillScreen(COLOR_DARK);
  
  tft.fillCircle(160, 80, 40, COLOR_WARNING);
  tft.setTextColor(COLOR_DARK);
  tft.setTextSize(3);
  tft.setCursor(140, 70);
  tft.print("~");
  
  tft.setTextColor(COLOR_LIGHT);
  tft.setTextSize(2);
  tft.setCursor(80, 150);
  tft.println("SCANNING...");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(60, 200);
  tft.println("Please wait");
  tft.setCursor(50, 220);
  tft.println("Identifying fingerprint");
  
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(30, 270);
  tft.println("Timeout in 10 seconds");
  
  currentButton.x1 = -1;
}

// ============ UI - ACCESS SCREEN ============
void showAccess(bool ok, bool already = false) {
  uint16_t bgColor = ok ? COLOR_SUCCESS : (already ? COLOR_WARNING : COLOR_SECONDARY);
  uint16_t textColor = COLOR_DARK;
  
  selectLeft();
  tft.fillScreen(bgColor);
  
  drawHeader(ok ? "GRANTED" : (already ? "ALREADY VOTED" : "DENIED"), bgColor);
  
  tft.setTextColor(textColor);
  tft.setTextSize(3);
  
  if (ok) {
    tft.setCursor(80, 130);
    tft.print("SUCCESS");
    tft.setCursor(50, 180);
    tft.setTextSize(2);
    tft.println("You are eligible");
    tft.setCursor(50, 210);
    tft.println("to vote");
  } else if (already) {
    tft.setCursor(40, 140);
    tft.print("ALREADY");
    tft.setCursor(50, 190);
    tft.print("VOTED");
  } else {
    tft.setCursor(60, 140);
    tft.print("DENIED");
    tft.setCursor(30, 200);
    tft.setTextSize(2);
    tft.println("Access not granted");
  }

  selectRight();
  tft.fillScreen(bgColor);
  
  tft.setTextColor(textColor);
  tft.setTextSize(2);
  
  if (ok) {
    tft.setCursor(50, 100);
    tft.println("Proceed to");
    tft.setCursor(50, 140);
    tft.println("vote casting");
  } else {
    tft.setCursor(30, 100);
    tft.println("Contact the");
    tft.setCursor(30, 140);
    tft.println("election officer");
  }
  
  drawRoundButton(40, 240, 240, 50, "CONTINUE", COLOR_LIGHT, bgColor);
  
  currentButton.x1 = 40;
  currentButton.x2 = 280;
  currentButton.y1 = 240;
  currentButton.y2 = 290;
  currentButton.action = "back";
}

// ============ UI - DRAW CANDIDATES (CALIBRATED) ============
void drawCandidates() {
  selectRight();
  tft.fillScreen(COLOR_DARK);
  
  Serial.println("\n========== CANDIDATES DRAWING (CALIBRATED) ==========");
  
  // Using calibrated coordinates from old code
  for (int i = 0; i < totalCandidates && i < 4; i++) {
    int boxY = 60 + (i * 90);  // Box spacing: 60, 150, 240, 330
    int boxHeight = 70;
    
    uint16_t color = (candidates[i].name == "NOTA") ? COLOR_SECONDARY : COLOR_PRIMARY;
    uint16_t textColor = COLOR_LIGHT;
    
    // Draw box using calibrated coordinates
    tft.fillRect(40, boxY, 240, boxHeight, color);
    tft.drawRect(40, boxY, 240, boxHeight, textColor);
    
    // Draw text
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    int textX = 70;
    int textY = boxY + (boxHeight - 16) / 2;
    tft.setCursor(textX, textY);
    tft.print(candidates[i].name);
    
    // Store calibrated zones - matching old code exactly
    candidateZones[i].x1 = 40;
    candidateZones[i].x2 = 280;
    candidateZones[i].y1 = boxY;
    candidateZones[i].y2 = boxY + boxHeight;
    candidateZones[i].candidateIndex = i;
    
    Serial.print("Candidate[");
    Serial.print(i);
    Serial.print("] X[40-280] Y[");
    Serial.print(candidateZones[i].y1);
    Serial.print("-");
    Serial.print(candidateZones[i].y2);
    Serial.print("] -> ");
    Serial.println(candidates[i].name);
  }
  Serial.println("======================================================\n");

  selectLeft();
  tft.fillScreen(COLOR_PRIMARY);
  
  drawHeader("CAST YOUR VOTE", COLOR_SUCCESS);
  
  tft.setTextColor(COLOR_LIGHT);
  tft.setTextSize(2);
  tft.setCursor(20, 80);
  tft.println("Select your");
  tft.setCursor(20, 120);
  tft.println("preferred");
  tft.setCursor(20, 160);
  tft.println("candidate");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(20, 210);
  tft.println("Touch the candidate");
  tft.setCursor(20, 230);
  tft.println("on the right screen");
  
  currentButton.x1 = -1;
}

// ============ FINGERPRINT FUNCTIONS ============
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    return -1;
  }
  
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    return -1;
  }
  
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    return -1;
  }
  
  int fid = finger.fingerID;
  Serial.print("Fingerprint found: ID=");
  Serial.println(fid);
  return fid;
}

// ============ SERVER VERIFICATION ============
bool verifyFromServer(int fid) {
  WiFiClientSecure client;
  client.setInsecure();  // Ignore SSL certificate

  HTTPClient http;
  http.begin(client, server + "/verify_fingerprint");
  http.addHeader("Content-Type", "application/json");

  String json = "{\"fingerprint_id\":" + String(fid) +
                ",\"state\":\"" + state +
                "\",\"district\":\"" + district +
                "\",\"constituency\":\"" + constituency + "\"}";

  Serial.println("Sending: " + json);
  int code = http.POST(json);
  String response = http.getString();
  http.end();

  Serial.println("Response Code: " + String(code));
  Serial.println("Response Body: " + response);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, response);

  if (error) {
    Serial.print("JSON error: ");
    Serial.println(error.c_str());
    return false;
  }

  const char* status = doc["status"];
  Serial.print("Status: ");
  Serial.println(status);

  if (status == NULL) {
    Serial.println("Status is NULL!");
    return false;
  }

  if (strcmp(status, "valid") == 0) {
    voter_id = doc["voter_id"];
    alreadyVoted = false;
    Serial.print("Voter verified: ID=");
    Serial.println(voter_id);
    return true;
  } else if (strcmp(status, "already_voted") == 0) {
    alreadyVoted = true;
    Serial.println("Already voted!");
    return false;
  } else {
    Serial.println("Access denied");
    alreadyVoted = false;
    return false;
  }
}

// ============ GET CANDIDATES ============
void getCandidates() {
  WiFiClientSecure client;
  client.setInsecure();
  // Clear arrays
  for (int i = 0; i < 5; i++) {
    candidates[i].id = -1;
    candidates[i].name = "";
  }
  totalCandidates = 0;

  HTTPClient http;
  http.begin(server + "/get_candidates");
  http.addHeader("Content-Type", "application/json");

  String json = "{\"constituency\":\"" + constituency + "\"}";
  
  Serial.println("Requesting candidates...");
  int code = http.POST(json);
  String response = http.getString();
  http.end();

  Serial.println("Response Code: " + String(code));
  Serial.println("Response Body: " + response);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, response);

  if (error) {
    Serial.print("JSON error: ");
    Serial.println(error.c_str());
    return;
  }

  totalCandidates = doc.size();
  Serial.print("Total Candidates: ");
  Serial.println(totalCandidates);
  
  if (totalCandidates > 4) {
    totalCandidates = 4;
  }
  
  for (int i = 0; i < totalCandidates; i++) {
    candidates[i].id = doc[i]["id"].as<int>();
    candidates[i].name = doc[i]["name"].as<String>();
    
    Serial.print("Index[");
    Serial.print(i);
    Serial.print("] -> ID=");
    Serial.print(candidates[i].id);
    Serial.print(" NAME=");
    Serial.println(candidates[i].name);
  }
}

// ============ SEND VOTE ============
void sendVote(int indexSelected) {
  WiFiClientSecure client;
  client.setInsecure();

  int candidateID = candidates[indexSelected].id;
  String candidateName = candidates[indexSelected].name;

  Serial.println("\n========== SENDING VOTE ==========");
  Serial.print("Selected Index: ");
  Serial.println(indexSelected);
  Serial.print("Voter ID: ");
  Serial.println(voter_id);
  Serial.print("Candidate ID: ");
  Serial.println(candidateID);
  Serial.print("Candidate Name: ");
  Serial.println(candidateName);
  Serial.println("===================================\n");

  HTTPClient http;
  http.begin(server + "/vote");
  http.addHeader("Content-Type", "application/json");

  String json = "{\"voter_id\":" + String(voter_id) +
                ",\"candidate_id\":" + String(candidateID) + "}";

  Serial.println("Sending: " + json);
  int code = http.POST(json);
  String response = http.getString();
  http.end();

  Serial.println("Vote Response Code: " + String(code));
  Serial.println("Vote Response Body: " + response);

  selectRight();
  tft.fillScreen(COLOR_SUCCESS);
  tft.setTextColor(COLOR_DARK);
  tft.setTextSize(3);
  tft.setCursor(60, 80);
  tft.println("VOTE");
  tft.setCursor(40, 130);
  tft.println("RECORDED");
  
  tft.setTextSize(2);
  tft.setTextColor(COLOR_DARK);
  tft.setCursor(50, 200);
  tft.println("Thank you!");

  selectLeft();
  tft.fillScreen(COLOR_SUCCESS);
  tft.setTextColor(COLOR_DARK);
  tft.setTextSize(2);
  tft.setCursor(20, 80);
  tft.println("Vote cast for:");
  tft.setCursor(20, 130);
  tft.setTextSize(3);
  tft.println(candidateName);
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_DARK);
  tft.setCursor(20, 200);
  tft.println("Your vote has been");
  tft.setCursor(20, 220);
  tft.println("securely recorded");

  delay(4000);
}

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n\n========== SYSTEM STARTING ==========\n");

  pinMode(CS_LEFT, OUTPUT);
  pinMode(CS_RIGHT, OUTPUT);
  pinMode(RST_LEFT, OUTPUT);
  pinMode(RST_RIGHT, OUTPUT);

  digitalWrite(CS_LEFT, HIGH);
  digitalWrite(CS_RIGHT, HIGH);

  SPI.begin(18, 19, 23);
  SPIFFS.begin(true);

  selectLeft();
  resetLeft();
  tft.init();
  tft.setRotation(2);

  selectRight();
  resetRight();
  tft.init();
  tft.setRotation(2);
  touchCalibrate();  // This will ask for 4 corner touches

  mySerial.begin(57600, SERIAL_8N1, 27, 26);
  finger.begin(57600);

  if (!finger.verifyPassword()) {
    Serial.println("ERROR: Fingerprint sensor!");
    selectRight();
    tft.fillScreen(COLOR_SECONDARY);
    tft.setTextColor(COLOR_LIGHT);
    tft.setTextSize(2);
    tft.setCursor(40, 150);
    tft.println("SENSOR ERROR");
    while (1) delay(100);
  }

  Serial.println("Fingerprint OK");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Failed!");
    selectRight();
    tft.fillScreen(COLOR_SECONDARY);
    tft.setTextColor(COLOR_LIGHT);
    tft.setTextSize(2);
    tft.setCursor(40, 150);
    tft.println("WIFI ERROR");
    delay(3000);
    ESP.restart();
  }
Serial.println("Testing server connection...");

WiFiClientSecure client;
client.setInsecure();
HTTPClient http;

http.begin(client, server + "/live_stats");
int code = http.GET();

if (code > 0) {
  Serial.println("Server Connected!");
  Serial.println(http.getString());
} else {
  Serial.println("Server Not Reachable!");
}

http.end();
  Serial.println("========== SYSTEM READY ==========\n");
  showWelcomeScreen();
}

// ============ MAIN LOOP ============
void loop() {
  static int step = 0;
  static uint16_t x, y;
  static unsigned long lastTouchTime = 0;
  unsigned long currentTime = millis();

  if (step == 2 && fingerprintScanning) {
    int fid = getFingerprintID();
    
    if (fid != -1) {
      fingerprintScanning = false;
      
      if (verifyFromServer(fid)) {
        showAccess(true, false);
        delay(2000);
        getCandidates();
        drawCandidates();
        allowVote = true;
        step = 3;
      } else {
        showAccess(false, alreadyVoted);
        fingerprintScanning = false;
      }
    } else if (currentTime - fingerprintScanStartTime > SCAN_TIMEOUT) {
      Serial.println("Fingerprint timeout");
      fingerprintScanning = false;
      showAccess(false, false);
    }
  }

  selectRight();
  if (tft.getTouch(&x, &y) && (currentTime - lastTouchTime) > TOUCH_DEBOUNCE) {
    lastTouchTime = currentTime;
    
    Serial.print("Touch: X=");
    Serial.print(x);
    Serial.print(" Y=");
    Serial.println(y);

    if (step == 0 && currentButton.action == "start_voting") {
      if (x >= currentButton.x1 && x <= currentButton.x2 && 
          y >= currentButton.y1 && y <= currentButton.y2) {
        Serial.println("-> Moving to Location Screen");
        step = 1;
        showLocationScreen();
      }
    }
    else if (step == 1 && currentButton.action == "next") {
      if (x >= currentButton.x1 && x <= currentButton.x2 && 
          y >= currentButton.y1 && y <= currentButton.y2) {
        Serial.println("-> Moving to Fingerprint Screen");
        step = 2;
        fingerprintScanning = true;
        fingerprintScanStartTime = currentTime;
        voter_id = -1;
        alreadyVoted = false;
        showPlaceFinger();
      }
    }
    else if (step == 2 && currentButton.action == "back") {
      if (x >= currentButton.x1 && x <= currentButton.x2 && 
          y >= currentButton.y1 && y <= currentButton.y2) {
        Serial.println("-> Back to Welcome");
        fingerprintScanning = false;
        voter_id = -1;
        allowVote = false;
        alreadyVoted = false;
        step = 0;
        showWelcomeScreen();
      }
    }
    else if (step == 3 && allowVote) {
      Serial.print("Voting touch at X=");
      Serial.print(x);
      Serial.print(" Y=");
      Serial.println(y);
      
      // CANDIDATE TOUCH DETECTION - CALIBRATED
      for (int i = 0; i < totalCandidates && i < 4; i++) {
        if (x >= candidateZones[i].x1 && x <= candidateZones[i].x2 &&
            y >= candidateZones[i].y1 && y <= candidateZones[i].y2) {
          
          int selectedIndex = candidateZones[i].candidateIndex;
          Serial.print("*** MATCH! Zone ");
          Serial.print(i);
          Serial.print(" -> Selected Index: ");
          Serial.print(selectedIndex);
          Serial.print(" (");
          Serial.print(candidates[selectedIndex].name);
          Serial.println(")");
          
          sendVote(selectedIndex);
          allowVote = false;
          voter_id = -1;
          alreadyVoted = false;
          step = 0;
          showWelcomeScreen();
          break;
        }
      }
    }
  }

  delay(10);
}