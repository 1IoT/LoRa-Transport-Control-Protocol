#pragma once
#include <Arduino.h>
#include "LinkedList.hpp"
#include "MyTimer.hpp"
#include "Message.hpp"
#include "Connection.hpp"

#define sendMsgTime 10000
#define msgRetryTime 20000

typedef void (*functionPointer)(DeviceIdentity *from, char *msg);

class LoRaCon
{

public:
    LoRaCon(DeviceIdentity *device, functionPointer callback);

    void addNewConnection(DeviceIdentity *device);
    void printConnections();

    bool sendDAT(uint8_t receiverId, char *data);
    bool sendFAF(uint8_t receiverId, char *data);

    void update();

    /*
    bool sendData(uint8_t receiverId, char *data);
    void printPendingMessage();

    void receiving();
    */

private:
    // Own identity
    DeviceIdentity *ownDevice;

    functionPointer callback;

    LinkedList<Connection> connections;
    Connection *findConnection(uint8_t deviceId);

    MyTimer timer;
    const LinkedListItem<Connection> *sendNext;
    void sendNextMessage();

    void receiveMessage();
};