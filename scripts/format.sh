#!/usr/bin/env bash
set -euo pipefail

target_dir="src"
clang_format="${CLANG_FORMAT:-}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        -d|--target-dir)
            target_dir="$2"
            shift 2
            ;;
        -f|--clang-format)
            clang_format="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--target-dir src] [--clang-format /path/to/clang-format]"
            exit 0
            ;;
        *)
            target_dir="$1"
            shift
            ;;
    esac
done

if [[ -z "$clang_format" ]]; then
    if command -v clang-format >/dev/null 2>&1; then
        clang_format="$(command -v clang-format)"
    else
        echo "clang-format was not found. Install it, add it to PATH, or set CLANG_FORMAT." >&2
        exit 1
    fi
fi

if [[ ! -d "$target_dir" ]]; then
    echo "Target directory was not found: $target_dir" >&2
    exit 1
fi

find "$target_dir" -type f \( \
    -name '*.cpp' -o \
    -name '*.hpp' -o \
    -name '*.h'   -o \
    -name '*.cxx' -o \
    -name '*.cc'  -o \
    -name '*.hh'  -o \
    -name '*.hxx' -o \
    -name '*.ipp' \
\) -print0 |
while IFS= read -r -d '' file; do
    echo "Formatting $file"
    "$clang_format" -i "$file"
done
