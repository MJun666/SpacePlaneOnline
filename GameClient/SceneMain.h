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
    std::map<int, RemotePlayer> other_players; // 存储所有的敌人/队友

    //创建每个物体模板
    ProjectilePlayer projectilePlayerTemplate;
    Enemy enemyTemplate;
    ProjectileEnemy projectileEnemyTemplate;
    Explosion explosionTemplate;
    Item itemLifeTemplate;

    //创建每个物体容器
    std::list<ProjectilePlayer*> projectilePlayer;
    std::list<Enemy*> enemies;
    std::list<ProjectileEnemy*> projectileEnemy;
    std::list<Explosion*> explosions;
    std::list<Item*> items;
    std::map<std::string, Mix_Chunk*> sounds;//音效
    
     //更新
     void update(float deltaTime) override;
     void updateProjectilePlayer(float deltaTime);
     void updateEnemy(float deltaTime);
     void updateProjectileEnemy(float deltaTime);
     void updatePlayer(float deltaTime);
     void updateExplosion(float deltaTime);
     void updateItems(float deltaTime);
    void chamgeSceneDelayed(float daltaTime,float delay);

      //渲染
    void render() override;
    void renderProjectilePlayer();
    void renderEnemy();
    void renderProjectileEnemy();
    void renderExplosion();
    void renderItems();
    void renderUi();
    // ===  封装好的网络渲染函数 ===
    void renderRemotePlayers();  // 画其他玩家
    void renderNetworkBullets(); // 画网络子弹
    void renderNetworkEnemies();// 画网络敌机 

    //其他
    void keyboardControl(float deltaTime);
    void shootPlayer();
    void spewEnemy();
    void shootEnemy(Enemy* enemy);
    SDL_FPoint getDirection( Enemy* enemy);
    void enemyExplode(Enemy* enemy);
    void dropItem(Enemy* enemy);
    void playerGetItem(Item* item);
    void shoot();//自动射击

};






#endif