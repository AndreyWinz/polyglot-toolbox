#include "client.h"
#include "protocol.h"
#include "server.h"
#include <iostream>
#include <string>

void print_usage(const char* prog) {
    std::cout << "\x1b[1;36mudp-bench\x1b[0m - High-Throughput UDP Socket Benchmark & Loss Analyzer\n\n"
              << "Usage:\n"
              << "  Server Mode: " << prog << " -s [-p port]\n"
              << "  Client Mode: " << prog << " -c <host> [-p port] [-b mbps] [-s size] [-d duration]\n\n"
              << "Options:\n"
              << "  -s, --server          Run in receiver mode\n"
              << "  -c, --client <host>   Run in transmitter mode targeting <host>\n"
              << "  -p, --port <int>      Port number (default: " << DEFAULT_PORT << ")\n"
              << "  -b, --mbps <float>    Target bandwidth limit in Mbps (0 for unlimited, default: 0)\n"
              << "  --size <bytes>        Payload size per packet in bytes (default: " << DEFAULT_PACKET_SIZE << ")\n"
              << "  -d, --duration <sec>  Client benchmark run time in seconds (default: 10)\n"
              << "  -h, --help            Display this help screen\n";
}

int main(int argc, char* argv[]) {
    bool is_server = false;
    std::string host = "127.0.0.1";
    uint16_t port = DEFAULT_PORT;
    size_t packet_size = DEFAULT_PACKET_SIZE;
    double target_mbps = 0.0;
    uint32_t duration_sec = 10;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-s" || arg == "--server") {
            is_server = true;
        } else if ((arg == "-c" || arg == "--client") && i + 1 < argc) {
            host = argv[++i];
        } else if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoul(argv[++i]));
        } else if ((arg == "-b" || arg == "--mbps") && i + 1 < argc) {
            target_mbps = std::stod(argv[++i]);
        } else if (arg == "--size" && i + 1 < argc) {
            packet_size = std::stoul(argv[++i]);
        } else if ((arg == "-d" || arg == "--duration") && i + 1 < argc) {
            duration_sec = static_cast<uint32_t>(std::stoul(argv[++i]));
        }
    }

    if (is_server) {
        run_server(port);
    } else {
        run_client(host, port, packet_size, target_mbps, duration_sec);
    }

    return 0;
}
