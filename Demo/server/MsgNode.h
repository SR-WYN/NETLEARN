#pragma once
#include <string>
#include <iostream>
#include "const.h"
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

class MsgNode
{
public:
    MsgNode(short max_len);
    ~MsgNode();
    void Clear();
    char* GetData();
    short GetTotalLen();
protected:
    short _cur_len;
    short _total_len;
    char* _data;
};

class RecvNode : public MsgNode
{
public:
    RecvNode(short max_len,short msg_id);
    short GetMsgId();
private:
    short msg_id;
};

class SendNode : public MsgNode
{
public:
    SendNode(const char* msg,short max_len,short msg_id);
    short GetMsgId();
private:
    short msg_id;
};
