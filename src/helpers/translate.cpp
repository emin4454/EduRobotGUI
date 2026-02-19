#include "translate.h"
#include <iostream>
#include <curl/curl.h> // libcurl kütüphanesi
#include <algorithm>   // std::remove_if için
#include "env_utils.h"
// nlohmann/json için gerekli
using json = nlohmann::json;

// Statik WriteCallback fonksiyonu Curl yanıtını işlemek için
size_t Translate::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

Translate::Translate()
{
    // Curl kütüphanesini başlatma
    api_key_ = env::get_env_or_default("GOOGLE_TRANSLATE_API_KEY", "");
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

std::vector<std::string> Translate::translate(const std::vector<std::string> &texts,
                                              const std::string &source_lang,
                                              const std::string &target_lang)
{
    std::vector<std::string> translated_texts;
    CURL *curl;
    CURLcode res;
    std::string read_buffer;

    curl = curl_easy_init();
    if (curl)
    {
        // API URL'sini ve anahtarı ayarla
        std::string full_url = api_url_ + "?key=" + api_key_;
        curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());

        // Yanıtı işlemek için callback fonksiyonunu ayarla
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Translate::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

        // POST verilerini oluştur
        json post_data;
        post_data["q"] = texts;
        post_data["source"] = source_lang;
        post_data["target"] = target_lang;
        post_data["format"] = "text";

        std::string post_fields = post_data.dump();

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

        // HTTP başlıklarını ayarla
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // İsteği gerçekleştir
        res = curl_easy_perform(curl);

        // Hata kontrolü
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else
        {
            // JSON yanıtını ayrıştır
            try
            {
                auto json_response = json::parse(read_buffer);
                if (json_response.count("data") && json_response["data"].count("translations"))
                {
                    for (const auto &translation_item : json_response["data"]["translations"])
                    {
                        if (translation_item.count("translatedText"))
                        {
                            translated_texts.push_back(translation_item["translatedText"].get<std::string>());
                        }
                    }
                }
                else if (json_response.count("error"))
                {
                    std::cerr << "API Error: " << json_response["error"]["message"] << std::endl;
                }
            }
            catch (const json::parse_error &e)
            {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
                std::cerr << "Received data: " << read_buffer << std::endl;
            }
        }

        // Belleği temizle
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return translated_texts;
}
std::string Translate::translate_one_sentence(const std::string &question)
{
    std::vector<std::string> questions;
    questions.push_back(question);
    auto response = translate(questions, "tr", "en");
    return response[0];
}
std::string Translate::translate_one_sentence_turkish(const std::string &question)
{
    std::vector<std::string> questions;
    questions.push_back(question);
    auto response = translate(questions, "en", "tr");
    return response[0];
}
