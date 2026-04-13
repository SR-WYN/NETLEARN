#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <chrono>
#include <vector>

using namespace std;
using namespace boost::asio::ip;
const int MAX_LENGTH = 1024 * 2;
const int HEAD_LENGTH = 2;
const int HEAD_TOTAL = 4;

std::vector<thread> vec_threads;

int main()
{
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        vec_threads.emplace_back([]() {
            try {
                boost::asio::io_context ioc;
                tcp::endpoint remote_ep(address::from_string("127.0.0.1"), 10086);
                tcp::socket sock(ioc);
                boost::system::error_code error = boost::asio::error::host_not_found;
                sock.connect(remote_ep, error);
                if (error) {
                    cout << "connect failed, code is " << error.value() << " error msg is " << error.message() << endl;
                    return;
                }
                int msg_count = 0;
                while (msg_count < 500) {
                    Json::Value root;
                    root["id"] = 1001;
                    root["data"] = "hello world";
                    std::string request = root.toStyledString();
                    size_t request_length = request.length();
                    
                    // 检查消息长度是否超过最大值
                    if (request_length > MAX_LENGTH - HEAD_TOTAL) {
                        cerr << "Error: request length " << request_length << " exceeds maximum allowed length" << endl;
                        break;
                    }
                    
                    // 使用动态内存分配
                    std::vector<char> send_data(request_length + HEAD_TOTAL);
                    short msgid = 1001;
                    short msgid_host = boost::asio::detail::socket_ops::host_to_network_short(msgid);
                    memcpy(send_data.data(), &msgid_host, HEAD_LENGTH);
                    
                    short request_length_short = static_cast<short>(request_length);
                    short request_host_length = boost::asio::detail::socket_ops::host_to_network_short(request_length_short);
                    memcpy(send_data.data() + HEAD_LENGTH, &request_host_length, HEAD_LENGTH);
                    memcpy(send_data.data() + HEAD_TOTAL, request.c_str(), request_length);
                    
                    boost::asio::write(sock, boost::asio::buffer(send_data.data(), request_length + HEAD_TOTAL));
                    
                    // 接收响应头部
                    std::vector<char> reply_head(HEAD_TOTAL);
                    size_t reply_length = boost::asio::read(sock, boost::asio::buffer(reply_head.data(), HEAD_TOTAL));
                    if (reply_length != HEAD_TOTAL) {
                        cerr << "Error: failed to read response header" << endl;
                        break;
                    }
                    
                    short recv_msgid = 0;
                    memcpy(&recv_msgid, reply_head.data(), HEAD_LENGTH);
                    short recv_msglen = 0;
                    memcpy(&recv_msglen, reply_head.data() + HEAD_LENGTH, HEAD_LENGTH);
                    
                    // 转换为本地字节序
                    recv_msglen = boost::asio::detail::socket_ops::network_to_host_short(recv_msglen);
                    recv_msgid = boost::asio::detail::socket_ops::network_to_host_short(recv_msgid);
                    
                    // 检查接收到的消息长度是否合法
                    if (recv_msglen <= 0 || recv_msglen > MAX_LENGTH) {
                        cerr << "Error: invalid message length " << recv_msglen << endl;
                        break;
                    }
                    
                    // 接收消息体
                    std::vector<char> msg(recv_msglen);
                    size_t msg_length = boost::asio::read(sock, boost::asio::buffer(msg.data(), recv_msglen));
                    if (msg_length != recv_msglen) {
                        cerr << "Error: failed to read complete message body" << endl;
                        break;
                    }
                    
                    // 解析JSON
                    Json::Reader reader;
                    Json::Value recv_root;
                    std::string msg_str(msg.data(), msg_length);
                    if (!reader.parse(msg_str, recv_root)) {
                        cerr << "Error: failed to parse JSON response" << endl;
                        continue;
                    }
                    
                    std::cout << "msg id is " << recv_root["id"] << " msg is " << recv_root["data"] << endl;
                    msg_count++;
                }
            }
            catch (std::exception& e) {
                std::cerr << "Exception: " << e.what() << endl;
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (auto& t : vec_threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Time spent: " << duration.count() << " seconds." << std::endl;
    getchar();
    return 0;
}
