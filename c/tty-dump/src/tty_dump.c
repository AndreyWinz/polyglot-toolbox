#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>
#include <errno.h>

#define BUFFER_SIZE 4096

/* ANSI Colors */
#define COLOR_RESET "\033[0m"
#define COLOR_CYAN  "\033[36m"
#define COLOR_DIM   "\033[2m"

static volatile bool keep_running = true;
static int serial_fd = -1;
static struct termios orig_tio;
static bool tio_saved = false;

static void handle_signal(int sig) {
    (void)sig;
    keep_running = false;
}

/* Map integer baud rate to termios speed_t */
static speed_t parse_baud_rate(int baud) {
    switch (baud) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
#ifdef B460800
        case 460800: return B460800;
#endif
#ifdef B921600
        case 921600: return B921600;
#endif
        default:     return 0;
    }
}

/* Configure serial port to raw 8N1 mode */
static bool configure_tty(int fd, speed_t speed) {
    if (tcgetattr(fd, &orig_tio) < 0) {
        perror("tcgetattr failed");
        return false;
    }
    tio_saved = true;

    struct termios tio;
    memset(&tio, 0, sizeof(tio));

    /* Set baud rate */
    cfsetispeed(&tio, speed);
    cfsetospeed(&tio, speed);

    /* 8N1 (8 data bits, no parity, 1 stop bit) */
    tio.c_cflag |= (CS8 | CLOCAL | CREAD);
    tio.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);

    /* Raw input mode */
    tio.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF);
    tio.c_lflag &= ~(ECHO | ECHOE | ECHONL | ICANON | ISIG | IEXTEN);
    tio.c_oflag &= ~OPOST;

    /* Read timeout controls: blocking read with 100ms timeout per byte */
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 1; /* 0.1s timeout */

    if (tcflush(fd, TCIFLUSH) < 0) {
        perror("tcflush failed");
        return false;
    }

    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        perror("tcsetattr failed");
        return false;
    }

    return true;
}

static void print_timestamp(FILE *out_stream, bool colorize) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm tm_info;
    localtime_r(&tv.tv_sec, &tm_info);

    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);

    if (colorize) {
        fprintf(out_stream, "[" COLOR_CYAN "%s.%06ld" COLOR_RESET "] ", time_str, (long)tv.tv_usec);
    } else {
        fprintf(out_stream, "[%s.%06ld] ", time_str, (long)tv.tv_usec);
    }
}

static void print_usage(const char *prog_name) {
    fprintf(stderr,
        "Usage: %s -p <port> [OPTIONS]\n"
        "High-precision microsecond-timestamped serial/TTY logger.\n\n"
        "Options:\n"
        "  -p <device>   Serial port device path (e.g. /dev/ttyUSB0, /dev/ttyACM0)\n"
        "  -b <baud>     Baud rate (default: 115200)\n"
        "  -o <file>     Log output to a file in addition to stdout\n"
        "  -N            Disable ANSI color outputs\n"
        "  -h            Show this help text\n",
        prog_name
    );
}

int main(int argc, char *argv[]) {
    char *port_path = NULL;
    char *log_filename = NULL;
    int baud_rate = 115200;
    bool colorize = isatty(STDOUT_FILENO);

    int opt;
    while ((opt = getopt(argc, argv, "p:b:o:Nh")) != -1) {
        switch (opt) {
            case 'p':
                port_path = optarg;
                break;
            case 'b':
                baud_rate = atoi(optarg);
                break;
            case 'o':
                log_filename = optarg;
                break;
            case 'N':
                colorize = false;
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (!port_path) {
        fprintf(stderr, "Error: Missing required serial port path (-p).\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    speed_t speed = parse_baud_rate(baud_rate);
    if (speed == 0) {
        fprintf(stderr, "Error: Unsupported baud rate '%d'.\n", baud_rate);
        return EXIT_FAILURE;
    }

    FILE *log_fp = NULL;
    if (log_filename) {
        log_fp = fopen(log_filename, "a");
        if (!log_fp) {
            perror("Failed to open log file");
            return EXIT_FAILURE;
        }
    }

    /* Open serial port non-blocking initially to avoid hang on OPEN */
    serial_fd = open(port_path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd < 0) {
        perror("Failed to open serial device");
        if (log_fp) fclose(log_fp);
        return EXIT_FAILURE;
    }

    /* Reset flags to blocking mode for termios read timeouts */
    fcntl(serial_fd, F_SETFL, 0);

    if (!configure_tty(serial_fd, speed)) {
        close(serial_fd);
        if (log_fp) fclose(log_fp);
        return EXIT_FAILURE;
    }

    /* Register signal handlers for clean exit */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("📡 Monitoring %s at %d baud (Press Ctrl+C to stop)...\n\n", port_path, baud_rate);

    char line_buf[BUFFER_SIZE];
    size_t line_pos = 0;
    bool new_line = true;

    while (keep_running) {
        uint8_t ch;
        ssize_t n = read(serial_fd, &ch, 1);

        if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) continue;
            perror("Read error");
            break;
        }

        if (n == 0) {
            /* Timeout occurred without receiving data */
            continue;
        }

        /* Start line with timestamp if at beginning */
        if (new_line) {
            print_timestamp(stdout, colorize);
            if (log_fp) print_timestamp(log_fp, false);
            new_line = false;
        }

        /* Echo byte */
        fputc(ch, stdout);
        if (log_fp) fputc(ch, log_fp);

        if (ch == '\n' || line_pos >= (BUFFER_SIZE - 1)) {
            fflush(stdout);
            if (log_fp) fflush(log_fp);
            line_pos = 0;
            new_line = true;
        } else {
            line_buf[line_pos++] = (char)ch;
        }
    }

    /* Restore TTY parameters and cleanup */
    if (tio_saved) {
        tcsetattr(serial_fd, TCSANOW, &orig_tio);
    }
    close(serial_fd);
    if (log_fp) fclose(log_fp);

    printf("\n\nStopped tty-dump.\n");
    return EXIT_SUCCESS;
}
