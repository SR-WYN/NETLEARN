#include "CSession.h"
#include "CServer.h"
#include "MsgNode.h"
#include "const.h"
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "LogicSystem.h"

using namespace std;

CSession::CSession(boost::asio::io_context &io_context, CServer *server)
    : _io_context(io_context), _server(server), _socket(io_context),
      _b_close(false)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
    _recv_head_node = make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

boost::asio::ip::tcp::socket &CSession::GetSocket()
{
    return _socket;
}

std::string &CSession::GetUuid()
{
    return _uuid;
}

void CSession::Start()
{
    auto shared_this = shared_from_this();
    // 开启协程接受
    boost::asio::co_spawn(
        _io_context,
        [=, this]() -> boost::asio::awaitable<void>
        {
            try
            {
                while (!_b_close)
                {
                    _recv_head_node->Clear();
                    std::size_t n = co_await boost::asio::async_read(
                        _socket,
                        boost::asio::buffer(_recv_head_node->GetData(),
                                            HEAD_TOTAL_LEN),
                        boost::asio::use_awaitable);
                    if (n == 0)
                    {
                        std::cout << "receive peer closed" << std::endl;
                        Close();
                        _server->ClearSession(_uuid);
                        co_return;
                    }

                    // 获取头部MSGID数据
                    short msg_id = 0;
                    memcpy(&msg_id, _recv_head_node->GetData(), HEAD_ID_LEN);
                    // 网络字节序转为本地
                    msg_id =
                        boost::asio::detail::socket_ops::network_to_host_short(
                            msg_id);
                    std::cout << "msg_id is " << msg_id << std::endl;
                    if (msg_id > MAX_LENGTH)
                    {
                        std::cout << "invaild msg id is " << msg_id
                                  << std::endl;
                        Close();
                        _server->ClearSession(_uuid);
                        co_return;
                    }

                    short msg_len = 0;
                    memcpy(&msg_len, _recv_head_node->GetData() + HEAD_ID_LEN,
                           HEAD_DATA_LEN);
                    msg_len =
                        boost::asio::detail::socket_ops::network_to_host_short(
                            msg_len);
                    std::cout << "msg_len is " << msg_len << std::endl;
                    if (msg_len > MAX_LENGTH)
                    {
                        std::cout << "invaild msg len is " << msg_len
                                  << std::endl;
                        Close();
                        _server->ClearSession(_uuid);
                        co_return;
                    }

                    _recv_msg_node = make_shared<RecvNode>(msg_len, msg_id);

                    // 读出包体
                    n = co_await boost::asio::async_read(
                        _socket,
                        boost::asio::buffer(_recv_msg_node->GetData(),
                                            _recv_msg_node->GetTotalLen()),
                        boost::asio::use_awaitable);
                    if (n == 0)
                    {
                        std::cout << "receive peer closed " << std::endl;
                        Close();
                        _server->ClearSession(_uuid);
                        co_return;
                    }
                    std::cout << "receive data is " << _recv_msg_node->GetData()
                              << std::endl;
                    // 投递逻辑线程处理
                    LogicSystem::GetInstance().PostMsgToQue(make_shared<LogicNode>(shared_this, _recv_msg_node));
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Exception: " << e.what() << std::endl;
                Close();
                _server->ClearSession(_uuid);
            }
        },
        boost::asio::detached);
}

void CSession::Close()
{
    _socket.close();
    _b_close = true;
}

CSession::~CSession()
{
    try
    {
        std::cout << "~CSession destruct " << std::endl;
        Close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

void CSession::Send(const char *msg, short max_length, short msgid)
{
    std::unique_lock<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE)
    {
        std::cout << "session : " << _uuid << "send que fulled,size is "
                  << MAX_SENDQUE << std::endl;
        return;
    }
    _send_que.push(make_shared<SendNode>(msg, max_length, msgid));
    if (send_que_size > 0)
    {
        return;
    }
    auto msgnode = _send_que.front();
    lock.unlock();
    auto self = shared_from_this();
    boost::asio::async_write(
        _socket,
        boost::asio::buffer(msgnode->GetData(), msgnode->GetTotalLen()),
        [this, self](const boost::system::error_code &error,
                     std::size_t bytes_transferred)
        { HandleWrite(error, self); });
}

void CSession::Send(std::string msg, short msgid)
{
    Send(msg.c_str(), msg.size(), msgid);
}

void CSession::HandleWrite(const boost::system::error_code &error,
                           std::shared_ptr<CSession> shared_self)
{
    try
    {
        if (!error)
        {
            std::unique_lock<std::mutex> lock(_send_lock);
            _send_que.pop();
            if (!_send_que.empty())
            {
                auto msgnode = _send_que.front();
                lock.unlock();
                boost::asio::async_write(
                    _socket,
                    boost::asio::buffer(msgnode->GetData(),
                                        msgnode->GetTotalLen()),
                    [this, shared_self](const boost::system::error_code &error,
                                        std::size_t bytes_transferred)
                    { HandleWrite(error, shared_self); });
            }
        }
        else
        {
            std::cout << "handle write failed,error is " << error.what()
                      << std::endl;
            Close();
            _server->ClearSession(_uuid);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception is: " << e.what() << std::endl;
        Close();
        _server->ClearSession(_uuid);
    }
}

LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode):_session(session),_recvnode(recvnode)
{

}
