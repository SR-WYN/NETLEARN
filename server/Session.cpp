#include "msg.pb.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "Session.hpp"
#include "Server.hpp"
#include <iostream>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <mutex>
#include "const.h"
#include <memory>
#include "LogicSystem.hpp"
#include "LogicNode.hpp"

using namespace std;

Session::Session(boost::asio::io_context &ioc, Server *server)
    : _socket(ioc), _server(server)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
    _recv_head_node = make_shared<MsgNode>(HEAD_TOTAL_LEN);
    cout << "The uuid is " << _uuid << endl;
}

Session::~Session()
{
    cout << "session destruct delete this " << this << endl;
}

void Session::Send(char *msg, int max_length, short msgid)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE)
    {
        std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << std::endl;
        return;
    }

    _send_que.push(make_shared<SendNode>(msg, max_length, msgid));
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

void Session::Send(std::string msg, short msgid)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE)
    {
        std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << std::endl;
        return;
    }

    _send_que.push(make_shared<SendNode>(msg.c_str(), static_cast<short>(msg.length()), msgid));
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
                            boost::asio::buffer(_recv_head_node->_data, HEAD_TOTAL_LEN),
                            [self](const boost::system::error_code &ec, std::size_t bytes_transferred)
                            {
                                if (ec || bytes_transferred != HEAD_TOTAL_LEN)
                                {
                                    std::cout << "read header failed, error is " << ec.message() << std::endl;
                                    self->_server->ClearSession(self->_uuid);
                                    return;
                                }

                                short msg_id = 0;
                                short data_len = 0;
                                memcpy(&msg_id, self->_recv_head_node->_data, HEAD_ID_LEN);
                                memcpy(&data_len, self->_recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
                                msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
                                data_len = boost::asio::detail::socket_ops::network_to_host_short(data_len);
                                std::cout << "msg_id is " << msg_id << " data_len is " << data_len << std::endl;
                                if (data_len <= 0 || data_len > MAX_LENGTH)
                                {
                                    std::cout << "invalid data length is " << data_len << std::endl;
                                    self->_server->ClearSession(self->_uuid);
                                    return;
                                }

                                self->_recv_msg_node = make_shared<RecvNode>(data_len, msg_id);
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
                                LogicSystem::GetInstance()->PostMsgToQueue(
                                    std::make_shared<LogicNode>(self, self->_recv_msg_node));
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