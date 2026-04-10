#pragma once
#include <memory>

class Session;
class RecvNode;

class LogicNode
{
    friend class LogicSystem;
public:
    LogicNode(std::shared_ptr<Session>,std::shared_ptr<RecvNode>);
private:
    std::shared_ptr<Session> _session;
    std::shared_ptr<RecvNode> _recvnode;
};