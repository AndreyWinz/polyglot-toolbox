#ifndef CLIENT_H
#define CLIENT_H

#include "protocol.h"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd>
#include <vector>

inline void run_client(
    const std::string& host,
    uint16_t port,
    size_t packet_size,
    double target_mbps,
    uint32_t duration_sec
) {
    if (packet_size < HEADER_SIZE) {
        packet_size = HEADER_SIZE;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "\033[1;31mError:\033[0m Could not create socket.\n";
        return;
    }

    // Set large socket send buffer (8 MB)
    int sndbuf = 8 * 1024 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "\033[1;31mError:\033[0m Invalid address/host: " << host << "\n";
        close(sockfd);
        return;
    }

    std::vector<uint8_t> buffer(packet_size, 0xAA);

    std::cout << "\x1b[1;36m🚀 Starting UDP Bench Client\x1b[0m\n"
              << "   Target Host : " << host << ":" << port << "\n"
              << "   Packet Size : " << packet_size << " bytes\n"
              << "   Bandwidth   : " << (target_mbps > 0 ? std::to_string(target_mbps) + " Mbps" : "Unlimited") << "\n"
              << "   Duration    : " << duration_sec << " seconds\n\n";

    uint64_t seq_num = 1;
    uint64_t total_sent_bytes = 0;
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration_sec);

    // Calculate inter-packet delay for bandwidth throttling
    double delay_per_packet_ns = 0.0;
    if (target_mbps > 0) {
        double bytes_per_sec = (target_mbps * 1'000'000.0) / 8.0;
        double packets_per_sec = bytes_per_sec / packet_size;
        delay_per_packet_ns = 1'000'000'000.0 / packets_per_sec;
    }

    auto next_send_time = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() < end_time) {
        pack_u64(buffer.data(), seq_num++);
        pack_u64(buffer.data() + 8, get_timestamp_ns());

        ssize_t sent = sendto(
            sockfd, buffer.data(), packet_size, 0,
            reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)
        );

        if (sent > 0) {
            total_sent_bytes += sent;
        }

        if (target_mbps > 0) {
            next_send_time += std::chrono::nanoseconds(static_cast<int64_t>(delay_per_packet_ns));
            std::this_thread::sleep_until(next_send_time);
        }
    }

    auto actual_duration = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();
    double actual_mbps = (total_sent_bytes * 8.0) / (actual_duration * 1'000'000.0);

    std::cout << "\n==================================================\n"
              << "\x1b[1;32mBenchmark Complete!\x1b[0m\n"
              << "   Total Packets Sent : " << (seq_num - 1) << "\n"
              << "   Total Bytes Sent   : " << total_sent_bytes << " bytes\n"
              << "   Elapsed Time       : " << std::fixed << std::setprecision(2) << actual_duration << " s\n"
              << "   Achieved Bandwidth : " << std::setprecision(2) << actual_mbps << " Mbps\n"
              << "==================================================\n";

    close(sockfd);
}

#endif // CLIENT_H
