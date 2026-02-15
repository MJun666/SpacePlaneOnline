#include "NetworkClient.h"
#include <iostream>

bool NetworkClient::Connect(std::string ip, int port) {
    try {
        tcp::resolver resolver(io_context_);
        boost::asio::connect(socket_, resolver.resolve(ip, std::to_string(port)));

        // === [新增] 阻塞接收第一条消息 (ID) ===
		char data[1024];
		boost::system::error_code error;
		size_t len = socket_.read_some(boost::asio::buffer(data), error);

        game::LoginResponse login;
        if (!error && login.ParseFromArray(data, len))
        {
            my_player_id_ = login.your_id();
            std::cout << "[Network] Logged in! My ID: " << my_player_id_ << std::endl;
        }
        // ======================================

        // 连接成功后，启动接收线程
        receive_thread_ = std::thread(&NetworkClient::ReceiveThread, this);
        receive_thread_.detach(); // 让它在后台跑
        return true;
    }
    catch (...) {
        return false;
    }
}

void NetworkClient::SendInput(game::PlayerInput::InputType key) {
    if (!socket_.is_open()) return;

    game::PlayerInput input;
    input.set_input(key);

    std::string data;
    input.SerializeToString(&data);

    // 异步发送，防止卡顿
    boost::asio::write(socket_, boost::asio::buffer(data));
}

void NetworkClient::ReceiveThread() {
    try {
        while (true) {
            char data[4096];
            boost::system::error_code error;
            size_t len = socket_.read_some(boost::asio::buffer(data), error);
            if (error) break;

            game::GameSnapshot snapshot;
            if (snapshot.ParseFromArray(data, len)) {
                // 加锁更新数据
                std::lock_guard<std::mutex> lock(data_mutex_);
                current_snapshot_ = snapshot;
            }
        }
    }
    catch (...) {}
}

game::GameSnapshot NetworkClient::GetState() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return current_snapshot_;
}