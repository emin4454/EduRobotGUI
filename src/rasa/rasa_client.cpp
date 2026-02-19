#include <nlohmann/json.hpp>
#include <iostream>
#include "rasa_client.h"
#include "globals.h"
#include "env_utils.h"
using namespace std;

using json = nlohmann::json;

namespace
{
std::string BuildRasaUrl(RasaRequestType::Type type)
{
    std::string base_url = env::get_env_or_default("RASA_BASE_URL", "http://localhost:5005");
    if (!base_url.empty() && base_url.back() == '/')
    {
        base_url.pop_back();
    }
    if (type == RasaRequestType::INTENT)
    {
        return base_url + "/model/parse";
    }
    return base_url + "/webhooks/rest/webhook";
}
} // namespace

size_t RasaClient::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    string *responsePtr = static_cast<string *>(userp);
    responsePtr->append(static_cast<char *>(contents), size * nmemb);
    return size * nmemb;
}

RasaResponse RasaClient::AskRasa(const std::string &question)
{
    nlohmann::json jRequest = {
        {"text", question}};
    RasaResponse result = {"", 0, false, "", "", ""};
    PerformRequest(RasaRequestType::INTENT, jRequest, result);
    cout << result.confidence << " " << result.intent << endl;
    if (result.intent == "nlu_fallback")
    {
        result.hasRasaResponse = false;
        return result;
    }
    else if (result.confidence < 0.50f)
    {
        gDisplay->setStatus("confidence degeri cok dusuk LLM e soruluyor");
        result.hasRasaResponse = false;
        return result;
    } // else
    gDisplay->setStatus("intent alındı şimdi ekstra cevap var mı bakılıyor");
    nlohmann::json jRequestWebhook = {
        {"sender", "user"},
        {"message", question}};
    RasaClient::PerformRequest(RasaRequestType::WEBHOOK, jRequestWebhook, result);

    return result;
}

RasaResponse RasaClient::AskRasaJson(const nlohmann::json jRequest) // jRequest should contain text
{

    RasaResponse result = {"", 0, false, "", "", ""};
    PerformRequest(RasaRequestType::INTENT, jRequest, result);
    cout << result.confidence << " " << result.intent << endl;
    if (result.intent == "nlu_fallback")
    {
        result.hasRasaResponse = false;
        return result;
    } // else
    nlohmann::json jRequestWebhook = {
        {"sender", "user"},
        {"message", jRequest["text"].get<string>()}};
    RasaClient::PerformRequest(RasaRequestType::WEBHOOK, jRequestWebhook, result);

    return result;
}
void RasaClient::PerformRequest(RasaRequestType::Type type, const nlohmann::json &jRequest, RasaResponse &result)
{

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        result.error = "curl cannot initialized ";
    }

    string requestBody = jRequest.dump();
    const std::string endpoint = BuildRasaUrl(type);
    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
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
        result.error = "Cannot get Response";
        result.rawIntentResponse = curl_easy_strerror(res);
    }
    else
    {
        try
        {
            auto jResponse = nlohmann::json::parse(responseString);
            if (type == RasaRequestType::INTENT)
            {
                result.rawIntentResponse = responseString;
                if (jResponse.contains("intent") && jResponse["intent"].contains("name") && jResponse["intent"].contains("confidence"))
                {
                    result.intent = jResponse["intent"]["name"].get<string>();
                    result.confidence = jResponse["intent"]["confidence"].get<double>();
                }
                else
                {
                    result.error = "response intent ve name yanlis";
                }
            }
            else
            {
                result.rawWebHookResponse = responseString;
                if (!jResponse.empty() && jResponse.front().contains("text"))
                {
                    result.hasRasaResponse = true;
                    result.rasaResponse = jResponse.front()["text"]
                                              .get<string>();
                }
                else
                {
                    result.hasRasaResponse = false;
                }
                return;
            }
        }
        catch (const exception &e)
        {
            result.error = "json parse hatasi";
            if (RasaRequestType::INTENT == type)
                result.rawIntentResponse = string("JSON parse hatası: ") + e.what();
            else
                result.rawWebHookResponse = string("JSON parse hatası: ") + e.what();
        }
    }
}
