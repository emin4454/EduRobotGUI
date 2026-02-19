#include "websocket_client.h"
#include "audio_capture.h" // audio_capture_start, vs.
#include <nlohmann/json.hpp>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include "text_helpers.h"
#include "globals.h"
#include <sstream>
#include "env_utils.h"


using json = nlohmann::json;

static int callback(lws *wsi, enum lws_callback_reasons reason, void *user,
                    void *in, size_t len) {
  client_data_t *data = (client_data_t *)user;
  switch (reason) {
  case LWS_CALLBACK_CLIENT_ESTABLISHED:
    data->write_start = true;
    gDisplay->setStatus("LISTENING");
    printf("baglanti basari ile kuruldu");
    lws_callback_on_writable(wsi);
    break;

  case LWS_CALLBACK_CLIENT_WRITEABLE: {
    pthread_mutex_lock(&data->mutex);
    if (data->write_start) {
      json start_req = {
        {"api_key",data->api_key},
        {"audio_format","pcm_s16le"},
        {"sample_rate" ,  16000},
        {"num_channels",1},
        {"model","stt-rt-preview"},
        {"language_hints", {"en","tr"}}
      };
      std::string msg = start_req.dump();   
      size_t msg_len = msg.size();

      unsigned char *buf = (unsigned char *)malloc(LWS_PRE + msg_len);
      memcpy(buf + LWS_PRE, msg.c_str(), msg_len);

      int n = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
      free(buf);
      if (n < (int)msg_len) {
        pthread_mutex_unlock(&data->mutex);
        return -1;
      }
      gettimeofday(&data->last_nonfinal_time,0);
      audio_capture_start(data);
      if(displayAudio)
        gDisplay->setMode(DisplayMode::SHOW_AUDIO_VISUALIZATION);
      gDisplay->ShowMicImage();
      gettimeofday(&data->websocket_start_time ,0);
      data->write_start = false;
    }
    if (data->write_audio) {
      unsigned char *audio_buf =
          (unsigned char *)malloc(LWS_PRE + data->audio_buffer_len);
      memcpy(audio_buf + LWS_PRE, data->audio_buffer, data->audio_buffer_len);
      int n = lws_write(wsi, audio_buf + LWS_PRE, data->audio_buffer_len,
                        LWS_WRITE_BINARY);
      free(audio_buf);
      if (n < (int)data->audio_buffer_len) {
        printf("Ses gönderme hatası\n");
      }
      data->write_audio = false;
    }
    pthread_mutex_unlock(&data->mutex);
    break;
  }

  case LWS_CALLBACK_CLIENT_RECEIVE: {
    
    std::string raw_msg((char*)in, len);
    json res;

    try {
      res = json::parse(raw_msg);
    } catch (const json::parse_error& e) {
      std::cerr << "JSON parse hatası: " << e.what() << "\n";
      return -1;
    }
    if (res.contains("error_code")) {
      int code = res["error_code"].get<int>();
      std::string errmsg = res.value("error_message", std::string("(unknown)"));
      printf("Error %d: %s\n", code, errmsg.c_str());
      lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
      lws_cancel_service(data->context);
      return -1;
    }

    data->nonfinal_text[0] = '\0';  
    if (res.contains("tokens") && res["tokens"].is_array()) {
        for (auto& token_obj : res["tokens"]) {
            std::string t = token_obj.value("text", std::string(""));
            bool is_final = token_obj.value("is_final", false);
            if (!t.empty()) {
                if (is_final) {
                    strcat(data->final_text, t.c_str());
                    data->got_first_final_word = true;
                } else {
                  strcat(data->nonfinal_text, t.c_str());
                  gettimeofday(&data->last_nonfinal_time,0);
                }
            }
        }
    }
    printf("\033[2J\033[H%s\033[34m%s\033[39m\n", data->final_text, data->nonfinal_text);
    std::ostringstream oss;
    oss<< "LISTENING - " << data-> final_text << data->nonfinal_text;
    gDisplay->setStatus(oss.str());
    if (res.value("finished", false)) {
      lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
      lws_cancel_service(data->context);
    }
    break;
  }

  case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    printf("Bağlantı hatası: %s\n", in ? (char *)in : "(null)");
    pthread_mutex_lock(&data->mutex);
    data->closed = true;
    pthread_mutex_unlock(&data->mutex);
    break;

  case LWS_CALLBACK_CLIENT_CLOSED:
    printf("\nTranskripsiyon tamamlandı.\n");
    pthread_mutex_lock(&data->mutex);
    data->closed = true;
    pthread_mutex_unlock(&data->mutex);
    break;

  default:
    break;
  }
  return 0;
}

int websocket_client_connect(client_data_t *data) {
  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));
  info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = (const struct lws_protocols[]){
      {"ws", callback, sizeof(client_data_t), 0}, {NULL, NULL, 0, 0}};

  data->context = lws_create_context(&info);
  if (!data->context) {
    fprintf(stderr, "Context oluşturulamadı\n");
    return -1;
  }

  struct lws_client_connect_info ci;
  memset(&ci, 0, sizeof(ci));
  
  ci.context = data->context;
  ci.address = "stt-rt.soniox.com";
  ci.host = "stt-rt.soniox.com";
  ci.port = 443;
  ci.path = "/transcribe-websocket";
  ci.ssl_connection = LCCSCF_USE_SSL;
  ci.protocol = "ws";
  ci.pwsi = &data->wsi;
  ci.userdata = data;

  data->wsi = lws_client_connect_via_info(&ci);
  if (!data->wsi) {
    fprintf(stderr, "WebSocket bağlantısı kurulamadı\n");
    lws_context_destroy(data->context);
    return -1;
  }
  return 0;
}

void websocket_client_run_loop(client_data_t *data) {
  while (lws_service(data->context, 100) >= 0 && !data->closed) {
    if(data->websocket_start_time.tv_sec == 0)
      continue;
    struct timeval current_time;
    gettimeofday(&current_time,0);
    

    if(!data->got_first_final_word)
    {
      int elapsed_time= (current_time.tv_sec  - data-> websocket_start_time.tv_sec);
      if(elapsed_time >= data->first_word_timeout_secs)
      {
        printf("sohbet zaman asimina ugradi hic kelime gelmedi - sohbet kapatiliyor");
        pthread_mutex_lock(&data->mutex);
        data->closed = true;
        pthread_mutex_unlock(&data->mutex);
        break;
      }
    }
    else
    {
      if(data->last_nonfinal_time.tv_sec == 0)
        continue;
      int elapsed_time= (current_time.tv_sec  - data->last_nonfinal_time.tv_sec);
      if(elapsed_time >= data->timeout_secs || helper::count_words(data->final_text) >= data->word_count_limit)
      {
        printf("sohbet zaman asimina ugradi - sohbet kapatiliyor");
        pthread_mutex_lock(&data->mutex);
        data->closed = true;
        pthread_mutex_unlock(&data->mutex);
        break;
      }
    }
    
  }
}

void websocket_client_cleanup(client_data_t *data) {
  if (data->context) {
    lws_context_destroy(data->context);
  }
}

int listen_speech_to_text(std::string& result,int first_word_timeout_seconds , int timeout_seconds , int word_limit=20){    //direkt listen komutu
  client_data_t data;
  memset(&data, 0, sizeof(data));
  static std::string soniox_api_key;
  soniox_api_key = env::get_env_or_default("SONIOX_API_KEY", "");
  data.api_key = soniox_api_key.data();
  if (soniox_api_key.empty()) {
    std::cerr << "SONIOX_API_KEY is not configured.\n";
    return 0;
  }
  data.first_word_timeout_secs = first_word_timeout_seconds;
  data.timeout_secs = timeout_seconds;
  data.word_count_limit = word_limit;

  pthread_mutex_init(&data.mutex, NULL);
  if (websocket_client_connect(&data) != 0) {
    return 0;
  }
  printf("Opening WebSocket connection...\n");
  websocket_client_run_loop(&data);// loop
  result += data.final_text;
  result += data.nonfinal_text;
  gDisplay->RemoveMicImage();
  websocket_client_cleanup(&data);
  audio_capture_stop(&data);
  pthread_mutex_destroy(&data.mutex);
  return 1;
}
