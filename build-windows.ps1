<#
.SYNOPSIS
    Native Windows build of PolyCall via CMake.

.DESCRIPTION
    The GNU Make build targets a POSIX shell (Linux, macOS, *BSD, MSYS2 shells).
    On native Windows -- PowerShell or cmd.exe, no MSYS2 -- use this script,
    which drives the independent CMake build. It never calls the Makefile.

    Requires: cmake (>= 3.20) and a C toolchain on PATH. With MSVC, run from a
    Developer PowerShell. With MinGW-w64 / UCRT64 GCC, pass -Generator "Ninja"
    or "MinGW Makefiles".

.EXAMPLE
    ./build-windows.ps1 -Generator "Ninja" -BuildType Release -Test
#>
[CmdletBinding()]
param(
    [string]$BuildDir  = "build/windows-cmake",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$BuildType = "Release",
    [string]$Generator = "",
    [string]$CC        = "",
    [switch]$Test,
    [switch]$Install,
    [string]$Prefix    = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $ProjectRoot
try {
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "cmake was not found on PATH (need >= 3.20)."
    }

    $cfg = @("-S", ".", "-B", $BuildDir, "-DCMAKE_BUILD_TYPE=$BuildType", "-DBUILD_TESTING=ON")
    if ($Generator) { $cfg += @("-G", $Generator) }
    if ($CC)        { $cfg += "-DCMAKE_C_COMPILER=$CC" }
    if ($Prefix)    { $cfg += "-DCMAKE_INSTALL_PREFIX=$Prefix" }

    Write-Host "cmake $($cfg -join ' ')"
    & cmake @cfg
    if ($LASTEXITCODE -ne 0) { throw "configure failed ($LASTEXITCODE)" }

    & cmake --build $BuildDir --config $BuildType --parallel
    if ($LASTEXITCODE -ne 0) { throw "build failed ($LASTEXITCODE)" }

    if ($Test) {
        & ctest --test-dir $BuildDir -C $BuildType --output-on-failure
        if ($LASTEXITCODE -ne 0) { throw "ctest failed ($LASTEXITCODE)" }
    }
    if ($Install) {
        & cmake --install $BuildDir --config $BuildType
        if ($LASTEXITCODE -ne 0) { throw "install failed ($LASTEXITCODE)" }
    }
    Write-Host "OK: artifacts under $BuildDir/bin and $BuildDir/lib"
} finally {
    Pop-Location
}
