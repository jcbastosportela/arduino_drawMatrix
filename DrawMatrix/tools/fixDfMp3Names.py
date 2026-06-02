"""
Filename: /home/portela/projects/Arduino/DrawMatrix/tools/fixDfMp3Names.py
Path: /home/portela/projects/Arduino/DrawMatrix/tools
Created Date: Sunday, October 5th 2025, 10:19:35 pm
Author: Joao Carlos Bastos Portela

Copyright (c) 2025 Your Company
"""

import os
import argparse


def rename_mp3_files(folder, dry_run=False):
    files = sorted(
        [f for f in os.listdir(folder) if os.path.isfile(os.path.join(folder, f))]
    )

    for i, filename in enumerate(files, start=1):
        prefix = f"{i:03d}-"
        new_name = prefix + filename
        old_path = os.path.join(folder, filename)
        new_path = os.path.join(folder, new_name)

        # Avoid renaming if it already has the prefix
        if not filename.startswith(prefix):
            if dry_run:
                print(f"Dry run: {filename} → {new_name}")
            else:
                os.rename(old_path, new_path)
                print(f"Renamed: {filename} → {new_name}")


def main():
    parser = argparse.ArgumentParser(description="Rename MP3 files in a folder")
    parser.add_argument(
        "folder", nargs="?", default=".", help="Folder to scan for MP3 files"
    )
    parser.add_argument("-n", "--dry-run", action="store_true", help="Dry run mode")
    args = parser.parse_args()
    rename_mp3_files(args.folder, args.dry_run)


if __name__ == "__main__":
    main()
