#include "LogicSystem.h"
#include "MsgNode.h"
#include "const.h"
#include "json/reader.h"

LogicSystem::LogicSystem() : _b_stop(false)
{
    RegisterCallBacks();
    _worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem()
{
    _b_stop = true;
    _consume.notify_one();
    _worker_thread.join();
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg)
{
    std::unique_lock<std::mutex> unique_lk(_mutex);
    _msg_que.push(msg);
    if (_msg_que.size() == 1)
    {
        _consume.notify_one();
    }
}

void LogicSystem::DealMsg()
{
    while (true)
    {
        std::unique_lock<std::mutex> unique_lk(_mutex);
        while (_msg_que.empty() && !_b_stop)
        {
            _consume.wait(unique_lk);
        }
        if (_b_stop)
        {
            while (!_msg_que.empty())
            {
                auto msg_node = _msg_que.front();
                std::cout << "recv msg id is "
                          << msg_node->_recvnode->GetMsgId() << std::endl;
                auto call_back_iter =
                    _fun_callbacks.find(msg_node->_recvnode->GetMsgId());
                if (call_back_iter == _fun_callbacks.end())
                {
                    _msg_que.pop();
                    continue;
                }
                call_back_iter->second(
                    msg_node->_session, msg_node->_recvnode->GetMsgId(),
                    std::string(msg_node->_recvnode->GetData(),
                                msg_node->_recvnode->GetTotalLen()));

                _msg_que.pop();
            }
            break;
        }
        // 没有停服且队列中有数据
        auto msg_node = _msg_que.front();
        std::cout << "recv_msg_id is " << msg_node->_recvnode->GetMsgId()
                  << std::endl;
        auto call_back_iter =
            _fun_callbacks.find(msg_node->_recvnode->GetMsgId());
        if (call_back_iter == _fun_callbacks.end())
        {
            _msg_que.pop();
            continue;
        }
        call_back_iter->second(msg_node->_session,
                               msg_node->_recvnode->GetMsgId(),
                               std::string(msg_node->_recvnode->GetData(),
                                           msg_node->_recvnode->GetTotalLen()));

        _msg_que.pop();
    }
}

void LogicSystem::RegisterCallBacks()
{
    _fun_callbacks[MSG_HELLO_WORLD] = [this](std::shared_ptr<CSession> session,
                                             const short &msg_id,
                                             const std::string &msg_data)
    { HelloWorldCallBack(session, msg_id, msg_data); };
}

void LogicSystem::HelloWorldCallBack(std::shared_ptr<CSession> session,
                                     const short &msg_id,
                                     const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    std::cout << "receive msg id is " << root["id"].asInt() << "msg data is"
              << root["data"].asString() << std::endl;
    root["data"] = "server has received msg,msg data is " + root["data"].asString();
    std::string return_str = root.toStyledString();
    session->Send(return_str,root["id"].asInt());
}

LogicSystem& LogicSystem::GetInstance()
{
    static LogicSystem instance;
    return instance;
}
