#ifndef GAME_H
#define GAME_H
#include"Scene.h"

#include"Object.h"
#include<map>
#include<string>
#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<SDL2/SDL_ttf.h>
#include<SDL2/SDL_mixer.h>
class Game
{
public:
   static Game& GetInstance()
   {
       static Game instance;
       return instance;
   }
    ~Game();
    void run();
    void init();
    void clean();
    void changeScene(Scene* scene);
    void handleEvent(SDL_Event* event);
    void update(float deltaTime);
    void render();
   //渲染文字函数
   SDL_Point renderTextCentered( std::string text, float posY,bool isTitle);
   void renderTextPosition(std::string text,int posX,int posY,bool isLeft =true);

   //getter
    SDL_Window* getWindow() {return window;}
    SDL_Renderer* getRenderer() {return renderer;}
    int getWindowWidth() {return windowWidth;}
    int getWindowHeight() {return windowHeight;}

    int getScore() {return finalScore;}
    void setScore(int score) {finalScore=score;}
    std::multimap<int,std::string,std::greater<int>>& getLeaderBoard() {return leaderBoard;}
    void insertLeaderBoard(int score,std::string name);
private:
    Game();
    //删除拷贝构造函数和赋值运算符
    Game(const Game&)=delete;
    Game& operator=(const Game&)=delete;
    bool isRunning=true;
    bool isFullScreen=false;
    Scene* currentScene=nullptr;
    SDL_Window* window=nullptr;
    SDL_Renderer* renderer=nullptr;
    int windowWidth=600;
    int windowHeight=800;
    int FPS=60;
    Uint32 frameTime;
    float deltaTime;
    TTF_Font* titleFont;
    TTF_Font* textFont;
   int finalScore=0;


   Background nearStar;
   Background farStar;

   std::multimap<int,std::string,std::greater<int>> leaderBoard;

   void backgroundUpdate(float deltaTime);
   void backgroundRender();
   void saveData();
   void loadData();

};



#endif