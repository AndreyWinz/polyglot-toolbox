use crate::hasher;
use dashmap::DashMap;
use rayon::prelude::*;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicU64, Ordering};
use walkdir::WalkDir;

pub struct ScanReport {
    pub total_scanned: u64,
    pub duplicate_groups: Vec<(u64, Vec<PathBuf>)>, // (file_size, list_of_paths)
    pub wasted_bytes: u64,
}

pub fn find_duplicates(root_path: &Path, min_size: u64) -> ScanReport {
    let total_scanned = AtomicU64::new(0);

    // -------------------------------------------------------------------------
    // Stage 1: Traverse Filesystem & Group by File Size
    // -------------------------------------------------------------------------
    let files_by_size = DashMap::<u64, Vec<PathBuf>>::new();

    WalkDir::new(root_path)
        .into_iter()
        .filter_map(|entry| entry.ok())
        .filter(|entry| entry.file_type().is_file())
        .for_each(|entry| {
            if let Ok(meta) = entry.metadata() {
                let size = meta.len();
                if size >= min_size {
                    total_scanned.fetch_add(1, Ordering::Relaxed);
                    files_by_size.entry(size).or_default().push(entry.into_path());
                }
            }
        });

    // Discard unique file sizes
    let candidate_size_groups: Vec<(u64, Vec<PathBuf>)> = files_by_size
        .into_iter()
        .filter(|(_, paths)| paths.len() > 1)
        .collect();

    // -------------------------------------------------------------------------
    // Stage 2: Parallel Partial Hashing (First 8KB)
    // -------------------------------------------------------------------------
    let partial_hash_map = DashMap::<(u64, u64), Vec<PathBuf>>::new();

    candidate_size_groups
        .into_par_iter()
        .for_each(|(size, paths)| {
            paths.into_par_iter().for_each(|path| {
                if let Ok(hash) = hasher::hash_partial(&path) {
                    partial_hash_map.entry((size, hash)).or_default().push(path);
                }
            });
        });

    let candidate_partial_groups: Vec<Vec<PathBuf>> = partial_hash_map
        .into_iter()
        .map(|(_, paths)| paths)
        .filter(|paths| paths.len() > 1)
        .collect();

    // -------------------------------------------------------------------------
    // Stage 3: Parallel Full Stream Hashing
    // -------------------------------------------------------------------------
    let full_hash_map = DashMap::<u64, Vec<PathBuf>>::new();

    candidate_partial_groups
        .into_par_iter()
        .for_each(|paths| {
            paths.into_par_iter().for_each(|path| {
                if let Ok(hash) = hasher::hash_full(&path) {
                    full_hash_map.entry(hash).or_default().push(path);
                }
            });
        });

    // -------------------------------------------------------------------------
    // Compile Final Report
    // -------------------------------------------------------------------------
    let mut duplicate_groups = Vec::new();
    let mut wasted_bytes = 0u64;

    for entry in full_hash_map.into_iter().filter(|(_, paths)| paths.len() > 1) {
        let paths = entry.1;
        let file_size = std::fs::metadata(&paths[0])
            .map(|m| m.len())
            .unwrap_or(0);

        wasted_bytes += file_size * (paths.len() as u64 - 1);
        duplicate_groups.push((file_size, paths));
    }

    // Sort output groups by file size (largest wasted space first)
    duplicate_groups.sort_by(|a, b| b.0.cmp(&a.0));

    ScanReport {
        total_scanned: total_scanned.load(Ordering::Relaxed),
        duplicate_groups,
        wasted_bytes,
    }
}
