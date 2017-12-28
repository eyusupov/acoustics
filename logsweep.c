#include <math.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "wav_header.h"

const int freq = 44100;
const double freq1 = 20;
const double freq2 = 22050;
const double duration = 20;

const char *out_file = "output.wav";
const char *rev_file = "reverse.wav";
const char *rec_file = "capture.wav";
FILE *fd;

double sample = 0;
double k, l, m;
int rec_len = 0;
double f = 0;

/*
void PlayCallback(void* userdata, Uint8* stream, int len) {
  float* streamf = (float*)stream;
  double time, sig, f;
  while (len > 0) {
    time = sample / freq;
    double amp = k * (pow(M_E, time / l) - 1);
    f = k / l * pow(M_E, time / l);
    sig = f > freq2 ? 0 : sinf(amp);
    *(streamf++) = (float)sig;
    sample++;
    len -= sizeof(float);
  }
  printf("%f %f %f\n", time, f, sig);
}
*/

void PlayCallback(void* userdata, Uint8* stream, int len) {
  float* streamf = (float*)stream;
  double sig;
  while (len > 0) {
    sig = sin(2 * M_PI * f / freq);
    f += pow(M_E, k + l * sample);
    if (sig > 1) {
      printf("%f\n", sig);
    }
    *(streamf++) = (float)sig;
    sample++;
    len -= sizeof(float);
  }
}

void revsignal(float *direct, float* stream, int len) {
  direct += len;
  double amp;
  sample = 0;
  while (len > 0) {
    direct--;
    amp = pow(M_E, m * sample);
    double sig = *direct * amp;
    if (sig > 1) {
      printf("%f\n", sig);
    }
    *(stream++) = (float)sig;
    len -= 1;
    sample++;
  }
}

void CaptureCallback(void* userdata, Uint8* stream, int len) {
  fwrite(stream, len, 1, fd);
  rec_len += len;
}

int main() {
  int samples = duration * freq;

  k = log(freq1);
  l = (log(freq2) - k) / samples;
  m = log(freq1 / freq2) / samples;
/*
  l = duration / log(freq2 / freq1);
  k = freq1 * l;
*/
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    printf("SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_AudioSpec want, have_play, have_rec;
  SDL_AudioDeviceID play, rec;

  SDL_zero(want);
  want.freq = freq;
  want.format = AUDIO_F32;
  want.channels = 1;
  want.samples = 4096;
  want.callback = PlayCallback;

  play = SDL_OpenAudioDevice(NULL, 0, &want, &have_play, 0);

  WavHeader header;

  size_t size = samples * sizeof(float);

  fd = fopen(out_file, "w");
  fill_header(&header);
  header.wav_size = sizeof(header) + size - 8;
  header.data_bytes = size;
  fwrite(&header, 1, sizeof(header), fd);

  float *stream = malloc(size);
  PlayCallback(NULL, (Uint8*)stream, size);
  fwrite(stream, size, 1, fd);
  fclose(fd);

  fd = fopen(rev_file, "w");
  fwrite(&header, 1, sizeof(header), fd);
  float *reverse = malloc(size);
  revsignal(stream, reverse, samples);
  fwrite(reverse, size, 1, fd);
  free(reverse);
  free(stream);
  fclose(fd);

  sample = 0;
  f = 0;

  fd = fopen(rec_file, "w");
  fwrite(&header, 1, sizeof(header), fd);

  want.callback = CaptureCallback;
  rec = SDL_OpenAudioDevice(NULL, 1, &want, &have_rec, 0);

  if (play == 0 || rec == 0) {
      SDL_Log("Failed to open audio: %s", SDL_GetError());
      return 1;
  }

  SDL_PauseAudioDevice(play, 0);
  SDL_PauseAudioDevice(rec, 0);
  SDL_Delay((duration + 0.5) * 1000);
  SDL_CloseAudioDevice(play);
  SDL_CloseAudioDevice(rec);

  header.wav_size = sizeof(header) + rec_len - 8;
  header.data_bytes = rec_len;
  rewind(fd);
  fwrite(&header, 1, sizeof(header), fd);
  fclose(fd);
}
