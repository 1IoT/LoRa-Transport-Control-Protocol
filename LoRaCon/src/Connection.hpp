#pragma once
#include <Arduino.h>
#include <Crypto.h>
#include <AES.h>
#include <SHA256.h>
#include <LoRa.h>
#include "Message.hpp"
#include "LinkedList.hpp"

/* Connection TODO: comment */

#define SESSION_KEY_SIZE 10
#define HASH_SIZE 16
#define AES_BLOCKSIZE 16
#define MESSAGE_METADATA 5 // NONCE + MSG_TYPE + MSG_ID + MSG_LENGTH

enum class ConnectionStatus
{
    NotConnected,
    SessionRequestSended,
    Connected
};

struct DeviceIdentity
{
    const char *name;
    uint8_t id;
    byte key[32];
};

typedef void (*functionPointer)(DeviceIdentity *from, char *msg);

class Connection
{
public:
    Connection(DeviceIdentity *ownDevice, DeviceIdentity *receivingDevice);
    ~Connection(){};

    void checkSession();

    DeviceIdentity *getDeviceIdentity() { return receivingDevice; }
    void addToAckMQ(char *data);
    void addToFaFMQ(char *data);

    void sendFromAckMQ();
    void sendFromFaFMQ();

    void receivePacket(byte *packet, int packetSize, functionPointer callback);

    int getLengthMessageQueue_Ack() { return messageQueue_Ack.getLength(); }
    int getLengthMessageQueue_FaF() { return messageQueue_FaF.getLength(); }

private:
    // Identity of this device
    DeviceIdentity *ownDevice;
    // Identity of receiving device
    DeviceIdentity *receivingDevice;

    // For messages to be acknowledged
    LinkedList<Message> messageQueue_Ack;
    // For "fire and forget" messages
    LinkedList<Message> messageQueue_FaF;

    ConnectionStatus connectionStatus = ConnectionStatus::NotConnected;
    byte currentSessionKey[SESSION_KEY_SIZE];
    byte lastSendSessionKey[SESSION_KEY_SIZE];

    uint16_t nextSendNonce = 1;
    uint16_t lastReceivedNonce = 0;
    uint8_t nextMsgId = 0;

    SHA256 sha256;
    AES128 aes128;

    void sendPacket(Message *msg);

    void acknowledgeMessage(uint8_t msgId);

    void genSessionKey(byte *data, size_t size);
    void calcSHA256(byte *hash, byte *data, size_t dataSize, byte *sessionKey, size_t sessionKeySize);
    void encryptAES128(byte *key, byte *dataIn, byte *dataOut, size_t size);
    void decryptAES128(byte *key, byte *dataIn, byte *dataOut, size_t size);
};
