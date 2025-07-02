# LibPolyCall v2 Setup Script - PowerShell
# Generated: 2025-07-02 22:26:00
# OBINexus Aegis Project - Sinphasé Governance

$ErrorActionPreference = "Stop"

# Color functions
function Write-Success { param($msg) Write-Host $msg -ForegroundColor Green }
function Write-Warning { param($msg) Write-Host $msg -ForegroundColor Yellow }
function Write-Error { param($msg) Write-Host $msg -ForegroundColor Red }
function Write-Info { param($msg) Write-Host $msg -ForegroundColor Cyan }

# Project paths
$ProjectRoot = (Get-Item $PSScriptRoot).Parent.Parent.FullName
$BuildDir = Join-Path $ProjectRoot "build"
$ScriptsDir = Join-Path $ProjectRoot "scripts"

Write-Info "=== LibPolyCall v2 Build Setup ==="
Write-Host "Project Root: $ProjectRoot"

# Check dependencies
Write-Info "`nChecking dependencies..."
$missingDeps = @()

$dependencies = @{ 
    "cl.exe" = "Visual Studio C++ Compiler"
    "nmake" = "Visual Studio Build Tools"
    "python" = "Python 3.x"
}

foreach ($dep in $dependencies.Keys) {
    $found = Get-Command $dep -ErrorAction SilentlyContinue
    if (-not $found) {
        $missingDeps += $dependencies[$dep]
        Write-Error "✗ $($dependencies[$dep]) not found"
    } else {
        Write-Success "✓ $($dependencies[$dep]) found"
    }
}

if ($missingDeps.Count -gt 0) {
    Write-Error "`nError: Missing dependencies:"
    $missingDeps | ForEach-Object { Write-Error "  - $_" }
    Write-Host "`nPlease install missing dependencies before continuing."
    exit 1
}

# Create build directories
Write-Info "`nCreating build directories..."
$directories = @(
    "$BuildDir\obj\core",
    "$BuildDir\obj\cli",
    "$BuildDir\lib",
    "$BuildDir\bin\debug",
    "$BuildDir\bin\prod",
    "$BuildDir\include\polycall"
)

foreach ($dir in $directories) {
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
}
Write-Success "✓ Build directories created"

# Fix include paths
Write-Info "`nFixing include paths..."
$fixScript = Join-Path $ScriptsDir "build\fix_include_paths.py"
if (Test-Path $fixScript) {
    & python $fixScript --project-root $ProjectRoot
} else {
    Write-Warning "Include path fixer not found, skipping..."
}

# Stage headers
Write-Info "`nStaging headers..."
$includeSource = Join-Path $ProjectRoot "include\polycall"
$includeDest = Join-Path $BuildDir "include\polycall"
if (Test-Path $includeSource) {
    Copy-Item -Path $includeSource -Destination (Join-Path $BuildDir "include") -Recurse -Force
    Write-Success "✓ Headers staged"
}

# Run build orchestrator
Write-Info "`nRunning build orchestrator..."
$orchestrator = Join-Path $ScriptsDir "build\build_orchestrator.py"
if (Test-Path $orchestrator) {
    & python $orchestrator `
        --project-root $ProjectRoot `
        --config debug `
        --verbose
} else {
    Write-Error "Build orchestrator not found!"
    exit 1
}

Write-Success "`n=== Setup Complete ==="
Write-Host "Build artifacts location: $BuildDir"
Write-Host "Run 'nmake' or use Visual Studio to compile the project"
