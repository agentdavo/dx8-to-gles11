#!/usr/bin/env python3
"""Summarise opcode coverage based on a usage report.

The report is a plain text file with one opcode per line optionally
followed by a usage count, e.g.::

    mov 10
    dp4 3

Lines starting with ``#`` are ignored.
"""
from __future__ import annotations

import os
import re
import sys
from pathlib import Path


def parse_report(path: Path) -> dict[str, int]:
    tokens: dict[str, int] = {}
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = re.split(r"[:\s]+", line, maxsplit=1)
            token = parts[0]
            count = 1
            if len(parts) > 1 and parts[1].strip():
                try:
                    count = int(parts[1])
                except ValueError:
                    pass
            tokens[token] = tokens.get(token, 0) + count
    return tokens


def load_sources(src_dir: Path) -> str:
    text_parts = []
    for root, _dirs, files in os.walk(src_dir):
        for name in files:
            if name.endswith(('.c', '.h')):
                p = Path(root) / name
                try:
                    text_parts.append(p.read_text(encoding='utf-8'))
                except OSError:
                    continue
    return "\n".join(text_parts)


def write_markdown(tokens: dict[str, int], sources: str, out_path: Path) -> None:
    rows: list[tuple[str, int, bool]] = []
    covered = 0
    for token, count in sorted(tokens.items()):
        implemented = f'"{token}"' in sources
        rows.append((token, count, implemented))
        if implemented:
            covered += 1

    total = len(tokens)
    ratio = (covered / total * 100) if total else 0.0

    md_lines = [
        "# Opcode coverage",
        "",
        "| Opcode | Count | Implemented |",
        "| ------ | ----: | :--------- |",
    ]
    for token, count, imp in rows:
        md_lines.append(f"| {token} | {count} | {'yes' if imp else 'no'} |")
    md_lines.append("")
    md_lines.append(f"Covered {covered}/{total} opcodes ({ratio:.1f}%)")
    out_path.write_text("\n".join(md_lines), encoding="utf-8")


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print("usage: python tools/usage_coverage.py REPORT [SRC_DIR]", file=sys.stderr)
        return 1
    report_path = Path(argv[1])
    src_dir = Path(argv[2]) if len(argv) > 2 else Path('src')
    if not report_path.is_file():
        print(f"report not found: {report_path}", file=sys.stderr)
        return 1
    tokens = parse_report(report_path)
    sources = load_sources(src_dir)
    covered = 0
    print(f"Coverage for report: {report_path}")
    for token, count in sorted(tokens.items()):
        implemented = f'"{token}"' in sources
        if implemented:
            covered += 1
        status = 'yes' if implemented else 'no'
        print(f"{token:10} {count:5d} implemented: {status}")
    total = len(tokens)
    ratio = (covered / total * 100) if total else 0.0
    print(f"\nCovered {covered}/{total} tokens ({ratio:.1f}%)")

    write_markdown(tokens, sources, Path("COVERAGE.md"))
    print("Saved coverage table to COVERAGE.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
