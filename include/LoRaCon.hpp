#pragma once
#include <Arduino.h>
#include <Crypto.h>
#include <AES.h>
#include <SHA256.h>
#include <LoRa.h>
#include "LinkedList.hpp"

#define HASH_SIZE 32
#define AES_BLOCKSIZE 16

enum MsgType
{
    MsgType_DATA,
    MsgType_ACK
};

struct DeviceIdentity
{
    const char *name;
    uint8_t id;
    byte key[32];
};

struct KnownDevice
{
    KnownDevice(DeviceIdentity *device)
    {
        this->device = device;
    }
    DeviceIdentity *device;
    uint8_t lastSendMsgId = 0;
    uint8_t lastRecievedMsgId = 0;
};

class Message
{
public:
    Message(KnownDevice *receiver, uint8_t messageId, byte *packet, size_t packetSize)
        : receiver(receiver), messageId(messageId), packet(packet), packetSize(packetSize){};
    ~Message()
    {
        delete[] packet;
    }

    void sendViaLoRa()
    {
        for (int i = 0; i < packetSize; i++)
        {
            Serial.print((char)packet[i]);
        }
        Serial.println();

        LoRa.beginPacket();
        LoRa.write((uint8_t *)packet, packetSize);
        LoRa.endPacket();
    }

    KnownDevice *receiver;
    uint8_t messageId;
    byte *packet;
    size_t packetSize;

private:
};

class LoRaCon
{

public:
    LoRaCon(DeviceIdentity *device);

    void addKnownDevice(DeviceIdentity *device);
    void printKnownDevices();

    bool sendData(uint8_t receiverId, char *data);
    void printPendingMessage();

    void receiving();

private:
    DeviceIdentity *device;

    LinkedList<KnownDevice> knownDevices;
    KnownDevice *findKnownDevice(uint8_t id);

    LinkedList<Message> pendingMessages;

    SHA256 sha256;
    AES128 aes128;

    void calcSHA256(byte *hash, byte *data, size_t size);
    void encryptAES128(byte *key, byte *dataIn, byte *dataOut, size_t size);
    void decryptAES128(byte *key, byte *dataIn, byte *dataOut, size_t size);
};