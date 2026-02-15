#include "SceneEnd.h"
#include"SceneMain.h"
#include "Game.h"
#include <string>

void SceneEnd::clean()
{
    if(bgm!=nullptr)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(bgm);
        bgm=nullptr;
    }
}

void SceneEnd::handleEvent(SDL_Event *event)
{
    if(isTyping)
    {
        if(event->type==SDL_TEXTINPUT)
        {
            name+=event->text.text;
        }
        if(event->type==SDL_KEYDOWN)
        {
            if(event->key.keysym.scancode==SDL_SCANCODE_RETURN )
            {
                isTyping=false;
                SDL_StopTextInput();
                if(name =="")
                {
                    name="NoName";
                }
              game.insertLeaderBoard(game.getScore(),name);
            }
            if(event->key.keysym.scancode==SDL_SCANCODE_BACKSPACE)
            {
                
                    removeLastUTF8Char(name);
                
            }
    
        }
    }
    else{
        //TODO
        if(event->type==SDL_KEYDOWN)
        {
            if(event->key.keysym.scancode==SDL_SCANCODE_SPACE)
            {
              auto sceneMain = new SceneMain();
              game.changeScene(sceneMain);
            }
        }
    }
}

void SceneEnd::init()
{
    //载入背景音乐
    bgm=Mix_LoadMUS("assets/music/06_Battle_in_Space_Intro.ogg");
    if(bgm==nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not load music: %s\n",Mix_GetError());
    }

    //播放背景音乐
    Mix_PlayMusic(bgm,-1);
    if(!SDL_IsTextInputActive())
    {
        SDL_StartTextInput(); 
    }
        if(!SDL_IsTextInputActive())
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Could not start text input: %s\n",SDL_GetError());
       
        }
    
}

void SceneEnd::render()
{
    if(isTyping)
    {
       renderPhase1();
    }
    else{
        renderPhase2();
    }
}

void SceneEnd::update(float deltaTime)
{
    blinkTimer-=deltaTime;
    if(blinkTimer<=0.0f)
    {
        blinkTimer+=1.0f;
    }
}

void SceneEnd::removeLastUTF8Char(std::string& str)
{
    if(str.empty())
    {
        return;
    }
    auto lastChar = str.back();
    if((lastChar & 0b10000000 ) == 0b10000000)//判断是否为中文后续字节
    {
        str.pop_back();
        while((str.back() & 0b11000000 )!= 0b11000000)//判断是否为中文开头字节
        {
            str.pop_back();
        }
    }
    str.pop_back();
}

void SceneEnd::renderPhase1()
{
    std::string scoreText = "你的得分是： " + std::to_string(game.getScore());
    std::string endText = "GAME OVER!";
    std::string restartText = "按下空格重新开始游戏";//"按下空格重新开始游戏";

    game.renderTextCentered(scoreText, 0.1f, false);
    game.renderTextCentered(endText, 0.4f, true);
    game.renderTextCentered(restartText, 0.6f, false);


    if(name!="")
    {
      SDL_Point p=  game.renderTextCentered(name,0.8f,false);
      if(blinkTimer<0.5f)
        game.renderTextPosition("_",p.x,p.y);
    }
    else{
        if(blinkTimer<0.5f)
        game.renderTextCentered("_",0.8f,false);
    }
}

void SceneEnd::renderPhase2()
{
    game.renderTextCentered("rank",0.05f,true);
    auto posy=0.2f*game.getWindowHeight();
    int i = 1;
    for(auto item :game.getLeaderBoard())
    {
        std::string name = std::to_string (i)+". "+ item.second;
        std::string score = std::to_string (item.first);
        game.renderTextPosition(name,100,posy);
        game.renderTextPosition(score,100,posy,false);
        posy+=45;
        i++;
    }
    if(blinkTimer<0.5f){
    game.renderTextCentered("按下空格重新开始游戏",0.8f,false);
       // game.renderTextCentered("start", 0.8f, false);
    }
}
