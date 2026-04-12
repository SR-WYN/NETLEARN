#include "Server.hpp"
#include "Session.hpp"
#include <iostream>
#include <csignal>
#include "AsioIOServicePool.hpp"
#include "AsioThreadPool.hpp"
using namespace std;

bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

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
    // auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
    shared_ptr<Session> new_session = make_shared<Session>(_ioc, this);
    _acceptor.async_accept(new_session->Socket(),
                           [this, new_session](const boost::system::error_code &error)
                           {
                               this->handle_accept(new_session, error);
                           });
}

void Server::handle_accept(shared_ptr<Session> new_session, const boost::system::error_code &error)
{
    if (!error)
    {
        new_session->Start();
        _sessions.insert(make_pair(new_session->GetUuid(), new_session));
    }
    else
    {

        std::cout << "Accept error: " << error.message() << std::endl;
    }
    start_accept();
}

int main()
{
    try
    {
        // auto pool = AsioIOServicePool::GetInstance();
        auto pool = AsioThreadPool::GetInstance();
        boost::asio::io_context ioc;
        boost::asio::signal_set signals(ioc,SIGINT,SIGTERM);
        signals.async_wait([&ioc,pool](auto ec,auto sig_num){
            if (ec)
            {
                cerr << "signal wait failed, error is " << ec.message() << endl;
                return;
            }
            cout << "signal " << sig_num << " received, stopping server..." << endl;
            ioc.stop();
            pool->Stop();
            std::unique_lock<std::mutex> lock(mutex_quit);
            bstop = true;
            cond_quit.notify_one();
        });
        Server s(pool->GetIOService(),10086);
        {
            std::unique_lock<std::mutex> lock(mutex_quit);
            while (!bstop)
            {
                cond_quit.wait(lock);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
    }
    return 0;
}