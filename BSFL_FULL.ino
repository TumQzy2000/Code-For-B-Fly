#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <HTTPClient.h>
#include "DHT.h"
#include "Adafruit_SGP30.h"

BlynkTimer timer;
int mode = 0;  // 0 = Auto, 1 = Manual
char auth[] = "5pqZdOeNRQx2ZVaCdos7yK087uayNxoq";
char ssid[] = "Projectstartup"; 
char pass[] = "GOGOStartup24";  

#define relay1 25  
#define relay2 26  
#define relay3 27
#define relay4 33
#define relay5 32
#define relay6 14
#define relay7 19

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

WidgetLED LED1 (V9);
WidgetLED LED2 (V10);
WidgetLED LED3 (V11);
WidgetLED LED4 (V12);
WidgetLED LED5 (V13);
WidgetLED LED6 (V14);
WidgetLED LED7 (V15);


Adafruit_SGP30 sgp;

uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

int counter = 0;
void sendSensor()
{
  //DHT22//
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    //delay(5000);
    return;
  }
  Serial.print("Humidity is: "); 
  Serial.println(h, 2);
  Serial.print("Temperature is: "); 
  Serial.println(t, 2);
  Blynk.virtualWrite(V0, h);
  Blynk.virtualWrite(V1, t);

  //SGP30/
  if (! sgp.IAQmeasure()) {
    Serial.println("Measurement failed");
    return;
  }
  Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
  Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.println(" ppm");
  Blynk.virtualWrite(V8, sgp.eCO2);
  
  counter++;
  if (counter == 30) {
      counter = 0;

    uint16_t TVOC_base, eCO2_base;
    if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
      Serial.println("Failed to get baseline readings");
      return;
    }
    Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
    Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
  }
  

  // Check WiFi connection before sending data
  if (WiFi.status() == WL_CONNECTED) {
    // Create HTTP client object
    HTTPClient http;

    // Construct URL with sensor value
    String url = "https://script.google.com/macros/s/AKfycbwtfXfizvRQtGe4FEvi_e4Aamyfkif3qpLGW1dJRCqtw6JnlqYmGRpz10SsaN2llucBBQ/exec?temp="+String(t)+"&hum="+String(h)+"&value="+String(sgp.eCO2);

    // Start HTTP request
    Serial.println("Making a request");
    http.begin(url.c_str());

    // Set HTTP request options
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    // Send GET request and get response code
    int httpCode = http.GET();

    // Check response code
    String payload;
    if (httpCode > 0) {
      // Read response payload
      payload = http.getString();

      // Print response code and payload to serial monitor
      Serial.println(httpCode);
      Serial.println(payload);
    } else {
      // Handle error if response code is not positive
      Serial.println("Error on HTTP request");
    }

    // Close HTTP connection
    http.end();
  } else {
    // Handle situation when Wi-Fi is not connected
    Serial.println("Wi-Fi not connected, skipping data transmission");
  }

  //Delay between transmissions
  //delay(5000);
}



void setup()
{
  Serial.begin(115200);
  pinMode(relay1, OUTPUT); //ปั๊มน้ำ
  pinMode(relay2, OUTPUT); //พัดลมลดอุณหภูมิ
  pinMode(relay3, OUTPUT); //หลอดไฟเพิ่มอุณหภูมิ
  pinMode(relay4, OUTPUT); //พัดลมลดความชื้น
  pinMode(relay5, OUTPUT); //ปั๊มลม
  pinMode(relay6, OUTPUT); //หลอดไฟอบ
  pinMode(relay7, OUTPUT); //หลอดไฟสแตนด์บาย
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);  
  digitalWrite(relay3, LOW);
  digitalWrite(relay4, LOW);
  digitalWrite(relay5, LOW);
  digitalWrite(relay6, LOW);
  digitalWrite(relay7, LOW);

 
 // Initialize Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Wi-Fi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize Blynk
  Blynk.begin(auth, ssid, pass, "blynk.iot-cm.com", 8080);

  // Initialize DHT sensor
  Serial.println(F("DHTxx test!"));
  dht.begin();
  Serial.println(F("DHT sensor initialized"));
  timer.setInterval(10000L, sendSensor);

  //Initialize SGP30 sensor
    Serial.println("SGP30 test");
  if (! sgp.begin()){
    Serial.println("Sensor not found :(");
    while (1);
  }
  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);
}

BLYNK_WRITE(V17) {
  mode = param.asInt();  // อ่านค่าจาก Virtual Pin 1 (สลับโหมด Auto/Manual)
}

BLYNK_WRITE(V2)
{
  if (mode == 1) {
  int pinValue = param.asInt();  
  if (pinValue == 1) {
    digitalWrite(relay1, HIGH); 
    Serial.println("Relay is ON");
    LED1.on();
  } else {
    digitalWrite(relay1, LOW);   
    Serial.println("Relay is OFF");
     LED1.off();
  }
}
}

BLYNK_WRITE(V3)
{
  if (mode == 1) {
  int pinValue = param.asInt();  
  if (pinValue == 1) {
    digitalWrite(relay2, HIGH);  
    Serial.println("Relay is ON");
    LED2.on();
  } else {
    digitalWrite(relay2, LOW);  
    Serial.println("Relay is OFF");
    LED2.off();
  }
}
}

BLYNK_WRITE(V4)
{
  if (mode == 1) {
  int pinValue = param.asInt();  
  if (pinValue == 1) {
    digitalWrite(relay3, HIGH);  
    Serial.println("Relay is ON");
    LED3.on();
  } else {
    digitalWrite(relay3, LOW);   
    Serial.println("Relay is OFF");
    LED3.off();
  }
}
} 

BLYNK_WRITE(V5)
{
  if (mode == 1) {
  int pinValue = param.asInt();    
  if (pinValue == 1) {
    digitalWrite(relay4, HIGH);  
    Serial.println("Relay is ON");
    LED4.on();
  } else {
    digitalWrite(relay4, LOW);   
    Serial.println("Relay is OFF");
    LED4.off();
  }
} 
}

BLYNK_WRITE(V6)
{
  int pinValue = param.asInt();  
  if (pinValue == 1) {
    digitalWrite(relay5, HIGH);  
    Serial.println("Relay is ON");
    LED5.on();
  } else {
    digitalWrite(relay5, LOW);   
    Serial.println("Relay is OFF");
    LED5.off();
  }
} 

BLYNK_WRITE(V7)
{
  int pinValue = param.asInt();  
  if (pinValue == 1) {
    digitalWrite(relay6, HIGH);  
    Serial.println("Relay is ON");
    LED6.on();
  } else {
    digitalWrite(relay6, LOW);   
    Serial.println("Relay is OFF");
    LED6.off();
  }
} 

BLYNK_WRITE(V8)
{
  int pinValue = param.asInt();  
  if (pinValue == 1) {
    digitalWrite(relay7, HIGH);  
    Serial.println("Relay is ON");
    LED6.on();
  } else {
    digitalWrite(relay7, LOW);   
    Serial.println("Relay is OFF");
    LED6.off();
  }
} 


void loop(){
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Blynk.run();
  timer.run();
  
  //การควบคุมความชื้นให้สูงขึ้น
  if (mode == 0) {
  if (h < 60)
  {
    digitalWrite(relay1, HIGH);
    Blynk.virtualWrite(V2, 1);
    LED1.on();
  }
  if (h > 60)
  {
    digitalWrite(relay1, LOW);
    Blynk.virtualWrite(V2, 0);
    LED1.off();
  }
 }

  //การควบคุมอุณหภูมิให้ต่ำลง
    if (mode == 0) {
    if (t > 25)
  {
    digitalWrite(relay2, HIGH);
    Blynk.virtualWrite(V3, 1);
    LED2.on();
  }
  if (t < 25)
  {
    digitalWrite(relay2, LOW);
    Blynk.virtualWrite(V3, 0);
    LED2.off();
  }
 }

  //การควบคุมอุณหภูมิให้สูงขึ้น
  if (mode == 0) {
  if (t < 25 )
  {
    digitalWrite(relay3, HIGH);
    Blynk.virtualWrite(V4, 1);
    LED3.on();
  }
  if (t > 25)
  {
    digitalWrite(relay3, LOW);
    Blynk.virtualWrite(V4, 0);
    LED3.off();
  }
  }

  //การควบคุมความชื้นให้ต่ำลง
  if (mode == 0) {
  if (h > 60 )
  {
    digitalWrite(relay4, HIGH);
    Blynk.virtualWrite(V5, 1);
    LED4.on();
  }
  if (h < 60)
  {
    digitalWrite(relay4, LOW);
    Blynk.virtualWrite(V5, 0);
    LED4.off();
  }
  }


  //การลดคาร์บอน
  if (mode == 0) {
  if (sgp.eCO2 > 450)
  {
    digitalWrite(relay5, HIGH);
     Blynk.virtualWrite(V6, 1);
     LED5.on();
  }
  if (sgp.eCO2 < 450)
  {
    digitalWrite(relay5, LOW);
    Blynk.virtualWrite(V6, 0);
    LED5.off();
  }
  }

}
