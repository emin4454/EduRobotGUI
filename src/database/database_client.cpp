#include <iostream>
#include "database_client.h"
#include <sstream>
#include <memory>
using namespace std;
Database::Database() : conn(nullptr)
{
    conn = mysql_init(nullptr);
    if (!conn)
    {
        cerr << "mysql_init_failed\n";
        return;
    }
    if (!mysql_real_connect(conn, DbHost(), DbUser(), DbPass(), DbName(), DbPort(), nullptr, 0))
    {
        cerr << "mysql_real_connect failed:" << mysql_error(conn) << "\n";
        mysql_close(conn);
        conn = nullptr;
        return;
    }
    if (mysql_set_character_set(conn, "utf8mb4"))
    {
        std::cerr << "mysql_set_character_set failed: " << mysql_error(conn) << "\n";
    }
    if (conn != nullptr)
        std::cout << "connection successful" << "\n";
}

Database::~Database()
{
    if (conn)
    {
        mysql_close(conn);
        conn = nullptr;
    }
}

bool Database::isConnected() const
{
    return conn != nullptr;
}

RasaResponse Database::searchResponses(const std::string &text)
{
    RasaResponse rasaResponse = {"", -1, false, "", "", "", "pholder"};
    if (!conn)
    {
        std::cerr << "mysql is not active\n";
        rasaResponse.error = "mysql is not active";
        return rasaResponse;
    }

    unsigned long orig_len = text.length();
    std::unique_ptr<char[]> esc_buf{new char[orig_len * 2 + 1]};
    unsigned long esc_len = mysql_real_escape_string(
        conn,
        esc_buf.get(),
        text.c_str(),
        orig_len);

    std::string query =
        "SELECT response, intent, confidence "
        "FROM responses "
        "WHERE question = '" +
        std::string(esc_buf.get(), esc_len) + "' "
                                              "LIMIT 1";

    if (mysql_query(conn, query.c_str()))
    {
        std::cerr << "mysql_query hatası: " << mysql_error(conn) << "\n";
        rasaResponse.error = "mysql_query hatası:";
        rasaResponse.error += mysql_error(conn);
        return rasaResponse;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res)
    {
        std::cerr << "mysql_store_result hatası: " << mysql_error(conn) << "\n";
        rasaResponse.error = "mysql_store_result hatası: ";
        rasaResponse.error += mysql_error(conn);
        return rasaResponse;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    {
        rasaResponse.error = "DBde question bulunamadi";
        return rasaResponse;
    }
    unsigned long *lengths = mysql_fetch_lengths(res);
    if (row[0])
    {
        rasaResponse.rasaResponse.assign(row[0], lengths[0]);
        if (rasaResponse.rasaResponse == "")
        {
            rasaResponse.hasRasaResponse = false;
        }
        else
        {
            rasaResponse.hasRasaResponse = true;
        }
    }
    if (row[2])
    {
        string confidence;
        confidence.assign(row[2], lengths[2]);
        rasaResponse.confidence = stod(confidence);
    }
    if (row[1])
    {
        rasaResponse.intent.assign(row[1], lengths[1]);
    }
    mysql_free_result(res);
    return rasaResponse;
}
bool Database::insertQuestion(const std::string &question,
                              const std::string &response,
                              const std::string &intent,
                              double confidence,
                              std::string &error_message)
{
    if (!conn)
    {
        error_message = "mysql is not active";
        return false;
    }

    const unsigned long qlen = question.length();
    std::unique_ptr<char[]> qbuf(new char[qlen * 2 + 1]);
    const unsigned long qelen = mysql_real_escape_string(
        conn, qbuf.get(), question.c_str(), qlen);

    const unsigned long rlen = response.length();
    std::unique_ptr<char[]> rbuf(new char[rlen * 2 + 1]);
    const unsigned long relen = mysql_real_escape_string(
        conn, rbuf.get(), response.c_str(), rlen);

    const unsigned long ilen = intent.length();
    std::unique_ptr<char[]> ibuf(new char[ilen * 2 + 1]);
    const unsigned long ielen = mysql_real_escape_string(
        conn, ibuf.get(), intent.c_str(), ilen);

    std::ostringstream os;
    os << "INSERT INTO responses (question, response, intent, confidence) VALUES ("
       << "'" << std::string(qbuf.get(), qelen) << "', "
       << "'" << std::string(rbuf.get(), relen) << "', "
       << "'" << std::string(ibuf.get(), ielen) << "', "
       << confidence
       << ")";

    const std::string query = os.str();
    if (mysql_query(conn, query.c_str()) != 0)
    {
        error_message = mysql_error(conn);
        return false;
    }

    return true;
}
bool Database::GetWordAndImageBase64(int id,
                                     std::string &out_word,
                                     std::string &out_base64,
                                     std::string &error_message)
{
    out_word.clear();
    out_base64.clear();

    if (!conn)
    {
        error_message = "mysql is not active";
        return false;
    }

    /* id tam sayı → SQL-escape gerekmez */
    std::ostringstream os;
    os << "SELECT word, image FROM images WHERE id = " << id << " LIMIT 1";
    const std::string query = os.str();

    if (mysql_query(conn, query.c_str()) != 0)
    {
        error_message = mysql_error(conn);
        return false;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res)
    { // store_result hatası
        error_message = mysql_error(conn);
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    { // id bulunamadı
        mysql_free_result(res);
        error_message = "record not found";
        return false;
    }

    unsigned long *lengths = mysql_fetch_lengths(res);
    if (!lengths)
    {
        mysql_free_result(res);
        error_message = "failed to read length";
        return false;
    }

    /* 0 = word, 1 = image (Base-64) */
    out_word.assign(row[0], lengths[0]);
    out_base64.assign(row[1], lengths[1]);

    mysql_free_result(res);
    return true; // başarı
}

std::vector<wordImage> Database::GetRandomImages(int count)
{
    std::vector<wordImage> results;
    if (!conn)
    {
        return results;
    }

    std::ostringstream os;
    os << "SELECT word, image FROM images "
       << "ORDER BY RAND() "
       << "LIMIT " << count;
    std::string query = os.str();

    if (mysql_query(conn, query.c_str()) != 0)
    {

        return results;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res)
    {
        return results;
    }

    MYSQL_ROW row;
    unsigned long *lengths;
    while ((row = mysql_fetch_row(res)) != nullptr)
    {
        lengths = mysql_fetch_lengths(res);
        if (!lengths)
            break;

        wordImage wi;
        wi.word.assign(row[0], lengths[0]);
        wi.b64img.assign(row[1], lengths[1]);
        results.push_back(std::move(wi));
    }

    mysql_free_result(res);
    return results;
}

std::vector<wordTranslate> Database::GetRandomWords(int count)
{
    std::vector<wordTranslate> results;
    if (!conn)
    {
        cout << "baglanti basarisiz";
        return results;
    }

    std::ostringstream os;
    os << "SELECT word, translation,topic,explanation_tr,explanation_en FROM words " //
       << "ORDER BY RAND() "
       << "LIMIT " << count;
    std::string query = os.str();
    if (mysql_query(conn, query.c_str()) != 0)
    {
        cout << "query 0";
        std::cerr << "MySQL Query Error (" << mysql_errno(conn) << "): " << mysql_error(conn) << std::endl;
        return results;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    if (!res)
    {
        cout << "results bos";
        return results;
    }
    MYSQL_ROW row;
    unsigned long *lengths;
    while ((row = mysql_fetch_row(res)) != nullptr)
    {
        lengths = mysql_fetch_lengths(res);
        if (!lengths)
            break;

        wordTranslate wt;
        wt.english_word.assign(row[0], lengths[0]);
        wt.turkish_translation.assign(row[1], lengths[1]);
        wt.topic.assign(row[2], lengths[2]);
        wt.explanation_tr.assign(row[3], lengths[3]);
        wt.explanation_en.assign(row[4], lengths[4]);
        results.push_back(std::move(wt));
    }
    mysql_free_result(res);
    return results;
}
