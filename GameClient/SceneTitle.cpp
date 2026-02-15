#include"SceneTitle.h"
#include"Game.h"
#include"SceneMain.h"
#include<string>

void SceneTitle::clean()
{
    if(bgm!=nullptr)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(bgm);
        bgm=nullptr;
    }
}

void SceneTitle::handleEvent(SDL_Event *event)
{
    if(event->type==SDL_KEYDOWN)
    {
        if(event->key.keysym.sym==SDLK_SPACE)
        {
            game.changeScene(new SceneMain());
        }
    }
}

void SceneTitle::init()
{   
    //载入背景音乐
    bgm=Mix_LoadMUS("assets/music/06_Battle_in_Space_Intro.ogg");
    if(bgm==nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not load music: %s\n",Mix_GetError()); 
    }
    //播放背景音乐
    Mix_PlayMusic(bgm,-1);


}

void SceneTitle::render()
{
    //渲染标题文字
    std::string title= "SDL 太空战机" ;
    game.renderTextCentered(title,0.4f,true);
    if(timer<0.5f){
    game.renderTextCentered("start",0.8f,false);
    }
}



void SceneTitle::update(float deltaTime)
{
    timer+=deltaTime;
    if(timer>1.0f)
    {
        timer-=1.0f; 
    }
}

