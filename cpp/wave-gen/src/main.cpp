#include "generator.h"
#include "wav.h"
#include <iostream>
#include <string>

void print_usage(const char* prog) {
    std::cout << "\x1b[1;36mwave-gen\x1b[0m - Raw PCM & WAV Signal Generator\n\n"
              << "Usage:\n"
              << "  " << prog << " [options]\n\n"
              << "Options:\n"
              << "  -t, --type <sine|square|sawtooth|triangle|dtmf|morse>  Waveform type (default: sine)\n"
              << "  -f, --freq <float>                                     Frequency in Hz (default: 440.0)\n"
              << "  -d, --duration <float>                                 Duration in seconds (default: 2.0)\n"
              << "  -r, --rate <int>                                       Sample rate in Hz (default: 44000)\n"
              << "  -o, --output <file.wav>                                Output WAV file path (default: output.wav)\n"
              << "  --text <string>                                        Text message for Morse or DTMF key sequence\n"
              << "  --wpm <float>                                          Words per minute for Morse CW (default: 20)\n"
              << "  -h, --help                                             Display this help message\n\n"
              << "Examples:\n"
              << "  " << prog << " -t sine -f 1000 -d 3 -o tone.wav\n"
              << "  " << prog << " -t dtmf --text \"1234#\" -o keypad.wav\n"
              << "  " << prog << " -t morse --text \"CQ CQ SOTA\" -f 700 --wpm 18 -o morse.wav\n";
}

int main(int argc, char* argv[]) {
    std::string wave_type_str = "sine";
    double freq = 440.0;
    double duration = 2.0;
    uint32_t sample_rate = 44100;
    std::string output_path = "output.wav";
    std::string input_text = "CQ CQ";
    double wpm = 20.0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if ((arg == "-h" || arg == "--help")) {
            print_usage(argv[0]);
            return 0;
        } else if ((arg == "-t" || arg == "--type") && i + 1 < argc) {
            wave_type_str = argv[++i];
        } else if ((arg == "-f" || arg == "--freq") && i + 1 < argc) {
            freq = std::stod(argv[++i]);
        } else if ((arg == "-d" || arg == "--duration") && i + 1 < argc) {
            duration = std::stod(argv[++i]);
        } else if ((arg == "-r" || arg == "--rate") && i + 1 < argc) {
            sample_rate = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "--text" && i + 1 < argc) {
            input_text = argv[++i];
        } else if (arg == "--wpm" && i + 1 < argc) {
            wpm = std::stod(argv[++i]);
        }
    }

    std::vector<int16_t> audio_data;

    if (wave_type_str == "sine") {
        audio_data = generate_waveform(WaveType::Sine, freq, duration, sample_rate);
    } else if (wave_type_str == "square") {
        audio_data = generate_waveform(WaveType::Square, freq, duration, sample_rate);
    } else if (wave_type_str == "sawtooth") {
        audio_data = generate_waveform(WaveType::Sawtooth, freq, duration, sample_rate);
    } else if (wave_type_str == "triangle") {
        audio_data = generate_waveform(WaveType::Triangle, freq, duration, sample_rate);
    } else if (wave_type_str == "dtmf") {
        audio_data = generate_dtmf_sequence(input_text, sample_rate);
    } else if (wave_type_str == "morse") {
        audio_data = generate_morse_code(input_text, sample_rate, freq, wpm);
    } else {
        std::cerr << "\033[1;31mError:\033[0m Unknown waveform type '" << wave_type_str << "'.\n";
        return 1;
    }

    std::cout << "🔊 Generating audio signal...\n"
              << "   Mode        : " << wave_type_str << "\n"
              << "   Sample Rate : " << sample_rate << " Hz\n"
              << "   Samples     : " << audio_data.size() << "\n";

    if (write_wav_file(output_path, sample_rate, audio_data)) {
        std::cout << "\x1b[1;32mDone!\x1b[0m Wrote WAV audio to \x1b[1m" << output_path << "\x1b[0m\n";
    } else {
        return 1;
    }

    return 0;
}
