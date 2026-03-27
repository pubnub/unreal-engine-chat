#!/usr/bin/env python3
"""
Build GitHub Job Summary markdown from Unreal editor log: pair Test Started / Test Completed
by Path, attach log lines between them. Emit <details> for failures; passes are counted only.

Optional index.json for aggregate counts when present (authoritative).
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional, Tuple


_RE_STARTED = re.compile(r"Test Started\.\s*(?:Name=\{[^}]*\}\s*)?Path=\{([^}]+)\}")
_RE_COMPLETED = re.compile(
    r"Test Completed\.\s*Result=\{([^}]+)\}(?:\s*Name=\{[^}]*\})?\s*Path=\{([^}]+)\}"
)


@dataclass
class Segment:
    path: str
    result: str
    lines: List[str] = field(default_factory=list)


def _redact_line(s: str) -> str:
    """Light redaction for common key patterns in Pubnub logs (CI safety)."""
    out = re.sub(r"\b(pub-c-[a-f0-9-]{10,})", "[REDACTED]", s, flags=re.I)
    out = re.sub(r"\b(sub-c-[a-f0-9-]{10,})", "[REDACTED]", out, flags=re.I)
    return out


def parse_segments(text: str) -> Tuple[List[Segment], List[str]]:
    """
    Sequential Started→Completed by matching Path. Returns segments and orphan lines
    (Started without Completed before EOF).
    """
    lines = text.splitlines()
    segments: List[Segment] = []
    orphans: List[str] = []
    state = "out"
    current_path: Optional[str] = None
    buf: List[str] = []

    for line in lines:
        ms = _RE_STARTED.search(line)
        mc = _RE_COMPLETED.search(line)

        if state == "out" and ms:
            current_path = ms.group(1).strip()
            buf = [line]
            state = "in"
            continue

        if state == "in":
            buf.append(line)
            if mc:
                result, path_completed = mc.group(1).strip(), mc.group(2).strip()
                if path_completed == current_path:
                    segments.append(Segment(path=current_path, result=result, lines=buf))
                    state = "out"
                    buf = []
                    current_path = None
                # else: mismatched Completed — keep buffering (rare)
            continue

        # state == out, no new Started
        continue

    if state == "in" and buf:
        orphans.extend(buf)

    return segments, orphans


def classify_result(result: str) -> str:
    r = result.strip()
    rl = r.lower()
    if "fail" in rl:
        return "failed"
    if "warning" in rl and "success" in rl:
        return "warned"
    return "passed"


def emit_summary_md(
    segments: List[Segment],
    orphans: List[str],
    json_path: Optional[Path],
    max_bytes: int,
    redact: bool,
) -> str:
    parts: List[str] = []

    succeeded = failed = warned = 0
    if json_path and json_path.is_file():
        try:
            with open(json_path, encoding="utf-8-sig") as f:
                data = json.load(f)
            succeeded = int(data.get("succeeded", 0))
            failed = int(data.get("failed", 0))
            warned = int(data.get("succeededWithWarnings", 0))
            parts.append("### Test run summary")
            parts.append("")
            parts.append("| Metric | Count |")
            parts.append("|--------|-------|")
            parts.append(f"| Passed | {succeeded} |")
            parts.append(f"| Failed | {failed} |")
            parts.append(f"| Warnings | {warned} |")
            parts.append("")
            parts.append("_Counts from `Saved/TestReport/index.json`._")
            parts.append("")
        except (OSError, json.JSONDecodeError, TypeError, ValueError) as e:
            parts.append(f"_(Could not read index.json: {e})_\n")
            succeeded = failed = warned = 0
            for seg in segments:
                c = classify_result(seg.result)
                if c == "failed":
                    failed += 1
                elif c == "warned":
                    warned += 1
                else:
                    succeeded += 1
            parts.append("### Test run summary")
            parts.append("")
            parts.append("| Metric | Count |")
            parts.append("|--------|-------|")
            parts.append(f"| Passed | {succeeded} |")
            parts.append(f"| Failed | {failed} |")
            parts.append(f"| With warnings | {warned} |")
            parts.append("")
            parts.append("_Counts from log segments (index.json unavailable)._\n")
    else:
        for seg in segments:
            c = classify_result(seg.result)
            if c == "failed":
                failed += 1
            elif c == "warned":
                warned += 1
            else:
                succeeded += 1
        parts.append("### Test run summary")
        parts.append("")
        parts.append("| Metric | Count |")
        parts.append("|--------|-------|")
        parts.append(f"| Passed | {succeeded} |")
        parts.append(f"| Failed | {failed} |")
        parts.append(f"| With warnings | {warned} |")
        parts.append("")
        parts.append("_Counts from log segments._")
        parts.append("")

    failed_segs = [s for s in segments if classify_result(s.result) == "failed"]
    warn_segs = [s for s in segments if classify_result(s.result) == "warned"]

    if failed_segs:
        parts.append("### Failed tests (expand for log)")
        parts.append("")
        for seg in failed_segs:
            body = "\n".join(seg.lines)
            if redact:
                body = "\n".join(_redact_line(x) for x in body.splitlines())
            if len(body.encode("utf-8")) > max_bytes:
                body = body.encode("utf-8")[:max_bytes].decode("utf-8", errors="ignore")
                body += "\n\n… (truncated; increase --max-bytes-per-block or use debug logging)\n"
            safe_path = seg.path.replace("|", "\\|")
            parts.append(f"<details>")
            parts.append(f"<summary><strong>❌</strong> <code>{safe_path}</code></summary>")
            parts.append("")
            parts.append("```text")
            parts.append(body)
            parts.append("```")
            parts.append("")
            parts.append("</details>")
            parts.append("")

    if warn_segs:
        parts.append("### Tests completed with warnings (expand for log)")
        parts.append("")
        for seg in warn_segs:
            body = "\n".join(seg.lines)
            if redact:
                body = "\n".join(_redact_line(x) for x in body.splitlines())
            if len(body.encode("utf-8")) > max_bytes:
                body = body.encode("utf-8")[:max_bytes].decode("utf-8", errors="ignore")
                body += "\n\n… (truncated)\n"
            safe_path = seg.path.replace("|", "\\|")
            parts.append("<details>")
            parts.append(f"<summary><strong>⚠️</strong> <code>{safe_path}</code></summary>")
            parts.append("")
            parts.append("```text")
            parts.append(body)
            parts.append("```")
            parts.append("")
            parts.append("</details>")
            parts.append("")

    if orphans:
        parts.append("### Incomplete log segment (no matching Test Completed)")
        parts.append("")
        ob = "\n".join(orphans)
        if redact:
            ob = "\n".join(_redact_line(x) for x in ob.splitlines())
        parts.append("```text")
        parts.append(ob[:max_bytes] if len(ob.encode("utf-8")) > max_bytes else ob)
        parts.append("```")
        parts.append("")

    if not failed_segs and not warn_segs and not segments:
        parts.append("_No Test Started / Completed pairs found in log — check log format._")
        parts.append("")

    return "\n".join(parts)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--log", type=Path, required=True)
    ap.add_argument("--json", type=Path, help="Saved/TestReport/index.json for counts")
    ap.add_argument("--max-bytes-per-block", type=int, default=65536)
    ap.add_argument(
        "--redact",
        action="store_true",
        help="Redact pub-c-/sub-c- style key fragments in slices",
    )
    args = ap.parse_args()

    if not args.log.is_file():
        print(f"automation_log_split_report: log not found: {args.log}", file=sys.stderr)
        return 1

    text = args.log.read_text(encoding="utf-8", errors="replace")
    segments, orphans = parse_segments(text)
    md = emit_summary_md(segments, orphans, args.json, args.max_bytes_per_block, args.redact)
    sys.stdout.write(md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
