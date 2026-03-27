#!/usr/bin/env bash
# Run Unreal Editor automation tests on macOS CI or locally.
# Prefers the real editor binary inside UnrealEditor.app; the UnrealEditor-Cmd shim
# often ignores -project= and falls back to Engine/<ProjectName>/Binaries (see UE log:
# "Failed to find game directory: .../UE_5.x/<ProjectName>/Binaries").
#
# Environment (optional):
#   UE_PATH            - defaults to /Users/Shared/Epic Games/UE_5.5
#   PROJ_DIR           - defaults to current project root
#   UPROJECT           - defaults to UnrealTestProject.uproject
#   AUTOMATION_FILTER  - passed to Automation RunTest (default: Pubnub.aUnit)
#   REPORT_DIR         - defaults to $PROJ_DIR/Saved/TestReport
#   LOG_FILE           - editor log filename (default: test_run.log). Resolved to an absolute path
#                        for -AbsLog=; on macOS, -Log= is relative to the system log dir, not cwd.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_PROJ_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"

UE_PATH="${UE_PATH:-/Users/Shared/Epic Games/UE_5.5}"
PROJ_DIR="${PROJ_DIR:-$DEFAULT_PROJ_DIR}"
UPROJECT="${UPROJECT:-UnrealTestProject.uproject}"
AUTOMATION_FILTER="${AUTOMATION_FILTER:-Pubnub.aUnit}"
REPORT_DIR="${REPORT_DIR:-$PROJ_DIR/Saved/TestReport}"
LOG_FILE="${LOG_FILE:-test_run.log}"

PROJECT_FILE="$PROJ_DIR/$UPROJECT"
if [[ ! -f "$PROJECT_FILE" ]]; then
  echo "ERROR: Project file not found: $PROJECT_FILE" >&2
  exit 1
fi

EDITOR=""
CANDIDATES=(
  "$UE_PATH/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
  "$UE_PATH/Engine/Binaries/Mac/UnrealEditor-Cmd"
  "$UE_PATH/Engine/Binaries/Mac/UnrealEditor"
)

for _c in "${CANDIDATES[@]}"; do
  if [[ -f "$_c" ]]; then
    EDITOR="$_c"
    break
  fi
done

if [[ -z "$EDITOR" ]]; then
  echo "ERROR: No Unreal Editor binary found under $UE_PATH/Engine/Binaries/Mac" >&2
  exit 1
fi

mkdir -p "$REPORT_DIR"

if [[ "$LOG_FILE" = /* ]]; then
  ABS_LOG="$LOG_FILE"
else
  ABS_LOG="$(pwd)/$LOG_FILE"
fi
mkdir -p "$(dirname "$ABS_LOG")"

echo "Editor binary: $EDITOR"
echo "Project file:  $PROJECT_FILE"
echo "Report dir:    $REPORT_DIR"
echo "Automation:    $AUTOMATION_FILTER"
echo "Abs log:       $ABS_LOG"

set +e
"$EDITOR" -project="$PROJECT_FILE" \
  -ExecCmds="Automation RunTest ${AUTOMATION_FILTER};Quit" \
  -unattended -nopause \
  -AbsLog="$ABS_LOG" \
  -ReportOutputPath="$REPORT_DIR" \
  -nullrhi -nosplash 2>&1 | tee /dev/stderr
EDITOR_EXIT=${PIPESTATUS[0]}
exit "$EDITOR_EXIT"
