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

#define SERIAL_TRANSMIT 22
#define SERIAL_RECIVE -1
/* #endregion */

/* #region Forward Declaration */
void msgReceived(DeviceIdentity *from, char *msg);
uint16_t findReplacementId(DeviceIdentity *deviceIdentity, int index);
void splitMessage(DeviceIdentity *deviceIdentity, char *msg);
size_t getDigitCount(char *msgItem, size_t msgItemLength);
void sendMessageToGateway(char *gatewayMessage);
void processMessageItem(DeviceIdentity *deviceIdentity, char *msgItem, size_t msgItemLength);
/* #endregion */

struct DeviceMappingEntry
{
  DeviceMappingEntry(uint8_t deviceId, uint16_t itemIdIn, uint16_t itemIdOut)
      : deviceId(deviceId), itemIdIn(itemIdIn), itemIdOut(itemIdOut) {}
  uint8_t deviceId;
  uint16_t itemIdIn;
  uint16_t itemIdOut;
};

LinkedList<DeviceMappingEntry> deviceMappingTable;

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

DeviceIdentity sensorDevice3 = {
    .name = "Sensor3",
    .id = 12,
    .key = {0x22, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
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
  Serial2.begin(115200, SERIAL_8N1, SERIAL_RECIVE, SERIAL_TRANSMIT);

  if (!LoRa.begin(868E6))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }

  Serial.println("\n\n\n\n\n\n\n\n\n\n");
  /* #endregion */

  deviceMappingTable.addFirst(new DeviceMappingEntry(10, 1, 1));
  deviceMappingTable.addFirst(new DeviceMappingEntry(10, 2, 2));
  deviceMappingTable.addFirst(new DeviceMappingEntry(11, 1, 3));
  deviceMappingTable.addFirst(new DeviceMappingEntry(11, 2, 4));
  deviceMappingTable.addFirst(new DeviceMappingEntry(12, 1, 5));
  deviceMappingTable.addFirst(new DeviceMappingEntry(12, 2, 6));

  loraCon = new LoRaCon(&gatewayDevice, &msgReceived);
  loraCon->addNewConnection(&sensorDevice1);
  loraCon->addNewConnection(&sensorDevice2);
  loraCon->printConnections();
}

void loop()
{
  loraCon->update();
}

uint16_t findReplacementId(DeviceIdentity *deviceIdentity, int index)
{
  const LinkedListItem<DeviceMappingEntry> *tempItem = deviceMappingTable.getFirst();
  while (tempItem)
  {
    if (tempItem->item->deviceId == deviceIdentity->id && tempItem->item->itemIdIn == index)
    {
      return tempItem->item->itemIdOut;
    }
    tempItem = tempItem->next;
  }
  Serial.print("No such item");
  return 0;
}

void splitMessage(DeviceIdentity *deviceIdentity, char *msg)
{
  // Split message after every '|' and start processing each part
  size_t msgLength = strlen(msg);
  size_t startIndex = 0;
  size_t endIndex = 0;

  for (size_t i = 0; i < msgLength; i++)
  {
    if (msg[i] == '|' || i == msgLength - 1)
    {
      if (i == msgLength - 1)
      {
        endIndex = i + 1;
      }
      else
      {
        endIndex = i;
      }

      size_t msgItemLength = endIndex - startIndex;
      char msgItem[msgItemLength + 1];
      memcpy(msgItem, msg + startIndex, msgItemLength);
      msgItem[msgItemLength] = '\0';
      startIndex = endIndex + 1;
      processMessageItem(deviceIdentity, msgItem, msgItemLength);
    }
  }
}

size_t getDigitCount(char *msgItem, size_t msgItemLength)
{
  // Get count of digits between the first two ':' of the message
  size_t indexLength = 1;
  for (size_t i = 3; i < msgItemLength; i++)
  {
    if (msgItem[i] != ':')
    {
      indexLength++;
    }
    else
    {
      break;
    }
  }
  return indexLength;
}

void sendMessageToGateway(char *gatewayMessage)
{
  Serial.println(gatewayMessage);
}

void processMessageItem(DeviceIdentity *deviceIdentity, char *msgItem, size_t msgItemLength)
{
  if (msgItem[0] == 'S')
  {
    size_t indexLength = getDigitCount(msgItem, msgItemLength);

    // Parse Id string in the message to a number
    char itemIndex[indexLength + 1];
    memcpy(itemIndex, msgItem + 2, indexLength);
    itemIndex[indexLength] = '\0';
    int index = atoi(itemIndex);

    uint16_t itemIdOut = findReplacementId(deviceIdentity, index);
    // Put the mapping Id into the string
    char itemIdOutString[6];
    sprintf(itemIdOutString, "%d", itemIdOut);

    // Build message item with new mapping id
    if (itemIdOut != 0)
    {
      char gatewayMessage[msgItemLength - 1 + strlen(itemIdOutString)];
      gatewayMessage[0] = msgItem[0];
      gatewayMessage[1] = msgItem[1];
      strcpy(gatewayMessage + 2, itemIdOutString);
      strcpy(gatewayMessage + 2 + strlen(itemIdOutString), msgItem + 2 + indexLength);

      sendMessageToGateway(gatewayMessage);
    }
  }
}

void msgReceived(DeviceIdentity *from, char *msg)
{
  /*
  * Received messag gets processed by following steps:
  * 
  * 1. Split message with the delimeter '|'
  *    For each item: 
  * 2. Find item id 
  * 3. Convert item id to decimal
  * 4. Find replacement id for the item id in the mapping table
  * 5. Convert replacement id to string 
  * 6. Build new message
  * 7. Send message to gateway
  * 
  * 
  * The item id is the number after the message type
  * 
  * S:1:20:DOUBLE:22.2|S:2:20:DOUBLE:33.3
  *   ^                  ^
  * 
  */
  splitMessage(from, msg);
}
