@echo off
:: Installs the comprehensive path fixer script into the LibPolyCall project structure
:: for Windows environments

setlocal EnableDelayedExpansion

:: Configuration
set "SCRIPT_NAME=fix_all_paths.py"
set "TARGET_DIR=scripts"
set "MAKEFILE=Makefile"

:: Function to display usage information
:usage
if "%~1"=="--help" (
    echo Usage: %0 [OPTIONS]
    echo Options:
    echo   --project-root DIR   Specify project root directory (default: current directory)
    echo   --help               Display this help message
    exit /b 1
)

:: Default values
set "PROJECT_ROOT=%CD%"

:: Parse command-line arguments
:parse_args
if "%~1"=="" goto :end_parse_args
if "%~1"=="--project-root" (
    set "PROJECT_ROOT=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="--help" (
    call :usage --help
    exit /b 0
)
echo Error: Unknown option: %~1
call :usage
exit /b 1

:end_parse_args

:: Ensure project root exists
if not exist "%PROJECT_ROOT%\" (
    echo Error: Project root directory '%PROJECT_ROOT%' does not exist.
    exit /b 1
)

:: Ensure scripts directory exists
set "SCRIPTS_DIR=%PROJECT_ROOT%\%TARGET_DIR%"
if not exist "%SCRIPTS_DIR%\" (
    echo Creating scripts directory...
    mkdir "%SCRIPTS_DIR%"
)

:: Copy fix script to scripts directory
echo Installing %SCRIPT_NAME% to %SCRIPTS_DIR%...
copy /Y "%SCRIPT_NAME%" "%SCRIPTS_DIR%\" > nul

:: Update Makefile with new target if it exists
set "MAKEFILE_PATH=%PROJECT_ROOT%\%MAKEFILE%"
if exist "%MAKEFILE_PATH%" (
    echo Updating Makefile with new fix-all-paths target...
    
    :: Check if target already exists to avoid duplicate entries
    findstr /C:"fix-all-paths:" "%MAKEFILE_PATH%" > nul
    if not errorlevel 1 (
        echo Target already exists in Makefile. Skipping...
    ) else (
        :: Add new target to Makefile
        echo.>> "%MAKEFILE_PATH%"
        echo # Fix all include path issues with comprehensive fixer>> "%MAKEFILE_PATH%"
        echo .PHONY: fix-all-paths>> "%MAKEFILE_PATH%"
        echo fix-all-paths:>> "%MAKEFILE_PATH%"
        echo 	@echo "Running comprehensive include path fixer...">> "%MAKEFILE_PATH%"
        echo 	@python3 $(SCRIPTS_DIR)/%SCRIPT_NAME% --project-root $(PROJECT_ROOT) $(if $(VERBOSE),--verbose,)>> "%MAKEFILE_PATH%"
        echo 	@echo "Complete path standardization completed.">> "%MAKEFILE_PATH%"
        echo 	@echo "Run 'make validate-includes-direct' to verify fixes.">> "%MAKEFILE_PATH%"
        echo Makefile updated successfully.
    )
) else (
    echo Warning: Makefile not found at %MAKEFILE_PATH%. No updates applied.
)

echo Installation complete!
echo To fix all include paths, run: make fix-all-paths
echo For manual execution: python %SCRIPTS_DIR%\%SCRIPT_NAME% --project-root ^<project_root^>

exit /b 0