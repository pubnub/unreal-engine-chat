#!/usr/bin/env python3
"""
Emit AUTOMATION_FILTER value for Unreal: OR (+) of all test name prefixes that end at
`.1Validation` under PubnubChat.Integration (from IMPLEMENT_* test strings in .cpp).

Unreal matches tests by prefix; there is no single common prefix for all 1Validation tests,
so we join one prefix per API group.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    tests_root = script_dir.parent / "Source/PubnubChatSDKTests"
    if not tests_root.is_dir():
        print(f"pubnubchat_integration_1validation_filter: not found: {tests_root}", file=sys.stderr)
        return 1

    pat = re.compile(r'"(PubnubChat\.Integration[^"]*?\.1Validation)\.[^"]+"')
    groups: set[str] = set()
    for p in tests_root.rglob("*.cpp"):
        t = p.read_text(encoding="utf-8", errors="replace")
        for m in pat.finditer(t):
            groups.add(m.group(1))

    if not groups:
        print(
            "pubnubchat_integration_1validation_filter: no PubnubChat.Integration.*.1Validation tests found",
            file=sys.stderr,
        )
        return 1

    filt = "+".join(sorted(groups))
    sys.stdout.write(filt)
    print(
        f"\npubnubchat_integration_1validation_filter: {len(groups)} OR groups, {len(filt)} chars",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
