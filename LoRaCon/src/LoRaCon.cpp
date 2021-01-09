#include "LoRaCon.hpp"

LoRaCon::LoRaCon(DeviceIdentity *ownDevice, functionPointer callback)
    : ownDevice(ownDevice), callback(callback),
      sessionCheckTimer(sessionCheckTime, 0, true, true),
      dutyCycleTimer(sendMsgTime, sendMsgTime, true, true), sendNext(nullptr)
{
}

void LoRaCon::addNewConnection(DeviceIdentity *receivingDevice)
{
    connections.addLast(new Connection(ownDevice, receivingDevice));
    connections.getLast()->item->checkSession();
    if (sendNext == nullptr)
    {
        sendNext = connections.getFirst();
    }
}

void LoRaCon::printConnections()
{
    const LinkedListItem<Connection> *tempItem = connections.getFirst();
    Serial.println("Connections: ");
    while (tempItem)
    {
        Serial.print("Device Id: ");
        Serial.println(tempItem->item->getDeviceIdentity()->id);
        tempItem = tempItem->next;
    }
    Serial.println();
}

bool LoRaCon::sendDAT(uint8_t receiverId, char *data)
{
    // Find receiver
    Connection *receiverDevice = findConnection(receiverId);
    if (!receiverDevice)
    {
        Serial.println("Receiver not found!");
        Serial.println();
        return false;
    }

    return receiverDevice->addToAckMQ(data);
}

bool LoRaCon::sendFAF(uint8_t receiverId, char *data)
{
    // Find receiver
    Connection *receiverDevice = findConnection(receiverId);
    if (!receiverDevice)
    {
        Serial.println("Receiver not found!");
        Serial.println();
        return false;
    }

    return receiverDevice->addToFaFMQ(data);
}

void LoRaCon::update()
{
    if (sessionCheckTimer.checkTimer())
    {
        const LinkedListItem<Connection> *tempItem = connections.getFirst();
        while (tempItem)
        {
            tempItem->item->checkSession();
            tempItem = tempItem->next;
        }
    }
    if (dutyCycleTimer.checkTimer())
    {
        sendNextMessage();
    }
    receiveMessage();
}

Connection *LoRaCon::findConnection(uint8_t deviceId)
{
    const LinkedListItem<Connection> *tempItem = connections.getFirst();

    while (tempItem && tempItem->item->getDeviceIdentity()->id != deviceId)
    {
        tempItem = tempItem->next;
    }

    if (tempItem)
    {
        return tempItem->item;
    }

    return nullptr;
}

void LoRaCon::sendNextMessage()
{
    const LinkedListItem<Connection> *startItem = sendNext;
    bool msgSended = false;

    if (sendNext != nullptr)
    {
        do
        {
            if (sendNext->item->getLengthMessageQueue_FaF() > 0)
            {
                sendNext->item->sendFromFaFMQ();
                msgSended = true;
            }
            else if (sendNext->item->getLengthMessageQueue_Ack() > 0)
            {
                sendNext->item->sendFromAckMQ();
                msgSended = true;
            }

            if (sendNext->next == nullptr)
            {
                sendNext = connections.getFirst();
            }
            else
            {
                sendNext = sendNext->next;
            }
        } while (sendNext != startItem && !msgSended);

        if (!msgSended)
        {
            dutyCycleTimer.startTimer(sendMsgTime, sendMsgTime - 100, true);
        }
    }
}

void LoRaCon::receiveMessage()
{
    int packetSize = LoRa.parsePacket();

    if (packetSize)
    {
        // Receive Packet
        byte packet[packetSize];
        for (int i = 0; i < packetSize; i++)
        {
            packet[i] = (byte)LoRa.read();
        }

        // Get Sender and Receiver Id
        uint8_t receiverId = packet[0];
        uint8_t senderId = packet[1];

        if (receiverId != ownDevice->id)
        {
            Serial.println("Message not for me!");
            Serial.println();
            return;
        }

        // Find sender
        Connection *senderDevice = findConnection(senderId);
        if (!senderDevice)
        {
            Serial.println("Sender unknown!");
            Serial.println();
            return;
        }

        senderDevice->receivePacket(packet, packetSize, callback);
    }
}