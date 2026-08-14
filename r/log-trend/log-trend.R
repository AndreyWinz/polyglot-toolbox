#!/usr/bin/env Rscript

# ==============================================================================
# log-trend.R - Telemetry & Time-Series Log Analyzer
# ==============================================================================

parse_args <- function() {
  args <- commandArgs(trailingOnly = TRUE)
  
  opts <- list(
    file = NULL,
    time_col = 1,
    metric_col = 2,
    window = 5,
    z_threshold = 2.5,
    sep = NULL,
    plot_height = 10,
    plot_width = 50,
    help = FALSE
  )
  
  if (length(args) == 0) {
    opts$help <- TRUE
    return(opts)
  }
  
  i <- 1
  while (i <= length(args)) {
    arg <- args[i]
    
    if (arg %in% c("-h", "--help")) {
      opts$help <- TRUE
      return(opts)
    } else if (arg %in% c("-t", "--time-col")) {
      i <- i + 1
      opts$time_col <- args[i]
    } else if (arg %in% c("-m", "--metric-col")) {
      i <- i + 1
      opts$metric_col <- args[i]
    } else if (arg %in% c("-w", "--window")) {
      i <- i + 1
      opts$window <- as.integer(args[i])
    } else if (arg %in% c("-z", "--z-score")) {
      i <- i + 1
      opts$z_threshold <- as.numeric(args[i])
    } else if (arg %in% c("-d", "--sep")) {
      i <- i + 1
      opts$sep <- args[i]
    } else if (is.null(opts$file) && !startsWith(arg, "-")) {
      opts$file <- arg
    }
    i <- i + 1
  }
  
  return(opts)
}

print_help <- function() {
  cat("
\033[1;36mlog-trend\033[0m - Telemetry & Time-Series Log Analyzer in Base R

\033[1mUsage:\033[0m
  Rscript log-trend.R <file.csv|file.log> [options]

\033[1mOptions:\033[0m
  -t, --time-col <name|idx>   Time/Timestamp column name or 1-based index (default: 1)
  -m, --metric-col <name|idx> Numeric metric column name or 1-based index (default: 2)
  -w, --window <int>          Moving average rolling window size (default: 5)
  -z, --z-score <num>         Z-score threshold for outlier detection (default: 2.5)
  -d, --sep <char>            Field delimiter (default: auto-detects ',' or '\\t')
  -h, --help                  Show this help message

\033[1mExamples:\033[0m
  Rscript log-trend.R server_response.csv -t timestamp -m latency_ms -w 10
  ./log-trend.R telemetry.tsv -m 3 -z 3.0
")
}

detect_delimiter <- function(filepath) {
  first_line <- readLines(filepath, n = 1, warn = FALSE)
  if (length(first_line) == 0) return(",")
  
  tabs <- length(gregexpr("\t", first_line)[[1]])
  commas <- length(gregexpr(",", first_line)[[1]])
  semis <- length(gregexpr(";", first_line)[[1]])
  
  if (tabs > commas && tabs > semis) return("\t")
  if (semis > commas) return(";")
  return(",")
}

compute_sma <- function(x, n) {
  if (length(x) < n) return(x)
  cx <- c(0, cumsum(x))
  sma <- (cx[(n + 1):length(cx)] - cx[1:(length(cx) - n)]) / n
  # Pad initial values with initial rolling partial means
  pad <- sapply(1:(n - 1), function(i) mean(x[1:i]))
  return(c(pad, sma))
}

render_sparkline <- function(values) {
  ticks <- c(" ", "▂", "▃", "▄", "▅", "▆", "▇", "█")
  min_v <- min(values, na.rm = TRUE)
  max_v <- max(values, na.rm = TRUE)
  range_v <- max_v - min_v
  
  if (range_v == 0) return(paste(rep(ticks[1], min(length(values), 40)), collapse = ""))
  
  # Downsample for sparkline if sequence is long
  max_len <- 50
  if (length(values) > max_len) {
    indices <- round(seq(1, length(values), length.out = max_len))
    sampled <- values[indices]
  } else {
    sampled <- values
  }
  
  scaled <- pmin(pmax(1 + floor((sampled - min_v) / range_v * 7), 1), 8)
  return(paste(ticks[scaled], collapse = ""))
}

render_ascii_chart <- function(time_labels, values, sma_values, is_outlier, height = 8, width = 50) {
  n <- length(values)
  if (n == 0) return()
  
  # Downsample x-axis indices
  step <- max(1, floor(n / width))
  sample_idx <- seq(1, n, by = step)
  if (tail(sample_idx, 1) != n) sample_idx <- c(sample_idx, n)
  
  v_sub <- values[sample_idx]
  sma_sub <- sma_values[sample_idx]
  out_sub <- is_outlier[sample_idx]
  
  min_v <- min(v_sub, na.rm = TRUE)
  max_v <- max(v_sub, na.rm = TRUE)
  range_v <- ifelse(max_v == min_v, 1, max_v - min_v)
  
  cat(sprintf("   \033[1mTime-Series Graph (%d data points)\033[0m\n", n))
  cat(sprintf("   Upper: %s  |  Lower: %s\n\n", sprintf("%.3f", max_v), sprintf("%.3f", min_v)))
  
  # Matrix plot setup
  plot_rows <- height
  plot_cols <- length(v_sub)
  grid <- matrix(" ", nrow = plot_rows, ncol = plot_cols)
  
  for (col in 1:plot_cols) {
    val <- v_sub[col]
    sma_val <- sma_sub[col]
    
    val_row <- pmin(plot_rows, pmax(1, round(((max_v - val) / range_v) * (plot_rows - 1)) + 1))
    sma_row <- pmin(plot_rows, pmax(1, round(((max_v - sma_val) / range_v) * (plot_rows - 1)) + 1))
    
    grid[sma_row, col] <- "\033[34m─\033[0m" # Moving average line
    if (out_sub[col]) {
      grid[val_row, col] <- "\033[1;31m▲\033[0m" # Red outlier peak
    } else {
      grid[val_row, col] <- "\033[1;32m•\033[0m" # Normal point
    }
  }
  
  for (r in 1:plot_rows) {
    cat("   │")
    cat(paste(grid[r, ], collapse = ""))
    cat("\n")
  }
  cat("   └" %+% paste(rep("─", plot_cols), collapse = "") %+% "\n")
  cat(sprintf("   Key: \033[1;32m• Value\033[0m | \033[34m─ %d-pt SMA\033[0m | \033[1;31m▲ Outlier (|Z| > threshold)\033[0m\n\n", length(v_sub)))
}

`%+%` <- function(a, b) paste0(a, b)

analyze_logs <- function(df, opts) {
  # Determine column selectors
  time_col <- opts$time_col
  metric_col <- opts$metric_col
  
  if (suppressWarnings(!is.na(as.integer(time_col)))) time_col <- as.integer(time_col)
  if (suppressWarnings(!is.na(as.integer(metric_col)))) metric_col <- as.integer(metric_col)
  
  time_vals <- df[[time_col]]
  metric_vals <- as.numeric(df[[metric_col]])
  
  valid_idx <- !is.na(metric_vals)
  time_vals <- time_vals[valid_idx]
  metric_vals <- metric_vals[valid_idx]
  
  n <- length(metric_vals)
  if (n == 0) {
    cat("\033[31mError: Metric column contains no valid numeric data.\033[0m\n")
    return()
  }
  
  # Calculate moving average & stats
  sma_vals <- compute_sma(metric_vals, opts$window)
  mean_v <- mean(metric_vals)
  sd_v <- sd(metric_vals)
  sd_v <- ifelse(is.na(sd_v) || sd_v == 0, 1e-9, sd_v)
  
  # Outlier detection (Z-Score)
  z_scores <- (metric_vals - mean_v) / sd_v
  is_outlier <- abs(z_scores) >= opts$z_threshold
  outlier_count <- sum(is_outlier)
  
  col_name <- ifelse(is.character(metric_col), metric_col, colnames(df)[metric_col])
  
  cat("\033[1;34m============================================================\033[0m\n")
  cat(sprintf("\033[1m📈 Telemetry Summary for Column:\033[0m \033[1;33m%s\033[0m\n", col_name))
  cat("\033[1;34m============================================================\033[0m\n")
  cat(sprintf("   Total Samples : %d\n", n))
  cat(sprintf("   Mean / SD     : %.3f / %.3f\n", mean_v, sd_v))
  cat(sprintf("   Min / Max     : %.3f / %.3f\n", min(metric_vals), max(metric_vals)))
  cat(sprintf("   Window (SMA)  : %d points\n", opts$window))
  cat(sprintf("   Sparkline     : %s\n", render_sparkline(metric_vals)))
  cat(sprintf("   Outliers      : \033[1;31m%d\033[0m (Threshold |Z| >= %.2f)\n\n", outlier_count, opts$z_threshold))
  
  # ASCII Chart Output
  render_ascii_chart(time_vals, metric_vals, sma_vals, is_outlier, height = opts$plot_height, width = opts$plot_width)
  
  # Outlier Table Listing
  if (outlier_count > 0) {
    cat("\033[1;31m🚨 Detected Outliers (First 10): \033[0m\n")
    outlier_indices <- which(is_outlier)[1:min(10, outlier_count)]
    cat(sprintf("   %-20s | %-12s | %-10s | %-10s\n", "Timestamp", "Value", "SMA", "Z-Score"))
    cat("   " %+% paste(rep("─", 60), collapse = "") %+% "\n")
    
    for (idx in outlier_indices) {
      ts_str <- as.character(time_vals[idx])
      if (nchar(ts_str) > 20) ts_str <- substr(ts_str, 1, 17) %+% "..."
      cat(sprintf("   %-20s | %-12.3f | %-10.3f | \033[31m%+-.2f\033[0m\n", 
                  ts_str, metric_vals[idx], sma_vals[idx], z_scores[idx]))
    }
    cat("\n")
  }
}

main <- function() {
  opts <- parse_args()
  
  if (opts$help || is.null(opts$file)) {
    print_help()
    quit(status = ifelse(opts$help, 0, 1))
  }
  
  if (!file.exists(opts$file)) {
    cat(sprintf("\033[31mError: File '%s' not found.\033[0m\n", opts$file))
    quit(status = 1)
  }
  
  sep <- if (is.null(opts$sep)) detect_delimiter(opts$file) else opts$sep
  
  df <- tryCatch({
    read.delim(opts$file, sep = sep, stringsAsFactors = FALSE, check.names = FALSE)
  }, error = function(e) {
    cat(sprintf("\033[31mError reading file: %s\033[0m\n", e$message))
    quit(status = 1)
  })
  
  analyze_logs(df, opts)
}

main()
