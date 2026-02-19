#pragma once
#include <string>
#include <nlohmann/json.hpp>
using namespace std;
namespace ChatBotError
{
    enum Error
    {
        None,
        CurlInitFailed,
        RequestFailed,
        JsonParseError,
        UnexpectedResponseFormat
    };
}
struct ChatBotResponse
{
    string generatedText;
    ChatBotError::Error error;
    string rawResponse;
};

struct ChatBotResponseForUser
{
    string finalText;
    bool isError;
};

class ChatBotClient
{
public:
    explicit ChatBotClient();
    ChatBotResponseForUser GeneratedContent(const string &pretext, const string &text, const string &posttext);
private:
    string _apiKey;
    string _modelName;
    string _baseUrl;
    ChatBotResponse PerformRequest(const nlohmann::json &jRequest);

    vector<nlohmann::json> _conversationHistory;
};
