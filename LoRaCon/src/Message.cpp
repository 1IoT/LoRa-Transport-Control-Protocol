#include "Message.hpp"

Message::Message(MsgType msgType, uint8_t msgId, size_t msgLength, char *msg)
    : msgType(msgType), msgId(msgId), msgLength(msgLength), msg(msg){};

Message::~Message()
{
    delete[] msg;
}