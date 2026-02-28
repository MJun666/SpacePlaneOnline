#ifndef SCENEMAIN_H
#define SCENEMAIN_H
#include "Scene.h"
#include"Object.h"
#include<list>
#include<map>
#include<random>
#include<SDL2/SDL.h>
#include<SDL2/SDL_mixer.h>
#include<SDL2/SDL_ttf.h>
// 简单的结构体存储其他玩家位置
struct RemotePlayer {
    float x, y;
    int id;
};
class Game;

class SceneMain : public Scene
{
    public:
  
    ~SceneMain();
    void init() override;
    void clean() override;
    void handleEvent(SDL_Event* event) override;

   

    private:
   
    Player player;
    Mix_Music* bgm;
    SDL_Texture* uiHealth;
    TTF_Font* scoreFont;
    int score=0;
    float timerEnd=0.0f;

    bool isDead=false;
    std::mt19937 gen;//随机数引擎
    std::uniform_real_distribution<float> dis;//随机数分布
    std::map<int, RemotePlayer> other_players;
    std::map<int, SDL_FPoint> last_enemies;

    ProjectilePlayer projectilePlayerTemplate;
    Enemy enemyTemplate;
    ProjectileEnemy projectileEnemyTemplate;
    Explosion explosionTemplate;
    Item itemLifeTemplate;

    std::list<Explosion*> explosions;
    std::map<std::string, Mix_Chunk*> sounds;
    
    void update(float deltaTime) override;
    void updateExplosion(float deltaTime);
    void chamgeSceneDelayed(float daltaTime,float delay);

    void render() override;
    void renderExplosion();
    void renderUi();
    void renderNetworkBullets();
    void renderNetworkEnemies();
    void renderNetworkItems();

    void keyboardControl(float deltaTime);
    void shoot();

};






#endif