#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H
#include <string>
#include "client_data_t.h"
#include "display_thread.h"

int websocket_client_connect(client_data_t *data);

void websocket_client_run_loop(client_data_t *data);

void websocket_client_cleanup(client_data_t *data);

int listen_speech_to_text(std::string& result,int first_word_timeout_seconds , int timeout_seconds , int word_limit);
#endif // WEBSOCKET_CLIENT_H
