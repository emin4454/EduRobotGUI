#pragma once
#include <string>
#include <cctype>
#include <cstdint>

#include <ctype.h>
#include <stddef.h>
#include <string>  // std::string için
#include <vector>  // Geçici olarak stringleri toplamak için
#include <random>  // Rastgele sayı üretimi için
#include <chrono>  // Rastgele sayı üreticisini seedlemek için
#include <utility> // std::forward için
namespace helper
{

    namespace detail
    {

        inline char32_t next_utf8(const std::string &s, std::size_t &i)
        {
            unsigned char c0 = s[i++];

            if (c0 < 0x80) // 1-bayt (ASCII)
                return c0;

            if ((c0 >> 5) == 0x06)
            { // 110xxxxx 10xxxxxx
                char32_t cp = (c0 & 0x1F) << 6;
                cp |= (s[i++] & 0x3F);
                return cp;
            }
            if ((c0 >> 4) == 0x0E)
            { // 1110xxxx 10xxxxxx 10xxxxxx
                char32_t cp = (c0 & 0x0F) << 12;
                cp |= (s[i++] & 0x3F) << 6;
                cp |= s[i++] & 0x3F;
                return cp;
            }
            /* 4-bayt kodlamayı da destekle (Türkçe’de gerekmez ama tam olsun) */
            char32_t cp = (c0 & 0x07) << 18;
            cp |= (s[i++] & 0x3F) << 12;
            cp |= (s[i++] & 0x3F) << 6;
            cp |= s[i++] & 0x3F;
            return cp;
        }

        inline void append_utf8(char32_t cp, std::string &out)
        {
            if (cp < 0x80)
            {
                out += static_cast<char>(cp);
            }
            else if (cp < 0x800)
            {
                out += static_cast<char>(0xC0 | (cp >> 6));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
            else if (cp < 0x10000)
            {
                out += static_cast<char>(0xE0 | (cp >> 12));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
            else
            {
                out += static_cast<char>(0xF0 | (cp >> 18));
                out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }

    } // namespace detail
    // ────────────────────────────────────────────────────────────────

    inline std::string normalize_text(const std::string &input)
    {
        /* Türkçe büyük→küçük eşlemeleri */
        static constexpr struct
        {
            char32_t upper, lower;
        } tr_lower[] = {
            {U'\u0049', U'\u0131'}, // I  → ı
            {U'\u0130', U'\u0069'}, // İ  → i
            {U'\u00C7', U'\u00E7'}, // Ç  → ç
            {U'\u011E', U'\u011F'}, // Ğ  → ğ
            {U'\u00D6', U'\u00F6'}, // Ö  → ö
            {U'\u00DC', U'\u00FC'}, // Ü  → ü
            {U'\u015E', U'\u015F'}  // Ş  → ş
        };

        auto to_lower_tr = [](char32_t cp) -> char32_t
        {
            if (cp >= U'A' && cp <= U'Z') // sade ASCII
                return cp + 32;

            for (auto m : tr_lower) // özel harf
                if (cp == m.upper)
                    return m.lower;

            return cp; // değişmedi
        };

        std::string out;
        out.reserve(input.size()); // kaba tahmin

        for (std::size_t i = 0; i < input.size();)
        {
            char32_t cp = detail::next_utf8(input, i);

            /* ASCII noktalama → atla */
            if (cp < 0x80 && std::ispunct(static_cast<unsigned char>(cp)))
                continue;

            cp = to_lower_tr(cp);
            detail::append_utf8(cp, out);
        }
        return out;
    }

    inline int count_words(const char *s)
    {
        int words = 0;
        int in_word = 0;

        for (; *s; ++s)
        {
            if (isspace((unsigned char)*s))
            {
                in_word = 0;
            }
            else if (!in_word)
            {
                in_word = 1;
                ++words;
            }
        }
        return words;
    }
    inline bool containsString(const std::vector<std::string> &vec, const std::string &search_str)
    {
        return std::find(vec.begin(), vec.end(), search_str) != vec.end();
    }

    inline std::mt19937 &get_random_engine()
    {
        static std::mt19937 generator(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        return generator;
    }

    inline std::string pick_random_string_from_list()
    {
        return "";
    }

    template <typename T, typename... Args>
    inline std::string pick_random_string_from_list(T &&first_arg, Args &&...args)
    {

        std::vector<std::string> string_collection;
        string_collection.reserve(1 + sizeof...(args));

        // İlk argümanı ekle
        string_collection.push_back(std::forward<T>(first_arg));

        (string_collection.push_back(std::forward<Args>(args)), ...);

        // Koleksiyon boşsa (ki en az bir argüman olduğu için bu durum olmamalı), boş string döndür.
        if (string_collection.empty())
        {
            return "";
        }

        // 0 ile koleksiyonun boyutu-1 arasında bir dağıtım tanımla
        std::uniform_int_distribution<> distribution(0, string_collection.size() - 1);

        // Rastgele indeksi üret ve ilgili string'i döndür
        return string_collection[distribution(get_random_engine())];
    }
    inline std::string generate_random(int start, int end)
    {
        std::uniform_int_distribution<int> distribution(start, end);
        return std::to_string(distribution(get_random_engine()));
    }

}
