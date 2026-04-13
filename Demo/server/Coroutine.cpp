#include <iostream>
#include <csignal>
#include <thread>
#include <mutex>
#include "CServer.h"
#include "AsioIOServicePool.h"

int main()
{
    try 
    {
        auto& pool = AsioIOServicePool::GetInstance();
        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context,SIGINT,SIGTERM);
        signals.async_wait([&io_context,&pool](auto,auto){
            io_context.stop();
            pool.Stop();
        });
        CServer s(io_context,10086);
        std::cout << "server start " << std::endl;
        io_context.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}