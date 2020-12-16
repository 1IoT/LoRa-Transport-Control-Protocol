#include "Connection.hpp"

Connection::Connection(DeviceIdentity *ownDevice, DeviceIdentity *receivingDevice)
    : ownDevice(ownDevice), receivingDevice(receivingDevice){};

bool Connection::addToAckMQ(char *data)
{
    char *msgString = new char[strlen(data) + 1];
    memcpy(msgString, data, strlen(data) + 1);

    Message *message = new Message(MsgType_DAT, nextMsgId, strlen(msgString), msgString);
    nextMsgId++;

    messageQueue_Ack.addFirst(message);
}

bool Connection::addToFaFMQ(char *data)
{
    char *msgString = new char[strlen(data) + 1];
    memcpy(msgString, data, strlen(data) + 1);

    Message *message = new Message(MsgType_FAF, nextMsgId, strlen(msgString), msgString);
    nextMsgId++;

    messageQueue_FaF.addFirst(message);
}

void Connection::sendFromAckMQ()
{
    sendPacket(messageQueue_Ack.getLast()->item);
}

void Connection::sendFromFaFMQ()
{
    sendPacket(messageQueue_FaF.getLast()->item);
    messageQueue_FaF.deleteLast();
}

void Connection::receivePacket(byte *packet, int packetSize, functionPointer callback)
{
    // Read Hash
    byte payloadHash[HASH_SIZE];
    memcpy(payloadHash, packet + 2, HASH_SIZE);

    // Read encrypted Payload
    uint8_t payloadSize = packetSize - HASH_SIZE - 2;
    byte encryptedPayload[payloadSize];
    memcpy(encryptedPayload, packet + HASH_SIZE + 2, payloadSize);

    // Decrypt Payload
    byte decryptedPayload[payloadSize];
    decryptAES128(receivingDevice->key, encryptedPayload, decryptedPayload, payloadSize);

    // Calculate Hash over decrypted Payload
    byte payloadHashCalc[HASH_SIZE];
    calcSHA256(payloadHashCalc, decryptedPayload, sizeof(decryptedPayload));

    // Check Hash
    if (memcmp(payloadHash, payloadHashCalc, HASH_SIZE) != 0)
    {
        Serial.println("Payload Hash does not equal calculated Hash!");
        Serial.println();
        return;
    }

    // Unpack Payload
    uint16_t nonce;
    memcpy(&nonce, decryptedPayload, sizeof(nonce));

    // Check Hash
    if (nonce <= lastReceivedNonce)
    {
        Serial.println("Received unexpcted nonce");
        Serial.println();
        return;
    }
    lastReceivedNonce = nonce;

    MsgType msgType = (MsgType)decryptedPayload[2];
    uint8_t msgId = decryptedPayload[3];
    size_t msgLength = decryptedPayload[4];

    char data[msgLength + 1];
    memcpy(data, decryptedPayload + MESSAGE_METADATA, msgLength);
    data[msgLength] = '\0';

    switch (msgType)
    {
    case MsgType_DAT:
        acknowledgeMessage(msgId);
        callback(receivingDevice, data);
        break;

    case MsgType_FAF:
        callback(receivingDevice, data);
        break;

    case MsgType_DAT_ACK:
        Serial.printf("ACK msgId: %d received", msgId);
        Serial.println();
        Serial.println();
        if (messageQueue_Ack.getLast()->item->getMsgId() == msgId)
        {
            messageQueue_Ack.deleteLast();
        }
        break;

    default:
        break;
    }
}

void Connection::sendPacket(Message *msg)
{
    // Create payload
    size_t payloadSize = msg->getMsgLength() + MESSAGE_METADATA;
    uint8_t blocks = (((uint8_t)(payloadSize - 1)) / AES_BLOCKSIZE) + 1;
    size_t paddedPayloadSize = blocks * AES_BLOCKSIZE;

    byte payload[paddedPayloadSize];

    // Fill payload
    memcpy(payload, &nextSendNonce, sizeof(nextSendNonce));
    nextSendNonce++;

    payload[2] = msg->getMsgType();
    payload[3] = msg->getMsgId();
    payload[4] = msg->getMsgLength();
    memcpy(payload + MESSAGE_METADATA, msg->getMsg(), msg->getMsgLength());

    // Hash payload
    byte payloadHash[HASH_SIZE];
    calcSHA256(payloadHash, payload, paddedPayloadSize);

    // Encrypt payload
    byte encryptedPayload[paddedPayloadSize];
    encryptAES128(ownDevice->key, payload, encryptedPayload, paddedPayloadSize);

    // Create packet
    byte packet[2 + sizeof(payloadHash) + sizeof(encryptedPayload)];
    packet[0] = receivingDevice->id;
    packet[1] = ownDevice->id;
    memcpy(packet + 2, payloadHash, sizeof(payloadHash));
    memcpy(packet + 2 + sizeof(payloadHash), encryptedPayload, sizeof(encryptedPayload));

    LoRa.beginPacket();
    LoRa.write((uint8_t *)packet, sizeof(packet));
    LoRa.endPacket();

    Serial.println("Message Send:");
    Serial.print("Sender: ");
    Serial.print(ownDevice->id);
    Serial.print(" | Receiver: ");
    Serial.println(receivingDevice->id);
    Serial.print("PayloadSize: ");
    Serial.print(payloadSize);
    Serial.print(" | PaddedPayloadSize: ");
    Serial.println(paddedPayloadSize);
    Serial.print("PacketSize: ");
    Serial.print(sizeof(packet));
    Serial.print(" | Packet: ");
    for (int i = 0; i < sizeof(packet); i++)
    {
        Serial.printf("%02X ", packet[i]);
    }
    Serial.println();
    Serial.println();
}

void Connection::acknowledgeMessage(uint8_t msgId)
{
    Message *message = new Message(MsgType_DAT_ACK, msgId, 0, nullptr);

    messageQueue_FaF.addFirst(message);
}

void Connection::calcSHA256(byte *hash, byte *data, size_t size)
{
    //Serial.print("Calc SHA256 (");
    sha256.reset();
    sha256.update(data, size);
    sha256.finalize(hash, HASH_SIZE);

    for (int i = 0; i < HASH_SIZE; i++)
    {
        //Serial.printf("%02X ", hash[i]);
    }
    //Serial.println(")");
}

void Connection::encryptAES128(byte *key, byte *dataIn, byte *dataOut, size_t size)
{
    //Serial.println("Encrypt AES128");
    uint8_t blocks = size / aes128.keySize();
    crypto_feed_watchdog();
    aes128.setKey(key, aes128.keySize());
    for (int i = 0; i < blocks; i++)
    {
        aes128.encryptBlock(dataOut + i * aes128.keySize(), dataIn + +i * aes128.keySize());
    }
}

void Connection::decryptAES128(byte *key, byte *dataIn, byte *dataOut, size_t size)
{
    //Serial.println("Decrypt AES128");
    uint8_t blocks = size / aes128.keySize();
    crypto_feed_watchdog();
    aes128.setKey(key, aes128.keySize());
    for (int i = 0; i < blocks; i++)
    {
        aes128.decryptBlock(dataOut + i * aes128.keySize(), dataIn + +i * aes128.keySize());
    }
}