#define BLYNK_TEMPLATE_ID "TMPL3ca0PeGpo"
#define BLYNK_TEMPLATE_NAME "Pothole Sensor"
#define BLYNK_AUTH_TOKEN "a78gryDH7refH3gprVgUn7a1YqIF6VPX"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <TinyGPS++.h>

// Blynk credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "realmeGT2";
char pass[] = "1234sonu";

// Sensor objects
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
TinyGPSPlus gps;

// Ultrasonic sensor pins
#define TRIG_PIN 26
#define ECHO_PIN 27

// GPS pins
#define RXD2 16
#define TXD2 17
HardwareSerial GPS(2);

// Constants
const float SENSOR_BASELINE_CM = 3;    // Baseline distance from sensor to the ground
const float POTHOLE_THRESHOLD_CM = 5; // Distance above which a pothole is detected
const float ACCEL_POTHOLE_Z = -3.0;   // Z-axis threshold for pothole detection
const float ACCEL_BUMP_Z = 3.0;       // Z-axis threshold for bump detection

void setup() {
    Serial.begin(115200);

    // Initialize Blynk
    Blynk.begin(auth, ssid, pass);
    Blynk.virtualWrite(V0, "Blynk is working!");

    // Initialize sensors
    if (!accel.begin()) {
        Serial.println("Failed to initialize ADXL345");
        while (1);
    }
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // Initialize GPS
    GPS.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
    Blynk.run();
    readSensors();
}

void readSensors() {
    // Read ultrasonic sensor
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH);

    float distance = (duration * 0.034) / 2; // Calculate distance in cm
    if (distance <= 0 || distance > 50) {   // Ignore invalid or out-of-range values
        distance = SENSOR_BASELINE_CM;      // Default to baseline if reading is invalid
    }

    Serial.print("Measured Distance: ");
    Serial.println(distance);

    // Read ADXL345 accelerometer
    sensors_event_t event;
    accel.getEvent(&event);
    float x = event.acceleration.x;
    float y = event.acceleration.y;
    float z = event.acceleration.z;

    Serial.print("Acceleration X: ");
    Serial.print(x);
    Serial.print(" Y: ");
    Serial.print(y);
    Serial.print(" Z: ");
    Serial.println(z);

    // Read GPS data
    while (GPS.available()) {
        gps.encode(GPS.read());
    }
    double lat = gps.location.lat();
    double lng = gps.location.lng();

    // Send latitude and longitude to Blynk
    Blynk.virtualWrite(V1, String(lat, 6)); // Latitude on V1
    Blynk.virtualWrite(V2, String(lng, 6)); // Longitude on V2

    // Combined pothole detection logic
    bool potholeDetected = false;
    bool bumpDetected = false;
    String message;

    // Ultrasonic detection for pothole
    if (distance > SENSOR_BASELINE_CM + POTHOLE_THRESHOLD_CM) { // Detect pothole
        potholeDetected = true;
        message += "Ultrasonic Detected Pothole! ";
    }

    // Accelerometer detection for potholes or bumps
    if (z < ACCEL_POTHOLE_Z) { // Downward dip (pothole)
        potholeDetected = true;
        message += "Accelerometer Detected Pothole! ";
    } else if (z > ACCEL_BUMP_Z) { // Upward spike (bump)
        bumpDetected = true;
        message += "Accelerometer Detected Bump! ";
    }

    // Display results
    if (potholeDetected) {
        Serial.println("Pothole Detected!");
        Blynk.virtualWrite(V0, "Pothole Detected!\n" + message);
    } else if (bumpDetected) {
        Serial.println("Bump Detected!");
        Blynk.virtualWrite(V0, "Bump Detected!\n" + message);
    } else {
        Serial.println("No Potholes or Bumps Detected");
        Blynk.virtualWrite(V0, "No Potholes or Bumps\nDistance: " + String(distance) + " cm");
    }

    // Add a delay to avoid rapid spamming
    delay(1000);
}
