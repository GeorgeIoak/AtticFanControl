#!/usr/bin/env python3
import argparse
from pathlib import Path
import gzip

MAX_DELIM_LEN = 16

def pick_delimiter(html: str) -> str:
    candidates = ["EMB1","EMB2","EMB3","HTML","RAW1","RAW2","RAWX","Z","YY","QQ","AA","BB"]
    for c in candidates:
        if c not in html:
            return c
    for i in range(1000, 9999):
        d = f"E{i}"
        if len(d) <= MAX_DELIM_LEN and d not in html:
            return d
    raise RuntimeError("Could not find a suitable raw-string delimiter not present in HTML.")

HEADER_PREAMBLE = """#pragma once
#include <pgmspace.h>

// Auto-generated from {src_name}. Do not edit by hand.
// To define the chunked handler automatically, keep WEBUI_EMIT_STREAM_HELPER=1
#ifndef WEBUI_EMIT_STREAM_HELPER
#define WEBUI_EMIT_STREAM_HELPER 1
#endif
"""

STREAM_HELPER = r"""
#if WEBUI_EMIT_STREAM_HELPER
// NOTE: Assumes you have a global 'ESP8266WebServer server(80);'
// If your instance is named differently, set WEBUI_EMIT_STREAM_HELPER=0
// and paste a custom handler in your route file.
#include <ESP8266WebServer.h>                                                   
static void {func_name}() {{
  extern ESP8266WebServer server;                                              
  server.sendHeader("Connection", "close");                                   
  server.send_P(200, "text/html",                                               
                {var_name},
                sizeof({var_name}) - 1);
}}
#endif
"""
TEMPLATE_PLAIN = """{preamble}
const char EMBEDDED_WEBUI[] PROGMEM = R"EMB1(
{html}
)EMB1";"""

TEMPLATE_PLAIN_NAMED = """{preamble}
const char {var_name}[] PROGMEM = R"EMB1(
{html}
)EMB1";
{stream_helper}
"""

TEMPLATE_GZIP = """{preamble}
{stream_helper}
const unsigned char EMBEDDED_WEBUI[] PROGMEM = {{
{bytes}
}};
const size_t EMBEDDED_WEBUI_LEN = sizeof(EMBEDDED_WEBUI);
"""

def main():
    ap = argparse.ArgumentParser(description="Embed an HTML file into a C header for ESP8266/ESP32.")
    ap.add_argument("input", help="Path to index.html")
    ap.add_argument("output", help="Path to write header, e.g. webui_embedded.h")
    ap.add_argument("--var-name", default="EMBEDDED_WEBUI", help="Name for the PROGMEM char array variable.")
    ap.add_argument("--func-name", default="handleEmbeddedWebUI", help="Name for the generated handler function.")
    ap.add_argument("--no-stream-helper", action="store_true", help="Do not emit the WEBUI_DEFINE_CHUNKED_HANDLER macro")
    ap.add_argument("--gzip", action="store_true", help="Store gzipped payload instead of plain HTML")
    args = ap.parse_args()

    src = Path(args.input)
    dst = Path(args.output)
    html = src.read_text(encoding="utf-8")

    preamble = HEADER_PREAMBLE.format(src_name=src.name)
    stream_helper = "" if args.no_stream_helper else STREAM_HELPER.format(var_name=args.var_name, func_name=args.func_name)

    if args.gzip:
        gz = gzip.compress(html.encode("utf-8"))
        lines = []
        line = []
        for b in gz:
            line.append(f"0x{b:02x}")
            if len(line) == 16:
                lines.append(", ".join(line))
                line = []
        if line:
            lines.append(", ".join(line))
        content = TEMPLATE_GZIP.format(
            preamble=preamble,
            stream_helper=stream_helper,
            bytes=",\n".join(lines),
        )
    else:
        delim = pick_delimiter(html)
        content = TEMPLATE_PLAIN_NAMED.format(
            preamble=preamble,
            stream_helper=stream_helper,
            html=html,
            var_name=args.var_name
        )

    dst.write_text(content, encoding="utf-8")
    print(f"Wrote {dst} ({'gzipped' if args.gzip else 'plain'}; stream helper {'ON' if not args.no_stream_helper else 'OFF'})")

if __name__ == "__main__":
    main()
