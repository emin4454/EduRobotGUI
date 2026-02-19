#pragma once
#include "audio_visualizer.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

enum class DisplayMode
{
    SHOW_IMAGE,
    SHOW_GIF,
    SHOW_AUDIO_VISUALIZATION
};

class DisplayThread
{
public:
    explicit DisplayThread(const std::string &assetsPath,
                           int winW = 960, int winH = 720);
    ~DisplayThread();

    void start();
    void stop();

    /* dış komutlar (thread-safe) */
    void setState(const std::string &stateName);
    void setMode(DisplayMode m);
    void showBase64Images(const std::vector<std::string> &);
    void setStatus(const std::string &txt);
    void ShowMicImage();
    void RemoveMicImage();
    /* FFT görselleştiriciye erişim (capture thread için) */
    AudioVisualizer audioVisualizer_;
    SDL_Window *window_{nullptr};
    SDL_Renderer *renderer_{nullptr};
    TTF_Font *font_{nullptr};
    const std::string assets_;
    bool b_pressed = false;
        bool isReady =false;
private:

    void runLoop();
    bool initAudio();
    static SDL_Surface *surfaceFromBase64(const std::string &b64);

    struct SDLTextureDeleter
    {
        void operator()(SDL_Texture *t) const
        {
            if (t)
                SDL_DestroyTexture(t);
        }
    };
    using TextureUPtr = std::unique_ptr<SDL_Texture, SDLTextureDeleter>;



    const int winW_;
    const int winH_;


    std::thread thread_;
    std::atomic<bool> running_{false};


    std::mutex stateMtx_;
    std::string pendingState_;
    std::string activeState_;

    std::mutex modeMtx_;
    DisplayMode pendingMode_{DisplayMode::SHOW_GIF};
    DisplayMode mode_{DisplayMode::SHOW_GIF};


    std::mutex imageMtx_;
    std::vector<std::string> pendingBase64_;
    std::vector<SDL_Texture *> imageTextures_;

    /* status overlay */
    std::mutex statusMtx_;
    std::string pendingStatus_;
    std::string statusText_;
    SDL_Texture *statusTex_;
    int statusW_{0}, statusH_{0};

    /* SDL objeleri */
    SDL_Texture *microphoneTex_;
    bool MicActivated = false;

};
