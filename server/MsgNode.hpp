#pragma once
#include <string.h>

#define HEAD_LENGTH 2

class MsgNode
{
    friend class Session;

public:
    MsgNode(short max_len);
    ~MsgNode();
    void Clear();
    short _cur_len;
    short _total_len;
    char *_data;
};

class RecvNode : public MsgNode
{
public:
    RecvNode(short max_len, short msg_id);
    short msgId() const { return _msg_id; }

private:
    short _msg_id;
};

class SendNode : public MsgNode
{
public:
    SendNode(const char *msg, short max_len, short msg_id);

private:
    short _msg_id;
};