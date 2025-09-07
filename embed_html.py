#!/usr/bin/env python3
import argparse
from pathlib import Path
import sys
import gzip

MAX_DELIM_LEN = 16

def pick_delimiter(html: str) -> str:
    # Try a set of short delimiters unlikely to appear
    candidates = ["EMB1","EMB2","EMB3","HTML","RAW1","RAW2","RAWX","Z","YY","QQ","AA","BB"]
    for c in candidates:
        if c not in html:
            return c
    # Fallback: generate short numeric suffixes
    for i in range(1000, 9999):
        d = f"E{i}"
        if len(d) <= MAX_DELIM_LEN and d not in html:
            return d
    raise RuntimeError("Could not find a suitable raw-string delimiter not present in HTML.")

TEMPLATE_PLAIN = """#pragma once

// Auto-generated from {src_name}. Do not edit by hand.
#ifndef USE_FS_WEBUI
#define USE_FS_WEBUI 0
#endif

#if !USE_FS_WEBUI
#include <pgmspace.h>
const char {symbol}[] PROGMEM = R"{delim}(
{html}
){delim}";
#endif
"""

TEMPLATE_GZIP = """#pragma once

// Auto-generated from {src_name} (gzipped). Do not edit by hand.
#ifndef USE_FS_WEBUI
#define USE_FS_WEBUI 0
#endif

#if !USE_FS_WEBUI
#include <pgmspace.h>
const unsigned char {symbol}[] PROGMEM = {{
{bytes}
}};
const size_t {symbol}_LEN = sizeof({symbol});
// Remember to set 'Content-Encoding: gzip' when sending this.
#endif
"""

def main():
    ap = argparse.ArgumentParser(description="Embed an HTML file into a C header for ESP8266/ESP32.")
    ap.add_argument("input", help="Path to index.html")
    ap.add_argument("output", help="Path to write header, e.g. webui_embedded.h")
    ap.add_argument("--symbol", default="EMBEDDED_WEBUI", help="C symbol name (default: EMBEDDED_WEBUI)")
    ap.add_argument("--gzip", action="store_true", help="Store gzipped payload instead of plain HTML")
    args = ap.parse_args()

    src = Path(args.input)
    dst = Path(args.output)
    html = src.read_text(encoding="utf-8")

    if args.gzip:
        gz = gzip.compress(html.encode("utf-8"))
        # Pretty hexdump as C array
        lines = []
        line = []
        for i,b in enumerate(gz):
            line.append(f"0x{b:02x}")
            if len(line) == 16:
                lines.append(", ".join(line))
                line = []
        if line:
            lines.append(", ".join(line))
        content = TEMPLATE_GZIP.format(
            src_name=src.name,
            symbol=args.symbol,
            bytes=",\n".join(lines),
        )
    else:
        delim = pick_delimiter(html)
        content = TEMPLATE_PLAIN.format(
            src_name=src.name,
            symbol=args.symbol,
            delim=delim,
            html=html
        )

    dst.write_text(content, encoding="utf-8")
    print(f"Wrote {dst} ({'gzipped' if args.gzip else 'plain'})")

if __name__ == "__main__":
    main()
