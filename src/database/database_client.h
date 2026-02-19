#pragma once
#include <mysql.h>
#include <string>
#include "config.h"
#include "rasa_client.h"
using namespace std;

struct wordImage{
    string word;
    string b64img;
};
struct wordTranslate{
    string english_word;
    string turkish_translation;
    string topic;
    string explanation_tr;
    string explanation_en;
};
class Database
{
public:
    Database();
    ~Database();
    bool isConnected() const;
    RasaResponse searchResponses(const string &text);
    bool insertQuestion(const std::string &question, const std::string &response,const std::string &intent , double confidence, std::string &error_message);
    bool GetWordAndImageBase64(int id,
                               std::string &out_word,
                               std::string &out_base64,
                               std::string &error_message);
    vector<wordImage> GetRandomImages(int count);
    vector<wordTranslate> GetRandomWords(int count);
private:
    MYSQL *conn;
};
