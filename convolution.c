#include <fftw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include <inttypes.h>
#include <math.h>
#include "wav_header.h"

size_t wav_length(char *filename) {
  WavHeader wav_header;

  FILE *fd = fopen(filename, "r");
  int i = 0;

  while (i < 100 && (strncmp("data", wav_header.data_header, 4) != 0)) {
    fseek(fd, i, SEEK_SET);
    fread(&wav_header, sizeof(wav_header), 1, fd);
    i++;
  }

  if ((strncmp("data", wav_header.data_header, 4) != 0)) {
    printf("can't read wav header\n");
    return -1;
  }
  return wav_header.data_bytes;
  fclose(fd);
}

void read_wav(char *filename, size_t padded, float *data) {
  WavHeader wav_header;

  FILE *fd = fopen(filename, "r");
  int i = 0;

  while (i < 100 && (strncmp("data", wav_header.data_header, 4) != 0)) {
    fseek(fd, i, SEEK_SET);
    fread(&wav_header, sizeof(wav_header), 1, fd);
    i++;
  }

  if ((strncmp("data", wav_header.data_header, 4) != 0)) {
    printf("can't read wav header\n");
  }

  memset(data, 0, padded);
  fread(data, wav_header.data_bytes, 1, fd);
  fclose(fd);
}

int main(int argc, char **argv) {
  char *signal_file = argv[1];
  char *filter_file = argv[2];

  size_t padded = wav_length(signal_file) + wav_length(filter_file);
  float *signal = malloc(padded), *filter = malloc(padded);

  read_wav(signal_file, padded, signal);
  for (size_t i = 0; i < 10; i++) {
    printf("->%f\n", signal[i]);
  }
  read_wav(filter_file, padded, filter);

  padded /= sizeof(float);

  fftwf_complex *signal_fft = fftwf_malloc(sizeof(fftwf_complex) * padded);
  fftwf_complex *filter_fft = fftwf_malloc(sizeof(fftwf_complex) * padded);

  fftwf_plan p_signal = fftwf_plan_dft_r2c_1d(padded, signal, signal_fft, FFTW_ESTIMATE);
  fftwf_execute(p_signal);

  fftwf_plan p_filter = fftwf_plan_dft_r2c_1d(padded, filter, filter_fft, FFTW_ESTIMATE);
  fftwf_execute(p_filter);

  for (size_t i = 0; i < padded; i++) {
    float re = signal_fft[i][0] * filter_fft[i][0] - signal_fft[i][1] * filter_fft[i][1];
    float im = signal_fft[i][0] * filter_fft[i][1] + signal_fft[i][1] * filter_fft[i][0];
    signal_fft[i][0] = re;
    signal_fft[i][1] = im;
  }

  fftwf_plan p_conv = fftwf_plan_dft_c2r_1d(padded, signal_fft, signal, FFTW_ESTIMATE);
  fftwf_execute(p_conv);

  float max = 0;
  for (size_t i = 0; i < padded; i++) {
    if (fabsf(signal[i]) > max) max = fabsf(signal[i]);
  }

  for (size_t i = 0; i < padded; i++) {
    signal[i] /= max;
  }

  FILE *fd = fopen("convolution.wav", "w");
  WavHeader header;
  fill_header(&header);
  header.data_bytes = padded * sizeof(float);
  header.wav_size = sizeof(header) + header.data_bytes - 8;
  fwrite(&header, 1, sizeof(header), fd);
  fwrite(signal, header.data_bytes, 1, fd);
  fclose(fd);

  fftwf_destroy_plan(p_signal);
  fftwf_destroy_plan(p_filter);
  fftwf_destroy_plan(p_conv);

  fftwf_free(filter_fft);
  free(filter);
  fftwf_free(signal_fft);
  free(signal);
}
