#include "AsioThreadPool.hpp"

AsioThreadPool::~AsioThreadPool()
{
}

boost::asio::io_context &AsioThreadPool::GetIOService()
{
    return _service;
}

void AsioThreadPool::Stop()
{
    _work.reset();
    for (auto &t : _threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

AsioThreadPool::AsioThreadPool(int threadNum) : _work(new boost::asio::io_context::work(_service))
{
    for (int i = 0; i < threadNum; i++)
    {
        _threads.emplace_back([this]()
                              { this->_service.run(); });
    }
}
