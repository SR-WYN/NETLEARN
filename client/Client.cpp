#include <boost/asio.hpp>
#include <iostream>
#include <cstring>
using namespace boost::asio::ip;
using namespace std;

#define MAX_LENGTH 1024
#define HEAD_LENGTH 2

int main()
{
    while (true)
    try
    {
        // 创建上下文服务
        boost::asio::io_context ioc;
        // 构造endpoint
        tcp::endpoint remote_ep(address::from_string("127.0.0.1"), 10086);
        tcp::socket sock(ioc);
        boost::system::error_code error = boost::asio::error::host_not_found;
        sock.connect(remote_ep, error);
        if (error)
        {
            cout << "connect failed, code is " << error.value() << " error msg is " << error.message();
            return 0;
        }
        std::cout << "Enter message: ";
        char request[MAX_LENGTH];
        std::cin.getline(request, MAX_LENGTH);
        short request_length = static_cast<short>(strlen(request));
        char send_data[MAX_LENGTH] = {0};
        short net_length = boost::asio::detail::socket_ops::host_to_network_short(request_length);
        memcpy(send_data, &net_length, HEAD_LENGTH);
        memcpy(send_data + HEAD_LENGTH, request, request_length);
        boost::asio::write(sock, boost::asio::buffer(send_data, request_length + HEAD_LENGTH));
        char reply_head[HEAD_LENGTH];
        boost::asio::read(sock, boost::asio::buffer(reply_head, HEAD_LENGTH));
        short net_msglen = 0;
        memcpy(&net_msglen, reply_head, HEAD_LENGTH);
        short msglen = boost::asio::detail::socket_ops::network_to_host_short(net_msglen);
        char msg[MAX_LENGTH] = {0};
        boost::asio::read(sock, boost::asio::buffer(msg, msglen));
        std::cout << "Reply is: ";
        std::cout.write(msg, msglen) << endl;
        std::cout << "Reply len is " << msglen;
        std::cout << "\n";
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << endl;
    }
    return 0;
}