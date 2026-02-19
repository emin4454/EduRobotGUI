#pragma once

#include <libwebsockets.h>
#include <portaudio.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>   
#define AUDIO_CHUNK_SIZE 3840

typedef struct client_data_t {
    char *api_key;

    struct lws *wsi;
    struct lws_context *context;

    pthread_mutex_t mutex;

    bool closed;
    bool write_start;
    bool write_audio;

    unsigned char audio_buffer[AUDIO_CHUNK_SIZE];
    size_t audio_buffer_len;

    PaStream *pa_stream;

    char final_text[1024 * 1024];
    char nonfinal_text[1024 * 1024];

    struct timeval last_nonfinal_time;
    struct timeval websocket_start_time;

    int first_word_timeout_secs;
    int timeout_secs;
    int word_count_limit;
    bool got_first_final_word = false;

} client_data_t;

