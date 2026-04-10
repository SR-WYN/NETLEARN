#include "LogicNode.hpp"
#include "Session.hpp"
#include "MsgNode.hpp"

LogicNode::LogicNode(std::shared_ptr<Session> session,std::shared_ptr<RecvNode> recvnode):_session(session),_recvnode(recvnode)
{

}