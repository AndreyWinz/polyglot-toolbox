# hexview

A zero-dependency, colourised terminal binary/hex inspector written in standard C99.

## Features
- **Semantic Byte Classification:** Colour-codes bytes by type in real time:
  - **Dim Gray:** `0x00` (NULL bytes)
  - **Green:** `0x20`–`0x7E` (Printable ASCII characters)
  - **Red:** `0x01`–`0x1F`, `0x7F` (Control characters)
  - **Magenta:** `0x80`–`0xFF` (High/Extended bytes)
- **Auto TTY Detection:** Automatically suppresses colour codes when piping output to files or other utilities (unless forced).
- **Stream & Pipe Friendly:** Accepts input from standard file paths or via `stdin` pipelines.
- **Configurable Layout:** Adjust column counts, skip byte offsets, or truncate maximum byte reads.

## Building

```bash
cd c/hexview
make
```

The compiled executable will be placed in `bin/hexview`.

## Usage

### 1. Inspect a Binary File

```bash
./bin/hexview /bin/ls
```

### 2. Inspect Pipeline Input (stdin)

```bash
echo "Hello World\x00\x01\xFF" | ./bin/hexview
```

### 3. Read 64 Bytes Starting from Offset 0x100

```bash
./bin/hexview -s 256 -n 64 target_file.bin
```

### 4. Adjust Column Width to 8 Bytes

```bash
./bin/hexview -c 8 image.png
```
