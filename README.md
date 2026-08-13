# polyglot-toolbox

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![GitHub commit activity](https://img.shields.io/github/commit-activity/t/AndreyWinz/polyglot-toolbox)
[![Status](https://img.shields.io/badge/status-active%20development-green.svg)]()
![GitHub repo size](https://img.shields.io/github/repo-size/AndreyWinz/polyglot-toolbox)
![GitHub Repo stars](https://img.shields.io/github/stars/AndreyWinz/polyglot-toolbox)

# 🛠️ Polyglot Toolbox

An evolving collection of lightweight, single-purpose CLI utilities and quality-of-life tools written across multiple programming languages.

This repository serves as a personal laboratory for building fast, dependency-light tools for daily terminal use, system diagnostics, and file processing—selecting the right language for each job. While starting with **C**, **Python**, and **Rust**, the project is structured to expand into additional languages over time seamlessly.

---

## Repository Structure

```text
polyglot-toolbox/
├── c/            # High-speed POSIX binaries, raw byte & memory inspection
│   ├── hexview/
│   └── tty-dump/
├── python/       # Automation, media pipelines, log processing
│   ├── exif-clean/
│   └── net-check/
├── rust/         # Concurrent CLI tools, fast filesystem crawlers
│   ├── dup-find/
│   └── cidr-calc/
# ── Future expansion (Go, Zig, Shell, C++, etc.) ──
├── LICENSE
├── .gitattributes
├── .gitignore
└── README.md
```


## Tools Matrix

### C

| Tool     | Description                                                    | Status  |
|----------|----------------------------------------------------------------|---------|
| `hexview`  | Colourised terminal hex/binary inspector with byte-type tagging | $\color{#008000}{\text{Completed}}$ |
| `tty-dump` | High-precision microsecond-timestamped serial/TTY logger       | $\color{#008000}{\text{Completed}}$ |

### Python

| Tool     | Description                                                    | Status  |
|----------|----------------------------------------------------------------|---------|
| `exif-clean`  | Recursive EXIF & metadata scrubber for images | $\color{#008000}{\text{Completed}}$ |
| `net-check` | Async port & endpoint monitor with terminal feedback       | $\color{#008000}{\text{Completed}}$|

### Rust

| Tool     | Description                                                    | Status  |
|----------|----------------------------------------------------------------|---------|
| `dup-find`  | Multi-threaded duplicate file finder using size & partial hashing | $\color{#008000}{\text{Completed}}$ |
| `cidr-calc` | Zero-dependency IPv4/IPv6 CIDR subnet calculator       | $\color{#008000}{\text{Completed}}$ |

### C++

| Tool     | Description                                                    | Status  |
|----------|----------------------------------------------------------------|---------|
| `wave-gen`  | Raw PCM & WAV signal generator (sine, square, sawtooth, DTMF tones, Morse code CW) | $\color{#008000}{\text{Completed}}$ |
| `udp-bench` | High-throughput UDP socket benchmark and packet loss analyser       | $\color{#008000}{\text{Completed}}$ |

### C#

| Tool     | Description                                                    | Status  |
|----------|----------------------------------------------------------------|---------|
| `dll-inspect`  | .NET assembly inspector (lists namespaces, class hierarchies, public API surface, dependencies) | $\color{#008000}{\text{Completed}}$ |
| `env-vault` | Encrypted `.env` secrets manager using AES-256-GCM and PBKDF2 key derivation       | $\color{#008000}{\text{Completed}}$ |

### R

| Tool     | Description                                                    | Status  |
|----------|----------------------------------------------------------------|---------|
| `eda-cli`  | Terminal exploratory data analysis tool for CSV/TSV with ASCII distribution histograms | $\color{#008000}{\text{Completed}}$ |
| `log-trend` | Telemetry and time-series log analyser with moving averages and outlier detection       | $\color{#008000}{\text{Completed}}$ |


## Future Languages

- Additional subdirectories and utilities (e.g., Go, Zig, C++, Bash/Zsh) will be added as new projects are created.


## Quick Start

Each tool is self-contained within its respective language directory and carries its own build target or entry point.
- **C:** Navigate to the project directory and run `make`.
- **Python:** Run scripts directly via `python3` (uses standard library where possible).
- **Rust:** Build or run using `cargo run --release`.
- **Other Languages:** This list will be updated for every language that will be added.

Refer to the `README.md` inside each tool's directory for specific usage instructions.


## License

Distributed under the [MIT License](LICENSE).
