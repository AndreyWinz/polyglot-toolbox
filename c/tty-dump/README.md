# tty-dump

A zero-dependency C99 serial port monitor that prepends high-precision microsecond timestamps to incoming UART/serial data streams.

## Features
- **Microsecond Timestamps:** Accurately logs line arrivals formatted as `[YYYY-MM-DD HH:MM:SS.uuuuuu]`.
- **Raw POSIX TTY Handling:** Configures baud rate, disables software flow control, canonical modes, and local echoing.
- **Dual Destination Output:** Displays colourised stream in the terminal while mirroring clean, uncoloured timestamps to an output log file.
- **Safe Signal Cleanup:** Restores original TTY terminal flags on `SIGINT` (Ctrl+C) or `SIGTERM`.

## Building

```bash
cd c/tty-dump
make
```

The compiled binary will be produced in `bin/tty-dump`.

## Usage

### 1. Monitor Serial Port (115200 Baud Default)

```bash
./bin/tty-dump -p /dev/ttyUSB0
```

### 2. Specify Custom Baud Rate & Output Log File

```bash
./bin/tty-dump -p /dev/ttyACM0 -b 9600 -o serial_capture.log
```

### 3. Disable Colour Codes

```bash
./bin/tty-dump -p /dev/ttyUSB0 -N
```
