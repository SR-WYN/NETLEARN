#include "Server.hpp"
#include "Session.hpp"
#include <iostream>
using namespace std;

Server::Server(boost::asio::io_context &ioc, short port) : _ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port)) 
{
    start_accept();
}

void Server::ClearSession(std::string uuid)
{
    _sessions.erase(uuid);
}

void Server::start_accept()
{
    shared_ptr<Session> new_session =  make_shared<Session>(_ioc,this);
    _acceptor.async_accept(new_session->Socket(), 
        [this, new_session](const boost::system::error_code& error) {
            this->handle_accept(new_session, error);
        }
    );
}

void Server::handle_accept(shared_ptr<Session> new_session, const boost::system::error_code& error) {
    if (!error) {
        new_session->Start();
        _sessions.insert(make_pair(new_session->GetUuid(),new_session));
    } else {

        std::cout << "Accept error: " << error.message() << std::endl;
    }
    start_accept();
}

int main()
{
    try
    {
        boost::asio::io_context ioc;
        Server s(ioc,10086);
        ioc.run();
    }
    catch(const std::exception& e)
    {
        std::cerr <<"Exception: " << e.what() << '\n';
    }
    return 0;
}