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
    std::cout << "[DEBUG] Player width: " << player.width << ", height: " << player.height << std::endl;
    player.position.x=game.getWindowWidth()/2-player.width/2;
    player.position.y=game.getWindowHeight()-player.height;

    //初始化模板
    projectilePlayerTemplate.texture=IMG_LoadTexture(game.getRenderer(),"assets/image/laser-1.png");
    SDL_QueryTexture(projectilePlayerTemplate.texture,NULL,NULL,&projectilePlayerTemplate.width,&projectilePlayerTemplate.height);
    projectilePlayerTemplate.width/=4;
    projectilePlayerTemplate.height/=4;
    std::cout << "[DEBUG] Bullet width: " << projectilePlayerTemplate.width << ", height: " << projectilePlayerTemplate.height << std::endl;

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
     //清理容器
    for(auto projectile:projectilePlayer)
    {
        if(projectile!=NULL)
        {
            delete projectile;
            projectile=NULL;
        }
    }
    projectilePlayer.clear();

    for(auto enemy:enemies)
    {
        if(enemy!=NULL)
        {
            delete enemy;
            enemy=NULL;
        }
    }
    enemies.clear();

    for(auto projectile:projectileEnemy)
    {
        if(projectile!=NULL)
        {
            delete projectile;
            projectile=NULL;
        }
    }
    projectileEnemy.clear();

    for(auto explosion:explosions)
    {
            if(explosion!=NULL)
            {
                delete explosion;
                explosion=NULL;
            }
    }
    explosions.clear();

    for(auto item:items)
    {
        if(item!=NULL)
        {
            delete item;
            item=NULL;
        }
    }
    items.clear();

    //清理uiHealth
    if(uiHealth!=NULL)
    {
        SDL_DestroyTexture(uiHealth);
        uiHealth=NULL;
    }
    //清理字体
    if(scoreFont!=NULL)
    {
        TTF_CloseFont(scoreFont);
        scoreFont=NULL;
    }

    //清理模板 
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


    //清理音乐
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
       }
       else
       {
           // --- 情况二：这是别人！存到 otherPlayers 列表里 ---
            // map 会自动处理：如果 ID 不存在就创建，存在就更新
           other_players[pid].x = server_player.x();
           other_players[pid].y = server_player.y();
       }
   }
   this->updateProjectilePlayer(deltaTime);
  this->updateProjectileEnemy(deltaTime);
   this->spewEnemy();
   this->updateEnemy(deltaTime);
   this->updatePlayer(deltaTime);
    this->updateExplosion(deltaTime);
   this->updateItems(deltaTime);

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
    //渲染敌人子弹
    this->renderProjectileEnemy();
    //渲染玩家
    // --- 渲染我自己 (Local Player) ---
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
    //渲染敌人
    this->renderEnemy();
    
    //渲染物品
    this->renderItems();

    //渲染爆炸
    this->renderExplosion();

    //渲染ui
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
       /* if(player.position.y>0)
        player.position.y-=deltaTime*player.speed;*/

        NetworkClient::GetInstance().SendInput(game::PlayerInput_InputType_UP);
       
    }
    if(keyboardSate[SDL_SCANCODE_S])
    {
        /*if(player.position.y<game.getWindowHeight()-player.height)
        player.position.y+=deltaTime*player.speed;*/
        NetworkClient::GetInstance().SendInput(game::PlayerInput_InputType_DOWN);
    }
    if(keyboardSate[SDL_SCANCODE_A])
    {
        /*if(player.position.x>0)
        player.position.x-=deltaTime*player.speed;*/
        NetworkClient::GetInstance().SendInput(game::PlayerInput_InputType_LEFT);
    }
    if(keyboardSate[SDL_SCANCODE_D])
    {
       /* if(player.position.x<game.getWindowWidth()-player.width)
        player.position.x+=deltaTime*player.speed; */
        NetworkClient::GetInstance().SendInput(game::PlayerInput_InputType_RIGHT);
    }

    //控制子弹发射
    // if(keyboardSate[SDL_SCANCODE_J])
    // {
    //     auto currentTime=SDL_GetTicks();
    //     if(currentTime-player.lastShootTime>player.Cooldown)
    //     {
    //        shootPlayer();
    //        player.lastShootTime=currentTime;
          
    //     }
    // }
    shoot();
}

void SceneMain::shootPlayer()
{
    auto projectile= new ProjectilePlayer(projectilePlayerTemplate);
    projectile->position.x=player.position.x+player.width/2-projectile->width/2;
    projectile->position.y=player.position.y;
    projectilePlayer.push_back(projectile);
    Mix_PlayChannel(0,sounds["player_shoot"],0);
}

void SceneMain::updateProjectilePlayer(float deltaTime)
{
    int margin= projectilePlayerTemplate.height;
    for(auto it=projectilePlayer.begin();it!=projectilePlayer.end();)
    {
        auto projectile=*it;
        projectile->position.y-=deltaTime*projectile->speed;
        //删除超出屏幕的子弹
        if(projectile->position.y+margin<0) 
        {
           delete projectile;
           it=projectilePlayer.erase(it);
       
        }
        else
        {

            //子弹与敌人碰撞检测
            bool hit=false;
            for(auto enemy:enemies)
            {
            
                SDL_Rect enemyRect={static_cast<int>(enemy->position.x),
                                   static_cast<int>(enemy->position.y),
                                   enemy->width,
                                   enemy->height};
                SDL_Rect projectileRect={static_cast<int>(projectile->position.x),
                                         static_cast<int>(projectile->position.y),
                                         projectile->width,
                                         projectile->height};
                if(SDL_HasIntersection(&enemyRect,&projectileRect))
                {
                    enemy->currenthealth-=projectile->damage;
                    delete projectile;
                    it=projectilePlayer.erase(it);
                    hit=true;
                    Mix_PlayChannel(-1,sounds["hit"],0);
                    break;

                }
                
            }
            if(!hit)  it++;


        }
      

    }
}

void SceneMain::renderProjectilePlayer()
{   
    for(auto projectile:projectilePlayer)
    {
        SDL_Rect projectileRect={static_cast<int>(projectile->position.x),
                                 static_cast<int>(projectile->position.y),
                                 projectile->width,
                                 projectile->height};
        SDL_RenderCopy(game.getRenderer(),projectile->texture,NULL,&projectileRect);
    }


}

void SceneMain::spewEnemy()
{
    if(dis(gen)>1/60.0f) return;
    Enemy* enemy= new Enemy(enemyTemplate);
    enemy->position.x=dis(gen)*(game.getWindowWidth()-enemy->width);
    enemy->position.y=-enemy->height;
    enemies.push_back(enemy);
    
}

void SceneMain::updateEnemy(float deltaTime)
{
    for(auto it=enemies.begin();it!=enemies.end();)
    {
        auto currentTime=SDL_GetTicks();
        auto enemy=*it;
        enemy->position.y+=deltaTime*enemy->speed;
        if(enemy->position.y>game.getWindowHeight())
        {
            delete enemy;
            it=enemies.erase(it);
        }
        else
        {
            if(enemy->lastShootTime+enemy->Cooldown<currentTime  &&!isDead)  
            {
                shootEnemy(enemy);
                enemy->lastShootTime=currentTime;
            }
            if(enemy->currenthealth<=0)
            {
                enemyExplode(enemy);
                it=enemies.erase(it);
            }
            else
            {
                it++;  
            }
            
        }

    }

}

void SceneMain::renderEnemy()
{
    for (auto enemy:enemies)
    {
        SDL_Rect enemyRect={static_cast<int>(enemy->position.x),
                            static_cast<int>(enemy->position.y),
                            enemy->width,
                            enemy->height};
        SDL_RenderCopy(game.getRenderer(),enemy->texture,NULL,&enemyRect);
    }
}

void SceneMain::shootEnemy(Enemy *enemy)
{
    auto projectile= new ProjectileEnemy(projectileEnemyTemplate);
    projectile->position.x=enemy->position.x+enemy->width/2-projectile->width/2;
    projectile->position.y=enemy->position.y+enemy->height/2-projectile->height/2;
    projectile->direction=getDirection(enemy);
    projectileEnemy.push_back(projectile);
    Mix_PlayChannel(-1,sounds["enemy_shoot"],0);
}

SDL_FPoint SceneMain::getDirection(Enemy *enemy)
{
    auto x= (player.position.x+player.width/2)-(enemy->position.x+enemy->width/2);
    auto y= (player.position.y+player.height/2)-(enemy->position.y+enemy->height/2);
    auto length=sqrt(x*x+y*y);
    x/=length;
    y/=length;
    return SDL_FPoint{x,y};
  
}

void SceneMain::updateProjectileEnemy(float deltaTime)
{
    auto margin= 32;
    for(auto it=projectileEnemy.begin();it!=projectileEnemy.end();)
    {
        auto projectile=*it;
        projectile->position.x+=deltaTime*projectile->speed*projectile->direction.x;
        projectile->position.y+=deltaTime*projectile->speed*projectile->direction.y;

        if(projectile->position.x+margin<0||projectile->position.x-margin>game.getWindowWidth()||
           projectile->position.y+margin<0||projectile->position.y-margin>game.getWindowHeight()) 
        {
            delete projectile;
            it=projectileEnemy.erase(it);
        }
        else
        {
            //子弹与玩家碰撞检测
            SDL_Rect playerRect={static_cast<int>(player.position.x),
                                 static_cast<int>(player.position.y),
                                 player.width,
                                 player.height};
            SDL_Rect projectileRect={static_cast<int>(projectile->position.x),
                                     static_cast<int>(projectile->position.y),
                                     projectile->width,
                                     projectile->height};
            if(SDL_HasIntersection(&playerRect,&projectileRect)&& !isDead)
            {
                player.currenthealth-=projectile->damage;
                delete projectile;
                it=projectileEnemy.erase(it);
                Mix_PlayChannel(-1,sounds["hit"],0);
            }else
            {
                it++;
            }
        }
    }
}

void SceneMain::updatePlayer(float )
{
    if(isDead) return;
    if(player.currenthealth<=0)
    {
    isDead=true;
        auto currentTime=SDL_GetTicks();
        auto explosion= new Explosion(explosionTemplate);
        explosion->position.x=player.position.x+player.width/2-explosion->width/2;
        explosion->position.y=player.position.y+player.height/2-explosion->height/2;
        explosion->startTime=currentTime;
        explosions.push_back(explosion);
        Mix_PlayChannel(-1,sounds["player_explode"],0);
        game.setScore(score);
        return;
    }
    for(auto enemy:enemies)
    {
     SDL_Rect enemyRect={static_cast<int>(enemy->position.x),
                         static_cast<int>(enemy->position.y),
                         enemy->width,
                         enemy->height};
     SDL_Rect playerRect={static_cast<int>(player.position.x),
                          static_cast<int>(player.position.y),
                          player.width,
                          player.height};
                          
     if(SDL_HasIntersection(&enemyRect,&playerRect))
     {
        player.currenthealth-=1;
        enemy->currenthealth=0;
     }
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
void SceneMain::updateItems(float deltaTime)
{
    for(auto it=items.begin();it!=items.end();)
    {
        auto item=*it;
        //更新位置
        item->position.x+=deltaTime*item->speed*item->direction.x;
        item->position.y+=deltaTime*item->speed*item->direction.y;
        //处理边缘反弹
        if((item->position.x<0||item->position.x+item->width>game.getWindowWidth()) && item->bouncesCount>0)
        {
            item->direction.x=-item->direction.x;
            item->bouncesCount--;
        }
        if((item->position.y<0||item->position.y+item->height>game.getWindowHeight()) && item->bouncesCount>0)
        {
            item->direction.y=-item->direction.y;
            item->bouncesCount--;
        }


      //如果超出屏幕，删除该物体
        if(item->position.x+item->width<0||item->position.x>game.getWindowWidth()||
           item->position.y+item->height<0||item->position.y>game.getWindowHeight())
        {
         delete item;
         it=items.erase(it); 
        }
        else {
            SDL_Rect playerRect={static_cast<int>(player.position.x),
                                 static_cast<int>(player.position.y),
                                 player.width,
                                 player.height};
            SDL_Rect itemRect={static_cast<int>(item->position.x),
                               static_cast<int>(item->position.y),
                               item->width,
                               item->height};
            if(SDL_HasIntersection(&playerRect,&itemRect)&&!isDead)
            {
               playerGetItem(item);
                delete item;
                it=items.erase(it);
                Mix_PlayChannel(-1,sounds["get_item"],0);
            }else
            {
                it++;
            }
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

void SceneMain::renderProjectileEnemy()
{
    
    for(auto projectile:projectileEnemy)
    {
        SDL_Rect projectileRect={static_cast<int>(projectile->position.x),
                                 static_cast<int>(projectile->position.y),
                                 projectile->width,
                                 projectile->height};
       float angle=atan2(projectile->direction.y,projectile->direction.x)*180/M_PI-90;
       SDL_RenderCopyEx(game.getRenderer(),projectile->texture,NULL,&projectileRect,angle,NULL,SDL_FLIP_NONE);
    }
}

void SceneMain::renderExplosion()
{
    for(auto explosion:explosions)
    {
        SDL_Rect src={explosion->currentframe*explosion->width,
                                0,
                                explosion->width/2,
                                explosion->height/2};//原始区域
        SDL_Rect dst={static_cast<int>(explosion->position.x),
                                static_cast<int>(explosion->position.y),
                                explosion->width,
                                explosion->height};//目标区域
        SDL_RenderCopy(game.getRenderer(),explosion->texture,&src,&dst);
    }

}

void SceneMain::renderItems()
{
    for(auto item:items)
    {
        SDL_Rect dst={static_cast<int>(item->position.x),
                                static_cast<int>(item->position.y),
                                item->width,
                                item->height};//目标区域
        SDL_RenderCopy(game.getRenderer(),item->texture,nullptr,&dst);
    }


}

void SceneMain::renderUi()
{
    //渲染血条
    int x=10;
    int y=10;
    int size=32;
    int offset=40;
    SDL_SetTextureColorMod(uiHealth,100,100,100);//颜色减淡
    for(int i = 0;i<player.maxhealth;i++)
    {   
        SDL_Rect src={x+i*offset,y,size,size};
        SDL_RenderCopy(game.getRenderer(),uiHealth,nullptr,&src);
    }
    SDL_SetTextureColorMod(uiHealth,255,255,255);//颜色恢复
    for(int i = 0;i<player.currenthealth;i++)
    {
        SDL_Rect src={x+i*offset,y,size,size};
        SDL_RenderCopy(game.getRenderer(),uiHealth,nullptr,&src);
    }
    //渲染分数
    auto text="Score:"+std::to_string(score);
    SDL_Color color={255,255,255,255};
    SDL_Surface* surface=TTF_RenderUTF8_Solid(scoreFont,text.c_str(),color);
    SDL_Texture* texture=SDL_CreateTextureFromSurface(game.getRenderer(),surface);
    SDL_Rect dst={game.getWindowWidth()-surface->w-10,10,surface->w,surface->h};
    SDL_RenderCopy(game.getRenderer(),texture,nullptr,&dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);


}

void SceneMain::renderRemotePlayers()
{
    // 遍历所有其他玩家
    for (auto& pair : other_players)
    {
        RemotePlayer& p = pair.second;

        SDL_Rect otherRect = {
            static_cast<int>(p.x),
            static_cast<int>(p.y),
            player.width,  // 假设大家飞机大小一样
            player.height
        };

        // 变成淡红色，区分敌我
        SDL_SetTextureColorMod(player.texture, 255, 100, 100);

        // 绘制
        SDL_RenderCopy(game.getRenderer(), player.texture, NULL, &otherRect);

        // 【重要】画完后把颜色还原回来！否则你自己也会变红
        SDL_SetTextureColorMod(player.texture, 255, 255, 255);
    }
}

//void SceneMain::renderNetworkBullets()
//{
//    game::GameSnapshot state = NetworkClient::GetInstance().GetState();
//    for (const auto& bullet : state.bullets())
//    {
//        SDL_Texture* textureToDraw = nullptr;
//        int w = 0, h = 0;
//        float angle = 0.0f;
//
//        // 根据类型选择贴图
//        if (bullet.type() == 0) {
//            SDL_Rect dst = {
//                static_cast<int>(bullet.x()),
//                static_cast<int>(bullet.y()),
//                w, h
//            };
//            SDL_RenderCopy(game.getRenderer(), textureToDraw, NULL, &dst);
//        }
//        else if (bullet.type() == 1) {
//            // 敌人子弹：复用原本的敌人子弹图
//            textureToDraw = projectileEnemyTemplate.texture;
//            w = projectileEnemyTemplate.width;
//            h = projectileEnemyTemplate.height;
//            angle = 180.0f; // 敌人子弹倒过来画
//        }
//    }
//}
void SceneMain::renderNetworkBullets()
{
    // 获取最新网络状态
    game::GameSnapshot state = NetworkClient::GetInstance().GetState();

    for (const auto& bullet : state.bullets())
    {
        SDL_Texture* textureToDraw = nullptr;
        int w = 0, h = 0;
        float angle = 0.0f;

        // 根据类型选择贴图
        if (bullet.type() == 0) {
            // 玩家子弹：复用原本的激光图
            textureToDraw = projectilePlayerTemplate.texture;
            w = projectilePlayerTemplate.width;
            h = projectilePlayerTemplate.height;
        }
        else if (bullet.type() == 1) {
            // 敌人子弹：复用原本的敌人子弹图
            textureToDraw = projectileEnemyTemplate.texture;
            w = projectileEnemyTemplate.width;
            h = projectileEnemyTemplate.height;
            angle = 180.0f; // 敌人子弹倒过来画
        }

        // 只要找到了贴图，就画出来
        if (textureToDraw) {
            SDL_Rect dst = {
                static_cast<int>(bullet.x()),
                static_cast<int>(bullet.y()),
                w, h
            };

            // 带角度的绘制 (RenderCopyEx)
            if (angle != 0.0f) {
                SDL_RenderCopyEx(game.getRenderer(), textureToDraw, NULL, &dst, angle, NULL, SDL_FLIP_NONE);
            }
            else {
                SDL_RenderCopy(game.getRenderer(), textureToDraw, NULL, &dst);
            }
        }
    }
}

void SceneMain::enemyExplode(Enemy *enemy)
{
    auto currentTime=SDL_GetTicks();
    Explosion* explosion= new Explosion(explosionTemplate);
    explosion->position.x=enemy->position.x+enemy->width/2-explosion->width/2;
    explosion->position.y=enemy->position.y+enemy->height/2-explosion->height/2;
    explosion->startTime=currentTime;
    explosions.push_back(explosion);
    Mix_PlayChannel(-1,sounds["enemy_explode"],0);
    if(dis(gen)<0.5f)
    {
        dropItem(enemy);
    }
    score+=10;
    
    delete enemy;
}

void SceneMain::dropItem(Enemy *enemy)
{
    auto item= new Item(itemLifeTemplate);
    item->position.x=enemy->position.x+enemy->width/2-item->width/2;
    item->position.y=enemy->position.y+enemy->height/2-item->height/2;
    float angle=dis(gen)*2*M_PI;
    item->direction=SDL_FPoint{cos(angle),sin(angle)};
    items.push_back(item);
}

void SceneMain::playerGetItem(Item *item)
{
    score+=5;
    if(item->type==ItemType::Life)
    {
        player.currenthealth++;
        if(player.currenthealth>player.maxhealth)
        {
            player.currenthealth=player.maxhealth;
        }
    }
}

void SceneMain::shoot()
{
    auto currentTime=SDL_GetTicks();
        if(currentTime-player.lastShootTime>player.Cooldown)
        {
           shootPlayer();
           player.lastShootTime=currentTime;
          
        }
}
