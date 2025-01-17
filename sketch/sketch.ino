#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>

#define BLYNK_TEMPLATE_ID   "TMPL6WakHUI6j"
#define BLYNK_TEMPLATE_NAME "gaslynk"
#define BLYNK_AUTH_TOKEN    "V7qSUHyZU5S_gJdjbjXdKB8XjwWq395J"
#include <BlynkSimpleEsp32.h>

// Updated ESP32 pin assignments
#define LED_PIN 2          // Onboard LED for ESP32
#define MQ9_PIN 35         // GPIO35 (ADC pin for MQ9 sensor)
#define MQ7_PIN 34         // GPIO34 (ADC pin for MQ7 sensor)
#define MQ2_PIN 33         // GPIO33 (ADC pin for MQ2 sensor)
#define RX_PIN 16          // GPIO16 for SIM900A RX
#define TX_PIN 17          // GPIO17 for SIM900A TX

HardwareSerial mySerial(1);  // Use HardwareSerial for SIM900A communication
LiquidCrystal_I2C lcd(0x27, 20, 4);  // Set the LCD address to 0x27 for a 20 chars and 4 line display

const char* ssid = "gaslynk";   // Your Wi-Fi SSID
const char* password = "gaslynk123";   // Your Wi-Fi Password
const char* reciever = "+639951140576"; // Your phone number
const char* message = "ALERT: High gas levels detected!";

float mq2_val = 0;
float mq9_val = 0;

void setup() {
  
  // Initialize pins and LCD
  pinMode(LED_PIN, OUTPUT);
  pinMode(MQ2_PIN, INPUT);
  pinMode(MQ9_PIN, INPUT);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);

  // Start serial communication for debugging
  Serial.begin(115200);
  
  // Initialize HardwareSerial for SIM900A
  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // Set RX, TX pins for SIM900A
  
  lcd.init();                     
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  
  // Wait until the ESP32 is connected to the Wi-Fi
  String waiting = "";
  while (WiFi.status() != WL_CONNECTED) {
    waiting += ".";
    lcd.setCursor(0, 1);
    lcd.print(waiting); 
    delay(1000); 
  }

  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  
  IPAddress ip = WiFi.localIP(); // Get the local IP address
  String ipString = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected: ");
  lcd.setCursor(0, 1);
  lcd.print(String(ipString));
  delay(3000);
}

void loop() {
  // Read analog values from sensors
  mq2_val = analogRead(MQ2_PIN);
  mq9_val = analogRead(MQ9_PIN);

  // Convert to voltage
  mq2_val *= (3.3 / 4095.0);  // ESP32 ADC resolution is 12-bit (0-4095)
  mq7_val *= (3.3 / 4095.0);
  mq9_val *= (3.3 / 4095.0);

  // Turn LED on if any sensor is greater than 1V 
  // if (mq2_val >= 1.0 || mq7_val >= 1.0 || mq9_val >= 1.0) { 
  if (mq2_val >= 1.0 || mq9_val >= 1.0) { 
    digitalWrite(LED_PIN, HIGH); 
    // Send alert notification to Blynk
    Blynk.logEvent("gas_alert", message);

    // Send text message notification
    SendMessage(message, reciever);
  } else { 
    digitalWrite(LED_PIN, LOW);  
  }

  // Send data to Blynk
  Blynk.virtualWrite(V0, mq2_val);  // Send MQ2 value to virtual pin V0
  Blynk.virtualWrite(V2, mq9_val);  // Send MQ9 value to virtual pin V2
  Blynk.virtualWrite(V3, digitalRead(LED_PIN));  // Send LED status to virtual pin V3

  // Display values on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MQ2:");
  lcd.setCursor(0, 1);
  lcd.print(String(mq2_val));

  lcd.setCursor(6, 0);
  lcd.print("MQ9:");
  lcd.setCursor(6, 1);
  lcd.print(String(mq9_val));

  delay(1000);
}

void SendMessage(const char* msg, const char* phoneNumber) {
  // Send "AT" command to check SIM900A status
  mySerial.println("AT");
  delay(1000);
  
  // Set SMS mode to text
  mySerial.println("AT+CMGF=1");
  delay(1000);

  // Send the recipient's phone number
  mySerial.print("AT+CMGS=\"");
  mySerial.print(phoneNumber);
  mySerial.println("\"");  
  delay(1000);

  // Send the message content
  mySerial.print(msg);
  mySerial.write(26);  // Send Ctrl+Z to send the message
  delay(1000);

  // Print any responses from the SIM900A for debugging
  while (mySerial.available()) {
    Serial.write(mySerial.read());
  }
}
