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

  Serial.println();
  Serial.println();
  /* #endregion */

  // loraCon = new LoRaCon(&sensorDevice1, &msgReceived);
  // loraCon->addNewConnection(&gatewayDevice);
  // loraCon->printConnections();

  // loraCon->sendDAT(100, "This is a DAT message");
  // loraCon->sendFAF(100, "This is a FAF message");

  loraCon = new LoRaCon(&gatewayDevice, &msgReceived);
  loraCon->addNewConnection(&sensorDevice1);
  loraCon->printConnections();

  //loraCon->sendData(100, "S:1:20:DOUBLE:20.3|S:2:20:DOUBLE:55.3");
}

void loop()
{
  loraCon->update();
}

void msgReceived(DeviceIdentity *from, char *msg)
{
  Serial.print("Received From: ");
  Serial.print(from->id);
  Serial.print(" | MSG: ");
  Serial.println(msg);
  Serial.println();
}