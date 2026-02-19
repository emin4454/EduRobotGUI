#pragma once
#include "states.h"
#include "websocket_client.h"
#include "database_client.h"
#include "tts_client.h"
#include "chatbot_client.h"
#include "rasa_client.h"
#include "display_thread.h"
#include "translate.h"
#include <vector>
#include <string>

class EduRobot
{
public:
    EduRobot();
    ~EduRobot();
    State state;

private:
    void MainLoop();
    RasaResponse HandleSttRequest(const std::string &question); // Handled Or Not
    void AskGeminiAndHandle(const std::string &text);
    bool AskToRasa(const std::string &question);
    Database db;
    TTSClient tts;
    ChatBotClient chatbot;
    RasaClient rasaclient;
    Translate translate;
    std::string last_response = "";
    std::vector<std::string> lookupable_intent_list =
        {
            "ask_fav_color",
            "ask_like_music",
            "shutdown_robot",
            "stop",
            "visual_card_game_start",
            "inform_age",
            "dont_understand_word",
            "i_dont_know_any_word"};
};
struct SentenceItem
{
    int id;
    std::string correct;
    std::string wrongs;
    std::string explanation;
};
