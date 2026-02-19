#include "env_utils.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>

namespace
{
std::string trim(const std::string &value)
{
    auto start = value.begin();
    while (start != value.end() && std::isspace(static_cast<unsigned char>(*start)))
    {
        ++start;
    }

    auto end = value.end();
    while (end != start && std::isspace(static_cast<unsigned char>(*(end - 1))))
    {
        --end;
    }

    return std::string(start, end);
}

void set_env_var(const std::string &key, const std::string &value)
{
#if defined(_WIN32)
    _putenv_s(key.c_str(), value.c_str());
#else
    setenv(key.c_str(), value.c_str(), 0);
#endif
}
} // namespace

namespace env
{
bool load_dotenv(const std::string &path)
{
    std::ifstream input(path);
    if (!input.is_open())
    {
        return false;
    }

    std::string line;
    while (std::getline(input, line))
    {
        std::string parsed = trim(line);
        if (parsed.empty() || parsed[0] == '#')
        {
            continue;
        }

        constexpr const char *export_prefix = "export ";
        if (parsed.rfind(export_prefix, 0) == 0)
        {
            parsed = trim(parsed.substr(7));
        }

        const std::size_t eq_pos = parsed.find('=');
        if (eq_pos == std::string::npos)
        {
            continue;
        }

        const std::string key = trim(parsed.substr(0, eq_pos));
        std::string value = trim(parsed.substr(eq_pos + 1));

        if (key.empty())
        {
            continue;
        }

        if (value.size() >= 2)
        {
            const char first = value.front();
            const char last = value.back();
            if ((first == '"' && last == '"') || (first == '\'' && last == '\''))
            {
                value = value.substr(1, value.size() - 2);
            }
        }

        if (std::getenv(key.c_str()) == nullptr)
        {
            set_env_var(key, value);
        }
    }

    return true;
}

const char *get_env_or_default(const char *key, const char *default_value)
{
    const char *value = std::getenv(key);
    if (value == nullptr || *value == '\0')
    {
        return default_value;
    }
    return value;
}

unsigned int get_env_uint_or_default(const char *key, unsigned int default_value)
{
    const char *value = std::getenv(key);
    if (value == nullptr || *value == '\0')
    {
        return default_value;
    }

    try
    {
        return static_cast<unsigned int>(std::stoul(value));
    }
    catch (...)
    {
        return default_value;
    }
}
} // namespace env
