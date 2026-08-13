use std::fs::File;
use std::io::{Read, Result as IoResult};
use std::path::Path;
use xxhash-rust::xxh3::Xxh3;

/// First-stage partial read limit (8 KB)
const PARTIAL_READ_SIZE: usize = 8192;

/// Stream chunk size for full file hashing (64 KB)
const STREAM_CHUNK_SIZE: usize = 65536;

/// Reads only up to the first 8 KB of a file and returns its XXH3 hash.
pub fn hash_partial(path: &Path) -> IoResult<u64> {
    let mut file = File::open(path)?;
    let mut buffer = [0u8; PARTIAL_READ_SIZE];
    let bytes_read = file.read(&mut buffer)?;

    let mut hasher = Xxh3::new();
    hasher.update(&buffer[..bytes_read]);
    Ok(hasher.digest())
}

/// Reads the entire file in 64 KB chunks and returns its XXH3 hash.
pub fn hash_full(path: &Path) -> IoResult<u64> {
    let mut file = File::open(path)?;
    let mut buffer = [0u8; STREAM_CHUNK_SIZE];
    let mut hasher = Xxh3::new();

    loop {
        let bytes_read = file.read(&mut buffer)?;
        if bytes_read == 0 {
            break;
        }
        hasher.update(&buffer[..bytes_read]);
    }

    Ok(hasher.digest())
}
