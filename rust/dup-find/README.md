# dup-find

A multi-threaded CLI utility written in Rust to quickly identify duplicate files across deep folder trees while minimising disk reads.

## How It Works

`dup-find` uses a 3-stage filtration pipeline:
1. **Size Bucketing:** Traverses the filesystem and groups files by byte count. Files with unique sizes are instantly dropped.
2. **Parallel Partial Hashing:** Reads only the first **8 KB** of remaining files and computes an **XXH3** hash in parallel using Rayon.
3. **Parallel Full Hashing:** Streams full contents in **64 KB** blocks *only* for files whose sizes and partial hashes matched.

## Building

```bash
cd rust/dup-find
cargo build --release
```

The output binary will be located at `target/release/dup-find`.

## Usage

### 1. Scan Current Directory

```bash
./target/release/dup-find
```

### 2. Scan Specific Directory

```bash
./target/release/dup-find /path/to/search
```

### 3. Ignore Small Files (e.g., ignore files smaller than 1 MB = 1048576 bytes)

```bash
./target/release/dup-find /path/to/search -m 1048576
```
