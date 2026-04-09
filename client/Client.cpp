#include <boost/asio.hpp>
#include <iostream>
using namespace boost::asio::ip;
using namespace std;

constexpr int MAX_LENGTH = 1024;

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
            cout << "connect failed,code is " << error.value() << " error msg is " << error.message() << endl;
            return 0;
        }
        
        cout << "Enter message: ";
        char request[MAX_LENGTH];
        cin.getline(request,MAX_LENGTH);
        size_t request_length = strlen(request);
        boost::asio::write(sock,boost::asio::buffer(request,request_length));

        char reply[MAX_LENGTH];
        size_t reply_length = boost::asio::read(sock,boost::asio::buffer(reply,request_length));
        cout << "Reply is: ";
        cout.write(reply,reply_length);
        cout << '\n';
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception:" << e.what() << endl;
    }
}