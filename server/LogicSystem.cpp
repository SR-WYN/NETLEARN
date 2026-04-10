#include "Session.hpp"
#include "LogicNode.hpp"
#include "const.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "LogicSystem.hpp"

LogicSystem::~LogicSystem()
{
    _b_stop = true;
    _consume.notify_one();
    _worker_thread.join();
}

void LogicSystem::PostMsgToQueue(shared_ptr<LogicNode> msg)
{
    std::unique_lock<std::mutex> unique_lk(_mutex);
    _msg_que.push(msg);

    if (_msg_que.size() == 1)
    {
        _consume.notify_one();
    }
}

LogicSystem::LogicSystem() : _b_stop(false)
{
    RegisterCallBacks();
    _worker_thread = std::thread([this]()
                                 { this->DealMsg(); });
}

void LogicSystem::RegisterCallBacks()
{
    _fun_callback[MSG_HELLO_WORD] = [this](std::shared_ptr<Session> session, const short &msg_id, const string &msg_data)
    {
        this->HelloWorldCallBack(session, msg_id, msg_data);
    };
}

void LogicSystem::HelloWorldCallBack(std::shared_ptr<Session> session, const short &msg_id, const string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    std::cout << "receive msg id is " << root["id"].asInt() << "msg data is "
              << root["data"].asString() << std::endl;
    root["data"] = "server has receive msg, msg data is " + root["data"].asString();
    std::string return_str = root.toStyledString();
    session->Send(return_str, root["id"].asInt());
}

void LogicSystem::DealMsg()
{
    for (;;)
    {
        std::unique_lock<std::mutex> unique_lk(_mutex);

        // 判断队列为空则用条件变量等待
        while (_msg_que.empty() && !_b_stop)
        {
            _consume.wait(unique_lk);
        }

        // 判断如果为关闭状态，取出逻辑队列所有数据及时处理并退出循环
        if (_b_stop)
        {
            while (!_msg_que.empty())
            {
                auto msg_node = _msg_que.front();
                cout << "recv msg id is " << msg_node->_recvnode->msgId() << std::endl;
                auto call_back_iter = _fun_callback.find(msg_node->_recvnode->msgId());
                if (call_back_iter == _fun_callback.end())
                {
                    _msg_que.pop();
                    continue;
                }
                call_back_iter->second(msg_node->_session,msg_node->_recvnode->msgId(),
                                    std::string(msg_node->_recvnode->_data,msg_node->_recvnode->_cur_len));
                _msg_que.pop();
            }
            break;
        }

        //如果没有停服，并且队列中有数据
        auto msg_node= _msg_que.front();
        std::cout << "recv msg id is " << msg_node->_recvnode->msgId() << std::endl;
        auto call_back_iter = _fun_callback.find(msg_node->_recvnode->msgId());
        if (call_back_iter == _fun_callback.end())
        {
            _msg_que.pop();
            continue;
        }

        call_back_iter->second(msg_node->_session,msg_node->_recvnode->msgId(),
                                    std::string(msg_node->_recvnode->_data,msg_node->_recvnode->_cur_len));
        _msg_que.pop();
    }
}
