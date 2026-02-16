#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <boost/asio.hpp>
#include "game.pb.h"

using boost::asio::ip::tcp;

// ==========================================
// 1. 基于你 Object.h 和 Game.h 的物理常量
// ==========================================
const int WINDOW_WIDTH = 600;  // Game.h: windowWidth=600
const int WINDOW_HEIGHT = 800; // Game.h: windowHeight=800

// 基于 Object.h 的数值
const int PLAYER_SHOOT_COOLDOWN = 300; // Object.h: Cooldown=300
const int PLAYER_BULLET_SPEED = 600;   // Object.h: ProjectilePlayer speed=600
const int SERVER_TICK_MS = 33;         // 服务器每帧耗时

// 实际尺寸 (从客户端 [DEBUG] 输出获取)
// Player width: 48, height: 37
// Bullet width: 20, height: 31
const int PLAYER_WIDTH_ESTIMATE = 48;
const int PLAYER_HEIGHT_ESTIMATE = 37;
const int BULLET_WIDTH_ESTIMATE = 20;
const int BULLET_HEIGHT_ESTIMATE = 31;
// ==========================================

struct Player
{
    int id;
    float x = WINDOW_WIDTH / 2.0f - PLAYER_WIDTH_ESTIMATE / 2.0f; // 居中
    float y = WINDOW_HEIGHT - 100.0f; // 靠下
    std::shared_ptr<tcp::socket> socket;

    // 射击计时器
    std::chrono::steady_clock::time_point last_shoot_time;
};

struct Bullet {
    int id;
    float x, y;
    int type; // 0=玩家, 1=敌人
};

// 全局变量
std::mutex g_mutex;
std::map<int, std::shared_ptr<Player>> g_players;
std::vector<Bullet> g_bullets;
int g_next_id = 1;
int g_bullet_next_id = 1;

// --- 1. 处理客户端连接 ---
void session(std::shared_ptr<tcp::socket> socket)
{
    int my_id = 0;
    try {
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            my_id = g_next_id++;
            auto player = std::make_shared<Player>();
            player->id = my_id;
            player->socket = socket;
            g_players[my_id] = player;
        }
        std::cout << "[Server] Player " << my_id << " joined!" << std::endl;

        // 发送登录响应
        game::LoginResponse login_msg;
        login_msg.set_your_id(my_id);
        login_msg.set_init_x(g_players[my_id]->x);
        login_msg.set_init_y(g_players[my_id]->y);

        std::string data;
        login_msg.SerializeToString(&data);
        boost::asio::write(*socket, boost::asio::buffer(data));

        while (true)
        {
            char data[1024];
            boost::system::error_code error;
            size_t length = socket->read_some(boost::asio::buffer(data), error);
            if (error) break;

            game::PlayerInput input;
            if (input.ParseFromArray(data, length))
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                if (g_players.find(my_id) == g_players.end()) break;

                auto& p = g_players[my_id];
                // Object.h 里的 player speed 是 300，服务器每帧跑 33ms
                // 300 * 0.033 ≈ 10 像素/帧
                float move_speed = 300.0f * (SERVER_TICK_MS / 1000.0f);

                if (input.input() == game::PlayerInput_InputType_UP)    p->y -= move_speed;
                if (input.input() == game::PlayerInput_InputType_DOWN)  p->y += move_speed;
                if (input.input() == game::PlayerInput_InputType_LEFT)  p->x -= move_speed;
                if (input.input() == game::PlayerInput_InputType_RIGHT) p->x += move_speed;

                // 修正后的边界检查 (600x800)
                if (p->x < 0) p->x = 0;
                if (p->x > WINDOW_WIDTH - PLAYER_WIDTH_ESTIMATE) p->x = WINDOW_WIDTH - PLAYER_WIDTH_ESTIMATE;
                if (p->y < 0) p->y = 0;
                if (p->y > WINDOW_HEIGHT - PLAYER_WIDTH_ESTIMATE) p->y = WINDOW_HEIGHT - PLAYER_WIDTH_ESTIMATE;
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << "[Server] Player " << my_id << " error: " << e.what() << std::endl;
    }

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_players.erase(my_id);
    }
}

// --- 2. 广播循环 (物理计算核心) ---
void broadcast_loop()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(SERVER_TICK_MS));

        // A. 自动射击 & 子弹移动
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto now = std::chrono::steady_clock::now();

            // 1. 自动射击
            for (auto& pair : g_players)
            {
                auto player = pair.second;
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - player->last_shoot_time).count();

                // 使用 Object.h 里的 Cooldown = 300
                if (duration > PLAYER_SHOOT_COOLDOWN)
                {
                    Bullet b;
                    b.id = g_bullet_next_id++;
                    // 【修正】使用与客户端完全一致的居中公式
                    // 客户端: projectile->position.x = player.position.x + player.width/2 - projectile->width/2
                    // 注意：这里计算的是子弹左上角的坐标
                    b.x = player->x + (PLAYER_WIDTH_ESTIMATE / 2) - (BULLET_WIDTH_ESTIMATE / 2);
                    b.y = player->y;
                    b.type = 0;
                    g_bullets.push_back(b);

                    player->last_shoot_time = now;
                }
            }

            // 2. 更新子弹位置
            // 每帧移动距离 = 速度(600) * 时间(0.033) ≈ 20
            float bullet_step = PLAYER_BULLET_SPEED * (SERVER_TICK_MS / 1000.0f);

            for (auto it = g_bullets.begin(); it != g_bullets.end(); )
            {
                if (it->type == 0) it->y -= bullet_step; // 向上
                else it->y += (bullet_step / 2);         // 敌人子弹(假设慢点)

                if (it->y < -50 || it->y > WINDOW_HEIGHT + 50) {
                    it = g_bullets.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        // B. 打包发送
        game::GameSnapshot snapshot;
        std::vector<std::shared_ptr<tcp::socket>> sockets_to_send;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            for (auto& pair : g_players) {
                auto p_data = snapshot.add_players();
                p_data->set_id(pair.second->id);
                p_data->set_x(pair.second->x);
                p_data->set_y(pair.second->y);
                sockets_to_send.push_back(pair.second->socket);
            }
            for (const auto& b : g_bullets) {
                auto b_data = snapshot.add_bullets();
                b_data->set_id(b.id);
                b_data->set_x(b.x);
                b_data->set_y(b.y);
                b_data->set_type(b.type);
            }
        }

        std::string data;
        snapshot.SerializeToString(&data);
        for (auto& sock : sockets_to_send) {
            try { boost::asio::write(*sock, boost::asio::buffer(data)); }
            catch (...) {}
        }
    }
}

int main()
{
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8888));
        std::cout << "[Server] Started on 8888. World: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << std::endl;
        std::cout << "[Config] Cooldown: " << PLAYER_SHOOT_COOLDOWN << "ms, Bullet Speed: " << PLAYER_BULLET_SPEED << std::endl;
        std::cout << "[Config] Player Size: " << PLAYER_WIDTH_ESTIMATE << "x" << PLAYER_HEIGHT_ESTIMATE << std::endl;
        std::cout << "[Config] Bullet Size: " << BULLET_WIDTH_ESTIMATE << "x" << BULLET_HEIGHT_ESTIMATE << std::endl;
      
        std::thread broadcaster(broadcast_loop);
        broadcaster.detach();

        while (true)
        {
            auto socket = std::make_shared<tcp::socket>(io_context);
            acceptor.accept(*socket);
            std::thread(session, socket).detach();
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}