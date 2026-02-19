

#include "tts_client.h"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <curl/curl.h>
#include <SDL2/SDL_mixer.h>
#include "nlohmann/json.hpp"
#include <chrono>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <base64.h>
#include <globals.h>
#include <iostream>
#include "env_utils.h"
// Statik callback fonksiyonunun implementasyonu
size_t TTSClient::write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    try
    {
        static_cast<std::string *>(userp)->append(static_cast<char *>(contents), size * nmemb);
        return size * nmemb;
    }
    catch (const std::bad_alloc &e)
    {
        std::cerr << "Bellek tahsisi hatası: " << e.what() << std::endl;
        return 0;
    }
}

TTSClient::TTSClient()
    : api_key_(env::get_env_or_default("ELEVENLABS_API_KEY", "")),
      voice_id_(env::get_env_or_default("ELEVENLABS_VOICE_ID", "ZgndNiUCwSn9oAFrs6rJ")),
      model_id_(env::get_env_or_default("ELEVENLABS_MODEL_ID", "eleven_multilingual_v2")),
      stability_(0.5f),
      similarity_boost_(0.75f)
{
    if (api_key_.empty())
    {
        throw std::invalid_argument("API anahtarı boş olamaz.");
    }
}

void TTSClient::set_voice_id(const std::string &voice_id)
{
    voice_id_ = voice_id;
}

void TTSClient::set_voice_settings(float stability, float similarity_boost)
{
    stability_ = stability;
    similarity_boost_ = similarity_boost;
}

void TTSClient::set_model_id(const std::string &model_id)
{
    model_id_ = model_id;
}

bool TTSClient::synthesize(const std::string &text)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Hata: curl_easy_init() başarısız oldu." << std::endl;
        return false;
    }

    std::string response_data;
    const std::string url = "https://api.elevenlabs.io/v1/text-to-speech/" + voice_id_;

    // JSON verisini nlohmann/json kütüphanesi ile güvenli ve kolay bir şekilde oluştur
    nlohmann::json json_payload;
    json_payload["text"] = text;
    json_payload["model_id"] = model_id_;
    json_payload["voice_settings"] = {
        {"stability", stability_},
        {"similarity_boost", similarity_boost_},
        {"speed" , 0.95f}};

    // JSON nesnesini string formatına dönüştür
    const std::string payload_str = json_payload.dump();

    struct curl_slist *headers = NULL;
    CURLcode res;
    bool success = true;

    try
    {
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string api_header = "xi-api-key: " + api_key_;
        headers = curl_slist_append(headers, api_header.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            throw std::runtime_error(std::string("curl_easy_perform() başarısız: ") + curl_easy_strerror(res));
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200)
        {
            throw std::runtime_error("HTTP isteği başarısız oldu. Sunucu kodu: " + std::to_string(http_code) + "\nSunucu Mesajı: " + response_data);
        }
        play_sound_from_binary(response_data);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Sentezleme hatası: " << e.what() << std::endl;
        success = false;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return success;
}
void TTSClient::PlaySoundFromFile(const std::string &fileName)
{
    std::filesystem::path soundPath = gDisplay->assets_;
    soundPath /= "sounds";
    soundPath /= fileName;

    if (!std::filesystem::exists(soundPath))
    {
        std::cerr << "Sound file not found: " << soundPath << '\n';
        return;
    }

    std::ifstream f(soundPath, std::ios::binary | std::ios::ate);
    if (!f)
    {
        std::cerr << "Couldn't open " << soundPath << '\n';
        return;
    }
    const std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);

    std::string audioBin(static_cast<std::size_t>(size), '\0');
    if (!f.read(audioBin.data(), size))
    {
        std::cerr << "Failed to read " << soundPath << '\n';
        return;
    }
    play_sound_from_binary(audioBin);
}
void TTSClient::play_sound_from_binary(const std::string &audio_bin)
{
    static std::vector<uint8_t> audioBuf;

    audioBuf.assign(audio_bin.begin(), audio_bin.end());

    SDL_RWops *rw = SDL_RWFromConstMem(audioBuf.data(), audioBuf.size());
    if (!rw)
    {
        std::cerr << SDL_GetError() << '\n';
        return;
    }

    Mix_Music *mus = Mix_LoadMUS_RW(rw, 1);
    if (!mus)
    {
        std::cerr << Mix_GetError() << '\n';
        return;
    }

    Mix_PlayMusic(mus, 1);
    while (Mix_PlayingMusic()){
        SDL_Delay(10);
        if(gDisplay->b_pressed){
            gDisplay->b_pressed = false;
            break;
        }
    }

    Mix_FreeMusic(mus);
}
