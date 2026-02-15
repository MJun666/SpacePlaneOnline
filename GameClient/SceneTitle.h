#ifndef SCENETITLE_H
#define SCENETITLE_H
#include"Scene.h"
#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<SDL2/SDL_ttf.h>
#include<SDL2/SDL_mixer.h>
class SceneTitle : public Scene
{
public:
   
    void clean() override;
    void handleEvent(SDL_Event* event) override;
    void init() override;
    void render() override;
    void update(float deltaTime) override;

private:
    Mix_Music* bgm;//背景音乐
    float timer = 0.0f;
};















#endif