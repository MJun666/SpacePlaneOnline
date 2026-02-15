#include "Game.h"
#include"SceneMain.h"
#include"SceneTitle.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include<fstream>
#include "NetworkClient.h"
Game::Game()
{
}

Game::~Game()
{
    saveData();
    this->clean();
}

void Game::run()
{

while(isRunning)
{ 
    auto frameStart=SDL_GetTicks();
    SDL_Event event;
    handleEvent(&event);
   
   
    this->update(deltaTime);
    
    render();
    auto frameEnd=SDL_GetTicks();
    auto  diff= frameEnd-frameStart;
    if(diff<frameTime)
    {
        SDL_Delay(frameTime-diff);
        deltaTime=frameTime/1000.0f;
    }
    else
    {
        deltaTime=diff/1000.0f;
    }
}

}

void Game::init()
{
    frameTime=1000/FPS;
    //SDL初始化
    if(SDL_Init(SDL_INIT_EVERYTHING)!=0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not initialize SDL: %s\n",SDL_GetError());
        isRunning=false;
    }
    //创建窗口
    window=SDL_CreateWindow("Game",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,this->windowWidth,this->windowHeight,SDL_WINDOW_SHOWN);
    if(window==nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not create window: %s\n",SDL_GetError());
        isRunning=false; 
    }
    //创建渲染器
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
    if(renderer==nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not create renderer: %s\n",SDL_GetError());
        isRunning=false;
    }
    //设计逻辑窗口
    SDL_RenderSetLogicalSize(renderer,windowWidth,windowHeight);


    //初始化SDL_image
    if(IMG_Init(IMG_INIT_PNG)!=IMG_INIT_PNG)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not initialize SDL_image: %s\n",IMG_GetError());
        isRunning=false;
    }

    //初始化SDL_mixer
    if(Mix_Init(MIX_INIT_MP3|MIX_INIT_OGG)!=(MIX_INIT_MP3|MIX_INIT_OGG))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not initialize SDL_mixer: %s\n",Mix_GetError());
        isRunning=false;
    }

    //打开音频设备
    if(Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,2048)<0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not open audio device: %s\n",Mix_GetError());
        isRunning=false;
    }
    //设置音效channel数量
    Mix_AllocateChannels(32);

    //设置音乐音量
    Mix_VolumeMusic(MIX_MAX_VOLUME/4);
    Mix_Volume(-1,MIX_MAX_VOLUME/8);

    //初始化SDL_ttf
    if(TTF_Init()==-1)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not initialize SDL_ttf: %s\n",TTF_GetError());
        isRunning=false;
    }

    //初始化背景卷轴
    nearStar.texture=IMG_LoadTexture(renderer,"assets/image/Stars-A.png");
    if(nearStar.texture==nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not load texture: %s\n",IMG_GetError());
        isRunning=false; 
    }
    SDL_QueryTexture(nearStar.texture,nullptr,nullptr,&nearStar.width,&nearStar.height);
    nearStar.width/=2;
    nearStar.height/=2;
    

    farStar.texture=IMG_LoadTexture(renderer,"assets/image/Stars-B.png");
    SDL_QueryTexture(farStar.texture,nullptr,nullptr,&farStar.width,&farStar.height);
    farStar.width/=2;
    farStar.height/=2;
    farStar.speed=20;

    //载入字体
    titleFont=TTF_OpenFont("assets/font/VonwaonBitmap-16px.ttf",64);
    textFont=TTF_OpenFont("assets/font/VonwaonBitmap-16px.ttf",32);
    if(titleFont==nullptr||textFont==nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not load font: %s\n",TTF_GetError());
        isRunning=false;
    }


    //载入得分
    loadData(); 
    currentScene=new SceneTitle();
    currentScene->init();
    // --- 新增：连接服务器 ---
    std::cout << "Connecting to server..." << std::endl;
    if (NetworkClient::GetInstance().Connect("127.0.0.1", 8888)) {
        std::cout << "Connected!" << std::endl;
    }
    else {
        std::cout << "Connection Failed!" << std::endl;
    }
}

void Game::clean()
{
    if(currentScene!=nullptr)
    {
        currentScene->clean();
     
    }

    if(nearStar.texture!=nullptr)
    {
        SDL_DestroyTexture(nearStar.texture);
    }
    if(farStar.texture!=nullptr)
    {
        SDL_DestroyTexture(farStar.texture); 
    }
    if(titleFont!=nullptr)
    {
        TTF_CloseFont(titleFont);
    }
    if(textFont!=nullptr)
    {
        TTF_CloseFont(textFont);
    }
    //清理SDL_image
    IMG_Quit();
    //关闭音频设备
    Mix_CloseAudio();

    //清理SDL_mixer
    Mix_Quit();

    //清理SDL_ttf
    TTF_Quit();

    
    //清理SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::changeScene(Scene *scene)
{
    if(currentScene==nullptr)
    {
        currentScene->clean();
        delete currentScene;
        
    }
    currentScene=scene;
    currentScene->init();
}

void Game::handleEvent(SDL_Event *event)
{
    while(SDL_PollEvent(event))
    {
        if(event->type==SDL_QUIT)
        {
            isRunning=false;
        } 
        if(event->type==SDL_KEYDOWN)
        {
            if(event->key.keysym.scancode==SDL_SCANCODE_F4)
            {
                isFullScreen=!isFullScreen;
                if(isFullScreen)
                {
                    SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN);
                }
                else
                {
                    SDL_SetWindowFullscreen(window,0);
                }
                
            }
        }

        currentScene->handleEvent(event);
    }
}

void Game::update(float deltaTime)
{
    backgroundUpdate(deltaTime);

    currentScene->update( deltaTime);
}

void Game::render()
{
    //清空
    SDL_RenderClear(renderer);
    //渲染星空背景
    backgroundRender();
    

    currentScene->render();

    //显示更新
    SDL_RenderPresent(renderer);
}

SDL_Point Game::renderTextCentered(const std::string text, float posY, bool isTitle)
{
    SDL_Color color={255,255,255,255};
    SDL_Surface* surface;
    if(isTitle)
    {
      surface=TTF_RenderUTF8_Solid(titleFont,text.c_str(),color);
      
    }
    else
    {
       surface=TTF_RenderUTF8_Solid(textFont,text.c_str(),color);
    }
    SDL_Texture* texture=SDL_CreateTextureFromSurface(renderer,surface);
    int y =static_cast<int> ((getWindowHeight()-surface->h)*posY);
    SDL_Rect dstRect={getWindowWidth()/2-surface->w/2,y,surface->w,surface->h};
    SDL_RenderCopy(renderer,texture,nullptr,&dstRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    return {dstRect.x+dstRect.w,dstRect.y};

}

void Game::renderTextPosition(std::string text, int posX, int posY, bool isLeft)
{
    SDL_Color color={255,255,255,255};
    SDL_Surface* surface=TTF_RenderUTF8_Solid(textFont,text.c_str(),color);
    SDL_Texture* texture=SDL_CreateTextureFromSurface(renderer,surface);
    SDL_Rect dstRect;
    if(isLeft)
    {
         dstRect={posX,posY,surface->w,surface->h};
    }
    else{
        dstRect={getWindowWidth()-  posX-surface->w,posY,surface->w,surface->h};
    }

   
    SDL_RenderCopy(renderer,texture,nullptr,&dstRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void Game::backgroundUpdate(float deltaTime)
{
    nearStar.offset+=nearStar.speed*deltaTime;
    if(nearStar.offset>=0)
    {
       nearStar.offset-=nearStar.height; 
    }

    farStar.offset+=farStar.speed*deltaTime;
    if(farStar.offset>=0)
    {
        farStar.offset-=farStar.height;
    }


}

void Game::backgroundRender()
{

//渲染远处星空背景
    

for(int posY= static_cast<int>( farStar.offset);posY<windowHeight;posY+=farStar.height)
{
   
    for(int posX=0;posX<windowWidth;posX+=farStar.width)
    {
        SDL_Rect srcRect={posX,posY,farStar.width,farStar.height}; 
        SDL_RenderCopy(renderer,farStar.texture,nullptr,&srcRect);
    }
}



    //渲染近处星空背景
    
    for(int posY= static_cast<int>( nearStar.offset);posY<windowHeight;posY+=nearStar.height)
    {
        for(int posX=0;posX<windowWidth;posX+=nearStar.width)
        {
            SDL_Rect srcRect={posX,posY,nearStar.width,nearStar.height}; 
            SDL_RenderCopy(renderer,nearStar.texture,nullptr,&srcRect);
        }
    }

}

void Game::saveData()
{
    std::ofstream file("assets/save.dat");
    if(!file.is_open())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not open file: %s\n",SDL_GetError());
       return;
    }
    for(auto &item:leaderBoard)
    {
        file<<item.first<<" "<<item.second<<std::endl;
    }
}

void Game::loadData()
{
    std::ifstream file("assets/save.dat");
    if(!file.is_open())
    {
        SDL_Log("Could not open file: ");
       return;
    }
    leaderBoard.clear();
    int score;
    std::string name;
    while(file>>score>>name)
    {
        leaderBoard.insert(make_pair(score,name));
    }

}

void Game::insertLeaderBoard(int score, std::string name)
{
    leaderBoard.insert(make_pair(score,name));
    if(leaderBoard.size()>8)
    {
        leaderBoard.erase(--leaderBoard.end());
    }
}
