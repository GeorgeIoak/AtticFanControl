#!/usr/bin/env python3
"""
A cross-platform script to manage embedding web UI files.

Usage:
  - python3 manage_ui.py update:  Regenerates all embedded C++ headers.
  - python3 manage_ui.py watch:   Watches source files and auto-regenerates headers on change.
  - python3 manage_ui.py buildfs: Builds the LittleFS filesystem.bin image from the /data folder.
"""
import argparse
import time
import subprocess
from pathlib import Path
import sys
import shutil

# --- Configuration ---
# Add file pairs here to manage more embedded files.
UI_FILES_TO_MANAGE = {
    "data/index.html": "webui_embedded.h",
    "data/help.html": "help_page.h"
}
POLL_INTERVAL_S = 1.0  # How often to check for file changes in watch mode.
# ---------------------

def run_embed_script(input_path: Path, output_path: Path):
    """Calls the embed_html.py script for a single file pair."""
    embed_script_path = Path(__file__).parent / "embed_html.py"
    if not embed_script_path.exists():
        print(f"[ERROR] embed_html.py not found at: {embed_script_path}", file=sys.stderr)
        return False

    print(f"Regenerating {output_path.name} from {input_path.name}...")
    cmd = [sys.executable, str(embed_script_path), str(input_path), str(output_path)]
    
    res = subprocess.run(cmd, capture_output=True, text=True, check=False)
    
    if res.returncode != 0:
        print(f"  [ERROR] Failed to generate {output_path.name}:\n{res.stderr}", file=sys.stderr)
        return False
    else:
        print(f"  > Success.")
        return True

def update_all_files():
    """Runs the update process for all configured files once."""
    print("Checking embedded UI files for updates...")
    files_updated = 0
    for src, dest in UI_FILES_TO_MANAGE.items():
        src_path = Path(src)
        dest_path = Path(dest)
        # Regenerate if destination doesn't exist or if source is newer
        if not dest_path.exists() or src_path.stat().st_mtime > dest_path.stat().st_mtime:
            if run_embed_script(src_path, dest_path):
                files_updated += 1
        else:
            print(f"{dest_path.name} is already up-to-date.")
    
    if files_updated > 0:
        print(f"\nSuccessfully updated {files_updated} file(s). They have been added to your commit.")
    print("Check complete.")

def build_filesystem():
    """Builds the LittleFS filesystem image from the 'data' directory."""
    print("Building LittleFS filesystem image (filesystem.bin)...")
    mklittlefs_path = shutil.which("mklittlefs")
    if not mklittlefs_path:
        print(
            "[ERROR] 'mklittlefs' command not found in your system's PATH.\n"
            "Please install it from: https://github.com/earlephilhower/mklittlefs/releases",
            file=sys.stderr
        )
        return False

    cmd = [
        mklittlefs_path,
        "-c", "data",      # create from 'data' directory
        "-b", "4096",      # block size
        "-p", "256",       # page size
        "-s", "2048000",   # total size for 2MB FS partition
        "filesystem.bin"   # output file
    ]
    
    print(f"  > Running command: {' '.join(cmd)}")
    res = subprocess.run(cmd, capture_output=True, text=True, check=False)
    
    if res.returncode != 0:
        print(f"  [ERROR] Failed to build filesystem image:\n{res.stderr}", file=sys.stderr)
        return False
    else:
        print("  > Success. 'filesystem.bin' created.")
        return True

def watch_all_files():
    """Watches all source files and regenerates them upon modification."""
    print("Starting watcher... Press Ctrl+C to stop.")
    last_mtimes = {Path(p): 0 for p in UI_FILES_TO_MANAGE.keys()}

    # Initial check and population of modification times
    for p in last_mtimes:
        if p.exists():
            last_mtimes[p] = p.stat().st_mtime

    try:
        while True:
            time.sleep(POLL_INTERVAL_S)
            for src_path, last_mtime in last_mtimes.items():
                if src_path.exists() and (mtime := src_path.stat().st_mtime) != last_mtime:
                    dest_path = Path(UI_FILES_TO_MANAGE[str(src_path)])
                    if run_embed_script(src_path, dest_path):
                        last_mtimes[src_path] = mtime
    except KeyboardInterrupt:
        print("\nWatcher stopped.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Manage web UI files and filesystem for the Attic Fan project.")
    parser.add_argument("command", choices=["update", "watch", "buildfs"], help="'update' for headers, 'watch' for monitoring, 'buildfs' to create filesystem.bin.")
    args = parser.parse_args()

    if args.command == "update":
        update_all_files()
    elif args.command == "watch":
        watch_all_files()
    elif args.command == "buildfs":
        build_filesystem()