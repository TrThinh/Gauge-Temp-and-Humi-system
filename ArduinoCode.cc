//Enter you blink code here if you have
#define BLYNK_TEMPLATE_ID "ID"
#define BLYNK_TEMPLATE_NAME "Name"
#define BLYNK_AUTH_TOKEN "Auth Token"

//Enter your database here
#define API_KEY "API Key"
#define DATABASE_URL "Enter your database url"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define BLYNK_PRINT Serial

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

//enter your network here
char ssid[] = "Your name network";
char pass[] = "pass network";
char auth[] = BLYNK_AUTH_TOKEN;


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
const int bt = 1;
int pinValue;

// declare WiFiManager Object
WiFiManager wifiManager;
bool signupOK = false;


//  Use the NTP (Network Time Protocol) to get the time and date
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// declare firebase database, authentication, and configuration objects
FirebaseData fbdo;
FirebaseAuth auth1;
FirebaseConfig config;
FirebaseJson json;

//define the function to get the datetime
String getDatetime(){
 	timeClient.update();
 	time_t epochTime = timeClient.getEpochTime();
 	struct tm *ptm = gmtime ((time_t *)&epochTime);
 	int monthDay = ptm->tm_mday;
 	int currentMonth = ptm->tm_mon+1;
 	int currentYear = ptm->tm_year+1900;
 	String formattedTime = timeClient.getFormattedTime();
 	return String(monthDay) + "-" + String(currentMonth) + "-" + String(currentYear) + " " + formattedTime;
}
//define path in the database
String prPath = "/temp";
String timePath = "/temp";
String hPath = "/Humidity";
String htimePath = "/Humidity";
//Name of database you created
String databasePath = "/Name database created";
String parentPath;


void setup() {
  if (digitalRead(bt) == HIGH) {
    Serial.println("Button Pressed");
    wifiManager.resetSettings();
    delay(1000);
  }

  Serial.begin(115200);
  //Create your name wifi to connect, anything name you want
  bool res = wifiManager.autoConnect("Any thing name wifi you want");
  if (!res) {
    Serial.println("Failed to connect");
  } else {
    Serial.println("Connecting to Wi-Fi");
    Serial.println("Connected...yay ");
  }

  	timeClient.begin();
	timeClient.setTimeOffset(25200);
	pinMode(DHTPIN, INPUT);
	// Assign the api key (required) 
	config.api_key = API_KEY;
	// Assign the RTDB URL (required) 
	config.database_url = DATABASE_URL;
	/* 
	Sign up on Firebase
	In this lab, the database is opened for anonymous 
	For a better security, you need to setup username and password
	And fill it in the statement Firebase.signUp
	*/
	if (Firebase.signUp(&config, &auth1, "", "")){
		Serial.println("ok");
		signupOK = true;
	}
	else{
		Serial.printf("%s\n", config.signer.signupError.message.c_str());
	} 
	//Assign the callback function for the long running token generation task
	config.token_status_callback = tokenStatusCallback;
	Firebase.begin(&config, &auth1);
	Firebase.reconnectWiFi(true);

  Blynk.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());
  Serial.begin(115200);
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
}

BLYNK_WRITE(V2) {
  int pinValue = param.asInt();

  if (pinValue == 1) {
    Serial.println("Button switched to 1");
  } else {
    Serial.println("Button switched to 0");
  }
}

void loop() {
  delay(5000);
  Blynk.run();
  //read temperature and humidity
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (pinValue == 1) {
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    Blynk.virtualWrite(V0, t);
    Blynk.virtualWrite(V1, h);
  }
  // clear display
  display.clearDisplay();

  //dislay value at blynk app
   Blynk.virtualWrite(V0, t);
    Blynk.virtualWrite(V1, h);
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print("   Humidity: ");
    Serial.println(h);

  // display temperature
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Temperature: ");
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print(t);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");

  // display humidity
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("Humidity: ");
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print(h);
  display.print(" %");

  display.display();

  if (Firebase.ready() && signupOK){
 		delay(5000);
		String datetime = getDatetime();
 		Serial.print ("time: ");
 		parentPath= databasePath + "/" + datetime;
 		//set the JSON string
 		json.set(timePath.c_str(), t);
    json.set(htimePath.c_str(), h);
 		//send data to the real-time database
 		if (Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json)){
 			Serial.println("PASSED");
 			Serial.println("PATH: " + fbdo.dataPath());
			Serial.println("TYPE: " + fbdo.dataType());
 		}
 		else {
 			Serial.println("FAILED");
 			Serial.println("REASON: " + fbdo.errorReason());
 		}
	}
}
