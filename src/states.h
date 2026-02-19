#pragma once

enum class State
{
        INITIALIZATION,
        SLEEPING,
        LISTENING,
        VISUAL_GAME_START,
        MUSIC_LISTENER_START,
        TOPIC_WORD_GAME_START,
        TRANSLATION_GAME_START,
        DEFAULT_INTENT_RESPONSE,
        SHUTDOWN,
};