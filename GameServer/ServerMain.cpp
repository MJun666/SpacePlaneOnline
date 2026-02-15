#include<iostream>
#include<map>
#include <vector>
#include<thread>
#include<mutex>
#include<chrono>
#include<boost/asio.hpp>
#include"game.pb.h"

using boost::asio::ip::tcp;

// --- 游戏数据 ---
struct Player
{
	int id;
	float x = 400;
	float y = 600;//初始位置
	std::shared_ptr<tcp::socket> socket;// 每个玩家对应一个连接
};

// 全局变量
std::mutex g_mutex;
std::map<int, std::shared_ptr<Player>> g_players;
int g_next_id = 1;

// --- 处理单个客户端连接 ---
void session(std::shared_ptr<tcp::socket> socket)
{
	int my_id = 0;
	try {
		// 1. 分配 ID 并加入游戏
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			my_id = g_next_id++;
			auto player = std::make_shared<Player>();
			player->id = my_id;
			player->socket = socket;
			g_players[my_id] = player;

		}
		std::cout << "[Server] Player " << my_id << " joined!" << std::endl;
		// === [新增] 发送登录响应 ===
		game::LoginResponse login_msg;
		login_msg.set_your_id(my_id);
		login_msg.set_init_x(400.0f); // 默认位置
		login_msg.set_init_y(300.0f);

		std::string data;
		login_msg.SerializeToString(&data);

		// 发送 ID 给客户端
		boost::asio::write(*socket, boost::asio::buffer(data));
		std::cout << "[Server] Sent ID " << my_id << " to client." << std::endl;
		// ============================

		// 2. 循环接收数据
		while (true)
		{
			// 先读包头 (Protobuf 没有固定的结束符，这里我们简化：假设客户端发定长或者直接读)
			// *注意*：为了快速演示，我们这里简化网络处理，假设每次 read 都能读到一个完整的 Input 包
			// 实际工程中需要处理粘包，但今天先跑通逻辑。
			char data[1024];
			boost::system::error_code error;
			size_t length = socket->read_some(boost::asio::buffer(data), error);
			if (error == boost::asio::error::eof) break;
			else if (error)
			{
				throw boost::system::system_error(error);
			}
			// 解析数据
			game::PlayerInput input;
			if (input.ParseFromArray(data, length))
			{
				// 更新坐标 (加锁!)
				std::lock_guard<std::mutex> lock(g_mutex);
				auto& p = g_players[my_id];
				float speed = 5.0f;

				if (input.input() == game::PlayerInput_InputType_UP) p->y -= speed;
				if (input.input() == game::PlayerInput_InputType_DOWN) p->y += speed;
				if (input.input() == game::PlayerInput_InputType_RIGHT) p->x += speed;
				if (input.input() == game::PlayerInput_InputType_LEFT) p->x -= speed;

				// 简单的边界检查
				
				if (p->x < 0) p->x = 0; if (p->x > 800) p->x = 800;
				if (p->y < 0) p->y = 0; if (p->y > 600) p->y = 600;
			}
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "[Server] Player " << my_id << " error: " << e.what() << std::endl;
	}

	// 3. 断开连接，移除玩家
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		g_players.erase(my_id);
	}
	std::cout << "[Server] Player " << my_id << " disconnected." << std::endl;
}


// --- 广播线程 (心脏) ---
// 每隔 33ms (约30帧) 向所有客户端发送一次当前世界状态
void broadcast_loop()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(33));
		// 1. 打包当前状态
		game::GameSnapshot snapshot;
		std::vector<std::shared_ptr<tcp::socket>> sockets_to_send;
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			for (auto& pair : g_players)
			{
				auto p_data = snapshot.add_players();
				p_data->set_id(pair.second->id);
				p_data->set_x(pair.second->x);
				p_data->set_y(pair.second->y);
				sockets_to_send.push_back(pair.second->socket);
			}
		}
		// 2. 序列化
		std::string data;
		snapshot.SerializeToString(&data);
		// 3. 发送 (简单的群发)
		for (auto& sock : sockets_to_send)
		{
			try {
				boost::asio::write(*sock, boost::asio::buffer(data));

			}
			catch (...) {
				// 发送失败忽略，session 线程会处理断开
			}
		}
	}
}

int main()
{
	try {
		boost::asio::io_context io_context;
		tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8888));
		std::cout << "[Server] Game Server Started on 8888..." << std::endl;

		// 启动广播线程
		std::thread broadcaster(broadcast_loop);
		broadcaster.detach();

		// 接收连接循环
		while (true)
		{
			auto socket = std::make_shared<tcp::socket>(io_context);
			acceptor.accept(*socket);
			// 为每个客户端启动一个新线程处理接收
			std::thread(session, socket).detach();
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception :" << e.what() << std::endl;
	}
}