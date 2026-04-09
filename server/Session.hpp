#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <queue>
using boost::asio::ip::tcp;

class Server;
class MsgNode;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::io_context &ioc, Server *server);
    void Send(char *msg, int max_length);
    tcp::socket &Socket() { return _socket; }
    void Start();
    std::string &GetUuid();

private:
    void handle_read(const boost::system::error_code &error, size_t bytes_transferred);
    void handle_write(const boost::system::error_code &error);
    tcp::socket _socket;
    enum
    {
        MAX_LENGTH = 1024
    };
    char _data[MAX_LENGTH];
    Server *_server;
    std::string _uuid;
    std::queue<std::shared_ptr<MsgNode>> _send_que;
    std::mutex _send_lock;
};