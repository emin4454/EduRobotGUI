#include "animator.h"
#include <SDL2/SDL_image.h>
#include <filesystem>
#include <iostream>
#include <algorithm>
namespace fs = std::filesystem;

Animator::Animator(SDL_Renderer* renderer, const std::string& assets_base_path)
    : renderer_(renderer), base_path_(assets_base_path)
{}

Animator::~Animator() {
    for (auto& [name, anim] : states_) {
        for (auto* tex : anim.frames) {
            SDL_DestroyTexture(tex);
        }
    }
}

bool Animator::loadAllStates() {
    fs::path eyes_dir = fs::path(base_path_) / "eyes";
    if (!fs::exists(eyes_dir) || !fs::is_directory(eyes_dir)) {
        std::cerr << "Assets dizini bulunamadı: " << eyes_dir << "\n";
        return false;
    }

    for (auto& entry : fs::directory_iterator(eyes_dir)) {
        if (entry.is_directory()) {
            std::string state = entry.path().filename().string();
            if (!loadStateFromFolder(state)) {
                std::cerr << "Durum yüklenemedi: " << state << "\n";
            }
        }
    }
    return true;
}

bool Animator::loadStateFromFolder(const std::string& state_name) {
    Animation anim;
    fs::path folder = fs::path(base_path_) / "eyes" / state_name;

    // .png dosyalarını alfabetik sırada al
    std::vector<fs::path> files;
    for (auto& f : fs::directory_iterator(folder)) {
        if (f.path().extension() == ".png"
    || f.path().extension() == ".jpg")
            files.push_back(f.path());
    }
    std::sort(files.begin(), files.end());

    for (auto& img_path : files) {
        SDL_Texture* tex = loadTexture(img_path.string());
        if (tex) anim.frames.push_back(tex);
    }

    if (anim.frames.empty()) return false;
    states_[state_name] = std::move(anim);
    return true;
}

SDL_Texture* Animator::loadTexture(const std::string& file) {
    SDL_Surface* surf = IMG_Load(file.c_str());
    if (!surf) {
        std::cerr << "Resim yüklenemedi: " << file << "  " << IMG_GetError() << "\n";
        return nullptr;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_FreeSurface(surf);
    return tex;
}

bool Animator::setState(const std::string& state_name) {
    auto it = states_.find(state_name);
    if (it == states_.end()) {
        std::cerr << "Böyle bir durum yok: " << state_name << "\n";
        return false;
    }

    current_anim_ = &it->second;
    current_anim_->current_frame = 0;
    current_anim_->last_tick = SDL_GetTicks();
    return true;
}

void Animator::update() {
    if (!current_anim_) return;
    Uint32 now = SDL_GetTicks();
    if (now - current_anim_->last_tick >= current_anim_->frame_delay_ms) {
        current_anim_->last_tick = now;
        current_anim_->current_frame++;
        if (current_anim_->current_frame >= (int)current_anim_->frames.size()) {
            if (current_anim_->loop)
                current_anim_->current_frame = 0;
            else
                current_anim_->current_frame = (int)current_anim_->frames.size() - 1;
        }
    }
}

void Animator::render(int x, int y, int w, int h) {
    if (!current_anim_) return;
    SDL_Rect dst{ x, y, w, h };
    SDL_Texture* tex = current_anim_->frames[current_anim_->current_frame];
    SDL_RenderCopy(renderer_, tex, nullptr, &dst);
}
