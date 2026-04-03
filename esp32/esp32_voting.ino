#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

String server = "http://192.168.1.5:5000";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  Serial.println("Connected!");
}

void sendFingerprint(int id) {
  HTTPClient http;
  http.begin(server + "/verify_fingerprint");
  http.addHeader("Content-Type", "application/json");

  String json = "{\"fingerprint_id\":" + String(id) +
                ",\"state\":\"Tamil Nadu\",\"district\":\"Chennai\",\"constituency\":\"ABC\"}";

  int code = http.POST(json);
  String res = http.getString();

  Serial.println(res);
  http.end();
}

void loop() {
  int fakeID = 1; // replace with fingerprint scan
  sendFingerprint(fakeID);
  delay(10000);
}