# exif-clean

A lightweight Python CLI utility that recursively strips EXIF metadata, GPS tracking points, camera parameters, and embedded profiles from images without degrading image quality.

## Features
- **Auto Orientation Fix:** Corrects image orientation (via EXIF tags) before stripping metadata so images don't rotate sideways after cleaning.
- **Batch & Recursive Processing:** Handles single files or entire nested folder trees.
- **Lossless Quality Retention:** Uses pixel remapping to ensure zero EXIF residual artefacts.
- **Safe Out-of-Place By Default:** Prevents accidental overwrites unless `--in-place` is explicitly specified.

## Installation

```bash
pip install -r requirements.txt
```

## Usage

1. Dry Run (Preview matching files)
```bash
python exif_clean.py /path/to/photos -r -n
```

2. Scrub a Directory (Outputs to a new `photos_clean/` directory)
```bash
python exif_clean.py /path/to/photos -r
```

3. Scrub In-Place and Preserve Original File Timestamps
```bash
python exif_clean.py /path/to/photos -r -i -p
```

4. Output to a Custom Folder
```bash
python exif_clean.py /path/to/photos -r -o /path/to/sanitized_photos
```

