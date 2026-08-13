#!/usr/bin/env Rscript

# ==============================================================================
# eda-cli.R - Terminal Exploratory Data Analysis Tool in R
# ==============================================================================

parse_args <- function() {
  args <- commandArgs(trailingOnly = TRUE)
  
  opts <- list(
    file = NULL,
    sep = NULL,
    bins = 10,
    hist_width = 25,
    max_cats = 5,
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
    } else if (arg %in% c("-d", "--sep", "--delimiter")) {
      i <- i + 1
      opts$sep <- args[i]
    } else if (arg %in% c("-b", "--bins")) {
      i <- i + 1
      opts$bins <- as.integer(args[i])
    } else if (arg %in% c("-w", "--width")) {
      i <- i + 1
      opts$hist_width <- as.integer(args[i])
    } else if (arg %in% c("-c", "--cats")) {
      i <- i + 1
      opts$max_cats <- as.integer(args[i])
    } else if (is.null(opts$file) && !startsWith(arg, "-")) {
      opts$file <- arg
    }
    i <- i + 1
  }
  
  return(opts)
}

print_help <- function() {
  cat("
\033[1;36meda-cli\033[0m - Terminal Exploratory Data Analysis in Base R

\033[1mUsage:\033[0m
  Rscript eda-cli.R <file.csv|file.tsv> [options]

\033[1mOptions:\033[0m
  -d, --sep <char>    Field delimiter (default: auto-detects ',' or '\\t')
  -b, --bins <int>    Number of histogram bins for numeric columns (default: 10)
  -w, --width <int>   Maximum width of ASCII histogram bars (default: 25)
  -c, --cats <int>    Number of top categories to show for text columns (default: 5)
  -h, --help          Show this help message

\033[1mExamples:\033[0m
  Rscript eda-cli.R dataset.csv
  Rscript eda-cli.R data.tsv -d '\\t' --bins 15
  ./eda-cli.R metrics.csv -w 30
")
}

detect_delimiter <- function(filepath) {
  ext <- tolower(tools::file_ext(filepath))
  if (ext == "tsv") return("\t")
  if (ext == "csv") return(",")
  
  # Peek first line
  first_line <- readLines(filepath, n = 1, warn = FALSE)
  if (length(first_line) == 0) return(",")
  
  tabs <- length(gregexpr("\t", first_line)[[1]])
  commas <- length(gregexpr(",", first_line)[[1]])
  semis <- length(gregexpr(";", first_line)[[1]])
  
  if (tabs > commas && tabs > semis) return("\t")
  if (semis > commas) return(";")
  return(",")
}

draw_bar <- function(count, max_count, width) {
  if (max_count == 0 || is.na(count)) return("")
  len <- round((count / max_count) * width)
  if (len == 0 && count > 0) len <- 1
  return(paste0(rep("█", len), collapse = ""))
}

format_num <- function(val) {
  if (is.na(val)) return("NA")
  if (abs(val) >= 1e5 || (abs(val) < 0.001 && val != 0)) {
    return(sprintf("%.3e", val))
  }
  return(sprintf("%.3f", val))
}

analyze_dataset <- function(df, opts) {
  n_rows <- nrow(df)
  n_cols <- ncol(df)
  
  cat("\033[1;34m============================================================\033[0m\n")
  cat(sprintf("\033[1m📊 Dataset Overview:\033[0m %d Rows × %d Columns\n", n_rows, n_cols))
  cat("\033[1;34m============================================================\033[0m\n\n")
  
  for (col_name in colnames(df)) {
    col_data <- df[[col_name]]
    n_na <- sum(is.na(col_data))
    na_pct <- (n_na / n_rows) * 100
    
    cat(sprintf("\033[1;33m📌 Column:\033[0m \033[1m%s\033[0m  ", col_name))
    
    if (is.numeric(col_data)) {
      cat("\033[36m[Numeric]\033[0m\n")
      cat(sprintf("   Missing: %d / %d (%.1f%%)\n", n_na, n_rows, na_pct))
      
      clean_data <- col_data[!is.na(col_data)]
      if (length(clean_data) > 0) {
        min_v <- min(clean_data)
        q1_v  <- quantile(clean_data, 0.25)
        med_v <- median(clean_data)
        mean_v<- mean(clean_data)
        q3_v  <- quantile(clean_data, 0.75)
        max_v <- max(clean_data)
        sd_v  <- sd(clean_data)
        
        cat(sprintf("   Stats  : Min=%s | Q1=%s | Med=%s | Mean=%s | Q3=%s | Max=%s | SD=%s\n",
                    format_num(min_v), format_num(q1_v), format_num(med_v),
                    format_num(mean_v), format_num(q3_v), format_num(max_v), format_num(sd_v)))
        
        # Build ASCII Histogram
        if (min_v != max_v) {
          h <- hist(clean_data, breaks = opts$bins, plot = FALSE)
          max_bin_count <- max(h$counts)
          
          cat("   Distribution:\n")
          for (j in seq_along(h$counts)) {
            range_str <- sprintf("[%s, %s)", format_num(h$breaks[j]), format_num(h$breaks[j+1]))
            bar <- draw_bar(h$counts[j], max_bin_count, opts$hist_width)
            cat(sprintf("     %-22s | %-25s (%d)\n", range_str, bar, h$counts[j]))
          }
        }
      }
    } else {
      cat("\033[35m[Categorical / String]\033[0m\n")
      cat(sprintf("   Missing: %d / %d (%.1f%%)\n", n_na, n_rows, na_pct))
      
      clean_data <- as.character(col_data[!is.na(col_data)])
      n_unique <- length(unique(clean_data))
      cat(sprintf("   Unique : %d values\n", n_unique))
      
      if (length(clean_data) > 0) {
        counts <- sort(table(clean_data), decreasing = TRUE)
        top_counts <- head(counts, opts$max_cats)
        max_cat_count <- max(top_counts)
        
        cat("   Top Frequencies:\n")
        for (cat_label in names(top_counts)) {
          count_val <- top_counts[[cat_label]]
          pct <- (count_val / n_rows) * 100
          display_label <- ifelse(nchar(cat_label) > 20, paste0(substr(cat_label, 1, 17), "..."), cat_label)
          bar <- draw_bar(count_val, max_cat_count, opts$hist_width)
          cat(sprintf("     %-20s | %-25s %d (%.1f%%)\n", display_label, bar, count_val, pct))
        }
      }
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
  
  analyze_dataset(df, opts)
}

main()
