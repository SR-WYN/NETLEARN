#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <chrono>
#include <vector>
#include <arpa/inet.h>
#include "const.h"

using namespace std;
using namespace boost::asio::ip;

std::vector<thread> vec_threads;

int main()
{
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; i++)
    {
        vec_threads.emplace_back([]()
                                 {
            try {
                boost::asio::io_context ioc;
                tcp::endpoint remote_ep(address::from_string("127.0.0.1"), 10086);
                tcp::socket sock(ioc);
                boost::system::error_code error;

                sock.connect(remote_ep, error);
                if (error) {
                    cout << "Connect failed: " << error.message() << endl;
                    return;
                }

                int j = 0;
                while (j < 500) {
                    // --- 1. 构造 JSON 数据 ---
                    Json::Value root;
                    root["id"] = 1001 + j;
                    root["data"] = "hello world";
                    
                    // 使用 FastWriter 减少不必要的空格，降低溢出风险
                    Json::FastWriter writer;
                    std::string request = writer.write(root);
                    uint16_t request_length = static_cast<uint16_t>(request.length());

                    // --- 2. 发送数据 (严格控制字节序和偏移) ---
                    std::vector<char> send_buf(HEAD_TOTAL_LEN + request_length);
                    uint16_t msgid_net = htons(MSG_HELLO_WORD);
                    uint16_t msglen_net = htons(request_length);

                    memcpy(send_buf.data(), &msgid_net, HEAD_ID_LEN);
                    memcpy(send_buf.data() + HEAD_ID_LEN, &msglen_net, HEAD_DATA_LEN);
                    memcpy(send_buf.data() + HEAD_TOTAL_LEN, request.c_str(), request_length);

                    boost::asio::write(sock, boost::asio::buffer(send_buf));

                    // --- 3. 接收头部 ---
                    char reply_head[HEAD_TOTAL_LEN];
                    boost::asio::read(sock, boost::asio::buffer(reply_head, HEAD_TOTAL_LEN));

                    uint16_t r_msgid_net, r_msglen_net;
                    memcpy(&r_msgid_net, reply_head, HEAD_ID_LEN);
                    memcpy(&r_msglen_net, reply_head + HEAD_ID_LEN, HEAD_DATA_LEN);

                    uint16_t r_msglen = ntohs(r_msglen_net);

                    // --- 4. 接收消息体 (安全校验) ---
                    if (r_msglen > MAX_LENGTH) {
                        throw std::runtime_error("Message too large, possible protocol error.");
                    }

                    std::vector<char> body_buf(r_msglen);
                    boost::asio::read(sock, boost::asio::buffer(body_buf.data(), r_msglen));

                    // --- 5. 解析 JSON ---
                    Json::Reader reader;
                    Json::Value res_root;
                    if (reader.parse(body_buf.data(), body_buf.data() + r_msglen, res_root)) {
                        std::cout << "Receive ID: " << res_root["id"] << " Data: " << res_root["data"] << endl;
                    }
                    
                    j++;
                }
            }
            catch (std::exception& e) {
                std::cerr << "Exception in thread: " << e.what() << endl;
            } });
    }

    for (auto &t : vec_threads)
    {
        if (t.joinable())
            t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "All tasks finished." << std::endl;
    std::cout << "Time spent: " << duration.count() / 1000.0 << " seconds." << std::endl;

    std::cout << "Press Enter to exit..." << std::endl;
    getchar();
    return 0;
}