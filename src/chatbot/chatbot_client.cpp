
#include <curl/curl.h>
#include <iostream>
#include "chatbot_client.h"
#include "env_utils.h"

using namespace std;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t totalSize = size * nmemb;
    string *str = static_cast<string *>(userp);
    str->append(static_cast<char *>(contents), totalSize);
    return totalSize;
}

ChatBotClient::ChatBotClient()
    : _apiKey(env::get_env_or_default("GEMINI_API_KEY", "")),
      _modelName(env::get_env_or_default("GEMINI_MODEL", "gemini-2.0-flash")),
      _baseUrl("https://generativelanguage.googleapis.com/v1beta/models/" + _modelName + ":generateContent?key=" + _apiKey) {

      };

ChatBotResponseForUser ChatBotClient::GeneratedContent(const string &pretext, const string &atext, const string &posttext = "")
{
    // Yeni kullanıcı istemini 'user' rolüyle geçmişe ekle
    string text  = pretext+atext+posttext;
    nlohmann::json userMessage = {
        {"role", "user"},
        {"parts", nlohmann::json::array({
            nlohmann::json::object({{"text", text}})
        })}
    };
    _conversationHistory.push_back(userMessage);

    // API isteği için JSON yapısını oluştur
    // 'contents' alanına tüm konuşma geçmişini ekle
    nlohmann::json jRequest = {
        {"model", _modelName},
        {"contents", nlohmann::json::array()} // Buraya geçmiş eklenecek
    };

    // Tüm geçmişi jRequest'in "contents" dizisine ekle
    for (const auto& message : _conversationHistory) {
        jRequest["contents"].push_back(message);
    }

    ChatBotResponse cbresponse = PerformRequest(jRequest);

    if (cbresponse.error == ChatBotError::None)
    {
        // Modelin yanıtını 'model' rolüyle geçmişe ekle
        nlohmann::json modelMessage = {
            {"role", "model"},
            {"parts", nlohmann::json::array({
                nlohmann::json::object({{"text", cbresponse.generatedText}})
            })}
        };
        _conversationHistory.push_back(modelMessage);

        return ChatBotResponseForUser{cbresponse.generatedText, false};
    }
    else
    {
        // Hata durumunda geçmişe ekleme yapmayabiliriz veya hata mesajını ekleyebiliriz
        // Bu tamamen uygulamanızın mantığına bağlı
        // Örneğin: _conversationHistory.push_back({{"role", "error"}, {"parts", nlohmann::json::array({nlohmann::json::object({{"text", cbresponse.rawResponse}})})}});
        return ChatBotResponseForUser{cbresponse.rawResponse, true};
    }
}

ChatBotResponse ChatBotClient::PerformRequest(const nlohmann::json &jRequest)
{
    ChatBotResponse result;
    result.error = ChatBotError::None;
    result.generatedText = "";
    result.rawResponse = "";

    // cURL'u başlat
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        result.error = ChatBotError::CurlInitFailed;
        return result;
    }

    string model = jRequest["model"].get<string>();

    string requestBody = jRequest.dump();

    curl_easy_setopt(curl, CURLOPT_URL, _baseUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestBody.size());

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    string responseString;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        result.error = ChatBotError::RequestFailed;
        result.rawResponse = curl_easy_strerror(res);
    }
    else
    {
        result.rawResponse = responseString;
        try
        {
            auto jResponse = nlohmann::json::parse(responseString);
            if (jResponse.contains("candidates") && jResponse["candidates"].is_array())
            {
                auto &candidate0 = jResponse["candidates"][0];
                if (candidate0.contains("content") && candidate0["content"].contains("parts") && candidate0["content"]["parts"].is_array() && !candidate0["content"]["parts"].empty() && candidate0["content"]["parts"][0].contains("text"))
                {
                    result.generatedText = candidate0["content"]["parts"][0]["text"].get<string>();
                }
                else
                {
                    result.error = ChatBotError::UnexpectedResponseFormat;
                }
            }
            else
            {
                result.error = ChatBotError::UnexpectedResponseFormat;
            }
        }
        catch (const exception &e)
        {
            result.error = ChatBotError::JsonParseError;
            result.rawResponse = string("JSON parse hatası: ") + e.what();
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}
