#!/usr/bin/env python3
"""
Prepare the Pubnub Chat Unreal SDK for release.

Updates:
  - PubnubChat.uplugin: increments Version (VersionName is unchanged)
  - PubnubChatVersion.h: sets PUBNUB_CHAT_VERSION_{MAJOR,MINOR,PATCH}
    and optionally PUBNUB_CHAT_REQUIRES_LIBRARY_VERSION

Run from any directory; paths are resolved from script location.

CLI examples:
  python prepare_for_release_chat.py --release-version 1.0.3
  python prepare_for_release_chat.py -r 1.0.3 --required-pubnub-sdk-version 2.0.5
  python prepare_for_release_chat.py -r 1.0.3 --required-pubnub-sdk-version
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


_VERSION_PATTERN = re.compile(r"^(\d+)\.(\d+)\.(\d+)$")


def _parse_semver(label: str, value: str) -> tuple[int, int, int]:
    match = _VERSION_PATTERN.match(value.strip())
    if not match:
        raise ValueError(f"{label} must be in MAJOR.MINOR.PATCH format (e.g. 1.0.2), got: {value!r}")
    return int(match.group(1)), int(match.group(2)), int(match.group(3))


def _encode_version(major: int, minor: int, patch: int) -> int:
    return (major * 10000) + (minor * 100) + patch


def _prompt(label: str, *, required: bool = True) -> str:
    while True:
        value = input(f"{label}: ").strip()
        if value or not required:
            return value
        print("  Value is required.")


def _resolve_input(label: str, arg_value: str | None, *, required: bool = True) -> str:
    if arg_value is not None:
        return arg_value.strip()
    return _prompt(label, required=required)


def _build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Prepare the Pubnub Chat Unreal SDK for release.")
    parser.add_argument(
        "-r",
        "--release-version",
        help="Release version in MAJOR.MINOR.PATCH format. Prompts when omitted.",
    )
    parser.add_argument(
        "-l",
        "--required-pubnub-sdk-version",
        nargs="?",
        const="",
        default=None,
        help=(
            "Required Pubnub SDK version in MAJOR.MINOR.PATCH format. "
            "Omit the flag to prompt; pass the flag alone to skip updating."
        ),
    )
    return parser


def _update_uplugin_version(uplugin_path: Path) -> int:
    with uplugin_path.open("r", encoding="utf-8") as f:
        uplugin = json.load(f)

    old_version = uplugin.get("Version")
    if not isinstance(old_version, int):
        raise SystemExit(f"Unexpected Version field in {uplugin_path}: {old_version!r}")

    new_version = old_version + 1
    uplugin["Version"] = new_version

    with uplugin_path.open("w", encoding="utf-8", newline="\n") as f:
        json.dump(uplugin, f, indent="\t")
        f.write("\n")

    return new_version


def _update_version_header(
    header_path: Path,
    major: int,
    minor: int,
    patch: int,
    required_library_version: int | None,
) -> None:
    content = header_path.read_text(encoding="utf-8")

    content, count = re.subn(
        r"(#define PUBNUB_CHAT_VERSION_MAJOR )\d+",
        rf"\g<1>{major}",
        content,
        count=1,
    )
    if count != 1:
        raise SystemExit(f"Could not update PUBNUB_CHAT_VERSION_MAJOR in {header_path}")

    content, count = re.subn(
        r"(#define PUBNUB_CHAT_VERSION_MINOR )\d+",
        rf"\g<1>{minor}",
        content,
        count=1,
    )
    if count != 1:
        raise SystemExit(f"Could not update PUBNUB_CHAT_VERSION_MINOR in {header_path}")

    content, count = re.subn(
        r"(#define PUBNUB_CHAT_VERSION_PATCH )\d+",
        rf"\g<1>{patch}",
        content,
        count=1,
    )
    if count != 1:
        raise SystemExit(f"Could not update PUBNUB_CHAT_VERSION_PATCH in {header_path}")

    if required_library_version is not None:
        content, count = re.subn(
            r"(#define PUBNUB_CHAT_REQUIRES_LIBRARY_VERSION )\d+",
            rf"\g<1>{required_library_version}",
            content,
            count=1,
        )
        if count != 1:
            raise SystemExit(f"Could not update PUBNUB_CHAT_REQUIRES_LIBRARY_VERSION in {header_path}")

    header_path.write_text(content, encoding="utf-8", newline="\n")


def main(argv: list[str] | None = None) -> None:
    args = _build_arg_parser().parse_args(argv)

    script_dir = Path(__file__).resolve().parent
    plugin_root = script_dir.parent
    uplugin_path = plugin_root / "PubnubChat.uplugin"
    header_path = plugin_root / "Source" / "PubnubChatSDK" / "Public" / "PubnubChatVersion.h"

    if not uplugin_path.is_file():
        raise SystemExit(f"Plugin descriptor not found: {uplugin_path}")
    if not header_path.is_file():
        raise SystemExit(f"Version header not found: {header_path}")

    print("Prepare Pubnub Chat Unreal SDK for release\n")

    release_version = _resolve_input("Release version (MAJOR.MINOR.PATCH)", args.release_version)
    try:
        major, minor, patch = _parse_semver("Release version", release_version)
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc

    library_input = _resolve_input(
        "Required Pubnub SDK version (MAJOR.MINOR.PATCH, leave empty to skip)",
        args.required_pubnub_sdk_version,
        required=False,
    )
    required_library_version: int | None = None
    required_library_version_str: str | None = None
    if library_input:
        try:
            lib_major, lib_minor, lib_patch = _parse_semver("Required Pubnub SDK version", library_input)
        except ValueError as exc:
            raise SystemExit(str(exc)) from exc
        required_library_version = _encode_version(lib_major, lib_minor, lib_patch)
        required_library_version_str = f"{lib_major}.{lib_minor}.{lib_patch}"

    release_version_str = f"{major}.{minor}.{patch}"

    new_uplugin_version = _update_uplugin_version(uplugin_path)
    _update_version_header(header_path, major, minor, patch, required_library_version)

    print("\nUpdated files:")
    print(f"  {uplugin_path}")
    print(f"    Version -> {new_uplugin_version} (VersionName unchanged)")
    print(f"  {header_path}")
    print(f"    PUBNUB_CHAT_VERSION -> {release_version_str}")
    if required_library_version is not None:
        print(
            f"    PUBNUB_CHAT_REQUIRES_LIBRARY_VERSION -> {required_library_version}"
            f" ({required_library_version_str})"
        )
    else:
        print("    PUBNUB_CHAT_REQUIRES_LIBRARY_VERSION -> unchanged")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nAborted.", file=sys.stderr)
        sys.exit(130)
