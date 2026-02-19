#pragma once

#include <string>

class TTSClient
{
public:
    /**
     * @brief TTSClient sınıfının kurucusu (constructor).
     * @param api_key ElevenLabs API anahtarınız.
     */
    explicit TTSClient();

    /**
     * @brief Varsayılan ses kimliğini ayarlar.
     * @param voice_id Sonraki sentezleme çağrıları için varsayılan olacak ses kimliği.
     */
    void set_voice_id(const std::string &voice_id);

    /**
     * @brief Varsayılan ses ayarlarını değiştirir.
     * @param stability Sesin kararlılık ayarı (0.0 - 1.0).
     * @param similarity_boost Benzerlik artırma ayarı (0.0 - 1.0).
     */
    void set_voice_settings(float stability, float similarity_boost);

    /**
     * @brief Varsayılan modeli ayarlar.
     * @param model_id Kullanılacak ses modeli (örn: "eleven_multilingual_v2").
     */
    void set_model_id(const std::string &model_id);

    /**
     * @brief Verilen metni sese dönüştürür ve dosyaya kaydeder.
     * * Sınıfta saklanan varsayılan ses kimliğini ve ayarları kullanır.
     * * @param text Sese dönüştürülecek metin.
     * @param output_filepath Oluşturulacak ses dosyasının yolu.
     * @return İşlem başarılıysa true, değilse false döner.
     */
    bool synthesize(const std::string &text);
    void PlaySoundFromFile(const std::string &fileName);
    void play_sound_from_binary(const std::string &bin);

private:
    std::string api_key_;
    std::string voice_id_;
    std::string model_id_;
    float stability_;
    float similarity_boost_;

    // libcurl'den gelen veriyi işlemek için statik callback fonksiyonu
    static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
};