#include "CServer.h"
#include "CSession.h"
#include "AsioIOServicePool.h"
#include <iostream>


using namespace std;

CServer::CServer(boost::asio::io_context &io_context, short port)
    : _io_context(io_context), _port(port),
      _acceptor(io_context, tcp::endpoint(tcp::v4(), port))
{
    StartAccept();
}

CServer::~CServer()
{
}

void CServer::HandleAccept(shared_ptr<CSession> new_session,const boost::system::error_code& error)
{
    if(!error)
    {
        new_session->Start();
        lock_guard<mutex> lock(_mutex);
        _sessions.insert(make_pair(new_session->GetUuid(),new_session));
    }
    else 
    {
        std::cout << "accept error:" << error.what() << std::endl;
    }

    StartAccept();
}

void CServer::StartAccept()
{
    auto& io_context = AsioIOServicePool::GetInstance().GetIOService();
    shared_ptr<CSession> new_session = make_shared<CSession>(io_context,this);
    _acceptor.async_accept(new_session->GetSocket(),[this,new_session](const boost::system::error_code& error){
        this->HandleAccept(new_session,error);
    });
}

void CServer::ClearSession(std::string uuid)
{
    lock_guard<mutex> lock(_mutex);
    _sessions.erase(uuid);
}
