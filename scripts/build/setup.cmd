@echo off
REM LibPolyCall v2 Setup Script - Windows CMD
REM Generated: 2025-07-02 22:32:19
REM OBINexus Aegis Project - Sinphasé Governance

setlocal enabledelayedexpansion

REM Project paths
set PROJECT_ROOT=%~dp0..\..
set BUILD_DIR=%PROJECT_ROOT%\build
set SCRIPTS_DIR=%PROJECT_ROOT%\scripts

echo === LibPolyCall v2 Build Setup ===
echo Project Root: %PROJECT_ROOT%

REM Check dependencies
echo.
echo Checking dependencies...
set MISSING_DEPS=
where cl.exe >nul 2>&1 || set MISSING_DEPS=!MISSING_DEPS! cl.exe
where nmake >nul 2>&1 || set MISSING_DEPS=!MISSING_DEPS! nmake
where python >nul 2>&1 || set MISSING_DEPS=!MISSING_DEPS! python

if not "!MISSING_DEPS!"=="" (
    echo Error: Missing dependencies:!MISSING_DEPS!
    echo Please install Visual Studio Build Tools and Python
    exit /b 1
)

REM Create build directories
echo.
echo Creating build directories...
mkdir "%BUILD_DIR%\obj\core" 2>nul
mkdir "%BUILD_DIR%\obj\cli" 2>nul
mkdir "%BUILD_DIR%\lib" 2>nul
mkdir "%BUILD_DIR%\bin\debug" 2>nul
mkdir "%BUILD_DIR%\bin\prod" 2>nul
mkdir "%BUILD_DIR%\include\polycall" 2>nul

REM Fix include paths
echo.
echo Fixing include paths...
if exist "%SCRIPTS_DIR%\build\fix_include_paths.py" (
    python "%SCRIPTS_DIR%\build\fix_include_paths.py" --project-root "%PROJECT_ROOT%"
) else (
    echo Include path fixer not found, skipping...
)

REM Stage headers
echo.
echo Staging headers...
if exist "%PROJECT_ROOT%\include\polycall" (
    xcopy /E /I /Y "%PROJECT_ROOT%\include\polycall" "%BUILD_DIR%\include\polycall"
    echo Headers staged
)

REM Run build orchestrator
echo.
echo Running build orchestrator...
if exist "%SCRIPTS_DIR%\build\build_orchestrator.py" (
    python "%SCRIPTS_DIR%\build\build_orchestrator.py" ^
        --project-root "%PROJECT_ROOT%" ^
        --config debug ^
        --verbose
) else (
    echo Build orchestrator not found!
    exit /b 1
)

echo.
echo === Setup Complete ===
echo Build artifacts location: %BUILD_DIR%
echo Run 'nmake' to compile the project

REM Clean up environment variables (optional)
set PROJECT_ROOT=
set BUILD_DIR=
set SCRIPTS_DIR=
set MISSING_DEPS=
endlocal