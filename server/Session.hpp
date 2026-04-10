#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <queue>
#include <memory>
#include "MsgNode.hpp"
using boost::asio::ip::tcp;

class Server;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::io_context &ioc, Server *server);
    ~Session();
    void Send(char *msg, int max_length);
    void Send(std::string msg);
    tcp::socket &Socket() { return _socket; }
    void Start();
    std::string &GetUuid();

private:
    void do_read_header();
    void do_read_body(short data_len);
    void handle_write(const boost::system::error_code &error);
    tcp::socket _socket;
    Server *_server;
    std::string _uuid;
    std::queue<std::shared_ptr<SendNode>> _send_que;
    std::mutex _send_lock;
    std::shared_ptr<RecvNode> _recv_msg_node;
    std::shared_ptr<MsgNode> _recv_head_node;
};