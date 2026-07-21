#include <SPI.h>
#include <MFRC522.h>
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

#define RST_PIN         D0          // Reset pin
#define SS_PIN          D2          // SDA pin
#define RED_LED         D4          // Red LED pin
#define GREEN_LED       D3          // Green LED pin

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

// Ultrasonic sensor configuration
#define TRIG_PIN        D1          // Trigger pin for ultrasonic sensor
#define ECHO_PIN        D8          // Echo pin for ultrasonic sensor

// Predefined UID for the correct card (replace with your card's UID)
byte correctUID[] = {0xFA, 0x57, 0xBD, 0xFB}; // Example UID

bool cardDetected = false;
bool correctCard = false;

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

    // Initialize SPI and RFID reader
    SPI.begin();
    mfrc522.PCD_Init();
    delay(4);
    mfrc522.PCD_DumpVersionToSerial();
    Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

    // Initialize LED pins
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);

    // Initialize ultrasonic sensor pins
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // Turn on the red LED by default
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
}

void loop() {
    if (mfrc522.PICC_IsNewCardPresent()) {
        if (mfrc522.PICC_ReadCardSerial()) {
            cardDetected = true;
            correctCard = checkUID(mfrc522.uid.uidByte, mfrc522.uid.size);

            if (correctCard) {
                Serial.println("Correct card: Unlock");
                logUserToFirebase(true);
                logAlertToFirebase(false);
                digitalWrite(RED_LED, LOW);
                digitalWrite(GREEN_LED, HIGH);
                while (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                    delay(100);
                }
            } else {
                logUserToFirebase(false);
                Serial.println("Wrong card detected");
                // logToFirebase("Unauthorized access attempt detected!");
                blinkRedLED();
                while (mfrc522.PICC_IsNewCardPresent()) {
                    blinkRedLED();
                }
                digitalWrite(GREEN_LED, LOW);
                digitalWrite(RED_LED, HIGH);
            }
        }
        mfrc522.PICC_HaltA();
    } else {
        logUserToFirebase(false);
        
        cardDetected = false;
        correctCard = false;

        float distance = measureDistance();
        if (distance > 4) {
            Serial.print("No card and object detected at distance: ");
            Serial.print(distance);
            Serial.println(" cm. Logging alert to Firebase...");
            logAlertToFirebase(true);
            digitalWrite(GREEN_LED, LOW);
            blinkRedLED();
        } else {
            logAlertToFirebase(false);
            digitalWrite(RED_LED, HIGH);
            digitalWrite(GREEN_LED, LOW);
        }
    }
}

bool checkUID(byte* scannedUID, byte length) {
    if (length != sizeof(correctUID)) return false;
    for (byte i = 0; i < length; i++) {
        if (scannedUID[i] != correctUID[i]) return false;
    }
    return true;
}

void blinkRedLED() {
    digitalWrite(RED_LED, LOW);
    delay(200);
    digitalWrite(RED_LED, HIGH);
    delay(200);
}

float measureDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    return (duration * 0.034) / 2;
}

bool lastAlertStatus = false;
bool lastUserStatus = false;

void logAlertToFirebase(bool alertStatus) {
    if (alertStatus != lastAlertStatus) { // Only update if the status has changed
        Serial.print("Pushing alert status to Firebase: ");
        Serial.println(alertStatus ? "true" : "false");

        if (Firebase.setBool(firebaseData, "/alert", alertStatus)) {
            Serial.println("Alert status updated successfully!");
            lastAlertStatus = alertStatus; // Update the local state
        } else {
            Serial.print("Error updating alert status: ");
            Serial.println(firebaseData.errorReason());
        }
    }
}

void logUserToFirebase(bool userStatus) {
    if (userStatus != lastUserStatus) { // Only update if the status has changed
        Serial.print("Pushing user status to Firebase: ");
        Serial.println(userStatus ? "true" : "false");

        if (Firebase.setBool(firebaseData, "/user", userStatus)) {
            Serial.println("User status updated successfully!");
            lastUserStatus = userStatus; // Update the local state
        } else {
            Serial.print("Error updating user status: ");
            Serial.println(firebaseData.errorReason());
        }
    }
}