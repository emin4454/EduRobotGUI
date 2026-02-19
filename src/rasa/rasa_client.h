#pragma once
#include <string>
#include <curl/curl.h>
#include "nlohmann/json.hpp"
using namespace std;
namespace RasaRequestType
{
    enum Type
    {
        WEBHOOK,
        INTENT
    };
};

struct RasaResponse
{
    string intent;
    double confidence;
    bool hasRasaResponse;
    string rasaResponse;
    string rawIntentResponse;
    string rawWebHookResponse;
    string error;
};
class RasaClient
{
public:
    RasaResponse AskRasa(const std::string &question);
    RasaResponse AskRasaJson(const nlohmann::json jRequest);
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

private:
    void PerformRequest(RasaRequestType::Type type, const nlohmann::json &jRequest, RasaResponse &result);
};