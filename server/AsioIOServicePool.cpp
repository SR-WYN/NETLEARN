#include "AsioIOServicePool.hpp"

AsioIOServicePool::~AsioIOServicePool()
{
    std::cout << "AsioIOServicePool destructor called" << std::endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService()
{
    auto& service = _ioServices[_nextIOService++ % _ioServices.size()];
    return service;
}

void AsioIOServicePool::Stop()
{
    for (auto& work:_works)
    {
        work.reset();
    }

    for (auto& t:_threads)
    {
        t.join();
    }

    std::cout << "AsioIOServicePool stopped" << std::endl;
}

AsioIOServicePool::AsioIOServicePool(std::size_t size):_ioServices(size),
    _works(size),_nextIOService(0)
{
    for (std::size_t i = 0;i < size; i++)
    {
        _works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));
    }
    //遍历多个ioservice,创建多个线程，每个线程内部启动ioservice
    for (std::size_t i = 0;i < _ioServices.size(); i++)
    {
        _threads.emplace_back([this,i](){
            _ioServices[i].run();
        });
    }
}
