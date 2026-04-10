#include "msg.pb.h"
#include "Client.hpp"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <boost/asio.hpp>
#include <iostream>
#include <cstring>
#include <array>
#include <vector>
using namespace boost::asio::ip;
using namespace std;

Client::Client(boost::asio::io_context &ioc)
    : _ioc(ioc), _sock(ioc)
{
}

bool Client::connect(const std::string &address, unsigned short port, boost::system::error_code &ec)
{
    boost::asio::ip::tcp::endpoint remote_ep(boost::asio::ip::address::from_string(address), port);
    _sock.connect(remote_ep, ec);
    return !ec;
}

void Client::send_request(const std::string &request)
{
    size_t request_length = request.length();
    vector<char> send_data(HEAD_LENGTH + request_length);
    int request_host_length = boost::asio::detail::socket_ops::host_to_network_short(request_length);
    memcpy(send_data.data(), &request_host_length, HEAD_LENGTH);
    memcpy(send_data.data() + HEAD_LENGTH, request.c_str(), request_length);
    boost::asio::write(_sock, boost::asio::buffer(send_data));
}

void Client::start_receive()
{
    do_read_header();
}

void Client::do_read_header()
{
    auto reply_head = make_shared<array<char, HEAD_LENGTH>>();
    boost::asio::async_read(_sock,
                            boost::asio::buffer(*reply_head),
                            [this, reply_head](const boost::system::error_code &ec, size_t bytes_transferred)
                            {
                                if (ec)
                                {
                                    cerr << "read header failed, error is " << ec.message() << endl;
                                    return;
                                }

                                short msglen = 0;
                                memcpy(&msglen, reply_head->data(), HEAD_LENGTH);
                                msglen = boost::asio::detail::socket_ops::network_to_host_short(msglen);
                                if (msglen <= 0 || msglen > static_cast<short>(MAX_LENGTH))
                                {
                                    cerr << "invalid reply length: " << msglen << endl;
                                    return;
                                }

                                auto reply_msg = make_shared<vector<char>>(msglen);
                                do_read_body(reply_msg);
                            });
}

void Client::do_read_body(std::shared_ptr<std::vector<char>> reply_msg)
{
    boost::asio::async_read(_sock,
                            boost::asio::buffer(*reply_msg),
                            [reply_msg](const boost::system::error_code &ec, size_t bytes_transferred)
                            {
                                if (ec)
                                {
                                    cerr << "read body failed, error is " << ec.message() << endl;
                                    return;
                                }

                                Json::Reader reader;
                                Json::Value root;
                                if (!reader.parse(string(reply_msg->data(), bytes_transferred), root))
                                {
                                    cerr << "json parse failed" << endl;
                                    return;
                                }

                                cout << "msg id is " << root["id"].asInt() << " msg is " << root["data"].asString() << endl;
                            });
}

int main()
{
    while (true)
        try
        {
		// 创建上下文服务
		boost::asio::io_context ioc;
		Client client(ioc);
		boost::system::error_code error;
		if (!client.connect("127.0.0.1", 10086, error)) {
			cout << "connect failed, code is " << error.value() << " error msg is " << error.message();
			return 0;
		}

		Json::Value root;
		root["id"] = 1001;
		root["data"] = "hello world";
		std::string request = root.toStyledString();
		client.send_request(request);
		cout << "begin to receive..." << endl;

		client.start_receive();
		ioc.run();
		getchar();
        }
        catch (std::exception &e)
        {
            std::cerr << "Exception: " << e.what() << endl;
        }
    return 0;
}