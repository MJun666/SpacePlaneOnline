#ifndef SCENEEND_H
#define SCENEEND_H
#include"Scene.h"
#include<string>
#include<SDL2/SDL_mixer.h>
class SceneEnd : public Scene{
public:
	void clean() ;
	void handleEvent(SDL_Event* event) ;
	void init() ;
	void render() ;
	void update(float deltaTime) ;





private:
	bool isTyping=true;
	std::string name="";
	float blinkTimer=1.0f;
	Mix_Music* bgm;


	void removeLastUTF8Char(std::string& str);
	void renderPhase1();
	void renderPhase2();
};

















#endif