param(
    [string]$BuildDir = "build-tidy",
    [string[]]$SourceDirs = @("src"),
    [string]$Output = "clang-tidy-findings.txt",
    [string]$Tidy = $env:CLANG_TIDY,

    # Optional: Scan only one source file from compile_commands.json
    [string]$File = ""
)

$ErrorActionPreference = "Stop"

if (-not $Tidy) {
    $Tidy = "clang-tidy"
}

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildPath = Join-Path $projectRoot $BuildDir
$compileCommands = Join-Path $buildPath "compile_commands.json"

function Resolve-TidyExecutable {
    param([string]$Executable)

    if (Test-Path $Executable) {
        return (Resolve-Path $Executable).Path
    }

    $command = Get-Command $Executable -ErrorAction SilentlyContinue

    if ($command) {
        return $command.Source
    }

    Write-Error "clang-tidy was not found. Install LLVM, add clang-tidy to PATH, or set CLANG_TIDY."
}

function Resolve-CompileCommandFile {
    param($Command)

    $file = $Command.file

    if ([System.IO.Path]::IsPathRooted($file)) {
        return [System.IO.Path]::GetFullPath($file)
    }

    if ($Command.directory) {
        return [System.IO.Path]::GetFullPath((Join-Path $Command.directory $file))
    }

    return [System.IO.Path]::GetFullPath((Join-Path $projectRoot $file))
}

$tidyExecutable = Resolve-TidyExecutable $Tidy

if (-not (Test-Path $compileCommands)) {
    Write-Error "compile_commands.json was not found: $compileCommands"
}

$sourceRoots = $SourceDirs |
    ForEach-Object { Join-Path $projectRoot $_ } |
    Where-Object { Test-Path $_ } |
    ForEach-Object { (Resolve-Path $_).Path }

if ($sourceRoots.Count -eq 0) {
    Write-Error "No source directories found. Searched for: $($SourceDirs -join ', ')"
}

$commands = Get-Content $compileCommands -Raw | ConvertFrom-Json

$allProjectFiles = $commands |
    ForEach-Object { Resolve-CompileCommandFile $_ } |
    Sort-Object -Unique |
    Where-Object {
        $currentFile = $_

        ($sourceRoots | Where-Object {
            $root = $_

            $currentFile.StartsWith(
                $root + [System.IO.Path]::DirectorySeparatorChar,
                [System.StringComparison]::OrdinalIgnoreCase
            )
        }).Count -gt 0
    }

if ($File) {
    if ([System.IO.Path]::IsPathRooted($File)) {
        $requestedFile = [System.IO.Path]::GetFullPath($File)
    }
    else {
        $requestedFile = [System.IO.Path]::GetFullPath((Join-Path $projectRoot $File))
    }

    $files = $allProjectFiles |
        Where-Object {
            [System.String]::Equals(
                $_,
                $requestedFile,
                [System.StringComparison]::OrdinalIgnoreCase
            )
        }

    if ($files.Count -eq 0) {
        Write-Error "File was not found in compile_commands.json: $requestedFile"
    }
}
else {
    $files = $allProjectFiles
}

if ($files.Count -eq 0) {
    Write-Error "No matching project files found in compile_commands.json."
}

$headerFilter = ($sourceRoots | ForEach-Object {
    [regex]::Escape($_) + ".*"
}) -join "|"

Write-Host "Project:       $projectRoot"
Write-Host "Build dir:     $buildPath"
Write-Host "clang-tidy:    $tidyExecutable"
Write-Host "Source dirs:   $($sourceRoots -join ', ')"
Write-Host "Files:         $($files.Count)"
Write-Host "Output:        $Output"
Write-Host ""

Push-Location $projectRoot

$previousErrorActionPreference = $ErrorActionPreference

try {
    # clang-tidy writes normal diagnostic output to stderr.
    # In Windows PowerShell, redirected native stderr can otherwise become a terminating error.
    $ErrorActionPreference = "Continue"

    $files | ForEach-Object {
        Write-Host "Checking $_"

        & $tidyExecutable -p $buildPath $_ -header-filter=$headerFilter 2>&1 |
            ForEach-Object { $_.ToString() }
    } | Tee-Object $Output
}
finally {
    $ErrorActionPreference = $previousErrorActionPreference
    Pop-Location
}
