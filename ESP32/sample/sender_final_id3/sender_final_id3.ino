#include <Wire.h>
#include <SoftwareSerial.h>

// Libraries for T-Beam
#include <SPI.h>
#include <LoRa.h>
#include <utilities.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// New
#include <xxtea-iot-crypt.h>

// LoRa define
#define frequency 915E6        // frequency in Hz (433E6, 868E6, 915E6)
#define signalBandwidth 125E3  // default 125E3, supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3

// Screen define
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RST -1       //

// A02YYUW Sensor pins
#define RX_PIN 13  
#define TX_PIN 14  

// LoRa define (b)
int64_t speed = 15;
int64_t remaind =  0;
int64_t counter = 0;
int txPower = 17;               // defaults to 17, TX power in dB
int spreadingFactor = 7;        // default 7, supported values are between 6 and 12,
int codingRateDenominator = 5;  // defaults to 5, denominator of the coding rate, corresponds to coding rates of 4/5 and 4/8 (4 is fixed numerator)
int senderID = 3;               // id

int send_rate = (speed *1000);  
String location = "MK1";  // 位置信息
String status = "green";      // 狀態信息：green, yellow, red

// New
// MUST be the same as the sender!
String loraKey = "Fill_in_password";


float waterLevel = 0;  //CM
SoftwareSerial sensorSerial(RX_PIN, TX_PIN); // RX, TX

const float MaxHeight = 75;
const float GREEN_THRESHOLD = 15;   
const float YELLOW_THRESHOLD = 18;  

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("LoRa Sender Test");

  // A02YYUW[]
  sensorSerial.begin(9600);
  
  // monitor
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) {  // Address 0x3C for 128x32
    Serial.println("SSD1306 allocation failed!");
    for (;;)
      ;  // Don't proceed, loop forever
  }
  Serial.println("SSD1306 allocation OK!");

  //New
  // Setup the Key - Once
  if(!xxtea.setKey(loraKey))
  {
    Serial.println(" Assignment Failed!");
    return;
  }

  //reset OLED display via software'
  
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  //delay(50);
  digitalWrite(OLED_RST, HIGH);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.display();
  
  //SPI LoRa pins
  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);

  //setup LoRa transceiver module
  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);

  if (!LoRa.begin(frequency)) {
    //Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }

  LoRa.setTxPower(txPower);
  LoRa.setSpreadingFactor(spreadingFactor);
  LoRa.setSignalBandwidth(signalBandwidth);
  LoRa.setCodingRate4(codingRateDenominator);
  LoRa.disableCrc();
  LoRa.enableInvertIQ();
  //Serial.println("LoRa Initializing OK!");
  
  //Serial.println("A02YYUW Ultrasonic Sensor Test");

}


bool readWaterLevelSensor() {
  unsigned char data[4] = {};
  

  while(sensorSerial.available()) {
    sensorSerial.read();
  }
  
  
  unsigned long startTime = millis();
  int dataCount = 0;
  
  while(dataCount < 4 && (millis() - startTime) < 1000) {
    if(sensorSerial.available()) {
      data[dataCount] = sensorSerial.read();
      dataCount++;
    }
  }
  
 
  if(dataCount == 4 && data[0] == 0xFF) {
    int sum = (data[0] + data[1] + data[2]) & 0x00FF;
    if(sum == data[3]) {
      float distance = (data[1] << 8) + data[2];
      waterLevel = MaxHeight - (distance / 10.0);  
      
      //0-1000cm
      if(waterLevel < 0) waterLevel = 0;
      if(waterLevel > 1000) waterLevel = 1000;
      
      //update water level
      updateStatus();
      
      //Serial.print("Water level: ");
      //Serial.print(waterLevel);
      //Serial.println(" cm");
      //Serial.print("Status: ");
      //Serial.println(status);
      return true;
    } else {
      //Serial.println("Checksum ERROR");
    }
  } else {
    //Serial.println("Data format ERROR or timeout");
  }
  
  return false;
}

//update status
void updateStatus() {
  if(waterLevel <= GREEN_THRESHOLD) {
    status = "green";
  } else if(waterLevel <= YELLOW_THRESHOLD) {
    status = "yellow";
  } else {
    status = "red";
  }
}



void loop() {
    
    readWaterLevelSensor();
    //senderID = (counter % speed) +29;
    // sending packe
    senderID = 3;
    
    String payload = String(senderID);
    payload += "," + String(location); 
    payload += "," + String(counter);
    payload += "," + String(txPower);
    payload += "," + String(spreadingFactor);
    payload += "," + String(codingRateDenominator);
    payload += "," + String(waterLevel);
    payload += "," + String(status);
    
    //Serial.print("Sending packet: ");
    //Serial.println(payload);

    // New 
    payload = xxtea.encrypt(payload);

    // Send LoRa packet to receiver
    LoRa.beginPacket();
    LoRa.print(payload);
    LoRa.endPacket();
    
    
    if(remaind == 0){
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("LoRa Sender " + String(senderID));
    display.setCursor(0, 10);
    display.println("LoRa location " + String(location));
    display.setCursor(0, 20);
    display.println("Pkt: " + String(counter));
    display.setCursor(0, 30);
    display.println("LVL: " + String(waterLevel) + " cm");
    display.setCursor(0, 40);
    display.print("remaind: ");
    display.println(remaind);

    // Status update display
    if(status == "green") {
        display.fillCircle(100, 40, 5, WHITE); 
    } else if(status == "yellow") {
        display.drawCircle(100, 40, 5, WHITE); 
        display.fillCircle(100, 40, 3, WHITE); 
    } else if(status == "red") {
        display.fillRect(95, 35, 10, 10, WHITE); 
    }
    
    display.display();
    }
    delay(send_rate);  // Refresh per 10 sec
    counter++;

}

