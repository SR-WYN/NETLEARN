#pragma once
#include "Singleton.hpp"
#include <queue>
#include <functional>
#include <thread>
#include <condition_variable>
#include <map>

class Session;
class LogicNode;

typedef function<void(std::shared_ptr<Session>, const short &msg_id, const string &msg_data)> FunCallBack;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    void PostMsgToQueue(shared_ptr<LogicNode> msg);
private:
    LogicSystem();
    void RegisterCallBacks();
    void HelloWorldCallBack(std::shared_ptr<Session>, const short &msg_id, const string &msg_data);
    void DealMsg();
    std::queue<shared_ptr<LogicNode>> _msg_que;
    std::mutex _mutex;
    std::condition_variable _consume;
    std::thread _worker_thread;
    bool _b_stop;
    std::map<short,FunCallBack> _fun_callback;
};