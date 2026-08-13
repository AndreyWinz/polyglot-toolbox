# udp-bench

A high-throughput C++17 UDP socket benchmark and packet loss analyser designed for local network performance testing and socket buffer validation.

## Features
- **Sequence Tracking:** Detects missing packets, out-of-order delivery, and percentage loss rates in real time.
- **Bandwidth Control:** Precision inter-packet pacing to limit client output to exact Mbps targets, or burst at maximum CPU/NIC capacity.
- **Zero External Dependencies:** Implemented using pure POSIX BSD sockets.

## Building

```bash
cd cpp/udp-bench
mkdir build && cd build
cmake ..
make
```

The compiled binary will be placed at `cpp/udp-bench/build/udp-bench`.

## Usage

### 1. Start Server Mode

```bash
./udp-bench -s -p 9000
```

### 2. Run Client Benchmark (Unlimited Speed)

```bash
./udp-bench -c 127.0.0.1 -p 9000 -d 10
```

### 3. Run Client Benchmark throttled to 500 Mbps with 1400-byte packets

```bash
./udp-bench -c 192.168.1.10 -p 9000 -b 500 --size 1400 -d 15
```
