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
    vector<char> send_data(HEAD_TOTAL_LEN + request_length);
    short msg_id = MSG_HELLO_WORD;
    short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    short data_len_host = boost::asio::detail::socket_ops::host_to_network_short(static_cast<short>(request_length));
    memcpy(send_data.data(), &msg_id_host, HEAD_ID_LEN);
    memcpy(send_data.data() + HEAD_ID_LEN, &data_len_host, HEAD_DATA_LEN);
    memcpy(send_data.data() + HEAD_TOTAL_LEN, request.c_str(), request_length);
    boost::asio::write(_sock, boost::asio::buffer(send_data));
}

void Client::start_receive()
{
    do_read_header();
}

void Client::do_read_header()
{
    auto reply_head = make_shared<array<char, HEAD_TOTAL_LEN>>();
    boost::asio::async_read(_sock,
                            boost::asio::buffer(*reply_head),
                            [this, reply_head](const boost::system::error_code &ec, size_t bytes_transferred)
                            {
                                if (ec)
                                {
                                    cerr << "read header failed, error is " << ec.message() << endl;
                                    return;
                                }

                                if (bytes_transferred != HEAD_TOTAL_LEN)
                                {
                                    cerr << "invalid header length: " << bytes_transferred << endl;
                                    return;
                                }

                                short msg_id = 0;
                                short data_len = 0;
                                memcpy(&msg_id, reply_head->data(), HEAD_ID_LEN);
                                memcpy(&data_len, reply_head->data() + HEAD_ID_LEN, HEAD_DATA_LEN);
                                msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
                                data_len = boost::asio::detail::socket_ops::network_to_host_short(data_len);
                                if (data_len <= 0 || data_len > static_cast<short>(MAX_LENGTH))
                                {
                                    cerr << "invalid reply length: " << data_len << endl;
                                    return;
                                }

                                cout << "reply msg_id=" << msg_id << " data_len=" << data_len << endl;
                                auto reply_msg = make_shared<vector<char>>(data_len);
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