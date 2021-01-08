#pragma once
#include <Arduino.h>

enum MsgType
{
    MsgType_SESSION_NEW,
    MsgType_SESSION_KEY,
    MsgType_SESSION_KEY_ACK,
    MsgType_FAF,
    MsgType_DAT,
    MsgType_DAT_ACK
};

class Message
{
public:
    Message(MsgType msgType, uint8_t msgId, size_t msgLength, char *msg);
    ~Message();

    MsgType getMsgType() { return msgType; }
    uint8_t getMsgId() { return msgId; }
    size_t getMsgLength() { return msgLength; }
    const char *getMsg() { return msg; }

private:
    MsgType msgType;
    uint8_t msgId;
    size_t msgLength;
    char *msg;
};
