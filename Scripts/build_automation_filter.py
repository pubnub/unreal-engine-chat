#!/usr/bin/env python3
"""
Emit the default Automation RunTest filter for PubnubChat CI:
  PubnubChat.Unit + every PubnubChat.Integration.* prefix for these scenario segments:
    2HappyPath, 1Validation, 3FullParameters
  (each OR'd with '+').

Unreal matches tests by prefix; there is no single glob for 'any path containing .2HappyPath.',
so we OR one prefix per distinct ...<segment> parent (derived from IMPLEMENT_* strings).
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

# Integration test path segments to include in CI (same naming as in IMPLEMENT_* strings).
SEGMENTS = ("2HappyPath", "1Validation", "3FullParameters")


def prefix_up_to_segment(path: str, segment: str) -> str | None:
    """Return path truncated after the `.segment` token (inclusive), or None if not present."""
    needle = f".{segment}."
    idx = path.find(needle)
    if idx != -1:
        return path[: idx + len(f".{segment}")]
    if path.endswith(f".{segment}"):
        return path
    return None


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    plugin_root = script_dir.parent
    tests_root = plugin_root / "Source" / "PubnubChatSDKTests"
    if not tests_root.is_dir():
        print(f"ERROR: Test sources not found: {tests_root}", file=sys.stderr)
        return 1

    prefixes: set[str] = set()
    for p in tests_root.rglob("*.cpp"):
        text = p.read_text(encoding="utf-8", errors="replace")
        for segment in SEGMENTS:
            pat = re.compile(rf'"(PubnubChat\.Integration\.[^"]*{re.escape(segment)}[^"]*)"')
            for m in pat.finditer(text):
                s = m.group(1)
                pref = prefix_up_to_segment(s, segment)
                if pref:
                    prefixes.add(pref)

    if not prefixes:
        print(
            "ERROR: No PubnubChat.Integration paths with 2HappyPath, 1Validation, or 3FullParameters found.",
            file=sys.stderr,
        )
        return 1

    parts = ["PubnubChat.Unit"] + sorted(prefixes)
    print("+".join(parts))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
