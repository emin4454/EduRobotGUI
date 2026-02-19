#pragma once

#include "env_utils.h"

inline const char *DbHost()
{
    return env::get_env_or_default("DB_HOST", "127.0.0.1");
}

inline unsigned int DbPort()
{
    return env::get_env_uint_or_default("DB_PORT", 3306);
}

inline const char *DbUser()
{
    return env::get_env_or_default("DB_USER", "root");
}

inline const char *DbPass()
{
    return env::get_env_or_default("DB_PASS", "");
}

inline const char *DbName()
{
    return env::get_env_or_default("DB_NAME", "robot");
}
