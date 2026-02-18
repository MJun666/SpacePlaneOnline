#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include<random>
#include<cmath>
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



const int PLAYER_WIDTH_ESTIMATE = 48;
const int PLAYER_HEIGHT_ESTIMATE = 37;
const int BULLET_WIDTH_ESTIMATE = 20;
const int BULLET_HEIGHT_ESTIMATE = 31;

// 敌机参数
const int ENEMY_WIDTH = 64;   
const int ENEMY_HEIGHT = 64;
const int ENEMY_SPEED = 200; 
const int ENEMY_SPAWN_RATE = 60; 
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
    float vx, vy;
    int type; // 0=玩家, 1=敌人
};

struct Enemy
{
    int id; 
	float x, y;
    std::chrono::steady_clock::time_point last_shoot_time;

};

// 全局变量
std::mutex g_mutex;
std::map<int, std::shared_ptr<Player>> g_players;
std::vector<Enemy> g_enemies;
std::vector<Bullet> g_bullets;

int g_next_id = 1;
int g_bullet_next_id = 1;
int g_enemy_next_id = 1;

// 随机数
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float>  dis(0.0f,1.0f);  


// --- 辅助：寻找最近的玩家 ---
std::shared_ptr<Player>GetNearestPlayer(float ex, float ey)
{
	std::shared_ptr<Player> target = nullptr;
    float min_dist = 1000000.0f;
    for (auto& pair : g_players)
    {
		float dx = pair.second->x - ex;
		float dy = pair.second->y - ey;
		float dist = dx * dx + dy * dy; // 距离的平方
        if (min_dist > dist)
        {
            min_dist = dist;
			target = pair.second;
        }
    }

    return target;
}


// ---  处理客户端连接 ---
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

// ---  广播循环 (物理计算核心) ---
void broadcast_loop()
{
	int frame_count = 0;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(SERVER_TICK_MS));
        float dt = SERVER_TICK_MS / 1000.0f;
        frame_count++;
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
                    b.x = player->x + (PLAYER_WIDTH_ESTIMATE - BULLET_WIDTH_ESTIMATE) / 2.0f;
                    b.y = player->y;
                    b.vx = 0;
                    b.vy = -PLAYER_BULLET_SPEED; // 向上
                    b.type = 0;
                    g_bullets.push_back(b);
                    player->last_shoot_time = now;
                }
            }

            // 2. [新增] 敌机生成 (每隔一段时间生成一个)
            if (frame_count % ENEMY_SPAWN_RATE == 0)
            {
                Enemy e;
				e.id = g_enemy_next_id++;
                e.x=dis(gen) *(WINDOW_WIDTH - ENEMY_WIDTH); // 随机生成在窗口内
                e.y = -ENEMY_HEIGHT; // 从顶部出现
                e.last_shoot_time = now;
				g_enemies.push_back(e);

            }

            // 3. [新增] 敌机逻辑 (移动 + 开火)
            for (auto it = g_enemies.begin(); it != g_enemies.end();)
            {
                // A. 移动
                it->y += ENEMY_SPEED * dt;
                // B. 开火 (瞄准玩家)
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->last_shoot_time).count();
                if (duration > 2000)
                {
                    auto target = GetNearestPlayer(it->x, it->y);
                    if (target)
                    {
                        Bullet b;
						b.id = g_bullet_next_id++;
						b.type = 1; // 敌人子弹
						b.x = it->x + (ENEMY_WIDTH / 2) - (BULLET_WIDTH_ESTIMATE / 2); // 从敌机中心发出
						b.y = it->y + ENEMY_HEIGHT; // 从敌机底部发出

                        // 计算瞄准向量
						float dx = (target->x + PLAYER_WIDTH_ESTIMATE / 2) - b.x;
						float dy = (target->y + PLAYER_HEIGHT_ESTIMATE / 2) - b.y;
						float len = std::sqrt(dx * dx + dy * dy);
                        if (len > 0) {
                            b.vx = (dx / len) * 300.0f; // 敌机子弹速度 300
                            b.vy = (dy / len) * 300.0f;
                        }
                        else {
                            b.vx = 0; b.vy = 300.0f;
                        }
                        g_bullets.push_back(b);
                        it->last_shoot_time = now;

                    }
                }
                if(it->y>WINDOW_HEIGHT) it = g_enemies.erase(it); // 超出底部则销毁
				else ++it;
            }

            // 4. 子弹移动 (支持斜着飞)
            for (auto it = g_bullets.begin(); it != g_bullets.end(); ) {
                if (it->vx == 0 && it->vy == 0) {
                    // 兼容旧代码，如果没有vx/vy
                    if (it->type == 0) it->y -= PLAYER_BULLET_SPEED * dt;
                    else it->y += 300.0f * dt;
                }
                else {
                    it->x += it->vx * dt;
                    it->y += it->vy * dt;
                }

                if (it->y < -50 || it->y > WINDOW_HEIGHT + 50 || it->x < -50 || it->x > WINDOW_WIDTH + 50) {
                    it = g_bullets.erase(it);
                }
                else ++it;
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

            // Enemies
            for (const auto& e : g_enemies)
            {
				auto ed = snapshot.add_enemies();
				ed->set_id(e.id);
				ed->set_x(e.x);
				ed->set_y(e.y);
                ed->set_type(0);
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