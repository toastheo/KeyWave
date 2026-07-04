param(
    [string]$TargetDir = "src",
    [string]$ClangFormat = $env:CLANG_FORMAT
)

$ErrorActionPreference = "Stop"

if (-not $ClangFormat) {
    $command = Get-Command "clang-format" -ErrorAction SilentlyContinue

    if ($command) {
        $ClangFormat = $command.Source
    }
}

if (-not $ClangFormat) {
    $scripts = py -c "import sysconfig; print(sysconfig.get_path('scripts', scheme='nt_user'))"
    $pythonUserClangFormat = Join-Path $scripts "clang-format.exe"

    if (Test-Path $pythonUserClangFormat) {
        $ClangFormat = $pythonUserClangFormat
    }
}

if (-not $ClangFormat -or -not (Test-Path $ClangFormat)) {
    Write-Error "clang-format was not found. Add clang-format to PATH, set CLANG_FORMAT, or try: py -m pip install --user clang-format"
    exit 1
}

if (-not (Test-Path $TargetDir)) {
    Write-Error "Target directory was not found: $TargetDir"
    exit 1
}

$extensions = @(
    ".cpp",
    ".hpp",
    ".h",
    ".cxx",
    ".cc",
    ".hh",
    ".hxx",
    ".ipp"
)

Get-ChildItem $TargetDir -Recurse -File |
    Where-Object { $extensions -contains $_.Extension } |
    ForEach-Object {
        Write-Host "Formatting $($_.FullName)"
        & $ClangFormat -i $_.FullName
    }