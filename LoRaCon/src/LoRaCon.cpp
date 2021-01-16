#include "LoRaCon.hpp"

LoRaCon::LoRaCon(DeviceIdentity *ownDevice, FunctionPointer callback)
    : ownDevice(ownDevice), messageReceivedCallback(callback),
      sessionCheckTimer(SESSION_CHECK_TIME, 0, true, true),
      dutyCycleTimer(UPDATE_REMAINING_AIRTIME, 0, true, true), sendNext(nullptr)
{
    randomSeed(analogRead(0));
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
        Serial.println(tempItem->item->getReceivingDeviceIdentity()->id);
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

    receiverDevice->addToAckMQ(data);
    return true;
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

    receiverDevice->addToFaFMQ(data);
    return true;
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

    // Update remaining airtime every minute
    if (dutyCycleTimer.checkTimer())
    {
        if (usedAirtime > 600)
        {
            usedAirtime -= 600;
        }
        else
        {
            usedAirtime = 0;
        }
    }

    if (usedAirtime < MAX_AIRTIME_PER_HOUR)
    {
        sendNextMessage();
    }

    receiveMessage();
}

Connection *LoRaCon::findConnection(uint8_t deviceId)
{
    const LinkedListItem<Connection> *tempItem = connections.getFirst();

    while (tempItem && tempItem->item->getReceivingDeviceIdentity()->id != deviceId)
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
                size_t packezSize = sendNext->item->sendFromFaFMQ();
                msgSended = true;
                usedAirtime += calculateAirtime(packezSize);
            }
            else if (sendNext->item->getLengthMessageQueue_Ack() > 0 && sendNext->item->ackReadyToSend())
            {
                size_t packezSize = sendNext->item->sendFromAckMQ();

                msgSended = true;
                uint16_t x = calculateAirtime(packezSize);
                usedAirtime += x;
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

        senderDevice->receivePacket(packet, packetSize, messageReceivedCallback);
    }
}

uint16_t LoRaCon::calculateAirtime(uint16_t sizeInBytes)
{
    double airtimeInMilliseconds = ((sizeInBytes * 8.0) / BitrateSF7_125kHz * 1000.0) + 1.0;
    return (uint16_t)airtimeInMilliseconds;
}