# log-trend

A fast terminal time-series and log telemetry analyser written in pure **base R**. It computes Simple Moving Averages (SMA), plots ASCII line trends, displays inline sparklines, and detects anomalous spikes using Z-score statistical thresholds.

## Features
- **Zero External Dependencies:** Built strictly with base R primitives and terminal formatting.
- **Outlier Spike Detection:** Flags metrics exceeding standard deviation thresholds ($|Z| \ge \text{threshold}$).
- **Rolling Moving Averages:** Computes configurable $N$-point Simple Moving Averages to smooth out noise.
- **Terminal Visuals:** Renders terminal ASCII plots, colour-coded status points, and UTF-8 sparklines (` ▂▃▄▅▆▇█`).

## Setup

Make the script executable:
```bash
chmod +x r/log-trend/log-trend.R
```

## Usage

```bash
./log-trend.R <log_or_csv_file> [options]
```

### Options

- `-t, --time-col <col>`: Column name or index for timestamp (default: `1`)
- `-m, --metric-col <col>`: Column name or index for numeric metric (default: `2`)
- `-w, --window <int>`: Rolling moving average window size (default: `5`)
- `-z, --z-score <num>`: Outlier threshold in $Z$-score standard deviations (default: `2.5`)
- `-d, --sep <char> `: File delimiter (default: auto-detected)
- `-h, --help`: Show usage information

## Examples

### Basic Log Analysis

```bash
./log-trend.R server_metrics.csv -t timestamp -m cpu_usage -w 10
```

### Outlier Detection with Custom $Z$-Threshold

```bash
./log-trend.R latency.tsv -m 3 -z 3.0 -w 5
```
