#include "Session.hpp"
#include "Server.hpp"
#include "MsgNode.hpp"
#include <iostream>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <mutex>
using namespace std;

Session::Session(boost::asio::io_context &ioc, Server *server) : _socket(ioc), _server(server)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
    cout << "The uuid is " << _uuid << endl;
}

Session::~Session() 
{
    cout << "session destruct delete this " << this << endl;
}

void Session::Send(char *msg, int max_length)
{
    bool pending = false;
    std::lock_guard<std::mutex> lock(_send_lock);
    if (_send_que.size() > 0)
    {
        pending = true;
    }
    _send_que.push(make_shared<MsgNode>(msg, max_length));
    if (pending)
    {
        return;
    }
    boost::asio::async_write(_socket, boost::asio::buffer(msg, max_length),
                             [self = shared_from_this()](const boost::system::error_code &error, std::size_t bytes_transferred)
                             {
                                 self->handle_write(error);
                             });
}

void Session::Start()
{
    auto self = shared_from_this();
    _socket.async_read_some(
        boost::asio::buffer(_data, MAX_LENGTH),
        [this](const boost::system::error_code &ec, size_t bytes_transferred)
        {
            this->handle_read(ec, bytes_transferred);
        });
}

std::string &Session::GetUuid()
{
    return _uuid;
}

void Session::handle_read(const boost::system::error_code &ec, size_t bytes_transferred)
{
    if (!ec)
    {
        Send(_data, bytes_transferred);
        _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
            [self = shared_from_this()](const boost::system::error_code &error, size_t bytes)
            {
                self->handle_read(error, bytes);
            });
    }
    else
    {
        cout << "Read error: " << ec.message() << endl;
        _server->ClearSession(_uuid);
    }
}

void Session::handle_write(const boost::system::error_code& error) {
    if (!error) {
        std::lock_guard<std::mutex> lock(_send_lock);
        _send_que.pop();
        if (!_send_que.empty()) {
            auto &msgnode = _send_que.front();
            boost::asio::async_write(_socket, 
                boost::asio::buffer(msgnode->_data, msgnode->_max_len),
                [self = shared_from_this()](const boost::system::error_code& ec, std::size_t /*bytes*/) {
                    self->handle_write(ec);
                }
            );
        }
    }
    else {
        std::cerr << "Handle write failed, error: " << error.message() << std::endl;
        _server->ClearSession(_uuid);
    }
}