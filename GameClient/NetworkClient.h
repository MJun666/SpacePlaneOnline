#pragma once
#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include "game.pb.h" // 引用协议

using boost::asio::ip::tcp;

class NetworkClient {
public:
    static NetworkClient& GetInstance() {
        static NetworkClient instance;
        return instance;
    }

    // 连接服务器
    bool Connect(std::string ip, int port);

    // 发送按键输入
    void SendInput(game::PlayerInput::InputType key);

    // 获取最新的游戏状态 (线程安全)
    game::GameSnapshot GetState();

private:
    NetworkClient() : socket_(io_context_) {}

    // 后台接收线程函数
    void ReceiveThread();

    boost::asio::io_context io_context_;
    tcp::socket socket_;
    std::thread receive_thread_;

    // 数据保护
    std::mutex data_mutex_;
    game::GameSnapshot current_snapshot_;
};