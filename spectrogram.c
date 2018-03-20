#include <fftw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_image.h>
#include "wav_header.h"

float hann(int i, int window) {
  return 0.5 * (1 - cosf(2 * M_PI * i / (window - 1)));
}

void amp2rgb(float value, uint8_t *rr, uint8_t *rg, uint8_t *rb) {
  float h, s, l, r, g, b;

  h = (1 - value);
  s = 1;
  l = value / 2;

  int i = (int)(h * 6);
  float f = h * 6 - i;
  float p = l * (1 - s);
  float q = l * (1 - f * s);
  float t = l * (1 - (1 - f) * s);

  switch (i % 6) {
    case 0: r = l; g = t; b = p; break;
    case 1: r = q; g = l; b = p; break;
    case 2: r = p; g = l; b = t; break;
    case 3: r = p; g = q; b = l; break;
    case 4: r = t; g = p; b = l; break;
    case 5: r = l; g = p; b = q; break;
  }

  *rr = r * 255;
  *rg = g * 255;
  *rb = b * 255;
}

void putpixel(SDL_Surface *surface, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    if (p[0] > 0 || p[1] > 0 || p[2] > 0) { return; }
    p[0] = b;
    p[1] = g;
    p[2] = r;
}

int main(int argc, char **argv) {
  WavHeader wav_header;

  char *filename = argv[1];
  FILE *fd = fopen(filename, "r");
  int i = 0;

  while (i < 100 && (strncmp("data", wav_header.data_header, 4) != 0)) {
    fseek(fd, i, SEEK_SET);
    fread(&wav_header, sizeof(wav_header), 1, fd);
    i++;
  }

  if ((strncmp("data", wav_header.data_header, 4) != 0)) {
    printf("can't read wav header: %s\n");
    return 1;
  }

  float *data = (float*)malloc(wav_header.data_bytes);
  
  fread(data, wav_header.data_bytes, 1, fd);

  unsigned int window = 16384;
  unsigned int shift = 1024;
  
  float *in = (float*) fftwf_malloc(sizeof(float) * window);
  fftwf_complex *out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * window);
  
  unsigned int sample = 0;
  unsigned int samples = wav_header.data_bytes / sizeof(float);

  SDL_Init(0);
  int scale = 50;
  int width = samples / shift;
  int height = logf(window / 2) * scale;
  printf("%u x %u\n", width, height);
  SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height, 24, 0, 0, 0, 0);
  printf("failed: %s\n", SDL_GetError());

  while (sample + window < samples) {
    for (unsigned int i = 0; i < window; i++) {
      in[i] = hann(i, window) * data[sample + i];
    }
    fftwf_plan p = fftwf_plan_dft_r2c_1d(window, in, out, FFTW_ESTIMATE);
    fftwf_execute(p);

    for (unsigned int i = 0; i < window / 2; i++) {
      float power = out[i][0] * out[i][0] + out[i][1] * out[i][1];
      power = 10.0 / log(10.0) * log(power + 1e-6);
      if (power <= 0) power = 0;
      power /= 96;
      if (power > 1) power = 1;

      uint8_t r, g, b;
      amp2rgb(power, &r, &g, &b);
      putpixel(surface, sample / shift, height - logf(i) * scale, r, g, b);
    }
    fftwf_destroy_plan(p);

    sample += shift;
  }
  SDL_SaveBMP(surface, "spectrogram.bmp");
  fclose(fd);
/*
  SDL_FreeSurface(surface);
  free(data);
  fftwf_free(in);
  fftwf_free(out);
  */
}
