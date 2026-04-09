#include <string.h>

class MsgNode
{
    friend class Session;

public:
    MsgNode(char *msg, int max_len);
    ~MsgNode();

private:
    int _cur_len;
    int _max_len;
    char *_data;
};