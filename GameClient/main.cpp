#include <iostream>
#define SDL_MAIN_HANDLED
#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<SDL2/SDL_mixer.h>
#include<SDL2/SDL_ttf.h>
#include "Game.h"
int main(int , char** )
{
    
    Game& game=Game::GetInstance();
    game.init();
    game.run();
   
    return 0;
}
