# eda-cli

A fast terminal-based Exploratory Data Analysis (EDA) tool written in pure **base R** with zero third-party library dependencies.

## Features
- **Zero Dependencies:** Pure Base R implementation using native `hist()`, `summary()`, and `table()` functions.
- **Auto-Delimiter Detection:** Seamlessly parses CSV, TSV, and semicolon-delimited datasets.
- **ASCII Visualisations:** High-density UTF-8 block histogram bars (`█`) for distribution inspection right in your terminal.
- **Numeric & Categorical Diagnostics:** Provides missingness ratios, standard deviations, five-number summaries, and top category frequency tables.

## Requirements & Setup

Ensure `Rscript` (part of a standard R installation) is installed on your system.

Make the script executable:
```bash
chmod +x r/eda-cli/eda-cli.R
```

## Usage

```bash
./eda-cli.R <path_to_file.csv> [options]
```

### Options

- `-d, --sep <char>`: Specify custom field delimiter (e.g., `\t`, `,`, `;`).

- `-b, --bins <int>`: Adjust number of distribution histogram bins (default: `10`).

- `-w, --width <int>`: Set maximum width of ASCII visual bars in characters (default: `25`).

- `-c, --cats <int>`: Set max number of top categorical values to display (default: `5`).

## Examples

### Basic EDA on CSV

```bash
./eda-cli.R data/housing.csv
```

### Custom Bins and Bar Width on TSV

```bash
./eda-cli.R data/gene_expressions.tsv -d '\t' -b 20 -w 35
```
