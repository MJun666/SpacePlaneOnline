#ifndef OBJECT_H
#define OBJECT_H
#include <SDL2/SDL.h>

enum class ItemType
{
    Life,
    Shield,
    Time,
};

struct Player
{
   SDL_Texture* texture=nullptr;//玩家材质
    SDL_FPoint position = {0,0};//玩家位置
    int width=0;
    int height=0;
    int speed=300;
    int currenthealth=5;
    int maxhealth=5;
    Uint32 lastShootTime=0;
    Uint32 Cooldown=300;
};

struct ProjectilePlayer
{
    SDL_Texture* texture=nullptr;//玩家子弹材质
    SDL_FPoint position = {0,0};//玩家子弹位置
    int width=0;
    int height=0;
    int damage=1;
    int speed=600;
};
struct Enemy
{
    SDL_Texture* texture=nullptr;//敌人材质
    SDL_FPoint position = {0,0};//敌人位置
    int width=0;
    int height=0;
    int speed=200;
    int currenthealth=2;
    Uint32 lastShootTime=0;
    Uint32 Cooldown=2000;
   
};

struct ProjectileEnemy
{
    SDL_Texture* texture=nullptr;//敌人子弹材质
    SDL_FPoint position = {0,0};//敌人子弹位置
    SDL_FPoint direction = {0,0};//敌人子弹方向
    int width=0;
    int height=0;
    int speed=300;
    int damage=1;
   
};

struct Explosion
{
    SDL_Texture* texture=nullptr;//爆炸材质
    SDL_FPoint position = {0,0};//爆炸位置
    int width=0;
    int height=0;
    int currentframe=0;
    int totalframe=0;
    Uint32 startTime=0;
    Uint32 FPS=10; 
};

struct Item
{
    SDL_Texture* texture=nullptr;//物品材质
    SDL_FPoint position = {0,0};//物品位置
    SDL_FPoint direction = {0,0};//物品方向
    int height=0;
    int width=0;
    int speed=200;
    int bouncesCount=3;
    ItemType type=ItemType::Life;
};

struct Background
{
    SDL_Texture* texture=nullptr;//背景材质
    SDL_FPoint position = {0,0};//背景位置
    float offset=0;
    int width=0;
    int height=0;
    int speed=30;
   
};









#endif // OBJECT_H