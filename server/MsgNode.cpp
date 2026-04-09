#include "MsgNode.hpp"

MsgNode::MsgNode(char *msg, short max_len) : _total_len(max_len + HEAD_LENGTH), _cur_len(0)
{
    _data = new char[_total_len + 1]();
    memcpy(_data, &max_len, HEAD_LENGTH);
    memcpy(_data + HEAD_LENGTH, msg, max_len);
    _data[_total_len] = '\0';
}

MsgNode::MsgNode(short max_len) : _total_len(max_len), _cur_len(0)
{
    _data = new char[_total_len + 1]();
}

MsgNode::~MsgNode()
{
    delete[] _data;
}

void MsgNode::Clear()
{
    ::memset(_data, 0, _total_len);
    _cur_len = 0;
}