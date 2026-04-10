#include "msg.pb.h"
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
    : _socket(ioc), _server(server), _b_head_parse(false)
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

void Session::Send(std::string msg) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << endl;
		return;
	}

	_send_que.push(make_shared<MsgNode>(msg.c_str(), msg.length()));
	if (send_que_size > 0) {
		return;
	}
	auto& msgnode = _send_que.front();
	auto self = shared_from_this();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		[self](const boost::system::error_code &ec, std::size_t)
		{
			self->handle_write(ec);
		});
}

void Session::Start()
{
    auto self = shared_from_this();
    _socket.async_read_some(
        boost::asio::buffer(_data, MAX_LENGTH),
        [this, self](const boost::system::error_code &ec, size_t bytes_transferred)
        {
            this->handle_read(ec, bytes_transferred);
        });
}

std::string &Session::GetUuid()
{
    return _uuid;
}

void Session::handle_read(const boost::system::error_code &error, size_t bytes_transferred)
{
    auto self = shared_from_this();
    if (!error)
    {
        // 已经移动的字符数
        int copy_len = 0;
        while (bytes_transferred > 0)
        {
            if (!_b_head_parse)
            {
                // 收到的数据不足头部大小
                if (bytes_transferred + _recv_head_node->_cur_len < HEAD_LENGTH)
                {
                    memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, bytes_transferred);
                    _recv_head_node->_cur_len += bytes_transferred;
                    ::memset(_data, 0, MAX_LENGTH);
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            [self](const boost::system::error_code &ec, size_t bytes)
                                            {
                                                self->handle_read(ec, bytes);
                                            });
                    return;
                }
                // 收到的数据比头部多
                // 头部剩余未复制的长度
                int head_remain = HEAD_LENGTH - _recv_head_node->_cur_len;
                memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, head_remain);
                // 更新已处理的data长度和剩余未处理的长度
                copy_len += head_remain;
                bytes_transferred -= head_remain;
                // 获取头部数据
                short data_len = 0;
                memcpy(&data_len, _recv_head_node->_data, HEAD_LENGTH);
                // 网络字节序转化为本地字节序
                data_len = boost::asio::detail::socket_ops::network_to_host_short(data_len);
                cout << "data_len is " << data_len << endl;
                // 头部长度非法
                if (data_len > MAX_LENGTH)
                {
                    std::cout << "invalid data length is " << data_len << endl;
                    _server->ClearSession(_uuid);
                    return;
                }
                _recv_msg_node = make_shared<MsgNode>(data_len);
                // 消息的长度小于头部规定的长度，说明数据未收全，则先将部分消息放到接收节点里
                if (bytes_transferred < data_len)
                {
                    memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_transferred);
                    _recv_msg_node->_cur_len += bytes_transferred;
                    ::memset(_data, 0, MAX_LENGTH);
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            [self](const boost::system::error_code &ec, size_t bytes)
                                            {
                                                self->handle_read(ec, bytes);
                                            });
                    // 头部处理完成
                    _b_head_parse = true;
                    return;
                }
                memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, data_len);
                _recv_msg_node->_cur_len += data_len;
                copy_len += data_len;
                bytes_transferred -= data_len;
                _recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
                // cout << "receive data is " << _recv_msg_node->_data << endl;
                // // 此处可以调用Send发送测试
                MsgData msgdata;
                std::string receive_data;
                msgdata.ParseFromString(std::string(_recv_msg_node->_data, _recv_msg_node->_total_len));
                cout << "recevie msg id  is " << msgdata.id() << " msg data is " << msgdata.data() << endl;
                string return_str = "server has received msg, msg data is " + msgdata.data();
                MsgData msgreturn;
                msgreturn.set_id(msgdata.id());
                msgreturn.set_data(return_str);
                msgreturn.SerializeToString(&return_str);
                Send(return_str);
                // 继续轮询剩余未处理数据
                _b_head_parse = false;
                _recv_head_node->Clear();
                if (bytes_transferred <= 0)
                {
                    ::memset(_data, 0, MAX_LENGTH);
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            [self](const boost::system::error_code &ec, size_t bytes)
                                            {
                                                self->handle_read(ec, bytes);
                                            });
                    return;
                }
                continue;
            }
            // 已经处理完头部，处理上次未接受完的消息数据
            // 接收的数据仍不足剩余未处理的
            int remain_msg = _recv_msg_node->_total_len - _recv_msg_node->_cur_len;
            if (bytes_transferred < remain_msg)
            {
                memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_transferred);
                _recv_msg_node->_cur_len += bytes_transferred;
                ::memset(_data, 0, MAX_LENGTH);
                _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                        [this, self](const boost::system::error_code &ec, size_t bytes)
                                        {
                                            this->handle_read(ec, bytes);
                                        });
                return;
            }
            memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, remain_msg);
            _recv_msg_node->_cur_len += remain_msg;
            bytes_transferred -= remain_msg;
            copy_len += remain_msg;
            _recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
            cout << "receive data is " << _recv_msg_node->_data << endl;
            // 此处可以调用Send发送测试
            Send(_recv_msg_node->_data, _recv_msg_node->_total_len);
            // 继续轮询剩余未处理数据
            _b_head_parse = false;
            _recv_head_node->Clear();
            if (bytes_transferred <= 0)
            {
                ::memset(_data, 0, MAX_LENGTH);
                _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                        [this, self](const boost::system::error_code &ec, size_t bytes)
                                        {
                                            this->handle_read(ec, bytes);
                                        });
                return;
            }
            continue;
        }
    }
    else
    {
        std::cout << "handle read failed, error is " << error.what() << endl;
        // Close();
        _server->ClearSession(_uuid);
    }
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