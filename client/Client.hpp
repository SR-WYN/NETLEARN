#pragma once

#include <boost/asio.hpp>
#include <array>
#include <memory>
#include <string>
#include <vector>

class Client
{
public:
    static constexpr std::size_t MAX_LENGTH = 1024;
    static constexpr std::size_t HEAD_LENGTH = 2;

    explicit Client(boost::asio::io_context &ioc);

    bool connect(const std::string &address, unsigned short port, boost::system::error_code &ec);
    void send_request(const std::string &request);
    void start_receive();

private:
    void do_read_header();
    void do_read_body(std::shared_ptr<std::vector<char>> reply_msg);

    boost::asio::io_context &_ioc;
    boost::asio::ip::tcp::socket _sock;
};
