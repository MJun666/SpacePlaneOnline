#include "SceneMain.h"
#include"SceneTitle.h"
#include"SceneEnd.h"
#include"Game.h"
#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<iostream>
#include<random>
#include <string>
#include "NetworkClient.h"
#include "game.pb.h"


SceneMain::~SceneMain()
{
}

void SceneMain::init()
{
    // 重置游戏状态
    isDead = false;
    score = 0;
    timerEnd = 0.0f;
    
    //读取并获取背景音乐
    bgm=Mix_LoadMUS("assets/music/03_Racing_Through_Asteroids_Loop.ogg");
    if(bgm==NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Mix_LoadMUS: %s\n",Mix_GetError());
    }
    Mix_PlayMusic(bgm,-1);


    //读取uiHealth
    uiHealth=IMG_LoadTexture(game.getRenderer(),"assets/image/Health UI Black.png");

    //载入字体
    scoreFont=TTF_OpenFont("assets/font/VonwaonBitmap-12px.ttf",24);

    //读取音效
    sounds["player_shoot"]=Mix_LoadWAV("assets/sound/laser_shoot4.wav");
    sounds["enemy_shoot"]=Mix_LoadWAV("assets/sound/xs_laser.wav");
    sounds.insert(std::make_pair("player_explode",Mix_LoadWAV("assets\sound\explosion1.wav")));
    sounds["enemy_explode"]=Mix_LoadWAV("assets/sound/explosion3.wav");
    sounds["hit"]=Mix_LoadWAV("assets/sound/eff11.wav");
    sounds["get_item"]=Mix_LoadWAV("assets/sound/eff5.wav");
    //创建随机数种子
    std::random_device rd;
    //创建随机数引擎
    gen= std::mt19937(rd());
    //创建随机数分布
    dis =std::uniform_real_distribution<float>(0.0f,1.0f);


    player.texture=IMG_LoadTexture(game.getRenderer(),"assets/image/SpaceShip.png");
    SDL_QueryTexture(player.texture,NULL,NULL,&player.width,&player.height);
    player.width/=5;
    player.height/=5;
    player.position.x=game.getWindowWidth()/2-player.width/2;
    player.position.y=game.getWindowHeight()-player.height;

    //初始化模板
    projectilePlayerTemplate.texture=IMG_LoadTexture(game.getRenderer(),"assets/image/laser-1.png");
    SDL_QueryTexture(projectilePlayerTemplate.texture,NULL,NULL,&projectilePlayerTemplate.width,&projectilePlayerTemplate.height);
    projectilePlayerTemplate.width/=4;
    projectilePlayerTemplate.height/=4;

    enemyTemplate.texture=IMG_LoadTexture(game.getRenderer(),"assets/image/insect-2.png");
    SDL_QueryTexture(enemyTemplate.texture,NULL,NULL,&enemyTemplate.width,&enemyTemplate.height);
    enemyTemplate.width/=4;
    enemyTemplate.height/=4;
  
    projectileEnemyTemplate.texture=IMG_LoadTexture(game.getRenderer(),"assets/image/bullet-1.png");
    SDL_QueryTexture(projectileEnemyTemplate.texture,NULL,NULL,&projectileEnemyTemplate.width,&projectileEnemyTemplate.height);
    projectileEnemyTemplate.width/=2;
    projectileEnemyTemplate.height/=2;

    explosionTemplate.texture=IMG_LoadTexture(game.getRenderer(),"assets/effect/explosion.png");
    SDL_QueryTexture(explosionTemplate.texture,NULL,NULL,&explosionTemplate.width,&explosionTemplate.height);
    explosionTemplate.totalframe=explosionTemplate.width/explosionTemplate.height;
    explosionTemplate.height*=2;
    explosionTemplate.width=explosionTemplate.height;

    itemLifeTemplate.texture=IMG_LoadTexture(game.getRenderer(),"assets/image/bonus_life.png");
    SDL_QueryTexture(itemLifeTemplate.texture,NULL,NULL,&itemLifeTemplate.width,&itemLifeTemplate.height);
    itemLifeTemplate.width/=4;
    itemLifeTemplate.height/=4;
    
    //  通知服务器：我进游戏了，给我满血复活！ ===
    NetworkClient::GetInstance().SendInput(game::PlayerInput_InputType_RESPAWN);
}
void SceneMain::clean()
{
    for(auto sound:sounds)
    {
        if(sound.second!=NULL)
        {
            Mix_FreeChunk(sound.second);
            sound.second=NULL;
        }
    }
    sounds.clear();

    for(auto explosion:explosions)
    {
        if(explosion!=NULL)
        {
            delete explosion;
            explosion=NULL;
        }
    }
    explosions.clear();

    if(uiHealth!=NULL)
    {
        SDL_DestroyTexture(uiHealth);
        uiHealth=NULL;
    }
    
    if(scoreFont!=NULL)
    {
        TTF_CloseFont(scoreFont);
        scoreFont=NULL;
    }

    if(player.texture!=NULL)
    {
        SDL_DestroyTexture(player.texture);
        player.texture=NULL;
    }
    if(projectilePlayerTemplate.texture!=NULL)
    {
        SDL_DestroyTexture(projectilePlayerTemplate.texture);
        projectilePlayerTemplate.texture=NULL;
    }
    if(enemyTemplate.texture!=NULL)
    {
        SDL_DestroyTexture(enemyTemplate.texture);
        enemyTemplate.texture=NULL;
    }
    if(projectileEnemyTemplate.texture!=NULL)
    {
        SDL_DestroyTexture(projectileEnemyTemplate.texture);
        projectileEnemyTemplate.texture=NULL;
    }
    if(explosionTemplate.texture!=NULL)
    {
        SDL_DestroyTexture(explosionTemplate.texture);
        explosionTemplate.texture=NULL;
    }
    if(itemLifeTemplate.texture!=NULL)
    {
        SDL_DestroyTexture(itemLifeTemplate.texture);
        itemLifeTemplate.texture=NULL;
    }

    if(bgm!=NULL)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(bgm);
        bgm=NULL;
    }
}

void SceneMain::handleEvent(SDL_Event *event)
{
    if(event->type==SDL_KEYDOWN)
    {
        if(event->key.keysym.scancode==SDL_SCANCODE_ESCAPE)
        {
            game.changeScene(new SceneTitle());
        }
    }
}

void SceneMain::update(float deltaTime)
{
    // 1. 本地控制 (发送按键)
   this->keyboardControl( deltaTime);
   // ================= 网络同步代码开始 =================

    // 获取服务器发来的最新状态
   game::GameSnapshot state = NetworkClient::GetInstance().GetState();
   int myID = NetworkClient::GetInstance().GetMyID();


   for (const auto& server_player : state.players())
   {
       int pid = server_player.id();
       if (pid == myID)
       {
           // --- 情况一：这是我！同步给本地主角 ---
           this->player.position.x = server_player.x();
           this->player.position.y = server_player.y();

           // [新增音效逻辑] 如果发现服务器发来的血量比我当前的血量多，说明我吃到道具了！
           if (server_player.health() > this->player.currenthealth && !this->isDead) {
               Mix_PlayChannel(-1, sounds["get_item"], 0);
           }
           this->player.currenthealth = server_player.health();
           this->score = server_player.score();
           //  死亡判定与特效触发
           if (this->player.currenthealth <= 0 && !this->isDead)
           {
               this->isDead = true;
               // 触发你原有的爆炸特效
               auto explosion = new Explosion(explosionTemplate);
               explosion->position.x = player.position.x + player.width / 2 - explosion->width / 2;
               explosion->position.y = player.position.y + player.height / 2 - explosion->height / 2;
               explosion->startTime = SDL_GetTicks();
               explosions.push_back(explosion);
               Mix_PlayChannel(-1, sounds["player_explode"], 0);
               game.setScore(score);
           }

           if (this->player.currenthealth > 0 && this->isDead)
           {
               this->isDead = false;
           }
       }
       else
       {
           other_players[pid].x = server_player.x();
           other_players[pid].y = server_player.y();
       }
   }

    this->updateExplosion(deltaTime);
   std::map<int, SDL_FPoint> current_enemies;
   for (const auto& enemy : state.enemies()) {
       current_enemies[enemy.id()] = { enemy.x(), enemy.y() };
   }

   // 查上帧活着的敌机，如果现在没了，说明它死了
   for (auto& pair : last_enemies) {
       int id = pair.first;
       if (current_enemies.find(id) == current_enemies.end()) {
           // 检查它是否是正常飞出屏幕底端 (给点余量，比如大于窗口高度减去自身高度)
           if (pair.second.y < game.getWindowHeight() - enemyTemplate.height) {
               // 是在半空中消失的，说明被打爆了！播放爆炸动画！
               auto explosion = new Explosion(explosionTemplate);
               explosion->position.x = pair.second.x + enemyTemplate.width / 2 - explosion->width / 2;
               explosion->position.y = pair.second.y + enemyTemplate.height / 2 - explosion->height / 2;
               explosion->startTime = SDL_GetTicks();
               explosions.push_back(explosion);
               Mix_PlayChannel(-1, sounds["enemy_explode"], 0);

               // 注意：这里去掉了掉落道具的逻辑，因为道具我们以后统一交由服务器生成
           }
       }
   }
   // 更新记录
   last_enemies = current_enemies;
   // =================================================================

   if(isDead)
   {
    this->chamgeSceneDelayed(deltaTime,2.0f);
   }
}

void SceneMain::render()
{
     //渲染玩家子弹
    // this->renderProjectilePlayer();
    
    //  渲染网络子弹 (替代了原本的 renderProjectilePlayer)
    this->renderNetworkBullets();
    
    if(!isDead){
        SDL_Rect playerRect={static_cast<int> (player.position.x),
                             static_cast<int>(player.position.y),
                             player.width,
                             player.height};
        SDL_RenderCopy(game.getRenderer(),player.texture,NULL,&playerRect); 
    }
    // ---  渲染其他玩家 (Remote Players) ---
    for (auto& pair : other_players)
    {
        int id = pair.first;
        RemotePlayer& p = pair.second;

        SDL_Rect otherRect = {
            static_cast<int>(p.x),
            static_cast<int>(p.y),
            player.width,  // 暂时假设大家大小一样
            player.height
        };
        SDL_SetTextureColorMod(player.texture, 255, 100, 100);
        SDL_RenderCopy(game.getRenderer(), player.texture, NULL, &otherRect);
    }
    
    this->renderNetworkEnemies();
    this->renderNetworkItems();
    this->renderExplosion();
    renderUi();
}

void SceneMain::keyboardControl(float deltaTime)
{
    if(isDead)
    {
        return;
    }
    auto keyboardSate=  SDL_GetKeyboardState(NULL);
    if(keyboardSate[SDL_SCANCODE_W])
    {
        NetworkClient::GetInstance().SendInput(game::PlayerInput_InputType_UP);
    }
    if(keyboardSate[SDL_SCANCODE_S])
    {
        NetworkClient::GetInstance().SendInput(game::PlayerInput_InputType_DOWN);
    }
    if(keyboardSate[SDL_SCANCODE_A])
    {
        NetworkClient::GetInstance().SendInput(game::PlayerInput_InputType_LEFT);
    }
    if(keyboardSate[SDL_SCANCODE_D])
    {
        NetworkClient::GetInstance().SendInput(game::PlayerInput_InputType_RIGHT);
    }

    shoot();
}

void SceneMain::shoot()
{
    auto currentTime=SDL_GetTicks();
    if(currentTime-player.lastShootTime>player.Cooldown)
    {
        player.lastShootTime=currentTime;
    }
}

void SceneMain::updateExplosion(float )
{
    auto currentTime=SDL_GetTicks();
    for(auto it=explosions.begin();it!=explosions.end();)
    {
        auto explosion=*it;
       explosion->currentframe=(currentTime-explosion->startTime)/explosion->FPS/10;
        if(explosion->currentframe>=explosion->totalframe)
        {
            delete explosion;
            it=explosions.erase(it);
        }  
        else
        {
            it++;
        }
    }
}

void SceneMain::chamgeSceneDelayed(float daltaTime, float delay)
{
    timerEnd+=daltaTime;
    if(timerEnd>delay)
    {
       game.changeScene(new SceneEnd());
    }
}

void SceneMain::renderExplosion()
{
    for(auto explosion:explosions)
    {
        SDL_Rect src={explosion->currentframe*explosion->width,
                      0,
                      explosion->width/2,
                      explosion->height/2};
        SDL_Rect dst={static_cast<int>(explosion->position.x),
                      static_cast<int>(explosion->position.y),
                      explosion->width,
                      explosion->height};
        SDL_RenderCopy(game.getRenderer(),explosion->texture,&src,&dst);
    }
}

void SceneMain::renderUi()
{
    int x=10;
    int y=10;
    int size=32;
    int offset=40;
    SDL_SetTextureColorMod(uiHealth,100,100,100);
    for(int i = 0;i<player.maxhealth;i++)
    {   
        SDL_Rect src={x+i*offset,y,size,size};
        SDL_RenderCopy(game.getRenderer(),uiHealth,nullptr,&src);
    }
    SDL_SetTextureColorMod(uiHealth,255,255,255);
    for(int i = 0;i<player.currenthealth;i++)
    {
        SDL_Rect src={x+i*offset,y,size,size};
        SDL_RenderCopy(game.getRenderer(),uiHealth,nullptr,&src);
    }
    
    auto text="Score:"+std::to_string(score);
    SDL_Color color={255,255,255,255};
    SDL_Surface* surface=TTF_RenderUTF8_Solid(scoreFont,text.c_str(),color);
    SDL_Texture* texture=SDL_CreateTextureFromSurface(game.getRenderer(),surface);
    SDL_Rect dst={game.getWindowWidth()-surface->w-10,10,surface->w,surface->h};
    SDL_RenderCopy(game.getRenderer(),texture,nullptr,&dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void SceneMain::renderNetworkBullets()
{
    game::GameSnapshot state = NetworkClient::GetInstance().GetState();

    for (const auto& bullet : state.bullets())
    {
        SDL_Texture* textureToDraw = nullptr;
        int w = 0, h = 0;
        float angle = bullet.angle();

        if (bullet.type() == 0) {
            textureToDraw = projectilePlayerTemplate.texture;
            w = projectilePlayerTemplate.width;
            h = projectilePlayerTemplate.height;
        }
        else if (bullet.type() == 1) {
            textureToDraw = projectileEnemyTemplate.texture;
            w = projectileEnemyTemplate.width;
            h = projectileEnemyTemplate.height;
        }

        if (textureToDraw) {
            SDL_Rect dst = {
                static_cast<int>(bullet.x()),
                static_cast<int>(bullet.y()),
                w, h
            };

            if (angle != 0.0f) {
                SDL_RenderCopyEx(game.getRenderer(), textureToDraw, NULL, &dst, angle, NULL, SDL_FLIP_NONE);
            }
            else {
                SDL_RenderCopy(game.getRenderer(), textureToDraw, NULL, &dst);
            }
        }
    }
}

void SceneMain::renderNetworkEnemies()
{
    game::GameSnapshot state = NetworkClient::GetInstance().GetState();
    for (const auto& enemy : state.enemies())
    {
        SDL_Rect dst = { static_cast<int>(enemy.x()),
                         static_cast<int>(enemy.y()),
                         enemyTemplate.width,
                         enemyTemplate.height };
        SDL_RenderCopy(game.getRenderer(), enemyTemplate.texture, NULL, &dst);
    }
}

void SceneMain::renderNetworkItems()
{
    game::GameSnapshot state = NetworkClient::GetInstance().GetState();
    for (const auto& item : state.items())
    {
        SDL_Rect dst = {
            static_cast<int>(item.x()),
            static_cast<int>(item.y()),
            itemLifeTemplate.width,
            itemLifeTemplate.height
        };
        SDL_RenderCopy(game.getRenderer(), itemLifeTemplate.texture, nullptr, &dst);
    }
}
