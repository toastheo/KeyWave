#!/usr/bin/env bash
set -euo pipefail

build_dir="build-tidy"
output="clang-tidy-findings.txt"
clang_tidy="${CLANG_TIDY:-}"
source_dirs=("src")

while [[ $# -gt 0 ]]; do
    case "$1" in
        -b|--build-dir)
            build_dir="$2"
            shift 2
            ;;
        -s|--source-dirs)
            IFS=',' read -r -a source_dirs <<< "$2"
            shift 2
            ;;
        -o|--output)
            output="$2"
            shift 2
            ;;
        -t|--clang-tidy)
            clang_tidy="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--build-dir build-tidy] [--source-dirs src,tests] [--output clang-tidy-findings.txt] [--clang-tidy /path/to/clang-tidy]"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "$script_dir/.." && pwd)"
build_path="$project_root/$build_dir"
compile_commands="$build_path/compile_commands.json"

if [[ -z "$clang_tidy" ]]; then
    if command -v clang-tidy >/dev/null 2>&1; then
        clang_tidy="$(command -v clang-tidy)"
    else
        echo "clang-tidy was not found. Install it, add it to PATH, or set CLANG_TIDY." >&2
        exit 1
    fi
fi

if [[ ! -f "$compile_commands" ]]; then
    echo "compile_commands.json was not found: $compile_commands" >&2
    echo "Generate it with something like:" >&2
    echo "cmake -S . -B $build_dir -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON" >&2
    exit 1
fi

source_roots=()

for dir in "${source_dirs[@]}"; do
    full_path="$project_root/$dir"

    if [[ -d "$full_path" ]]; then
        source_roots+=("$(cd "$full_path" && pwd)")
    fi
done

if [[ ${#source_roots[@]} -eq 0 ]]; then
    echo "No source directories found. Searched for: ${source_dirs[*]}" >&2
    exit 1
fi

mapfile -t files < <(
    python3 - "$compile_commands" "$project_root" "${source_roots[@]}" <<'PY'
import json
import os
import sys

compile_commands_path = sys.argv[1]
project_root = os.path.abspath(sys.argv[2])
source_roots = [os.path.abspath(path) for path in sys.argv[3:]]

with open(compile_commands_path, "r", encoding="utf-8") as file:
    commands = json.load(file)

files = set()

for command in commands:
    file_path = command.get("file")

    if not file_path:
        continue

    if not os.path.isabs(file_path):
        directory = command.get("directory", project_root)
        file_path = os.path.join(directory, file_path)

    file_path = os.path.abspath(file_path)

    for root in source_roots:
        try:
            common = os.path.commonpath([file_path, root])
        except ValueError:
            continue

        if common == root:
            files.add(file_path)
            break

for file_path in sorted(files):
    print(file_path)
PY
)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "No matching project files found in compile_commands.json." >&2
    exit 1
fi

header_filter="$(
    python3 - "${source_roots[@]}" <<'PY'
import re
import sys

roots = sys.argv[1:]
print("|".join(re.escape(root) + ".*" for root in roots))
PY
)"

echo "Project:       $project_root"
echo "Build dir:     $build_path"
echo "clang-tidy:    $clang_tidy"
echo "Source dirs:   ${source_roots[*]}"
echo "Files:         ${#files[@]}"
echo "Output:        $output"
echo ""

cd "$project_root"

: > "$output"

had_errors=0

for file in "${files[@]}"; do
    echo "Checking $file" | tee -a "$output"

    if ! "$clang_tidy" -p "$build_path" "$file" -header-filter="$header_filter" 2>&1 | tee -a "$output"; then
        had_errors=1
        echo "clang-tidy failed for $file" | tee -a "$output"
    fi
done

exit "$had_errors"
