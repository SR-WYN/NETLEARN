#include "msg.pb.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "Session.hpp"
#include "Server.hpp"
#include "MsgNode.hpp"
#include <iostream>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <mutex>

#define MAX_SENDQUE 1000

using namespace std;

Session::Session(boost::asio::io_context &ioc, Server *server)
    : _socket(ioc), _server(server)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
    _recv_head_node = make_shared<MsgNode>(HEAD_LENGTH);
    cout << "The uuid is " << _uuid << endl;
}

Session::~Session()
{
    cout << "session destruct delete this " << this << endl;
}

void Session::Send(char *msg, int max_length)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE)
    {
        cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << endl;
        return;
    }
    _send_que.push(make_shared<MsgNode>(msg, max_length));
    if (send_que_size > 0)
    {
        return;
    }
    auto &msgnode = _send_que.front();
    auto self = shared_from_this();
    boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
                             [self](const boost::system::error_code &ec, std::size_t)
                             { self->handle_write(ec); });
}

void Session::Send(std::string msg)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE)
    {
        std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << endl;
        return;
    }

    _send_que.push(make_shared<MsgNode>(msg.c_str(), msg.length()));
    if (send_que_size > 0)
    {
        return;
    }
    auto &msgnode = _send_que.front();
    auto self = shared_from_this();
    boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
                             [self](const boost::system::error_code &ec, std::size_t)
                             {
                                 self->handle_write(ec);
                             });
}

void Session::Start()
{
    _recv_head_node->Clear();
    do_read_header();
}

std::string &Session::GetUuid()
{
    return _uuid;
}

void Session::do_read_header()
{
    auto self = shared_from_this();
    boost::asio::async_read(_socket,
                            boost::asio::buffer(_recv_head_node->_data, HEAD_LENGTH),
                            [self](const boost::system::error_code &ec, std::size_t bytes_transferred)
                            {
                                if (ec || bytes_transferred != HEAD_LENGTH)
                                {
                                    std::cout << "read header failed, error is " << ec.message() << std::endl;
                                    self->_server->ClearSession(self->_uuid);
                                    return;
                                }

                                short data_len = 0;
                                memcpy(&data_len, self->_recv_head_node->_data, HEAD_LENGTH);
                                data_len = boost::asio::detail::socket_ops::network_to_host_short(data_len);
                                std::cout << "data_len is " << data_len << std::endl;
                                if (data_len <= 0 || data_len > MAX_LENGTH)
                                {
                                    std::cout << "invalid data length is " << data_len << std::endl;
                                    self->_server->ClearSession(self->_uuid);
                                    return;
                                }

                                self->_recv_msg_node = make_shared<MsgNode>(data_len);
                                self->do_read_body(data_len);
                            });
}

void Session::do_read_body(short data_len)
{
    auto self = shared_from_this();
    boost::asio::async_read(_socket,
                            boost::asio::buffer(self->_recv_msg_node->_data, data_len),
                            [self](const boost::system::error_code &ec, std::size_t bytes_transferred)
                            {
                                if (ec || bytes_transferred != static_cast<std::size_t>(self->_recv_msg_node->_total_len))
                                {
                                    std::cout << "read body failed, error is " << ec.message() << std::endl;
                                    self->_server->ClearSession(self->_uuid);
                                    return;
                                }

                                self->_recv_msg_node->_data[self->_recv_msg_node->_total_len] = '\0';
                                Json::Reader reader;
                                Json::Value root;
                                if (!reader.parse(std::string(self->_recv_msg_node->_data, self->_recv_msg_node->_total_len), root))
                                {
                                    std::cout << "json parse failed" << std::endl;
                                    self->do_read_header();
                                    return;
                                }

                                std::cout << "recevie msg id  is " << root["id"].asInt() << " msg data is "
                                          << root["data"].asString() << std::endl;
                                root["data"] = "server has received msg,msg data is " + root["data"].asString();
                                std::string return_str = root.toStyledString();
                                self->Send(return_str);
                                self->do_read_header();
                            });
}

void Session::handle_write(const boost::system::error_code &error)
{
    if (!error)
    {
        std::lock_guard<std::mutex> lock(_send_lock);
        _send_que.pop();
        if (!_send_que.empty())
        {
            auto &msgnode = _send_que.front();
            auto self = shared_from_this();
            boost::asio::async_write(_socket,
                                     boost::asio::buffer(msgnode->_data, msgnode->_total_len),
                                     [self](const boost::system::error_code &ec, std::size_t /*bytes*/)
                                     {
                                         self->handle_write(ec);
                                     });
        }
    }
    else
    {
        std::cerr << "Handle write failed, error: " << error.message() << std::endl;
        _server->ClearSession(_uuid);
    }
}