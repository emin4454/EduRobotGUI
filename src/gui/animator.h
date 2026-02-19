#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <map>

struct Animation {
    std::vector<SDL_Texture*> frames;
    int current_frame = 0;
    Uint32 frame_delay_ms = 1000/24;    // kareler arası gecikme
    Uint32 last_tick = 0;
    bool loop = true;
    
};

class Animator {
public:
    Animator(SDL_Renderer* renderer, const std::string& assets_base_path);
    ~Animator();

  
    bool loadAllStates();

    
    bool setState(const std::string& state_name);

   
    void update();

   
    void render(int x, int y, int w, int h);

private:
    SDL_Renderer* renderer_;
    std::string base_path_;
    std::map<std::string, Animation> states_;
    Animation* current_anim_ = nullptr;

   
    bool loadStateFromFolder(const std::string& state_name);
    SDL_Texture* loadTexture(const std::string& file);
};

