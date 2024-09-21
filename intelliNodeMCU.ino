#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

const char* ssid = "Madhav";
const char* password = "6305991151";

#define FIREBASE_HOST "sensor-data-26db9-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "UZjoSxe0bUUAcHbeKqLqY4zgUF136rWXOLkAEIDU"
// Define sensor pins and relay control pins
#define SOIL_SENSOR_1_PIN A0
#define SOIL_SENSOR_2_PIN D1
#define RELAY_1_PIN D2
#define RELAY_2_PIN D3

int moisture1;
int moisture2;
int threshold = 20;  // Threshold value for triggering solenoids

FirebaseData firebaseData;

void setup() {
  // Initialize Serial
  Serial.begin(115200);

  // Initialize the relay pins
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);

  // Ensure relays are off initially
  digitalWrite(RELAY_1_PIN, LOW);
  digitalWrite(RELAY_2_PIN, LOW);

  // Connect to Wi-Fi
  connectToWiFi();

  // Connect to Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  Serial.println("Setup done!");
}

void loop() {
  // Read soil moisture levels
  moisture1 = analogRead(SOIL_SENSOR_1_PIN); // Reading from sensor 1
  moisture2 = analogRead(SOIL_SENSOR_2_PIN); // Reading from sensor 2

  // Normalize sensor values (0 to 1024) for more readable values, optional
  moisture1 = map(moisture1, 0, 1024, 0, 100);  // Assuming values 0 to 100% moisture
  moisture2 = map(moisture2, 0, 1024, 0, 100);

  // Check moisture levels and control the relays
  controlRelays(moisture1, moisture2);

  // Update Firebase with the current data
  updateFirebase();

  // Check for any changes in valve state from Firebase
  fetchFirebaseData();

  // Small delay to avoid overwhelming Firebase
  delay(5000);  // 5 seconds delay
}

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
}

void controlRelays(int moisture1, int moisture2) {
  // If moisture1 is below threshold, trigger relay 1
  if (moisture1 < threshold) {
    digitalWrite(RELAY_1_PIN, HIGH);  // Activate solenoid 1
  } else {
    digitalWrite(RELAY_1_PIN, LOW);   // Deactivate solenoid 1
  }

  // If moisture2 is below threshold, trigger relay 2
  if (moisture2 < threshold) {
    digitalWrite(RELAY_2_PIN, HIGH);  // Activate solenoid 2
  } else {
    digitalWrite(RELAY_2_PIN, LOW);   // Deactivate solenoid 2
  }
}

void updateFirebase() {
  // Push moisture levels to Firebase
  Firebase.setInt(firebaseData, "/sensors/moisture1", moisture1);
  Firebase.setInt(firebaseData, "/sensors/moisture2", moisture2);

  // Push relay states to Firebase (as valve states)
  int valve1State = digitalRead(RELAY_1_PIN);
  int valve2State = digitalRead(RELAY_2_PIN);

  Firebase.setInt(firebaseData, "/valves/valve1", valve1State);
  Firebase.setInt(firebaseData, "/valves/valve2", valve2State);
}

void fetchFirebaseData() {
  // Fetch Firebase data for valve control and trigger relays accordingly
  if (Firebase.getInt(firebaseData, "/valves/valve1")) {
    if (firebaseData.intData() == 1) {
      // If solenoid 1 is not currently watering
      if (millis() - lastWaterTime1 >= wateringDuration) {
        Serial.println("Valve 1 Opened from Firebase: Watering for 3 seconds...");
        digitalWrite(RELAY_1_PIN, HIGH);  // Open solenoid 1
        lastWaterTime1 = millis();        // Record the time of opening
      }
    } else {
      // Only close the valve if we're not in the middle of a watering delay
      if (millis() - lastWaterTime1 >= wateringDuration) {
        digitalWrite(RELAY_1_PIN, LOW);   // Close solenoid 1
      }
    }
  }

  if (Firebase.getInt(firebaseData, "/valves/valve2")) {
    if (firebaseData.intData() == 1) {
      // If solenoid 2 is not currently watering
      if (millis() - lastWaterTime2 >= wateringDuration) {
        Serial.println("Valve 2 Opened from Firebase: Watering for 3 seconds...");
        digitalWrite(RELAY_2_PIN, HIGH);  // Open solenoid 2
        lastWaterTime2 = millis();        // Record the time of opening
      }
    } else {
      // Only close the valve if we're not in the middle of a watering delay
      if (millis() - lastWaterTime2 >= wateringDuration) {
        digitalWrite(RELAY_2_PIN, LOW);   // Close solenoid 2
      }
    }
  }
}
