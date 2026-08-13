#ifndef GENERATOR_H
#define GENERATOR_H

#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

constexpr double PI = 3.14159265358979323846;

enum class WaveType {
    Sine,
    Square,
    Sawtooth,
    Triangle,
    DTMF,
    Morse
};

inline std::vector<int16_t> generate_waveform(
    WaveType type,
    double freq,
    double duration_sec,
    uint32_t sample_rate,
    double amplitude = 0.8
) {
    size_t total_samples = static_cast<size_t>(sample_rate * duration_sec);
    std::vector<int16_t> buffer(total_samples);
    int16_t max_amp = static_cast<int16_t>(32767.0 * amplitude);

    for (size_t i = 0; i < total_samples; ++i) {
        double t = static_cast<double>(i) / sample_rate;
        double sample = 0.0;

        switch (type) {
            case WaveType::Sine:
                sample = std::sin(2.0 * PI * freq * t);
                break;

            case WaveType::Square:
                sample = (std::sin(2.0 * PI * freq * t) >= 0.0) ? 1.0 : -1.0;
                break;

            case WaveType::Sawtooth:
                sample = 2.0 * (t * freq - std::floor(t * freq + 0.5));
                break;

            case WaveType::Triangle:
                sample = 2.0 * std::abs(2.0 * (t * freq - std::floor(t * freq + 0.5))) - 1.0;
                break;

            default:
                break;
        }

        buffer[i] = static_cast<int16_t>(sample * max_amp);
    }

    return buffer;
}

// DTMF Frequency Pairs Matrix
inline std::pair<double, double> get_dtmf_freqs(char key) {
    switch (std::toupper(key)) {
        case '1': return {697.0, 1209.0};
        case '2': return {697.0, 1336.0};
        case '3': return {697.0, 1477.0};
        case 'A': return {697.0, 1633.0};
        case '4': return {770.0, 1209.0};
        case '5': return {770.0, 1336.0};
        case '6': return {770.0, 1477.0};
        case 'B': return {770.0, 1633.0};
        case '7': return {852.0, 1209.0};
        case '8': return {852.0, 1336.0};
        case '9': return {852.0, 1477.0};
        case 'C': return {852.0, 1633.0};
        case '*': return {941.0, 1209.0};
        case '0': return {941.0, 1336.0};
        case '#': return {941.0, 1477.0};
        case 'D': return {941.0, 1633.0};
        default:  return {0.0, 0.0};
    }
}

inline std::vector<int16_t> generate_dtmf_sequence(
    const std::string& sequence,
    uint32_t sample_rate,
    double tone_dur = 0.1,
    double gap_dur = 0.05
) {
    std::vector<int16_t> buffer;
    int16_t max_amp = static_cast<int16_t>(32767.0 * 0.4); // Summed 2 sines

    for (char ch : sequence) {
        auto freqs = get_dtmf_freqs(ch);
        size_t tone_samples = static_cast<size_t>(sample_rate * tone_dur);
        size_t gap_samples  = static_cast<size_t>(sample_rate * gap_dur);

        for (size_t i = 0; i < tone_samples; ++i) {
            double t = static_cast<double>(i) / sample_rate;
            double sample = std::sin(2.0 * PI * freqs.first * t) + std::sin(2.0 * PI * freqs.second * t);
            buffer.push_back(static_cast<int16_t>(sample * max_amp));
        }

        // Silence gap
        buffer.insert(buffer.end(), gap_samples, 0);
    }

    return buffer;
}

inline std::vector<int16_t> generate_morse_code(
    const std::string& text,
    uint32_t sample_rate,
    double freq = 700.0,
    double wpm = 20.0
) {
    static const std::map<char, std::string> morse_table = {
        {'A', ".-"},   {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},  {'E', "."},
        {'F', "..-."}, {'G', "--."},  {'H', "...."}, {'I', ".."},   {'J', ".---"},
        {'K', "-.-"},  {'L', ".-.."}, {'M', "--"},   {'N', "-."},   {'O', "---"},
        {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."},  {'S', "..."},  {'T', "-"},
        {'U', "..-"},  {'V', "...-"}, {'W', ".--"},  {'X', "-..-"}, {'Y', "-.--"},
        {'Z', "--.."}, {'1', ".----"},{'2', "..---"},{'3', "...--"},{'4', "....-"},
        {'5', "....."},{'6', "-...."},{'7', "--..."},{'8', "---.."},{'9', "----."},
        {'0', "-----"},{' ', " "}
    };

    double unit_dur = 1.2 / wpm; // Standard Paris WPM formula
    int16_t max_amp = static_cast<int16_t>(32767.0 * 0.7);

    auto append_tone = [&](double duration) {
        size_t count = static_cast<size_t>(sample_rate * duration);
        std::vector<int16_t> chunk(count);
        for (size_t i = 0; i < count; ++i) {
            double t = static_cast<double>(i) / sample_rate;
            chunk[i] = static_cast<int16_t>(std::sin(2.0 * PI * freq * t) * max_amp);
        }
        return chunk;
    };

    auto append_silence = [&](double duration) {
        size_t count = static_cast<size_t>(sample_rate * duration);
        return std::vector<int16_t>(count, 0);
    };

    std::vector<int16_t> buffer;

    for (char c : text) {
        char upper_c = std::toupper(c);
        if (morse_table.find(upper_c) == morse_table.end()) continue;

        std::string code = morse_table.at(upper_c);
        if (code == " ") {
            auto silence = append_silence(unit_dur * 7.0);
            buffer.insert(buffer.end(), silence.begin(), silence.end());
            continue;
        }

        for (size_t i = 0; i < code.length(); ++i) {
            char symbol = code[i];
            if (symbol == '.') {
                auto tone = append_tone(unit_dur);
                buffer.insert(buffer.end(), tone.begin(), tone.end());
            } else if (symbol == '-') {
                auto tone = append_tone(unit_dur * 3.0);
                buffer.insert(buffer.end(), tone.begin(), tone.end());
            }

            // Inter-element space (1 unit)
            if (i + 1 < code.length()) {
                auto silence = append_silence(unit_dur);
                buffer.insert(buffer.end(), silence.begin(), silence.end());
            }
        }

        // Inter-letter space (3 units)
        auto silence = append_silence(unit_dur * 3.0);
        buffer.insert(buffer.end(), silence.begin(), silence.end());
    }

    return buffer;
}

#endif // GENERATOR_H
