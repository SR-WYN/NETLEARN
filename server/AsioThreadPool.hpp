#pragma once
#include "Singleton.hpp"
#include <boost/asio.hpp>

class AsioThreadPool:public Singleton<AsioThreadPool>
{
public:
    friend class Singleton<AsioThreadPool>;
    ~AsioThreadPool();
    AsioThreadPool(const AsioThreadPool&) = delete;
    AsioThreadPool& operator = (const AsioThreadPool&) = delete;
    boost::asio::io_context& GetIOService();
    void Stop();
private:
    AsioThreadPool(int threadNum = std::thread::hardware_concurrency());
    boost::asio::io_context _service;
    std::unique_ptr<boost::asio::io_context::work> _work;
    std::vector<std::thread> _threads;
};