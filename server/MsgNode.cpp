#include "MsgNode.hpp"

MsgNode::MsgNode(char *msg, int max_len)
{
    _data = new char[max_len];
    memcpy(_data, msg, max_len);
}

MsgNode::~MsgNode()
{
    delete[] _data;
}