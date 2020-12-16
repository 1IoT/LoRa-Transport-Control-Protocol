/* #region Includes */
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <U8g2lib.h>
#include <DHT.h>
#include "LoRaCon.hpp"
/* #endregion */

/* #region Defines */
#define OLED_SCL 15
#define OLED_SDA 4
#define OLED_RST 16

#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_CS 18
#define LORA_RST 14
#define LORA_DIO0 26
#define LORA_DIO1 35
#define LORA_DIO2 34
#define LORA_LED 25

#define DHT_PIN 13
#define DHT_TYPE DHT22
/* #endregion */

/* #region Forward Declarations */
void readDHT();
void sendReading();
void drawScreen();
/* #endregion */

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA, OLED_RST);

DHT dht(DHT_PIN, DHT_TYPE);

bool dhtActive = false;
float temp = 0;
float hum = 0;
bool displaySend = false;

MyTimer *dhtReadTimer;
MyTimer *sendLoRaTimer;
MyTimer *displaySendTimer;

void msgReceived(DeviceIdentity *from, char *msg);

LoRaCon *loraCon;

DeviceIdentity gatewayDevice = {
    .name = "Gateway",
    .id = 100,
    .key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F}};

DeviceIdentity sensorDevice1 = {
    .name = "Sensor1",
    .id = 10,
    .key = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};

DeviceIdentity sensorDevice2 = {
    .name = "Sensor2",
    .id = 11,
    .key = {0x11, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};

void setup()
{

  /* #region Serial and LoRa Setup */

  // Setup Serial
  Serial.begin(115200);
  while (!Serial)
    ;

  // Setup LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(868E6))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }

  // Setup Display
  u8g2.begin();
  // Setup DHT
  dht.begin();
  // Read DHT every 2 seconds
  dhtReadTimer = new MyTimer(5000, 5000, true, true);
  // Send LoRa
  sendLoRaTimer = new MyTimer(20000, 0, true, true);
  // Display send only 5 seconds
  displaySendTimer = new MyTimer(5000, 0, false, false);

  Serial.println("\n\n\n\n\n\n\n\n\n\n");
  /* #endregion */

  loraCon = new LoRaCon(&sensorDevice1, &msgReceived);
  loraCon->addNewConnection(&gatewayDevice);
  loraCon->printConnections();

  loraCon->sendFAF(100, "N:LOG:1:INFO: Sender device online!");
}

void loop()
{
  if (dhtReadTimer->checkTimer())
  {
    readDHT();
  }

  if (sendLoRaTimer->checkTimer())
  {
    sendReading();
  }

  if (displaySendTimer->checkTimer())
  {
    displaySend = false;
    drawScreen();
  }

  loraCon->update();
}

void msgReceived(DeviceIdentity *from, char *msg)
{
  // TODO: Handle received message
}

void readDHT()
{
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h))
  {
    dhtActive = false;
    //Serial.println("DHT not connected!");
  }
  else
  {
    dhtActive = true;
    temp = t;
    hum = h;
    //Serial.print("Read from DHT. (Temp: ");
    //Serial.print(t);
    //Serial.print("°C Hum: ");
    //Serial.print(h);
    //Serial.println("%)");
  }

  drawScreen();
}

void sendReading()
{
  displaySend = true;
  displaySendTimer->startTimer();
  drawScreen();

  char msgBuffer[64];

  if (dhtActive)
  {

    char tempBuff[8];
    char humBuff[8];

    dtostrf(temp, 4, 2, tempBuff);
    dtostrf(hum, 4, 2, humBuff);

    sprintf(msgBuffer, "S:1:20:DOUBLE:%s|S:2:20:DOUBLE:%s", humBuff, tempBuff);
  }
  else
  {
    sprintf(msgBuffer, "N:LOG:1:ERROR:DHT sensor not present!");
  }

  loraCon->sendDAT(100, msgBuffer);
}

void drawScreen()
{
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_helvB12_tf);
  u8g2.drawFrame(0, 0, 128, 64);

  char buffer[16];

  if (dhtActive)
  {
    dtostrf(temp, 4, 2, buffer);
  }
  else
  {
    sprintf(buffer, "%s", "NA");
  }
  sprintf(buffer + strlen(buffer), "%cC", (char)176);
  u8g2.drawStr(4, 16, "Temp: ");
  u8g2.drawStr(56, 16, buffer);

  if (dhtActive)
  {
    dtostrf(hum, 4, 2, buffer);
  }
  else
  {
    sprintf(buffer, "%s", "NA");
  }
  sprintf(buffer + strlen(buffer), "%s", "%");
  u8g2.drawStr(4, 32, "Hum: ");
  u8g2.drawStr(56, 32, buffer);

  if (displaySend)
  {
    u8g2.drawStr(4, 48, "LoRa send");
  }

  u8g2.sendBuffer();
}