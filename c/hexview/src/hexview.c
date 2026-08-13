#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

/* ANSI Escape Sequences for Color-Coding */
#define COLOR_RESET   "\033[0m"
#define COLOR_DIM     "\033[90m" /* NULL / 0x00 */
#define COLOR_RED     "\033[31m" /* Control Characters (0x01..0x1F, 0x7F) */
#define COLOR_GREEN   "\033[32m" /* Printable ASCII (0x20..0x7E) */
#define COLOR_MAGENTA "\033[35m" /* High / Extended Bytes (0x80..0xFF) */

typedef struct {
    size_t cols;         /* Bytes per line (default: 16) */
    size_t max_bytes;    /* Max total bytes to process (0 = unlimited) */
    size_t skip_offset;  /* Initial offset to skip */
    bool colorize;       /* Enable ANSI color coding */
} Config;

/* Return ANSI color sequence according to byte classification */
static const char *get_byte_color(uint8_t byte) {
    if (byte == 0x00) {
        return COLOR_DIM;
    } else if (byte >= 0x20 && byte <= 0x7E) {
        return COLOR_GREEN;
    } else if (byte < 0x20 || byte == 0x7F) {
        return COLOR_RED;
    } else {
        return COLOR_MAGENTA;
    }
}

static void print_usage(const char *prog_name) {
    fprintf(stderr,
        "Usage: %s [OPTIONS] [FILE]\n"
        "Colorized terminal hex and binary inspector.\n\n"
        "Options:\n"
        "  -c <cols>     Set number of byte columns per line (default: 16)\n"
        "  -n <bytes>    Limit total number of bytes to read\n"
        "  -s <offset>   Skip given offset bytes from the start\n"
        "  -N            Disable colorized output (plain text)\n"
        "  -h            Show this help text\n\n"
        "Byte Color Legend:\n"
        "  " COLOR_DIM "0x00" COLOR_RESET " : NULL bytes (Dim Gray)\n"
        "  " COLOR_GREEN "0x20-0x7E" COLOR_RESET " : Printable ASCII (Green)\n"
        "  " COLOR_RED "0x01-0x1F" COLOR_RESET " : Control Characters (Red)\n"
        "  " COLOR_MAGENTA "0x80-0xFF" COLOR_RESET " : High / Extended Bytes (Magenta)\n",
        prog_name
    );
}

static void inspect_stream(FILE *fp, const Config *config) {
    uint8_t *buffer = malloc(config->cols);
    if (!buffer) {
        perror("malloc failed");
        return;
    }

    size_t current_offset = config->skip_offset;
    size_t total_read = 0;

    /* Seek or stream-skip requested offset */
    if (config->skip_offset > 0) {
        if (fseek(fp, (long)config->skip_offset, SEEK_SET) != 0) {
            /* Fallback manual byte discard for non-seekable streams (like stdin) */
            size_t discarded = 0;
            int ch;
            while (discarded < config->skip_offset && (ch = fgetc(fp)) != EOF) {
                discarded++;
            }
        }
    }

    while (!feof(fp) && !ferror(fp)) {
        size_t to_read = config->cols;
        if (config->max_bytes > 0 && (total_read + to_read) > config->max_bytes) {
            to_read = config->max_bytes - total_read;
            if (to_read == 0) break;
        }

        size_t bytes_read = fread(buffer, 1, to_read, fp);
        if (bytes_read == 0) break;

        /* Print Address Header */
        printf("%08ZX  ", current_offset);

        /* Print Hex Representation */
        for (size_t i = 0; i < config->cols; i++) {
            if (i < bytes_read) {
                uint8_t b = buffer[i];
                if (config->colorize) {
                    printf("%s%02x" COLOR_RESET " ", get_byte_color(b), b);
                } else {
                    printf("%02x ", b);
                }
            } else {
                printf("   "); /* Padding for incomplete final line */
            }

            if (i == (config->cols / 2) - 1) {
                printf(" "); /* Middle separator space */
            }
        }

        printf(" |");

        /* Print Printable ASCII Representation */
        for (size_t i = 0; i < bytes_read; i++) {
            uint8_t b = buffer[i];
            char display_char = (isprint(b)) ? (char)b : '.';
            if (config->colorize) {
                printf("%s%c" COLOR_RESET, get_byte_color(b), display_char);
            } else {
                fputc(display_char, stdout);
            }
        }

        printf("|\n");

        current_offset += bytes_read;
        total_read += bytes_read;

        if (config->max_bytes > 0 && total_read >= config->max_bytes) {
            break;
        }
    }

    free(buffer);
}

int main(int argc, char *argv[]) {
    Config config = {
        .cols = 16,
        .max_bytes = 0,
        .skip_offset = 0,
        .colorize = isatty(STDOUT_FILENO) /* Enable color automatically if outputting to tty */
    };

    int opt;
    while ((opt = getopt(argc, argv, "c:n:s:Nh")) != -1) {
        switch (opt) {
            case 'c':
                config.cols = (size_t)strtoul(optarg, NULL, 10);
                if (config.cols == 0) config.cols = 16;
                break;
            case 'n':
                config.max_bytes = (size_t)strtoul(optarg, NULL, 10);
                break;
            case 's':
                config.skip_offset = (size_t)strtoul(optarg, NULL, 10);
                break;
            case 'N':
                config.colorize = false;
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    FILE *input = stdin;
    const char *filename = NULL;

    if (optind < argc) {
        filename = argv[optind];
        if (strcmp(filename, "-") != 0) {
            input = fopen(filename, "rb");
            if (!input) {
                perror("Error opening input file");
                return EXIT_FAILURE;
            }
        }
    }

    inspect_stream(input, &config);

    if (input != stdin) {
        fclose(input);
    }

    return EXIT_SUCCESS;
}
