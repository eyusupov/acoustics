typedef struct WavHeader {
  // RIFF Header
  char riff_header[4]; // Contains "RIFF"
  unsigned int wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
  char wave_header[4]; // Contains "WAVE"
  
  // Format Header
  char fmt_header[4]; // Contains "fmt " (includes trailing space)
  unsigned int fmt_chunk_size; // Should be 16 for PCM
  unsigned short audio_format; // Should be 1 for PCM. 3 for IEEE Float
  short num_channels;
  unsigned int sample_rate;
  unsigned int byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
  unsigned short sample_alignment; // num_channels * Bytes Per Sample
  unsigned short bit_depth; // Number of bits per sample
  
  // Data
  char data_header[4]; // Contains "data"
  unsigned int data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
  // uint8_t bytes[]; // Remainder of wave file is bytes
} WavHeader;

void fill_header(WavHeader *header) {
  memcpy(header->riff_header, "RIFF", 4);
  memcpy(header->wave_header, "WAVE", 4);
  memcpy(header->fmt_header, "fmt ", 4);
  header->fmt_chunk_size = 16;
  header->audio_format = 3;
  header->num_channels = 1;
  header->sample_rate = 44100;
  header->byte_rate = header->sample_rate * header->num_channels * sizeof(float);
  header->sample_alignment = header->num_channels * sizeof(float);
  header->bit_depth = sizeof(float) * 8;
  memcpy(header->data_header, "data", 4);
}

