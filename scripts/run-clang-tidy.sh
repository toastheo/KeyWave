#!/usr/bin/env bash
set -euo pipefail

build_dir="build-tidy"
output="clang-tidy-findings.txt"
clang_tidy="${CLANG_TIDY:-}"
source_dirs=("src")
requested_file=""

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
        -f|--file)
            requested_file="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--build-dir build-tidy] [--source-dirs src,tests] [--output clang-tidy-findings.txt] [--clang-tidy /path/to/clang-tidy] [--file src/foo.cpp]"
            echo ""
            echo "Examples:"
            echo "  $0"
            echo "  $0 --file src/platform/Window.cpp"
            echo "  $0 src/platform/Window.cpp"
            exit 0
            ;;
        *)
            if [[ -z "$requested_file" && "$1" != -* ]]; then
                requested_file="$1"
                shift
            else
                echo "Unknown argument: $1" >&2
                exit 1
            fi
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
elif command -v "$clang_tidy" >/dev/null 2>&1; then
    clang_tidy="$(command -v "$clang_tidy")"
elif [[ ! -x "$clang_tidy" ]]; then
    echo "clang-tidy was not found or is not executable: $clang_tidy" >&2
    exit 1
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

temp_files="$(mktemp)"
trap 'rm -f "$temp_files"' EXIT

python3 - "$compile_commands" "$project_root" "$requested_file" "${source_roots[@]}" > "$temp_files" <<'PY'
import json
import os
import sys

compile_commands_path = sys.argv[1]
project_root = os.path.abspath(sys.argv[2])
requested_file_arg = sys.argv[3]
source_roots = [os.path.abspath(path) for path in sys.argv[4:]]

def resolve_path(path, base_dir):
    if os.path.isabs(path):
        return os.path.abspath(path)

    return os.path.abspath(os.path.join(base_dir, path))

requested_file = None

if requested_file_arg:
    requested_file = resolve_path(requested_file_arg, project_root)

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

    is_in_source_root = False

    for root in source_roots:
        try:
            common = os.path.commonpath([file_path, root])
        except ValueError:
            continue

        if common == root:
            is_in_source_root = True
            break

    if not is_in_source_root:
        continue

    if requested_file and file_path != requested_file:
        continue

    files.add(file_path)

for file_path in sorted(files):
    print(file_path)
PY

file_count="$(wc -l < "$temp_files" | tr -d '[:space:]')"

if [[ "$file_count" -eq 0 ]]; then
    if [[ -n "$requested_file" ]]; then
        echo "Requested file was not found in compile_commands.json: $requested_file" >&2
        echo "Note: header files usually do not appear in compile_commands.json directly." >&2
        echo "Try scanning the corresponding .cpp file that includes this header." >&2
    else
        echo "No matching project files found in compile_commands.json." >&2
    fi

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

if [[ -n "$requested_file" ]]; then
    echo "File:          $requested_file"
fi

echo "Files:         $file_count"
echo "Output:        $output"
echo ""

cd "$project_root"

: > "$output"

had_errors=0

while IFS= read -r file || [[ -n "$file" ]]; do
    echo "Checking $file" | tee -a "$output"

    if ! "$clang_tidy" -p "$build_path" "$file" -header-filter="$header_filter" 2>&1 | tee -a "$output"; then
        had_errors=1
        echo "clang-tidy failed for $file" | tee -a "$output"
    fi
done < "$temp_files"

exit "$had_errors"
