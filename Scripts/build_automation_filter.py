#!/usr/bin/env python3
"""
Emit the default Automation RunTest filter for PubnubChat CI:
  PubnubChat.Unit + every PubnubChat.Integration.*.2HappyPath.* prefix (OR with '+').

Unreal matches tests by prefix; there is no single glob for 'any path containing .2HappyPath.',
so we OR one prefix per distinct ...2HappyPath parent (derived from IMPLEMENT_* strings).
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    plugin_root = script_dir.parent
    tests_root = plugin_root / "Source" / "PubnubChatSDKTests"
    if not tests_root.is_dir():
        print(f"ERROR: Test sources not found: {tests_root}", file=sys.stderr)
        return 1

    pat = re.compile(r'"(PubnubChat\.Integration\.[^"]*2HappyPath[^"]*)"')
    prefixes: set[str] = set()
    for p in tests_root.rglob("*.cpp"):
        text = p.read_text(encoding="utf-8", errors="replace")
        for m in pat.finditer(text):
            s = m.group(1)
            idx = s.find(".2HappyPath.")
            if idx == -1:
                if s.endswith(".2HappyPath"):
                    pref = s
                else:
                    continue
            else:
                pref = s[: idx + len(".2HappyPath")]
            prefixes.add(pref)

    if not prefixes:
        print("ERROR: No PubnubChat.Integration.*.2HappyPath test paths found.", file=sys.stderr)
        return 1

    parts = ["PubnubChat.Unit"] + sorted(prefixes)
    print("+".join(parts))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
