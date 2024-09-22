#include <ESP8266WiFi.h>  // Include the ESP8266 Wi-Fi library
#include <FirebaseESP8266.h>  // Include Firebase library

// Define your Wi-Fi credentials
const char* ssid = "Madhav";        // Replace with your Wi-Fi SSID
const char* password = "6305991151"; // Replace with your Wi-Fi password

// Define Firebase credentials
#define FIREBASE_HOST "sensor-data-26db9-default-rtdb.firebaseio.com"  // Replace with your Firebase project URL
#define FIREBASE_AUTH "AIzaSyAG3yXXSSjtwxCa6RPQbIjlQU51ICGZCMQ"         // Replace with your Firebase database secret

FirebaseData firebaseData;

FirebaseAuth auth;
FirebaseConfig config;

const int soil1Pin = A0;   // A0 for Soil Moisture Sensor 1
const int relayPin = D5;    // Relay control pin (adjust pin as needed)

void setup() {
  Serial.begin(9600);  // Start serial communication at 9600 baud
  pinMode(relayPin, OUTPUT);  // Set the relay pin as output
  digitalWrite(relayPin, HIGH); // Ensure the relay is off initially

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");  // Print dots while connecting
  }
  Serial.println("\nConnected to Wi-Fi!");

  // Initialize Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  delay(1000);
}

void loop() {
  int soil1Value = analogRead(soil1Pin);  // Read the analog value from A0
  int scaledSoilValue = map(soil1Value, 0, 1023, 1, 100);
  Serial.print("Soil 1 Value: ");
  Serial.println(soil1Value);  // Print the raw value for debugging

  // Read valve1 value from Firebase
  if (Firebase.getInt(firebaseData, "/valve1")) {
    if (firebaseData.dataAvailable()) {
      int valveStatus = firebaseData.intData();  // Get valve1 value
      if (valveStatus == 1) {
        digitalWrite(relayPin, LOW);  // Open the relay
        Serial.println("Relay opened from Firebase command.");
      } else {
        digitalWrite(relayPin, HIGH);  // Close the relay
        Serial.println("Relay closed from Firebase command.");
      }
    } else {
      Serial.println("No data available for valve1.");
    }
  } else {
    Serial.println("Failed to read valve1 from Firebase.");
  }

  // Update Firebase with the soil moisture value
  Firebase.setInt(firebaseData, "/soil1", 100-scaledSoilValue);

  // Check if the soil moisture value indicates dryness
  if (soil1Value > 600) {  // Threshold for dry soil
    // Update Firebase to indicate watering is in progress
    Firebase.setInt(firebaseData, "/valve1", 1); // Set valve1 to 1
    digitalWrite(relayPin, LOW);  // Turn on the relay (open valve)
    delay(3000);                   // Water for 3 seconds
    digitalWrite(relayPin, HIGH); // Turn off the relay (close valve)
    Firebase.setInt(firebaseData, "/valve1", 0); // Set valve1 to 0
  }

  delay(1000);  // Wait for 1 second before the next reading
}
