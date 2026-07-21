#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Wi-Fi Configuration
#define WIFI_SSID "Joan's Hotpot" // Replace with your Wi-Fi SSID
#define WIFI_PASSWORD "yummyhotpot" // Replace with your Wi-Fi Password

// Firebase Configuration
#define FIREBASE_HOST "cashier-security-default-rtdb.firebaseio.com" // Replace with your Firebase database URL
#define FIREBASE_AUTH "EDrv8aFlP9WqTKj8yn6LIiWs8F383xrP4bPdSngW"        // Replace with your Firebase Database Secret

FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;

// Hardware Configuration
#define RED_LED_PIN D5  // Red LED GPIO pin
#define BUZZER_PIN D6   // Buzzer GPIO pin

void setup() {
    Serial.begin(9600);

    // Initialize Wi-Fi
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi connected!");

    // Firebase configuration
    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;

    // Initialize Firebase
    Firebase.begin(&config, &auth);

    if (Firebase.ready()) {
        Serial.println("Connected to Firebase!");
    } else {
        Serial.println("Firebase initialization failed!");
    }

    // Initialize LED and Buzzer
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    // Turn off LED and Buzzer initially
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
    // Check Firebase for the alert status
    if (Firebase.getBool(firebaseData, "/alert")) {
        if (firebaseData.dataType() == "boolean") {
            bool alertStatus = firebaseData.boolData();

            if (alertStatus) {
                Serial.println("Alert detected! Turning ON red LED and buzzer.");
                digitalWrite(RED_LED_PIN, HIGH);
                digitalWrite(BUZZER_PIN, HIGH);
            } else {
                Serial.println("No alert. Turning OFF red LED and buzzer.");
                digitalWrite(RED_LED_PIN, LOW);
                digitalWrite(BUZZER_PIN, LOW);
            }
        } else {
            Serial.println("Unexpected data type from Firebase.");
        }
    } else {
        Serial.print("Error getting data from Firebase: ");
        Serial.println(firebaseData.errorReason());
    }

    // Add a delay to avoid rapid requests
    delay(1000);
}
