#pragma once

#include <string>

namespace env
{
bool load_dotenv(const std::string &path = ".env");
const char *get_env_or_default(const char *key, const char *default_value = "");
unsigned int get_env_uint_or_default(const char *key, unsigned int default_value);
} // namespace env
