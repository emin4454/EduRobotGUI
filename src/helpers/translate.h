#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <string>
#include <vector>
#include <map>

#include "nlohmann/json.hpp"

class Translate
{
public:
    Translate();

    /**
     * @brief Metinleri çevirir.
     * @param texts Çevrilecek metinlerin vektörü.
     * @param source_lang Kaynak dil kodu (örn: "tr").
     * @param target_lang Hedef dil kodu (örn: "en").
     * @return Çevrilen metinleri içeren bir vektör. Hata durumunda boş vektör döner.
     */
    std::vector<std::string> translate(const std::vector<std::string> &texts,
                                       const std::string &source_lang,
                                       const std::string &target_lang);
    std::string translate_one_sentence(const std::string &question);
    std::string translate_one_sentence_turkish(const std::string &question);

private:
    std::string api_key_;
    std::string api_url_ = "https://translation.googleapis.com/language/translate/v2";

    /**
     * @brief Curl isteğinden gelen yanıtı işler.
     */
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
};

#endif // TRANSLATE_H