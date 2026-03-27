#!/usr/bin/env python3
"""
Emit a compact Markdown table of Unreal automation test results for CI (e.g. GITHUB_STEP_SUMMARY).

Tries index.json first (walks nested dicts/lists for UE-style entries), then parses editor log lines:
  LogAutomationController: Display: Test Completed. Result={Success} Path={...}
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any, List, Tuple


def _md_cell(s: str) -> str:
    return s.replace("|", "\\|").replace("\n", " ").strip()


def collect_tests_from_json(obj: Any, out: List[Tuple[str, str]]) -> None:
    """Collect (fullTestPath, state) from typical UE automation export shapes."""
    if isinstance(obj, dict):
        path = (
            obj.get("fullTestPath")
            or obj.get("FullTestPath")
            or obj.get("testPath")
            or obj.get("TestPath")
        )
        state = (
            obj.get("state")
            or obj.get("State")
            or obj.get("result")
            or obj.get("Result")
        )
        if path and state:
            out.append((str(path), str(state)))
        for v in obj.values():
            collect_tests_from_json(v, out)
    elif isinstance(obj, list):
        for item in obj:
            collect_tests_from_json(item, out)


# UE log: Test Completed. Result={Success} Name={...} Path={PubnubChat.Unit...}
_LOG_COMPLETED = re.compile(
    r"Test Completed\.\s*Result=\{([^}]+)\}(?:\s*Name=\{[^}]*\})?\s*Path=\{([^}]+)\}"
)


def collect_tests_from_log(text: str) -> List[Tuple[str, str]]:
    """Last occurrence per path wins (re-runs / noise)."""
    by_path: dict[str, str] = {}
    for m in _LOG_COMPLETED.finditer(text):
        result, path = m.group(1).strip(), m.group(2).strip()
        by_path[path] = result
    return sorted(by_path.items(), key=lambda x: x[0].lower())


def emit_markdown(rows: List[Tuple[str, str]], title: str) -> str:
    if not rows:
        return f"_{title}: no per-test rows found._\n"
    lines = [
        f"#### {title}",
        "",
        "| Result | Test path |",
        "|--------|-----------|",
    ]
    for path, state in rows:
        lines.append(f"| {_md_cell(state)} | {_md_cell(path)} |")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--json", type=Path, help="Saved/TestReport/index.json")
    ap.add_argument("--log", type=Path, help="Editor log (e.g. test_run.log)")
    ap.add_argument(
        "--source-note",
        action="store_true",
        help="Append which source was used (json vs log)",
    )
    args = ap.parse_args()

    rows: List[Tuple[str, str]] = []
    source = ""

    if args.json and args.json.is_file():
        try:
            with open(args.json, encoding="utf-8-sig") as f:
                data = json.load(f)
            buf: List[Tuple[str, str]] = []
            collect_tests_from_json(data, buf)
            if buf:
                # Deduplicate by path, keep last
                by_path: dict[str, str] = {}
                for p, s in buf:
                    by_path[p] = s
                rows = sorted(by_path.items(), key=lambda x: x[0].lower())
                source = "index.json"
        except (OSError, json.JSONDecodeError) as e:
            print(f"parse_automation_ci_summary: JSON skipped: {e}", file=sys.stderr)

    if not rows and args.log and args.log.is_file():
        try:
            text = args.log.read_text(encoding="utf-8", errors="replace")
            rows = collect_tests_from_log(text)
            source = "editor log"
        except OSError as e:
            print(f"parse_automation_ci_summary: log read failed: {e}", file=sys.stderr)

    title = "Per-test results"
    out = emit_markdown(rows, title)
    if args.source_note and source:
        out += f"\n_Source: {source}._\n"
    sys.stdout.write(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
