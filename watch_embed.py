#!/usr/bin/env python3
"""
Tiny polling watcher: regenerates header when index.html changes.
Usage:
  python3 watch_embed.py --input path/to/index.html --output path/to/webui_embedded.h [--gzip] [--symbol NAME]
Stop with Ctrl+C.
"""
import time
import subprocess
from pathlib import Path
import argparse
import sys
import os

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", required=True)
    ap.add_argument("--output", required=True)
    ap.add_argument("--symbol", default="EMBEDDED_WEBUI")
    ap.add_argument("--gzip", action="store_true")
    ap.add_argument("--interval", type=float, default=0.5, help="poll interval seconds (default 0.5)")
    args = ap.parse_args()

    src = Path(args.input)
    if not src.exists():
        print(f"Input not found: {src}", file=sys.stderr)
        sys.exit(1)

    out = Path(args.output)
    last_mtime = None

    cmd = [sys.executable, str(Path(__file__).parent / "embed_html.py"), str(src), str(out), "--symbol", args.symbol]
    if args.gzip:
        cmd.append("--gzip")

    print(f"Watching {src} → {out}")
    while True:
        try:
            m = src.stat().st_mtime
            if last_mtime is None or m != last_mtime:
                last_mtime = m
                print("Change detected. Regenerating...")
                res = subprocess.run(cmd, capture_output=True, text=True)
                if res.returncode != 0:
                    print("Generation failed:\n", res.stdout, res.stderr, file=sys.stderr)
                else:
                    print(res.stdout.strip())
            time.sleep(args.interval)
        except KeyboardInterrupt:
            print("\nWatcher stopped.")
            break

if __name__ == "__main__":
    main()
