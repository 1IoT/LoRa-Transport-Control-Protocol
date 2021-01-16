#pragma once
#include <Arduino.h>
#include "LinkedList.hpp"
#include "MyTimer.hpp"
#include "Message.hpp"
#include "Connection.hpp"

#define UPDATE_REMAINING_AIRTIME 60000
#define SESSION_CHECK_TIME 120000
#define BitrateSF7_125kHz 5470.0
#define MAX_AIRTIME_PER_HOUR 35000

typedef void (*FunctionPointer)(DeviceIdentity *from, char *msg);

class LoRaCon
{

public:
    LoRaCon(DeviceIdentity *ownDevice, FunctionPointer messageReceivedCallback);

    void addNewConnection(DeviceIdentity *device);
    void printConnections();

    bool sendDAT(uint8_t receiverId, char *data);
    bool sendFAF(uint8_t receiverId, char *data);

    void update();

private:
    // Own identity
    DeviceIdentity *ownDevice;

    FunctionPointer messageReceivedCallback;

    LinkedList<Connection> connections;
    Connection *findConnection(uint8_t deviceId);

    MyTimer sessionCheckTimer;
    MyTimer dutyCycleTimer;

    const LinkedListItem<Connection> *sendNext;

    uint16_t usedAirtime = 0;

    void sendNextMessage();
    void receiveMessage();
    uint16_t calculateAirtime(uint16_t sizeInBytes);
};