#include "Session.hpp"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>

#include "LogicNode.hpp"
#include "LogicSystem.hpp"
#include "Server.hpp"
#include "const.h"
#include "msg.pb.h"

Session::Session(boost::asio::io_context& ioc, Server* server)
    : _socket(ioc), _strand(boost::asio::make_strand(ioc)), _server(server) {
  boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
  _uuid = boost::uuids::to_string(a_uuid);
  _recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
  std::cout << "Session created, uuid: " << _uuid << std::endl;
}

Session::~Session() {
  std::cout << "Session destroyed, uuid: " << _uuid << std::endl;
}

void Session::Send(const char* msg, int max_length, short msgid) {
  auto self = shared_from_this();
  // 使用strand.post确保操作在strand中串行执行
  boost::asio::post(_strand, [self, msg, max_length, msgid]() {
    if (self->_send_que.size() >= MAX_SENDQUE) {
      std::cout << "Session " << self->_uuid
                << " send queue full, max size: " << MAX_SENDQUE << std::endl;
      return;
    }

    self->_send_que.push(
        std::make_shared<SendNode>(msg, max_length, msgid));

    // 如果队列中只有当前消息，则开始发送
    if (self->_send_que.size() == 1) {
      self->DoSend();
    }
  });
}

void Session::Send(const std::string& msg, short msgid) {
  auto self = shared_from_this();
  // 使用strand.post确保操作在strand中串行执行
  boost::asio::post(_strand, [self, msg, msgid]() {
    if (self->_send_que.size() >= MAX_SENDQUE) {
      std::cout << "Session " << self->_uuid
                << " send queue full, max size: " << MAX_SENDQUE << std::endl;
      return;
    }

    self->_send_que.push(
        std::make_shared<SendNode>(msg.c_str(), static_cast<short>(msg.length()), msgid));

    // 如果队列中只有当前消息，则开始发送
    if (self->_send_que.size() == 1) {
      self->DoSend();
    }
  });
}

void Session::DoSend() {
  if (_send_que.empty()) {
    return;
  }

  const auto& msgnode = _send_que.front();
  auto self = shared_from_this();

  // 使用bind_executor确保回调在strand中执行
  boost::asio::async_write(
      _socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
      boost::asio::bind_executor(
          _strand, [self](const boost::system::error_code& ec, std::size_t) {
            self->HandleWrite(ec);
          }));
}

void Session::Start() {
  _recv_head_node->Clear();
  DoReadHeader();
}

void Session::DoReadHeader() {
  auto self = shared_from_this();
  boost::asio::async_read(
      _socket,
      boost::asio::buffer(_recv_head_node->_data, HEAD_TOTAL_LEN),
      [self](const boost::system::error_code& ec,
             std::size_t bytes_transferred) {
        if (ec || bytes_transferred != HEAD_TOTAL_LEN) {
          std::cout << "Read header failed, error: " << ec.message()
                    << std::endl;
          self->_server->ClearSession(self->_uuid);
          return;
        }

        short msg_id = 0;
        short data_len = 0;
        memcpy(&msg_id, self->_recv_head_node->_data, HEAD_ID_LEN);
        memcpy(&data_len, self->_recv_head_node->_data + HEAD_ID_LEN,
               HEAD_DATA_LEN);
        msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
        data_len =
            boost::asio::detail::socket_ops::network_to_host_short(data_len);

        std::cout << "msg_id: " << msg_id << ", data_len: " << data_len
                  << std::endl;

        if (data_len <= 0 || data_len > MAX_LENGTH) {
          std::cout << "Invalid data length: " << data_len << std::endl;
          self->_server->ClearSession(self->_uuid);
          return;
        }

        self->_recv_msg_node =
            std::make_shared<RecvNode>(data_len, msg_id);
        self->DoReadBody(data_len);
      });
}

void Session::DoReadBody(short data_len) {
  auto self = shared_from_this();
  boost::asio::async_read(
      _socket,
      boost::asio::buffer(self->_recv_msg_node->_data, data_len),
      [self](const boost::system::error_code& ec,
             std::size_t bytes_transferred) {
        if (ec ||
            bytes_transferred !=
                static_cast<std::size_t>(self->_recv_msg_node->_total_len)) {
          std::cout << "Read body failed, error: " << ec.message()
                    << std::endl;
          self->_server->ClearSession(self->_uuid);
          return;
        }

        self->_recv_msg_node->_cur_len =
            static_cast<short>(bytes_transferred);
        self->_recv_msg_node->_data[self->_recv_msg_node->_total_len] = '\0';

        LogicSystem::GetInstance()->PostMsgToQueue(
            std::make_shared<LogicNode>(self, self->_recv_msg_node));

        self->DoReadHeader();
      });
}

void Session::HandleWrite(const boost::system::error_code& error) {
  if (!error) {
    _send_que.pop();
    if (!_send_que.empty()) {
      DoSend();
    }
  } else {
    std::cerr << "Write failed, error: " << error.message() << std::endl;
    _server->ClearSession(_uuid);
  }
}
