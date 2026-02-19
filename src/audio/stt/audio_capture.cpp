#include "audio_capture.h"
#include "websocket_client.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "globals.h"
static void *capture_audio_thread(void *arg) {
  client_data_t *data = (client_data_t *)arg;
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    fprintf(stderr, "PortAudio initialize hatası: %s\n", Pa_GetErrorText(err));
    pthread_mutex_lock(&data->mutex);
    data->closed = true;
    pthread_mutex_unlock(&data->mutex);
    return NULL;
  }

  PaStreamParameters inputParams;
  memset(&inputParams, 0, sizeof(inputParams));
  inputParams.device = Pa_GetDefaultInputDevice();
  if (inputParams.device == paNoDevice) {
    fprintf(stderr, "Mikrofon cihazı bulunamadı.\n");
    Pa_Terminate();
    pthread_mutex_lock(&data->mutex);
    data->closed = true;
    pthread_mutex_unlock(&data->mutex);
    return NULL;
  }
  inputParams.channelCount = 1;
  inputParams.sampleFormat = paInt16;
  inputParams.suggestedLatency =
      Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
  inputParams.hostApiSpecificStreamInfo = NULL;

  err = Pa_OpenStream(&data->pa_stream, &inputParams, NULL, 16000,
                      3840 / sizeof(int16_t), paNoFlag, NULL, NULL);
  if (err != paNoError) {
    fprintf(stderr, "PortAudio OpenStream hatası: %s\n", Pa_GetErrorText(err));
    Pa_Terminate();
    pthread_mutex_lock(&data->mutex);
    data->closed = true;
    pthread_mutex_unlock(&data->mutex);
    return NULL;
  }

  err = Pa_StartStream(data->pa_stream);
  if (err != paNoError) {
    fprintf(stderr, "PortAudio StartStream hatası: %s\n", Pa_GetErrorText(err));
    Pa_CloseStream(data->pa_stream);
    Pa_Terminate();
    pthread_mutex_lock(&data->mutex);
    data->closed = true;
    pthread_mutex_unlock(&data->mutex);
    return NULL;
  }

  while (1) {
    pthread_mutex_lock(&data->mutex);
    if (data->closed) {
      pthread_mutex_unlock(&data->mutex);
      break;
    }
    pthread_mutex_unlock(&data->mutex);

    int16_t temp_buf[3840 / sizeof(int16_t)];
    err = Pa_ReadStream(data->pa_stream, temp_buf, 3840 / sizeof(int16_t));
    if (err != paNoError) {
      fprintf(stderr, "PortAudio ReadStream hatası: %s\n",
              Pa_GetErrorText(err));
      break;
    }
    gDisplay->audioVisualizer_.pushSamples(temp_buf,3840 / sizeof(int16_t));
    pthread_mutex_lock(&data->mutex);
    memcpy(data->audio_buffer, temp_buf, 3840);
    data->audio_buffer_len = 3840;
    data->write_audio = true;
    lws_callback_on_writable(data->wsi); // libwebsockets’e haber ver
    lws_cancel_service(data->context);   // event loop’u uyandır
    pthread_mutex_unlock(&data->mutex);
  }

  Pa_StopStream(data->pa_stream);
  Pa_CloseStream(data->pa_stream);
  Pa_Terminate();
  return NULL;
}

int audio_capture_start(client_data_t *data) {
  pthread_t thread;
  return pthread_create(&thread, NULL, capture_audio_thread, data);
}

void audio_capture_stop(client_data_t *data) {
  pthread_mutex_lock(&data->mutex);
  data->closed = true;
  pthread_mutex_unlock(&data->mutex);
}
