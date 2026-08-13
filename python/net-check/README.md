# net-check

A lightweight, zero-dependency Python CLI tool for asynchronous monitoring of TCP ports and HTTP/HTTPS web endpoints.

## Features
- **Zero External Dependencies:** Built purely on standard library `asyncio` and `ssl`.
- **Concurrent Execution:** Checks dozens or hundreds of endpoints in parallel without blocking.
- **Dual Target Support:** Automatically routes `http://`/`https://` to HTTP HEAD probes and `host:port` strings to socket TCP handshakes.
- **Real-Time Terminal Dashboard:** Optional live-refreshing terminal UI with colourised status and latency measurements.

## Usage

### 1. Single-Shot Health Check
```bash
python net_check.py [https://google.com](https://google.com) 1.1.1.1:53 127.0.0.1:22
```

### 2. Continuous Monitoring with a 2-Second Interval
```bash
python net_check.py [https://1.1.1.1](https://1.1.1.1) 8.8.8.8:53 -c -i 2
```

### 3. Load Targets from a File
```bash
python net_check.py -f targets.example.txt -c -i 5
```
