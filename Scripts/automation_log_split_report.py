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


# Tolerant of column layout (Display / LogAutomationController / etc.)
_RE_PATH = re.compile(r"Path=\{([^}]+)\}")
_RE_RESULT = re.compile(r"Result=\{([^}]+)\}")


def _line_started_path(line: str) -> Optional[str]:
    if "Test Started" not in line:
        return None
    m = _RE_PATH.search(line)
    return m.group(1).strip() if m else None


def _line_completed_result_paths(line: str) -> Optional[Tuple[str, str]]:
    if "Test Completed" not in line:
        return None
    mr = _RE_RESULT.search(line)
    mp = _RE_PATH.search(line)
    if not mr or not mp:
        return None
    return mr.group(1).strip(), mp.group(1).strip()


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
        started_path = _line_started_path(line)
        completed = _line_completed_result_paths(line)

        if state == "out" and started_path:
            current_path = started_path
            buf = [line]
            state = "in"
            continue

        if state == "in":
            buf.append(line)
            if completed:
                result, path_completed = completed
                if path_completed == current_path:
                    segments.append(Segment(path=current_path, result=result, lines=buf))
                    state = "out"
                    buf = []
                    current_path = None
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


def _segment_counts(segments: List[Segment]) -> Tuple[int, int, int]:
    succeeded = failed = warned = 0
    for seg in segments:
        c = classify_result(seg.result)
        if c == "failed":
            failed += 1
        elif c == "warned":
            warned += 1
        else:
            succeeded += 1
    return succeeded, failed, warned


def _append_summary_table(
    parts: List[str],
    succeeded: int,
    failed: int,
    warned: int,
    footnote: str,
) -> None:
    parts.append("### Test run summary")
    parts.append("")
    parts.append("| Metric | Count |")
    parts.append("|--------|-------|")
    parts.append(f"| Passed | {succeeded} |")
    parts.append(f"| Failed | {failed} |")
    parts.append(f"| Warnings | {warned} |")
    parts.append("")
    parts.append(footnote)
    parts.append("")


def _emit_count_section(
    parts: List[str],
    segments: List[Segment],
    json_path: Optional[Path],
) -> None:
    if json_path and json_path.is_file():
        try:
            with open(json_path, encoding="utf-8-sig") as f:
                data = json.load(f)
            succeeded = int(data.get("succeeded", 0))
            failed = int(data.get("failed", 0))
            warned = int(data.get("succeededWithWarnings", 0))
            _append_summary_table(
                parts,
                succeeded,
                failed,
                warned,
                "_Counts from `Saved/TestReport/index.json`._",
            )
        except (OSError, json.JSONDecodeError, TypeError, ValueError) as e:
            parts.append(f"_(Could not read index.json: {e})_\n")
            succeeded, failed, warned = _segment_counts(segments)
            _append_summary_table(
                parts,
                succeeded,
                failed,
                warned,
                "_Counts from log segments (index.json unavailable)._\n",
            )
    else:
        succeeded, failed, warned = _segment_counts(segments)
        _append_summary_table(
            parts,
            succeeded,
            failed,
            warned,
            "_Counts from log segments._",
        )


def _format_segment_body(seg: Segment, redact: bool, max_bytes: int, trunc_note: str) -> str:
    body = "\n".join(seg.lines)
    if redact:
        body = "\n".join(_redact_line(x) for x in body.splitlines())
    if len(body.encode("utf-8")) > max_bytes:
        body = body.encode("utf-8")[:max_bytes].decode("utf-8", errors="ignore")
        body += trunc_note
    return body


def _emit_detail_blocks(
    parts: List[str],
    title: str,
    segs: List[Segment],
    icon: str,
    redact: bool,
    max_bytes: int,
    trunc_note: str,
) -> None:
    if not segs:
        return
    parts.append(title)
    parts.append("")
    for seg in segs:
        body = _format_segment_body(seg, redact, max_bytes, trunc_note)
        safe_path = seg.path.replace("|", "\\|")
        parts.append("<details>")
        parts.append(f"<summary><strong>{icon}</strong> <code>{safe_path}</code></summary>")
        parts.append("")
        parts.append("```text")
        parts.append(body)
        parts.append("```")
        parts.append("")
        parts.append("</details>")
        parts.append("")


def _emit_orphans_section(parts: List[str], orphans: List[str], redact: bool, max_bytes: int) -> None:
    if not orphans:
        return
    parts.append("### Incomplete log segment (no matching Test Completed)")
    parts.append("")
    ob = "\n".join(orphans)
    if redact:
        ob = "\n".join(_redact_line(x) for x in ob.splitlines())
    parts.append("```text")
    parts.append(ob[:max_bytes] if len(ob.encode("utf-8")) > max_bytes else ob)
    parts.append("```")
    parts.append("")


def _emit_all_paths_list(parts: List[str], segments: List[Segment]) -> None:
    if not segments:
        return
    parts.append("### All tests (paths from log)")
    parts.append("")
    parts.append(
        f"_**{len(segments)}** bounded runs in `test_run.log` (from `Test Started` to matching `Test Completed`)._"
    )
    parts.append("")
    parts.append("<details>")
    parts.append(f"<summary><strong>Expand</strong> — list paths and raw results ({len(segments)})</summary>")
    parts.append("")
    max_rows = 500
    for i, seg in enumerate(segments):
        if i >= max_rows:
            parts.append(f"_… and {len(segments) - max_rows} more._")
            break
        c = classify_result(seg.result)
        icon = "✅" if c == "passed" else ("❌" if c == "failed" else "⚠️")
        safe_path = seg.path.replace("|", "\\|")
        safe_res = seg.result.replace("|", "\\|")
        parts.append(f"- {icon} `{safe_path}` — `{safe_res}`")
    parts.append("")
    parts.append("</details>")
    parts.append("")


def _emit_no_pairs_notice(parts: List[str], segments: List[Segment], orphans: List[str]) -> None:
    if segments or orphans:
        return
    parts.append(
        "_No `Test Started` / `Test Completed` pairs found in `test_run.log` — "
        "check that automation wrote `LogAutomationController` lines into this file._"
    )
    parts.append("")


def emit_summary_md(
    segments: List[Segment],
    orphans: List[str],
    json_path: Optional[Path],
    max_bytes: int,
    redact: bool,
) -> str:
    parts: List[str] = []
    trunc_fail = "\n\n… (truncated; use --max-bytes-per-block to raise limit)\n"
    trunc_warn = "\n\n… (truncated)\n"

    _emit_count_section(parts, segments, json_path)

    failed_segs = [s for s in segments if classify_result(s.result) == "failed"]
    warn_segs = [s for s in segments if classify_result(s.result) == "warned"]

    _emit_detail_blocks(
        parts,
        "### Failed tests (expand for log)",
        failed_segs,
        "❌",
        redact,
        max_bytes,
        trunc_fail,
    )
    _emit_detail_blocks(
        parts,
        "### Tests completed with warnings (expand for log)",
        warn_segs,
        "⚠️",
        redact,
        max_bytes,
        trunc_warn,
    )
    _emit_orphans_section(parts, orphans, redact, max_bytes)
    _emit_all_paths_list(parts, segments)
    _emit_no_pairs_notice(parts, segments, orphans)

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
    line_count = len(text.splitlines())
    segments, orphans = parse_segments(text)
    print(
        f"automation_log_split_report: log={args.log} lines={line_count} "
        f"segments={len(segments)} orphans={len(orphans)}",
        file=sys.stderr,
    )
    md = emit_summary_md(segments, orphans, args.json, args.max_bytes_per_block, args.redact)
    sys.stdout.write(md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
