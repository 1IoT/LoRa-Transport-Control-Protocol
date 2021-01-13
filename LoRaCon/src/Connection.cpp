#include "Connection.hpp"

Connection::Connection(DeviceIdentity *ownDevice, DeviceIdentity *receivingDevice)
    : ownDevice(ownDevice), receivingDevice(receivingDevice),
      retrySendingAckTimer(RETRY_TIME_ACK, RETRY_TIME_ACK, true, false)
{
    randomSeed(analogRead(0));
    genSessionKey(currentSessionKey, SESSION_KEY_SIZE);
}

void Connection::checkSession()
{
    switch (connectionStatus)
    {
    case ConnectionStatus::NotConnected:
    {
        Message *message = new Message(MsgType_SESSION_NEW, 0, 0, nullptr);
        messageQueue_FaF.addFirst(message);

        connectionStatus = ConnectionStatus::SessionRequestSended;
    }
    break;
    case ConnectionStatus::SessionRequestSended:
    {
        Message *message = new Message(MsgType_SESSION_NEW, 0, 0, nullptr);
        messageQueue_FaF.addFirst(message);

        connectionStatus = ConnectionStatus::SessionRequestSended;
    }
    break;
    case ConnectionStatus::Connected:

        break;
    }
}

void Connection::addToAckMQ(char *data)
{
    char *msgString = new char[strlen(data) + 1];
    memcpy(msgString, data, strlen(data) + 1);

    Message *message = new Message(MsgType_DAT, nextMsgId, strlen(msgString), msgString);
    nextMsgId++;

    if (messageQueue_Ack.getLength() >= MAX_MESSAGES_IN_MQ)
    {
        messageQueue_Ack.deleteLast();
    }

    messageQueue_Ack.addFirst(message);
}

void Connection::addToFaFMQ(char *data)
{
    char *msgString = new char[strlen(data) + 1];
    memcpy(msgString, data, strlen(data) + 1);

    Message *message = new Message(MsgType_FAF, nextMsgId, strlen(msgString), msgString);
    nextMsgId++;

    if (messageQueue_FaF.getLength() >= MAX_MESSAGES_IN_MQ)
    {
        messageQueue_FaF.deleteLast();
    }

    messageQueue_FaF.addFirst(message);
}

bool Connection::ackReadyToSend()
{
    if (retrySendingAckTimer.checkTimer())
    {
        retrySendingAckTimer.startTimer(RETRY_TIME_ACK, 0, false);
        return true;
    }
    else
    {
        return false;
    }
}

size_t Connection::sendFromAckMQ()
{
    size_t packetSize = sendPacket(messageQueue_Ack.getLast()->item);
    return packetSize;
}

size_t Connection::sendFromFaFMQ()
{
    size_t packetSize = sendPacket(messageQueue_FaF.getLast()->item);
    messageQueue_FaF.deleteLast();
    return packetSize;
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
    calcSHA256(payloadHashCalc, decryptedPayload, sizeof(decryptedPayload), currentSessionKey, SESSION_KEY_SIZE);

    MsgType msgType = (MsgType)decryptedPayload[2];

    bool sessionMsg = (msgType == MsgType_SESSION_NEW) ||
                      (msgType == MsgType_SESSION_KEY) ||
                      (msgType == MsgType_SESSION_KEY_ACK);

    // Check Hash
    if (memcmp(payloadHash, payloadHashCalc, HASH_SIZE) != 0 && !sessionMsg)
    {
        Serial.println("Payload Hash does not equal calculated Hash!");
        Serial.println();

        if (connectionStatus != ConnectionStatus::Connected)
        {
            checkSession();
        }
        return;
    }

    // Unpack Payload
    uint16_t nonce;
    memcpy(&nonce, decryptedPayload, sizeof(nonce));

    // Check Hash
    if (nonce <= lastReceivedNonce)
    {
        if (!sessionMsg)
        {
            Serial.println("Received unexpcted nonce");
            Serial.println();
            return;
        }
    }
    else
    {
        lastReceivedNonce = nonce;
    }

    uint8_t msgId = decryptedPayload[3];
    size_t msgLength = decryptedPayload[4];

    char data[msgLength + 1];
    memcpy(data, decryptedPayload + MESSAGE_METADATA, msgLength);
    data[msgLength] = '\0';

    switch (msgType)
    {
    case MsgType_SESSION_NEW:
    {
        Serial.printf("Received SESSION_NEW message from: %d | msgId: %d\n\n", receivingDevice->id, msgId);

        genSessionKey(lastSendSessionKey, SESSION_KEY_SIZE);

        char *msgString = new char[SESSION_KEY_SIZE];
        memcpy(msgString, lastSendSessionKey, SESSION_KEY_SIZE);

        Message *message = new Message(MsgType_SESSION_KEY, 0, SESSION_KEY_SIZE, msgString);
        messageQueue_FaF.addFirst(message);
    }
    break;
    case MsgType_SESSION_KEY:
    {
        Serial.printf("Received SESSION_KEY message from: %d | msgId: %d\n\n", receivingDevice->id, msgId);

        if (connectionStatus == ConnectionStatus::SessionRequestSended)
        {
            memcpy(currentSessionKey, data, SESSION_KEY_SIZE);
            connectionStatus = ConnectionStatus::Connected;

            char *msgString = new char[SESSION_KEY_SIZE];
            memcpy(msgString, currentSessionKey, SESSION_KEY_SIZE);

            // Clear MQ because in case of parallel session building a SESSSION_KEY message can still be in the MQ
            messageQueue_FaF.clearList();

            Message *message = new Message(MsgType_SESSION_KEY_ACK, 0, SESSION_KEY_SIZE, msgString);
            messageQueue_FaF.addFirst(message);

            nextSendNonce = 1;
            lastReceivedNonce = 0;
            nextMsgId = 0;
        }
    }
    break;
    case MsgType_SESSION_KEY_ACK:
    {
        Serial.printf("Received SESSION_KEY_ACK message from: %d | msgId: %d\n\n", receivingDevice->id, msgId);

        if (memcmp(data, lastSendSessionKey, SESSION_KEY_SIZE) == 0)
        {
            memcpy(currentSessionKey, lastSendSessionKey, SESSION_KEY_SIZE);
            connectionStatus = ConnectionStatus::Connected;

            nextSendNonce = 1;
            lastReceivedNonce = 0;
            nextMsgId = 0;

            messageQueue_FaF.clearList();
        }
    }
    break;
    case MsgType_DAT:
    {
        Serial.printf("Received DAT message from: %d | msgId: %d | Data: \"%s\"\n\n", receivingDevice->id, msgId, data);
        acknowledgeMessage(msgId);
        callback(receivingDevice, data);
    }
    break;
    case MsgType_FAF:
    {
        Serial.printf("Received FAF message from: %d | msgId: %d | Data: \"%s\"\n\n", receivingDevice->id, msgId, data);
        callback(receivingDevice, data);
    }
    break;

    case MsgType_DAT_ACK:
    {
        Serial.printf("Received DAT_ACK message from: %d | for msgId: %d\n\n", receivingDevice->id, msgId);
        if (messageQueue_Ack.getLast()->item->getMsgId() == msgId)
        {
            messageQueue_Ack.deleteLast();
            retrySendingAckTimer.startTimer(RETRY_TIME_ACK, RETRY_TIME_ACK, false);
        }
    }
    break;

    default:
        break;
    }
}

size_t Connection::sendPacket(Message *msg)
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
    calcSHA256(payloadHash, payload, paddedPayloadSize, currentSessionKey, SESSION_KEY_SIZE);

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

    switch (msg->getMsgType())
    {
    case MsgType_SESSION_NEW:
        Serial.printf("Send SESSION_NEW message to: %d | msgId: %d\n\n", receivingDevice->id, msg->getMsgId());
        break;
    case MsgType_SESSION_KEY:
        Serial.printf("Send SESSION_KEY message to: %d | msgId: %d\n\n", receivingDevice->id, msg->getMsgId());
        break;
    case MsgType_SESSION_KEY_ACK:
        Serial.printf("Send SESSION_KEY_ACK message to: %d | msgId: %d\n\n", receivingDevice->id, msg->getMsgId());
        break;
    case MsgType_DAT:
        Serial.printf("Send DAT message to: %d | msgId: %d | Data: \"%s\"\n\n", receivingDevice->id, msg->getMsgId(), msg->getMsg());
        break;
    case MsgType_FAF:
        Serial.printf("Send FAF message to: %d | msgId: %d | Data: \"%s\"\n\n", receivingDevice->id, msg->getMsgId(), msg->getMsg());
        break;
    case MsgType_DAT_ACK:
        Serial.printf("Send DAT_ACK message to: %d | for msgId: %d\n\n", receivingDevice->id, msg->getMsgId());
        break;

    default:
        break;
    }

    return sizeof(packet);
}

void Connection::acknowledgeMessage(uint8_t msgId)
{
    Message *message = new Message(MsgType_DAT_ACK, msgId, 0, nullptr);

    messageQueue_FaF.addFirst(message);
}

void Connection::genSessionKey(byte *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        data[i] = random(256);
    }
}

void Connection::calcSHA256(byte *hash, byte *data, size_t dataSize, byte *sessionKey, size_t sessionKeySize)
{
    //Serial.print("Calc SHA256 (");
    sha256.reset();
    sha256.update(data, dataSize);
    sha256.update(sessionKey, sessionKeySize);
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