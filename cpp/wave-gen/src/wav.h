#ifndef WAV_H
#define WAV_H

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#pragma pack(push, 1)
struct WavHeader {
    char     riff_header[4] = {'R', 'I', 'F', 'F'};
    uint32_t wav_size       = 0;
    char     wave_header[4] = {'W', 'A', 'V', 'E'};
    char     fmt_header[4]  = {'f', 'm', 't', ' '};
    uint32_t fmt_chunk_size = 16; // PCM
    uint16_t audio_format   = 1;  // Uncompressed PCM
    uint16_t num_channels   = 1;  // Mono
    uint32_t sample_rate    = 44100;
    uint32_t byte_rate      = 88200; // sample_rate * num_channels * (bits_per_sample / 8)
    uint16_t sample_alignment = 2;   // num_channels * (bits_per_sample / 8)
    uint16_t bits_per_sample  = 16;
    char     data_header[4] = {'d', 'a', 't', 'a'};
    uint32_t data_bytes     = 0;
};
#pragma pack(pop)

inline bool write_wav_file(const std::string& filename, uint32_t sample_rate, const std::vector<int16_t>& samples) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "\033[1;31mError:\033[0m Could not open file for writing: " << filename << "\n";
        return false;
    }

    uint32_t data_size = static_cast<uint32_t>(samples.size() * sizeof(int16_t));

    WavHeader header;
    header.sample_rate = sample_rate;
    header.bits_per_sample = 16;
    header.num_channels = 1;
    header.sample_alignment = 2;
    header.byte_rate = sample_rate * 2;
    header.data_bytes = data_size;
    header.wav_size = 36 + data_size;

    out.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader));
    out.write(reinterpret_cast<const char*>(samples.data()), data_size);

    return true;
}

#endif // WAV_H
