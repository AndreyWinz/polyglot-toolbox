#!/usr/bin/env python3
"""
exif-clean - Recursive EXIF & Image Metadata Scrubber
Strips EXIF, GPS, camera profiles, and embedded IPTC/XMP data from images.
"""

import sys
import os
import argparse
from pathlib import Path
from typing import List, Tuple
from PIL import Image, ImageOps

SUPPORTED_EXTENSIONS = {".jpg", ".jpeg", ".png", ".webp", ".tiff", ".tif"}


def is_image(file_path: Path) -> bool:
    """Check if the file has a supported image extension."""
    return file_path.suffix.lower() in SUPPORTED_EXTENSIONS


def clean_image(
    src_path: Path,
    dest_path: Path,
    preserve_mtime: bool = False,
    dry_run: bool = False,
) -> bool:
    """
    Opens an image, strips metadata by saving raw pixel data, and writes to dest_path.
    """
    if dry_run:
        return True

    try:
        # Save original modification time if needed
        original_mtime = src_path.stat().st_mtime if preserve_mtime else None

        with Image.open(src_path) as img:
            # Correct orientation based on EXIF before stripping it out
            try:
                img = ImageOps.exif_transpose(img)
            except Exception:
                pass  # Ignore orientation transpose errors on weird metadata

            # Extract image mode & pixel data (discarding img.info metadata dictionary)
            data = list(img.getdata())
            clean_img = Image.new(img.mode, img.size)
            clean_img.putdata(data)

            # Ensure parent directories exist
            dest_path.parent.mkdir(parents=True, exist_ok=True)

            # Determine format options for high quality output
            fmt = img.format if img.format else src_path.suffix[1:].upper()
            save_kwargs = {}

            if fmt in ("JPEG", "JPG"):
                save_kwargs["quality"] = "keep" if hasattr(img, "quality") else 95
                save_kwargs["subsampling"] = 0
            elif fmt == "PNG":
                save_kwargs["optimize"] = True
            elif fmt == "WEBP":
                save_kwargs["quality"] = 95
                save_kwargs["lossless"] = True

            clean_img.save(dest_path, format=fmt, **save_kwargs)

        # Restore modified timestamp if requested
        if preserve_mtime and original_mtime is not None:
            os.utime(dest_path, (original_mtime, original_mtime))

        return True

    except Exception as e:
        print(f"❌ Error processing {src_path.name}: {e}", file=sys.stderr)
        return False


def collect_files(target: Path, recursive: bool) -> List[Path]:
    """Gather all valid image files from a given path."""
    if target.is_file():
        return [target] if is_image(target) else []

    if target.is_dir():
        pattern = "**/*" if recursive else "*"
        return [p for p in target.glob(pattern) if p.is_file() and is_image(p)]

    return []


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Recursively strip EXIF, GPS, and metadata from images."
    )
    parser.add_argument(
        "target",
        type=Path,
        help="Target file or directory containing images.",
    )
    parser.add_argument(
        "-r",
        "--recursive",
        action="store_true",
        help="Recursively scan subdirectories.",
    )
    parser.add_argument(
        "-i",
        "--in-place",
        action="store_true",
        help="Overwrite original files with scrubbed versions.",
    )
    parser.add_argument(
        "-o",
        "--output-dir",
        type=Path,
        default=None,
        help="Output directory for scrubbed files (default: appends '_clean' if not in-place).",
    )
    parser.add_argument(
        "-p",
        "--preserve-mtime",
        action="store_true",
        help="Preserve original file modification timestamp.",
    )
    parser.add_argument(
        "-n",
        "--dry-run",
        action="store_true",
        help="Simulate actions without saving any files.",
    )

    args = parser.parse_args()

    if not args.target.exists():
        print(f"Error: Target path '{args.target}' does not exist.", file=sys.stderr)
        sys.exit(1)

    files = collect_files(args.target, args.recursive)

    if not files:
        print("No supported image files found.")
        sys.exit(0)

    print(f"🔍 Found {len(files)} image(s) to process.")
    if args.dry_run:
        print("⚡ [DRY RUN] No files will be modified on disk.\n")

    success_count = 0

    for file_path in files:
        # Determine target output destination
        if args.in_place:
            dest_path = file_path
        elif args.output_dir:
            if args.target.is_dir():
                rel_path = file_path.relative_to(args.target)
                dest_path = args.output_dir / rel_path
            else:
                dest_path = args.output_dir / file_path.name
        else:
            # Default behavior: create a _cleaned folder next to source file/dir
            if args.target.is_dir():
                clean_dir_name = f"{args.target.name}_clean"
                rel_path = file_path.relative_to(args.target)
                dest_path = args.target.parent / clean_dir_name / rel_path
            else:
                dest_path = file_path.parent / f"{file_path.stem}_clean{file_path.suffix}"

        action = "Scrubbing" if not args.in_place else "Scrubbing (in-place)"
        print(f"  ➜ {action}: {file_path}")

        if clean_image(file_path, dest_path, args.preserve_mtime, args.dry_run):
            success_count += 1

    print(f"\n✨ Done! Processed {success_count}/{len(files)} image(s) successfully.")


if __name__ == "__main__":
    main()
