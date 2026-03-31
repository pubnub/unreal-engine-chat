#!/usr/bin/env python3
"""
Merge Unreal Saved/TestReport/index.json files by summing succeeded, failed, succeededWithWarnings.
Writes a single JSON for combined reporting (e.g. automation_log_split_report --json).
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("-o", "--output", type=Path, required=True)
    ap.add_argument("inputs", nargs="+", type=Path, help="index.json paths")
    args = ap.parse_args()

    succeeded = failed = warned = 0
    for p in args.inputs:
        if not p.is_file():
            print(f"merge_automation_index_json: skip missing {p}", file=sys.stderr)
            continue
        try:
            with open(p, encoding="utf-8-sig") as f:
                data = json.load(f)
            succeeded += int(data.get("succeeded", 0))
            failed += int(data.get("failed", 0))
            warned += int(data.get("succeededWithWarnings", 0))
        except (OSError, json.JSONDecodeError, TypeError, ValueError) as e:
            print(f"merge_automation_index_json: skip {p}: {e}", file=sys.stderr)

    out = {
        "succeeded": succeeded,
        "failed": failed,
        "succeededWithWarnings": warned,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")
    print(
        f"merge_automation_index_json: wrote {args.output} "
        f"(passed={succeeded} failed={failed} warnings={warned})",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
