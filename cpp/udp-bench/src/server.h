#ifndef SERVER_H
#define SERVER_H

#include "protocol.h"
#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd>

inline void run_server(uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "\033[1;31mError:\033[0m Could not create socket.\n";
        return;
    }

    // Set large socket receive buffer (8 MB)
    int rcvbuf = 8 * 1024 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "\033[1;31mError:\033[0m Could not bind to port " << port << ".\n";
        close(sockfd);
        return;
    }

    std::cout << "\x1b[1;36m📡 UDP Bench Server listening on port " << port << "...\x1b[0m\n"
              << "Press Ctrl+C to stop.\n\n";

    uint8_t buffer[65536];
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);

    uint64_t total_packets = 0;
    uint64_t total_bytes = 0;
    uint64_t expected_seq = 1;
    uint64_t packets_lost = 0;
    uint64_t out_of_order = 0;

    auto last_report_time = std::chrono::steady_clock::now();
    uint64_t interval_packets = 0;
    uint64_t interval_bytes = 0;

    while (true) {
        ssize_t bytes_read = recvfrom(
            sockfd, buffer, sizeof(buffer), 0,
            reinterpret_cast<sockaddr*>(&client_addr), &addr_len
        );

        if (bytes_read < static_cast<ssize_t>(HEADER_SIZE)) {
            continue;
        }

        uint64_t seq = unpack_u64(buffer);
        total_packets++;
        total_bytes += bytes_read;
        interval_packets++;
        interval_bytes += bytes_read;

        if (seq == expected_seq) {
            expected_seq++;
        } else if (seq > expected_seq) {
            packets_lost += (seq - expected_seq);
            expected_seq = seq + 1;
        } else {
            out_of_order++;
        }

        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - last_report_time;

        if (elapsed.count() >= 1.0) {
            double mbps = (interval_bytes * 8.0) / (elapsed.count() * 1'000'000.0);
            double loss_pct = (total_packets + packets_lost > 0)
                ? (static_cast<double>(packets_lost) / (total_packets + packets_lost)) * 100.0
                : 0.0;

            std::cout << "\r[Stats] Throughput: " << std::fixed << std::setprecision(2) << std::setw(7) << mbps << " Mbps"
                      << " | Pkts/s: " << std::setw(7) << static_cast<uint64_t>(interval_packets / elapsed.count())
                      << " | Total Pkts: " << std::setw(8) << total_packets
                      << " | Lost: " << std::setw(6) << packets_lost
                      << " (" << std::setprecision(2) << loss_pct << "%)"
                      << " | OOO: " << out_of_order << std::flush;

            last_report_time = now;
            interval_packets = 0;
            interval_bytes = 0;
        }
    }

    close(sockfd);
}

#endif // SERVER_H
