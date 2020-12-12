#include "LoRaCon.hpp"

LoRaCon::LoRaCon(DeviceIdentity *device)
    : device(device)
{
}

void LoRaCon::addKnownDevice(DeviceIdentity *device)
{
    knownDevices.addLast(new KnownDevice(device));
}

void LoRaCon::printKnownDevices()
{
    const LinkedListItem<KnownDevice> *tempItem = knownDevices.getFirst();
    while (tempItem)
    {
        Serial.println(tempItem->item->device->id);
        tempItem = tempItem->next;
    }
}

bool LoRaCon::sendData(uint8_t receiverId, char *data)
{
    // Find receiver
    KnownDevice *receiverDevice = findKnownDevice(receiverId);
    if (!receiverDevice)
    {
        Serial.println("Receiver not found!");
        return false;
    }

    // Create payload
    size_t dataSize = strlen(data);
    size_t payloadSize = dataSize + 3;
    uint8_t blocks = (((uint8_t)(payloadSize - 1)) / AES_BLOCKSIZE) + 1;
    size_t paddedPayloadSize = blocks * AES_BLOCKSIZE;

    byte payload[paddedPayloadSize];

    payload[0] = receiverDevice->lastSendMsgId + 1; // Message Id
    receiverDevice->lastSendMsgId++;
    payload[1] = MsgType_DATA; // Message type
    payload[2] = strlen(data); // Message length
    memcpy(payload + 3, data, dataSize);

    // Hash payload
    byte payloadHash[HASH_SIZE];
    calcSHA256(payloadHash, payload, payloadSize);

    // Encrypt payload
    byte encryptedPayload[paddedPayloadSize];
    encryptAES128(device->key, payload, encryptedPayload, paddedPayloadSize);

    // Create message

    Serial.print("MEssage Size: ");
    Serial.println(2 + sizeof(payloadHash) + sizeof(encryptedPayload));

    byte *packet = new byte[2 + sizeof(payloadHash) + sizeof(encryptedPayload)];
    packet[0] = receiverDevice->device->id;
    packet[1] = device->id;
    memcpy(packet + 2, payloadHash, sizeof(payloadHash));
    memcpy(packet + 2 + sizeof(payloadHash), encryptedPayload, sizeof(encryptedPayload));

    Message *message = new Message(receiverDevice, receiverDevice->lastSendMsgId, packet, 2 + sizeof(payloadHash) + sizeof(encryptedPayload));
    pendingMessages.addLast(message);
    message->sendViaLoRa();
}

void LoRaCon::printPendingMessage()
{
    const LinkedListItem<Message> *tempItem = pendingMessages.getFirst();
    while (tempItem)
    {
        Serial.printf("Pending Message: Id Sender:%d, Id Receiver:%d, Message Id:%d ", device->id, tempItem->item->receiver->device->id, tempItem->item->messageId);
        Serial.println();
        tempItem = tempItem->next;
    }
}

void LoRaCon::receiving()
{
    int packetSize = LoRa.parsePacket();

    if (packetSize)
    {

        byte packet[packetSize];

        for (int i = 0; i < packetSize; i++)
        {
            packet[i] = (byte)LoRa.read();
        }

        uint8_t receiverId = packet[0];
        uint8_t senderId = packet[1];

        if (receiverId != device->id)
        {
            return;
        }

        // Find sender
        KnownDevice *senderDevice = findKnownDevice(senderId);
        if (!senderDevice)
        {
            Serial.println("Sender not found!");
            return;
        }

        byte payloadHash[HASH_SIZE];
        memcpy(payloadHash, packet + 2, 32);

        byte encryptedPayload[packetSize - 34];
        memcpy(encryptedPayload, packet + 2 + 32, packetSize - 34);

        byte decryptedPayload[packetSize - 34];
        decryptAES128(senderDevice->device->key, encryptedPayload, decryptedPayload, packetSize - 34);

        byte payloadHashCalc[HASH_SIZE];
        calcSHA256(payloadHashCalc, decryptedPayload, sizeof(decryptedPayload));

        if (memcmp(payloadHash, payloadHashCalc, HASH_SIZE))
        {
            Serial.println("Match");
        }
        else
        {
            Serial.println("No Match");
        }

        uint8_t messageId = decryptedPayload[0];
        uint8_t messageType = decryptedPayload[1];
        uint8_t messageLength = decryptedPayload[2];
        char theMessage[messageLength + 1];
        memcpy(theMessage, decryptedPayload + 3, messageLength);
        theMessage[messageLength] = '\0';

        //Serial.print("Message: ");
        Serial.println(theMessage);
    }
}

KnownDevice *LoRaCon::findKnownDevice(uint8_t id)
{
    const LinkedListItem<KnownDevice> *tempItem = knownDevices.getFirst();

    while (tempItem && tempItem->item->device->id != id)
    {
        tempItem = tempItem->next;
    }

    if (tempItem)
    {
        return tempItem->item;
    }

    return nullptr;
}

void LoRaCon::calcSHA256(byte *hash, byte *data, size_t size)
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

void LoRaCon::encryptAES128(byte *key, byte *dataIn, byte *dataOut, size_t size)
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

void LoRaCon::decryptAES128(byte *key, byte *dataIn, byte *dataOut, size_t size)
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