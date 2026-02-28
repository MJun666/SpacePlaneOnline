#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include<random>
#include<cmath>
#include<algorithm>
#include <boost/asio.hpp>
#include <fstream>
#include "game.pb.h"

using boost::asio::ip::tcp;

const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 800;
const int PLAYER_SHOOT_COOLDOWN = 300;
const int PLAYER_BULLET_SPEED = 1000;
const int SERVER_TICK_MS = 33;
const int PLAYER_WIDTH_ESTIMATE = 48;
const int PLAYER_HEIGHT_ESTIMATE = 37;
const int BULLET_WIDTH_ESTIMATE = 20;
const int BULLET_HEIGHT_ESTIMATE = 31;
const int ENEMY_WIDTH = 64;
const int ENEMY_HEIGHT = 64;
const int ENEMY_SPEED = 200;
const int ENEMY_SPAWN_RATE = 60;
const int ITEM_WIDTH = 32;
const int ITEM_HEIGHT = 32;
const int ITEM_SPEED = 100;
struct Player
{
	int id;
	float x = WINDOW_WIDTH / 2.0f - PLAYER_WIDTH_ESTIMATE / 2.0f; // 居中
	float y = WINDOW_HEIGHT - 100.0f; // 靠下
	std::shared_ptr<tcp::socket> socket;

	std::chrono::steady_clock::time_point last_shoot_time;
	std::chrono::steady_clock::time_point game_start_time;
	int health = 5;
	int score = 0;
};

struct Bullet {
	int id;
	float x, y;
	float vx, vy;
	int type;
	float angle;
	bool active = true;
	int owned_id = 0;
};

struct Item {
	int id;
	float x, y;
	float vx, vy;
	int bounces_count = 3;
	int type = 0;
	bool active = true;
};

struct Enemy
{
	int id; 
	float x, y;
	std::chrono::steady_clock::time_point last_shoot_time;
	bool active = true;
	int health = 2;
};

struct LeaderboardEntryData {
	std::string name;
	int score;
};

// 全局变量
std::mutex g_mutex;
std::map<int, std::shared_ptr<Player>> g_players;
std::vector<Enemy> g_enemies;
std::vector<Bullet> g_bullets;
std::vector<Item> g_items;
std::vector<LeaderboardEntryData> g_leaderboard;

int g_next_id = 1;
int g_bullet_next_id = 1;
int g_enemy_next_id = 1;
int g_item_next_id = 1;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float>  dis(0.0f,1.0f);

bool CheckCollision(float x1, float y1, float w1, float h1,
	float x2, float y2, float w2, float h2)
{
	return (x1 < x2 + w2 && x1 + w1 > x2 &&
		y1 < y2 + h2 && y1 + h1 > y2);
}

const std::string SAVE_FILE = "leaderboard_save.dat";

void SaveLeaderboard() {
	game::LeaderboardSaveData save_data;
	for (const auto& entry : g_leaderboard)
	{
		auto e = save_data.add_entries();
		e->set_name(entry.name);
		e->set_score(entry.score);
	}

	std::ofstream out(SAVE_FILE, std::ios::binary);
	if (out) {
		save_data.SerializeToOstream(&out);
	}
}

void LoadLeaderboard() {
	std::ifstream in(SAVE_FILE, std::ios::binary);
	if (in) {
		game::LeaderboardSaveData save_data;
		if (save_data.ParseFromIstream(&in)) {
			g_leaderboard.clear();
			for (const auto& e : save_data.entries()) {
				g_leaderboard.push_back({ e.name(), e.score() });
			}
			std::cout << "[Server] Loaded " << g_leaderboard.size() << " records from save file." << std::endl;
		}
	}
	else {
		std::cout << "[Server] No existing save file found. Starting fresh." << std::endl;
	}
}

std::shared_ptr<Player>GetNearestPlayer(float ex, float ey)
{
	std::shared_ptr<Player> target = nullptr;
	float min_dist = 1000000.0f;
	for (auto& pair : g_players)
	{
		float dx = pair.second->x - ex;
		float dy = pair.second->y - ey;
		float dist = dx * dx + dy * dy;
		if (min_dist > dist)
		{
			min_dist = dist;
			target = pair.second;
		}
	}
	return target;
}


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
			player->game_start_time = std::chrono::steady_clock::now();
			g_players[my_id] = player;
		}
		std::cout << "[Server] Player " << my_id << " joined!" << std::endl;

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
				
				if (input.input() == game::PlayerInput_InputType_SUBMIT_SCORE)
				{
					LeaderboardEntryData entry;
					entry.name = input.name();
					entry.score = p->score;
					g_leaderboard.push_back(entry);

					std::sort(g_leaderboard.begin(), g_leaderboard.end(),
						[](const LeaderboardEntryData& a, const LeaderboardEntryData& b) {
							return a.score > b.score;
						});
					
					if (g_leaderboard.size() > 10) {
						g_leaderboard.resize(10);
					}
					
					SaveLeaderboard();
					p->score = 0;
					continue;
				}

				if (input.input() == game::PlayerInput_InputType_RESPAWN) {
					p->health = 5;
					p->score = 0;
					p->x = WINDOW_WIDTH / 2.0f - PLAYER_WIDTH_ESTIMATE / 2.0f;
					p->y = WINDOW_HEIGHT - 100.0f;
					p->game_start_time = std::chrono::steady_clock::now();
					continue;
				}
				
				if (p->health > 0)
				{
					float move_speed = 250.0f * (SERVER_TICK_MS / 1000.0f);

					if (input.input() == game::PlayerInput_InputType_UP)    p->y -= move_speed;
					if (input.input() == game::PlayerInput_InputType_DOWN)  p->y += move_speed;
					if (input.input() == game::PlayerInput_InputType_LEFT)  p->x -= move_speed;
					if (input.input() == game::PlayerInput_InputType_RIGHT) p->x += move_speed;

					if (p->x < 0) p->x = 0;
					if (p->x > WINDOW_WIDTH - PLAYER_WIDTH_ESTIMATE) p->x = WINDOW_WIDTH - PLAYER_WIDTH_ESTIMATE;
					if (p->y < 0) p->y = 0;
					if (p->y > WINDOW_HEIGHT - PLAYER_WIDTH_ESTIMATE) p->y = WINDOW_HEIGHT - PLAYER_WIDTH_ESTIMATE;
				}
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

void broadcast_loop()
{
	int spawn_counter = 0;
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(SERVER_TICK_MS));
		float dt = SERVER_TICK_MS / 1000.0f;
		spawn_counter++;

		int difficulty_level = 0;
		int alive_player_count = 0;
		
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			auto now = std::chrono::steady_clock::now();
			
			for (auto& pair : g_players)
			{
				if (pair.second->health > 0)
				{
					auto game_duration = std::chrono::duration_cast<std::chrono::seconds>(
						now - pair.second->game_start_time).count();
					difficulty_level += game_duration / 10;
					alive_player_count++;
				}
			}
			
			if (alive_player_count > 0)
			{
				difficulty_level = difficulty_level / alive_player_count;
			}
		}

		int current_spawn_rate = 60 - (difficulty_level * 2);
		if (current_spawn_rate < 15) current_spawn_rate = 15;
		int current_enemy_cooldown = 2000 - (difficulty_level * 200);
		if (current_enemy_cooldown < 500) current_enemy_cooldown = 500;

		{
			std::lock_guard<std::mutex> lock(g_mutex);
			auto now = std::chrono::steady_clock::now();

			for (auto& pair : g_players)
			{
				auto player = pair.second;
				if (player->health <= 0) continue;
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
					now - player->last_shoot_time).count();

				if (duration > PLAYER_SHOOT_COOLDOWN)
				{
					Bullet b;
					b.id = g_bullet_next_id++;
					b.x = player->x + (PLAYER_WIDTH_ESTIMATE - BULLET_WIDTH_ESTIMATE) / 2.0f;
					b.y = player->y;
					b.vx = 0;
					b.vy = -PLAYER_BULLET_SPEED;
					b.type = 0;
					b.angle = 0.0f;
					b.owned_id = player->id;
					g_bullets.push_back(b);
					player->last_shoot_time = now;
				}
			}

			if (spawn_counter >= current_spawn_rate)
			{
				spawn_counter = 0;
				Enemy e;
				e.id = g_enemy_next_id++;
				e.x=dis(gen) *(WINDOW_WIDTH - ENEMY_WIDTH);
				e.y = -ENEMY_HEIGHT;
				e.last_shoot_time = now;
				g_enemies.push_back(e);
			}

			for (auto it = g_enemies.begin(); it != g_enemies.end();)
			{
				it->y += ENEMY_SPEED * dt;
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->last_shoot_time).count();
				if (duration > current_enemy_cooldown)
				{
					auto target = GetNearestPlayer(it->x, it->y);
					if (target)
					{
						Bullet b;
						b.id = g_bullet_next_id++;
						b.type = 1;
						b.x = it->x + (ENEMY_WIDTH / 2) - (BULLET_WIDTH_ESTIMATE / 2);
						b.y = it->y + ENEMY_HEIGHT;

						float dx = (target->x + PLAYER_WIDTH_ESTIMATE / 2) - b.x;
						float dy = (target->y + PLAYER_HEIGHT_ESTIMATE / 2) - b.y;
						float len = std::sqrt(dx * dx + dy * dy);
						if (len > 0) {
							b.vx = (dx / len) * 300.0f;
							b.vy = (dy / len) * 300.0f;
							b.angle = atan2(dy, dx) * 180.0f / 3.14159265f - 90.0f;
						}
						else {
							b.vx = 0; b.vy = 300.0f;
							b.angle = 180.0f;
						}
						g_bullets.push_back(b);
						it->last_shoot_time = now;
					}
				}
				if(it->y>WINDOW_HEIGHT) it = g_enemies.erase(it);
				else ++it;
			}

			for (auto it = g_bullets.begin(); it != g_bullets.end(); ) {
				if (it->vx == 0 && it->vy == 0) {
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
		for (auto& b : g_bullets)
		{
			if (!b.active) continue;

			if (b.type == 0)
			{
				for (auto& e : g_enemies)
				{
					if (!e.active) continue;
					if (CheckCollision(b.x, b.y, BULLET_WIDTH_ESTIMATE, BULLET_HEIGHT_ESTIMATE,
						e.x, e.y, ENEMY_WIDTH, ENEMY_HEIGHT))
					{
						e.health -= 1;
						b.active = false;
						if (e.health <= 0)
						{
							e.active = false;
							if (g_players.find(b.owned_id) != g_players.end())
							{
								g_players[b.owned_id]->score += 10;
							}
							
							if (dis(gen) < 0.25f) {
								Item item;
								item.id = g_item_next_id++;
								item.x = e.x + ENEMY_WIDTH / 2.0f - ITEM_WIDTH / 2.0f;
								item.y = e.y + ENEMY_HEIGHT / 2.0f - ITEM_HEIGHT / 2.0f;
								float random_angle = dis(gen) * 2 * 3.1415926f;
								item.vx = cos(random_angle) * ITEM_SPEED;
								item.vy = sin(random_angle) * ITEM_SPEED;
								g_items.push_back(item);
							}
						}
						break;
					}
				}
			}
			else if (b.type == 1)
			{
				for (auto& pair : g_players)
				{
					auto p = pair.second;
					if (p->health <= 0) continue;
					if (CheckCollision(b.x, b.y, BULLET_WIDTH_ESTIMATE, BULLET_HEIGHT_ESTIMATE,
						p->x, p->y, PLAYER_WIDTH_ESTIMATE, PLAYER_HEIGHT_ESTIMATE)) {
						p->health -= 1;
						b.active = false;
						break;
					}
				}
			}
		}
		
		for (auto& e : g_enemies)
		{
			if (!e.active) continue;

			for (auto& pair : g_players)
			{
				auto p = pair.second;
				if (p->health <= 0) continue;
				if (CheckCollision(e.x, e.y, ENEMY_WIDTH, ENEMY_HEIGHT,
					p->x, p->y, PLAYER_WIDTH_ESTIMATE, PLAYER_HEIGHT_ESTIMATE)) {
					p->health -= 1;
					e.active = false;
					break;
				}
			}
		}
		
		for (auto& item : g_items) {
			if (!item.active) continue;

			item.x += item.vx * dt;
			item.y += item.vy * dt;

			if ((item.x < 0 || item.x + ITEM_WIDTH > WINDOW_WIDTH) && item.bounces_count > 0) {
				item.vx = -item.vx;
				item.bounces_count--;
			}
			if ((item.y < 0 || item.y + ITEM_HEIGHT > WINDOW_HEIGHT) && item.bounces_count > 0) {
				item.vy = -item.vy;
				item.bounces_count--;
			}

			if (item.x + ITEM_WIDTH < 0 || item.x > WINDOW_WIDTH ||
				item.y + ITEM_HEIGHT < 0 || item.y > WINDOW_HEIGHT) {
				item.active = false;
			}
			else {
				for (auto& pair : g_players) {
					auto p = pair.second;
					if (p->health <= 0) continue;
					if (CheckCollision(item.x, item.y, ITEM_WIDTH, ITEM_HEIGHT, p->x, p->y, PLAYER_WIDTH_ESTIMATE, PLAYER_HEIGHT_ESTIMATE)) {
						item.active = false;
						p->health++;
						if (p->health > 5) p->health = 5;
						p->score += 5;
						break;
					}
				}
			}
		}

		g_bullets.erase(std::remove_if(g_bullets.begin(), g_bullets.end(),
			[](const Bullet& b) { return !b.active; }), g_bullets.end());
		g_enemies.erase(std::remove_if(g_enemies.begin(), g_enemies.end(),
			[](const Enemy& e) { return !e.active; }), g_enemies.end());
		g_items.erase(std::remove_if(g_items.begin(), g_items.end(), [](const Item& i) { return !i.active; }), g_items.end());

		game::GameSnapshot snapshot;
		std::vector<std::shared_ptr<tcp::socket>> sockets_to_send;
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			for (auto& pair : g_players) {
				auto p_data = snapshot.add_players();
				p_data->set_id(pair.second->id);
				p_data->set_x(pair.second->x);
				p_data->set_y(pair.second->y);
				p_data->set_health(pair.second->health);
				p_data->set_score(pair.second->score);
				sockets_to_send.push_back(pair.second->socket);
			}
			for (const auto& b : g_bullets) {
				auto b_data = snapshot.add_bullets();
				b_data->set_id(b.id);
				b_data->set_x(b.x);
				b_data->set_y(b.y);
				b_data->set_type(b.type);
				b_data->set_angle(b.angle);
				b_data->set_owned_id(b.owned_id);
			}

			for (const auto& e : g_enemies)
			{
				auto ed = snapshot.add_enemies();
				ed->set_id(e.id);
				ed->set_x(e.x);
				ed->set_y(e.y);
				ed->set_type(0);
				ed->set_health(e.health);
			}

			for (const auto& i : g_items) {
				auto i_data = snapshot.add_items();
				i_data->set_id(i.id);
				i_data->set_x(i.x);
				i_data->set_y(i.y);
				i_data->set_type(i.type);
			}
			
			for (const auto& entry : g_leaderboard)
			{
				auto l_data = snapshot.add_leaderboard();
				l_data->set_name(entry.name);
				l_data->set_score(entry.score);
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
		LoadLeaderboard();

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