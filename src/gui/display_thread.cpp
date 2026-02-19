#include "animator.h" // kendi Animator sınıfınız
#include "display_thread.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <base64.h>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include "edu_robot.h"
static SDL_Texture *loadTextureFromBase64(const std::string &b64, SDL_Renderer *ren)
{
    try
    {
        std::string raw;
        Base64::Decode(b64, raw);
        SDL_RWops *rw = SDL_RWFromConstMem(raw.data(), raw.size());
        if (!rw)
            throw std::runtime_error(SDL_GetError());
        SDL_Surface *surf = IMG_Load_RW(rw, 1);
        if (!surf)
            throw std::runtime_error(IMG_GetError());
        SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FreeSurface(surf);
        if (!tex)
            throw std::runtime_error(SDL_GetError());
        return tex;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Base64->Texture error: " << e.what() << "\n";
        return nullptr;
    }
}
static SDL_Texture *loadTextureFromPNG(const std::filesystem::path &pngPath,
                                       SDL_Renderer *ren)
{
    try
    {
        SDL_Surface *surf = IMG_Load(pngPath.string().c_str());
        if (!surf)
            throw std::runtime_error(IMG_GetError());

        SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FreeSurface(surf);
        if (!tex)
            throw std::runtime_error(SDL_GetError());

        return tex;
    }
    catch (const std::exception &e)
    {
        std::cerr << "PNG->Texture error (" << pngPath << "): "
                  << e.what() << '\n';
        return nullptr;
    }
}

static SDL_Texture *loadTextureFromText(const std::string &text, SDL_Renderer *ren, TTF_Font *font)
{
    try
    {
        if (!font || !ren)
            std::cout << "font veya renderer tanimli degil" << std::endl;
        SDL_Color white{255, 255, 255, 255};
        SDL_Surface *text_surf = TTF_RenderUTF8_Solid_Wrapped(font, text.c_str(), white, 0);
        if (!text_surf)
        {
            std::cout << "text surf hatasi" << std::endl;
        }
        SDL_Texture *text_texture = SDL_CreateTextureFromSurface(ren, text_surf);
        SDL_FreeSurface(text_surf);
        if (!text_surf)
            std::cout << "cant get surface";
        return text_texture;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Font Texture Error " << e.what() << "\n";
        return nullptr;
    }
}

DisplayThread::DisplayThread(const std::string &assets, int w, int h)
    : assets_(assets), winW_(w), winH_(h) {}

DisplayThread::~DisplayThread() { stop(); }

void DisplayThread::start()
{
    if (running_)
        return;
    running_ = true;
    thread_ = std::thread(&DisplayThread::runLoop, this);
}

void DisplayThread::stop()
{
    if (!running_)
        return;
    running_ = false;
    if (thread_.joinable())
        thread_.join();
}
void DisplayThread::ShowMicImage()
{
    MicActivated = true;
}
void DisplayThread::RemoveMicImage()
{
    MicActivated = false;
}
void DisplayThread::setState(const std::string &s)
{
    std::lock_guard lk(stateMtx_);
    pendingState_ = s;
}

void DisplayThread::setMode(DisplayMode m)
{
    std::lock_guard lk(modeMtx_);
    pendingMode_ = m;
}
void DisplayThread::setStatus(const std::string &txt)
{
    std::lock_guard lk(statusMtx_);
    pendingStatus_ = txt;
}
void DisplayThread::showBase64Images(
    const std::vector<std::string> &base64List)
{
    std::lock_guard<std::mutex> lk(imageMtx_);
    pendingBase64_ = base64List;
}

bool DisplayThread::initAudio()
{
    if (Mix_OpenAudio(48'000, MIX_DEFAULT_FORMAT, 2, 4096) < 0)
    {
        std::cerr << "Mix_OpenAudio: " << Mix_GetError() << '\n';
        return false;
    }
    return true;
}

void DisplayThread::runLoop()
{
    initAudio();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        std::cerr << SDL_GetError() << '\n';
        return;
    }
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    if (TTF_Init() < 0)
    {
        std::cerr << TTF_GetError() << '\n';
        return;
    }
    window_ =
        SDL_CreateWindow("Robot", SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
                         winW_, winH_, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    std::filesystem::path fontPath = assets_;
    fontPath /= "font/Roboto-Medium.ttf";
    font_ = TTF_OpenFont(fontPath.string().c_str(), 20);
    if (!font_)
    {
        throw std::runtime_error("Hiç font bulunamadı");
    }
    int screenW, screenH;
    SDL_GetWindowSize(window_, &screenW, &screenH);
    Animator animator(renderer_, assets_);
    animator.loadAllStates();
    animator.setState("sleep");
    activeState_ = "sleep";
    std::filesystem::path microphoneImagePath = assets_;
    microphoneImagePath /= "ui/microphone.png";
    microphoneTex_ = loadTextureFromPNG(microphoneImagePath, renderer_);
    isReady = true;
    SDL_Event ev;
    while (running_)
    {
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
                running_ = false;
            if (!b_pressed && ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_b)
            {
                b_pressed = true;
            }
        }
        {
            std::lock_guard lk(modeMtx_);
            if (pendingMode_ != mode_)
                mode_ = pendingMode_;
        }

        {
            std::lock_guard lk(stateMtx_);
            if (!pendingState_.empty())
            {
                animator.setState(pendingState_);
                activeState_.swap(pendingState_);
                pendingState_.clear();
            }
        }

        {
            std::lock_guard<std::mutex> lk(imageMtx_);
            if (!pendingBase64_.empty())
            {

                for (auto *t : imageTextures_)
                    SDL_DestroyTexture(t);
                imageTextures_.clear();

                for (auto &b64 : pendingBase64_)
                {
                    if (auto *tex = loadTextureFromBase64(b64, renderer_))
                        imageTextures_.push_back(tex);
                }
                pendingBase64_.clear();
            }
        }

        {
            std::lock_guard<std::mutex> lk(statusMtx_);
            if (!pendingStatus_.empty())
            {
                if (statusText_ != pendingStatus_)
                {
                    statusText_ = pendingStatus_;
                    if (statusTex_)
                    {
                        SDL_DestroyTexture(statusTex_);
                    }
                    statusTex_ = loadTextureFromText(statusText_, renderer_, font_);
                }
            }
        }
        switch (mode_)
        {
        case DisplayMode::SHOW_GIF:
            animator.update();
            break;
        case DisplayMode::SHOW_AUDIO_VISUALIZATION:
            audioVisualizer_.update();
            break;
        case DisplayMode::SHOW_IMAGE:
        default:
            break;
        }
        SDL_RenderClear(renderer_);

        switch (mode_)
        {
        case DisplayMode::SHOW_GIF:
            animator.render(0, 0, screenW, screenH);
            break;
        case DisplayMode::SHOW_IMAGE:
        {
            for (auto *tex : imageTextures_)
            {
                SDL_RenderCopy(renderer_, tex, nullptr, nullptr);
            }
        }
        break;
        case DisplayMode::SHOW_AUDIO_VISUALIZATION:
            audioVisualizer_.render(renderer_, screenW, screenH);
            break;
        }

        {
            std::lock_guard lk(statusMtx_);
            if (statusTex_)
            {
                int tw, th;
                SDL_QueryTexture(statusTex_, nullptr, nullptr, &tw, &th);
                SDL_Rect dst{(screenW - tw) / 2, screenH - th - 20, tw, th};
                SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 120);
                SDL_Rect bg{dst.x - 8,  // sol   (x koordinatı)
                            dst.y - 4,  // üst   (y koordinatı)
                            dst.w + 16, // genişlik  (w)
                            dst.h + 8};
                SDL_RenderFillRect(renderer_, &bg);
                SDL_RenderCopy(renderer_, statusTex_, nullptr, &dst);
            }
        }

        if (MicActivated)
        {
            int tw, th;
            SDL_Rect dst{screenW - 150, 150, 100, 100};
            SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
            SDL_Rect bg{dst.x - 4, // sol   (x koordinatı)
                        dst.y - 2, // üst   (y koordinatı)
                        dst.w + 8, // genişlik  (w)
                        dst.h + 4};
            SDL_RenderFillRect(renderer_, &bg);
            SDL_RenderCopy(renderer_, microphoneTex_, nullptr, &dst);
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 120);
        }

        SDL_RenderPresent(renderer_);

        const Uint32 delayMs = 16;
        SDL_Delay(delayMs);
    }
    SDL_DestroyTexture(statusTex_);
    if (font_)
    {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
    TTF_Quit();
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    IMG_Quit();
    Mix_CloseAudio();
    SDL_Quit();
}
