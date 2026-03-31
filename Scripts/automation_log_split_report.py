#!/usr/bin/env python3
"""
Build markdown from Unreal editor log: pair Test Started / Test Completed by Path.

- **stdout**: GitHub Job Summary–safe (under ~1MiB per step); failures truncated/count-limited.
- **--full-output**: complete report for CI artifact (no size cap).

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
    max_segments: Optional[int] = None,
    artifact_note: str = "automation_report_full.md",
) -> None:
    if not segs:
        return
    remainder = 0
    if max_segments is not None and len(segs) > max_segments:
        remainder = len(segs) - max_segments
        segs = segs[:max_segments]
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
    if remainder > 0:
        parts.append(
            f"_… and **{remainder}** more — full log slices in artifact `{artifact_note}`._"
        )
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


def _emit_github_paths_note(parts: List[str], segments: List[Segment]) -> None:
    """Single paragraph: full list lives in artifact (Job Summary 1MiB/step limit)."""
    if not segments:
        return
    parts.append("### All tests")
    parts.append("")
    parts.append(
        f"_**{len(segments)}** bounded runs in `test_run.log`. The expandable path list and "
        f"full per-test log slices are in the **`automation_report_full.md`** artifact "
        "(omitted here so the Job Summary stays under GitHub’s 1 MiB per-step limit)._"
    )
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


def _truncate_utf8_bytes(s: str, max_bytes: int) -> str:
    """Truncate so encoded size <= max_bytes without splitting a UTF-8 codepoint."""
    data = s.encode("utf-8")
    if len(data) <= max_bytes:
        return s
    data = data[:max_bytes]
    while data and (data[-1] & 0xC0) == 0x80:
        data = data[:-1]
    return data.decode("utf-8", errors="ignore")


def emit_summary_md(
    segments: List[Segment],
    orphans: List[str],
    json_path: Optional[Path],
    max_bytes: int,
    redact: bool,
    *,
    mode: str = "full",
    github_max_failures: int = 25,
    github_max_warnings: int = 15,
    github_max_bytes_per_block: int = 16384,
    github_orphan_max_bytes: int = 32768,
) -> str:
    parts: List[str] = []
    trunc_fail = "\n\n… (truncated; use --max-bytes-per-block to raise limit)\n"
    trunc_warn = "\n\n… (truncated)\n"

    _emit_count_section(parts, segments, json_path)

    failed_segs = [s for s in segments if classify_result(s.result) == "failed"]
    warn_segs = [s for s in segments if classify_result(s.result) == "warned"]

    if mode == "github":
        fail_limit: Optional[int] = github_max_failures
        warn_limit: Optional[int] = github_max_warnings
        block_b = github_max_bytes_per_block
        orphan_cap = github_orphan_max_bytes
    else:
        fail_limit = None
        warn_limit = None
        block_b = max_bytes
        orphan_cap = max_bytes

    _emit_detail_blocks(
        parts,
        "### Failed tests (expand for log)",
        failed_segs,
        "❌",
        redact,
        block_b,
        trunc_fail,
        max_segments=fail_limit,
    )
    _emit_detail_blocks(
        parts,
        "### Tests completed with warnings (expand for log)",
        warn_segs,
        "⚠️",
        redact,
        block_b,
        trunc_warn,
        max_segments=warn_limit,
    )
    _emit_orphans_section(parts, orphans, redact, orphan_cap)

    if mode == "github":
        _emit_github_paths_note(parts, segments)
    else:
        _emit_all_paths_list(parts, segments)

    _emit_no_pairs_notice(parts, segments, orphans)

    return "\n".join(parts)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--log", type=Path, required=True)
    ap.add_argument("--json", type=Path, help="Saved/TestReport/index.json for counts")
    ap.add_argument(
        "--full-output",
        type=Path,
        help="Write complete report (full mode) for CI artifact; not size-capped",
    )
    ap.add_argument("--max-bytes-per-block", type=int, default=65536)
    ap.add_argument(
        "--max-summary-bytes",
        type=int,
        default=950_000,
        help="Hard cap for stdout (GitHub Job Summary per-step limit is 1MiB)",
    )
    ap.add_argument(
        "--github-max-failures",
        type=int,
        default=25,
        help="Max failed-test detail blocks on stdout",
    )
    ap.add_argument(
        "--github-max-warnings",
        type=int,
        default=15,
        help="Max warning-test detail blocks on stdout",
    )
    ap.add_argument(
        "--github-max-bytes-per-block",
        type=int,
        default=16384,
        help="Per-test log slice size cap for stdout",
    )
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

    if args.full_output:
        full_md = emit_summary_md(
            segments,
            orphans,
            args.json,
            args.max_bytes_per_block,
            args.redact,
            mode="full",
        )
        args.full_output.parent.mkdir(parents=True, exist_ok=True)
        args.full_output.write_text(full_md, encoding="utf-8")
        print(
            f"automation_log_split_report: wrote full report ({len(full_md.encode('utf-8'))} bytes) → {args.full_output}",
            file=sys.stderr,
        )

    gh_md = emit_summary_md(
        segments,
        orphans,
        args.json,
        args.max_bytes_per_block,
        args.redact,
        mode="github",
        github_max_failures=args.github_max_failures,
        github_max_warnings=args.github_max_warnings,
        github_max_bytes_per_block=args.github_max_bytes_per_block,
    )
    raw = gh_md.encode("utf-8")
    if len(raw) > args.max_summary_bytes:
        gh_md = _truncate_utf8_bytes(gh_md, args.max_summary_bytes)
        gh_md += (
            "\n\n---\n_**Hard-truncated** to `--max-summary-bytes` so the Job Summary upload "
            "succeeds (GitHub limit 1 MiB per step). Use the `automation_report_full.md` artifact "
            "for the complete report._\n"
        )
        print(
            f"automation_log_split_report: GitHub summary truncated "
            f"{len(raw)} → {len(gh_md.encode('utf-8'))} bytes",
            file=sys.stderr,
        )
    sys.stdout.write(gh_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
