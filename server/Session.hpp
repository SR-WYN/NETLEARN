#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <queue>
#include <string>

#include "MsgNode.hpp"

class Server;

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(boost::asio::io_context& ioc, Server* server);
  ~Session();

  void Send(const char* msg, int max_length, short msgid);
  void Send(const std::string& msg, short msgid);

  tcp::socket& Socket() { return _socket; }
  void Start();
  const std::string& GetUuid() const { return _uuid; }

 private:
  void DoReadHeader();
  void DoReadBody(short data_len);
  void HandleWrite(const boost::system::error_code& error);

  // 使用strand保证发送队列的线程安全
  void DoSend();

  tcp::socket _socket;
  boost::asio::strand<boost::asio::io_context::executor_type> _strand;
  Server* _server;
  std::string _uuid;
  std::queue<std::shared_ptr<SendNode>> _send_que;
  std::shared_ptr<RecvNode> _recv_msg_node;
  std::shared_ptr<MsgNode> _recv_head_node;
};