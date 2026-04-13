#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <queue>

class CServer;
class SendNode;
class RecvNode;
class MsgNode;

class CSession : public std::enable_shared_from_this<CSession>
{
  public:
    CSession(boost::asio::io_context &io_context, CServer *server);
    ~CSession();
    boost::asio::ip::tcp::socket &GetSocket();
    std::string &GetUuid();
    void Start();
    void Close();
    void Send(const char *msg, short max_length, short msgid);
    void Send(std::string msg, short msgid);

  private:
    void HandleWrite(const boost::system::error_code &error,
                     std::shared_ptr<CSession> shared_self);
    boost::asio::io_context &_io_context;
    CServer *_server;
    boost::asio::ip::tcp::socket _socket;
    std::string _uuid;
    bool _b_close;
    std::mutex _send_lock;
    std::queue<std::shared_ptr<SendNode>> _send_que;
    std::shared_ptr<RecvNode> _recv_msg_node;
    std::shared_ptr<MsgNode> _recv_head_node;
};

class LogicNode
{
  public:
    LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);
    std::shared_ptr<CSession> _session;
    std::shared_ptr<RecvNode> _recvnode;
};