mod hasher;
mod scanner;

use clap::Parser;
use std::path::PathBuf;
use std::time::Instant;

#[derive(Parser, Debug)]
#[command(
    author,
    version,
    about = "Fast multi-threaded duplicate file scanner using size bucketing and XXH3 hashing."
)]
struct Cli {
    /// Root directory path to scan
    #[arg(default_value = ".")]
    path: PathBuf,

    /// Minimum file size in bytes to consider
    #[arg(short, long, default_value_t = 1)]
    min_size: u64,
}

fn main() {
    let cli = Cli::parse();

    if !cli.path.exists() {
        eprintln!("Error: Target path '{}' does not exist.", cli.path.display());
        std::process::exit(1);
    }

    println!("🔍 Scanning: {}", cli.path.display());
    let start_time = Instant::now();

    let report = scanner::find_duplicates(&cli.path, cli.min_size);
    let elapsed = start_time.elapsed();

    if report.duplicate_groups.is_empty() {
        println!(
            "\n✨ No duplicate files found across {} file(s) scanned in {:.2?}.",
            report.total_scanned, elapsed
        );
        return;
    }

    println!(
        "\nFound {} duplicate group(s) across {} scanned file(s):\n",
        report.duplicate_groups.len(),
        report.total_scanned
    );

    for (group_idx, (size, paths)) in report.duplicate_groups.iter().enumerate() {
        let human_size = format_bytes(*size);
        println!(
            " Group #{} ({} per file, {} copies):",
            group_idx + 1,
            human_size,
            paths.len()
        );
        for path in paths {
            println!("  └─ {}", path.display());
        }
        println!();
    }

    println!("--------------------------------------------------");
    println!(
        "Done in {:.2?}. Total reclaimable space: {}",
        elapsed,
        format_bytes(report.wasted_bytes)
    );
}

fn format_bytes(bytes: u64) -> String {
    const KB: f64 = 1024.0;
    const MB: f64 = KB * 1024.0;
    const GB: f64 = MB * 1024.0;

    let b = bytes as f64;
    if b >= GB {
        format!("{:.2} GB", b / GB)
    } else if b >= MB {
        format!("{:.2} MB", b / MB)
    } else if b >= KB {
        format!("{:.2} KB", b / KB)
    } else {
        format!("{} B", bytes)
    }
}
