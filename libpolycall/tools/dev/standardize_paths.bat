@echo off
:: standardize_paths.bat - LibPolyCall Include Path Standardization Automation Script
:: 
:: This script systematically standardizes include paths across the LibPolyCall codebase
:: by executing the Python standardization scripts in the optimal sequence.
::
:: Usage: standardize_paths.bat [OPTIONS]
::   Options:
::     --project-root DIR   Specify project root directory (default: current directory)
::     --verbose            Enable verbose logging
::     --no-backup          Skip backup creation
::     --dry-run            Show what would be changed without modifying files
::     --help               Display this help message
::
:: Author: Nnamdi Okpala, OBINexusComputing

setlocal EnableDelayedExpansion

:: Default values
set "PROJECT_ROOT=%CD%"
set "SCRIPTS_DIR=%CD%\scripts"
set "VERBOSE="
set "BACKUP=--no-backup"
set "DRY_RUN="
set "BUILD_DIR=%CD%\build"

:: Timestamp for logs
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "DATETIME=%%a"
set "TIMESTAMP=%DATETIME:~0,8%_%DATETIME:~8,6%"
set "LOG_DIR=%PROJECT_ROOT%\logs"
set "LOG_FILE=%LOG_DIR%\standardization_%TIMESTAMP%.log"

:: Parse command line arguments
:parse_args
if "%~1"=="" goto end_parse_args
if "%~1"=="--project-root" (
    set "PROJECT_ROOT=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="--verbose" (
    set "VERBOSE=--verbose"
    shift
    goto parse_args
)
if "%~1"=="--no-backup" (
    set "BACKUP=--no-backup"
    shift
    goto parse_args
)
if "%~1"=="--dry-run" (
    set "DRY_RUN=--dry-run"
    shift
    goto parse_args
)
if "%~1"=="--help" (
    echo Usage: standardize_paths.bat [OPTIONS]
    echo Options:
    echo   --project-root DIR   Specify project root directory (default: current directory)
    echo   --verbose            Enable verbose logging
    echo   --no-backup          Skip backup creation
    echo   --dry-run            Show what would be changed without modifying files
    echo   --help               Display this help message
    exit /b 0
)
echo Unknown option: %~1
echo Use --help for usage information
exit /b 1

:end_parse_args

:: Create log directory if it doesn't exist
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

:: Log function
call :log "Starting LibPolyCall include path standardization process"
call :log "Project root: %PROJECT_ROOT%"
call :log "Using scripts from: %SCRIPTS_DIR%"

:: Validate scripts directory
if not exist "%SCRIPTS_DIR%" (
    call :log "ERROR: Scripts directory not found: %SCRIPTS_DIR%"
    exit /b 1
)

:: Step 1: Create backup if enabled
if "%BACKUP%"=="" if not "%DRY_RUN%"=="--dry-run" (
    set "BACKUP_DIR=%PROJECT_ROOT%\backup_%TIMESTAMP%"
    call :log "Creating backup in %BACKUP_DIR%..."
    if not exist "%BACKUP_DIR%" mkdir "%BACKUP_DIR%"
    
    :: Backup src directory
    if exist "%PROJECT_ROOT%\src" (
        call :log "Backing up src directory..."
        xcopy /E /I /Y "%PROJECT_ROOT%\src" "%BACKUP_DIR%\src" > nul
    )
    
    :: Backup include directory
    if exist "%PROJECT_ROOT%\include" (
        call :log "Backing up include directory..."
        xcopy /E /I /Y "%PROJECT_ROOT%\include" "%BACKUP_DIR%\include" > nul
    )
    
    call :log "Backup completed successfully"
) else (
    call :log "Backup skipped"
)

:: Step 2: Basic standardization
call :log "Step 1/5: Performing basic include standardization..."
set "CMD=python %SCRIPTS_DIR%\standardize_includes.py --root %PROJECT_ROOT% %VERBOSE% %DRY_RUN%"
call :log "Executing: %CMD%"
%CMD% >> "%LOG_FILE%" 2>&1
if %ERRORLEVEL% neq 0 (
    call :log "ERROR: Basic standardization failed. See log for details."
    exit /b 1
)

:: Step 3: Enhanced pattern-specific fixes
call :log "Step 2/5: Applying enhanced pattern-specific fixes..."
set "CMD=python %SCRIPTS_DIR%\enhanced_fix_includes.py --project-root %PROJECT_ROOT% --fix-polycall-paths %VERBOSE% %DRY_RUN%"
call :log "Executing: %CMD%"
%CMD% >> "%LOG_FILE%" 2>&1
if %ERRORLEVEL% neq 0 (
    call :log "ERROR: Enhanced pattern fixes failed. See log for details."
    exit /b 1
)

:: Step 4: Validate includes
call :log "Step 3/5: Validating include paths..."
set "VALIDATION_OUTPUT=%LOG_DIR%\validation_%TIMESTAMP%.txt"
set "CMD=python %SCRIPTS_DIR%\validate_includes.py --root %PROJECT_ROOT% --standard polycall"
call :log "Executing: %CMD%"
%CMD% > "%VALIDATION_OUTPUT%" 2>&1
type "%VALIDATION_OUTPUT%" >> "%LOG_FILE%"

:: Check if there are still issues
findstr /c:"Issues in" "%VALIDATION_OUTPUT%" > nul
if %ERRORLEVEL% equ 0 (
    call :log "Found remaining issues in validation"
    
    :: Step 5: Fix any remaining issues based on validation
    call :log "Step 4/5: Fixing remaining validation issues..."
    set "CMD=python %SCRIPTS_DIR%\include_path_validator.py --project-root %PROJECT_ROOT% --error-log %VALIDATION_OUTPUT% %DRY_RUN% %VERBOSE%"
    call :log "Executing: %CMD%"
    %CMD% >> "%LOG_FILE%" 2>&1
) else (
    call :log "No validation issues found, skipping additional fixes"
)

:: Step 6: Generate unified header
call :log "Step 5/5: Generating unified header..."
set "UNIFIED_HEADER=%PROJECT_ROOT%\include\polycall.h"
set "CMD=python %SCRIPTS_DIR%\generate_unified_header.py --project-root %PROJECT_ROOT% --output %UNIFIED_HEADER% %VERBOSE%"
call :log "Executing: %CMD%"
%CMD% >> "%LOG_FILE%" 2>&1

call :log "Path standardization process completed"
call :log "Log file: %LOG_FILE%"

:: Final status report
if not "%DRY_RUN%"=="--dry-run" (
    call :log "Summary of changes:"
    call :log "  - Basic standardization completed"
    call :log "  - Enhanced pattern fixes applied"
    call :log "  - Validation issues addressed"
    call :log "  - Unified header generated: %UNIFIED_HEADER%"
    
    call :log "Recommendation: Build the project and fix any remaining build errors:"
    call :log "  if not exist %BUILD_DIR% mkdir %BUILD_DIR% && cd %BUILD_DIR% && cmake .. && make 2> build_errors.txt"
    call :log "  python %SCRIPTS_DIR%\header_include_fixer.py build_errors.txt --verbose"
) else (
    call :log "Dry run completed. No files were modified."
    call :log "Run without --dry-run to apply changes."
)

echo.
echo Path standardization completed successfully.
echo See %LOG_FILE% for details.
exit /b 0

:: Log function
:log
echo [%date% %time%] %~1
echo [%date% %time%] %~1 >> "%LOG_FILE%"
exit /b 0