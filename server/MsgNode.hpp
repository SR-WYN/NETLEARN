#include <string.h>

#define HEAD_LENGTH 2

class MsgNode
{
    friend class Session;
public:
    MsgNode(const char * msg, short max_len);
    MsgNode(short max_len);
    ~MsgNode();
    void Clear();
private:
    short _cur_len;
    short _total_len;
    char* _data;
};