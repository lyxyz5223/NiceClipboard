@echo off
setlocal enabledelayedexpansion

:: Color codes for better readability
for /f %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "GREEN=%ESC%[32m"
set "RED=%ESC%[31m"
set "YELLOW=%ESC%[33m"
set "CYAN=%ESC%[36m"
set "RESET=%ESC%[0m"

:: 修正路径变量（在 setlocal 之前获取）
set "SCRIPT_DIR=%~dp0"
set "VERSION_H=%SCRIPT_DIR%NiceClipboard\version.h"

if "%~1"=="" goto :usage
if /i "%~1"=="get" goto :get
if /i "%~1"=="set" goto :set
if /i "%~1"=="bump" goto :bump

:usage
echo %CYAN%Usage:%RESET% %~nx0 ^<command^> [arguments]
echo.
echo %CYAN%Commands:%RESET%
echo   get                          Show both file and product versions
echo   get MacroName                Get a specific macro value
echo   set FileVer ProductVer       Generate version.h with both versions
echo   bump [major^|minor^|revision]  Increment version number
echo.
echo %CYAN%Examples:%RESET%
echo   %~nx0 get
echo   %~nx0 get VER_FILEVERSION_STR
echo   %~nx0 set 1.2.3 1.0.0
echo   %~nx0 bump revision
exit /b 1

:: ---------------------------------------------------------------
:get
if not "%~2"=="" goto :get_macro

:: Check if file exists
if not exist "%VERSION_H%" (
    echo %RED%Error:%RESET% version.h not found at %VERSION_H%
    exit /b 1
)

:: Show both versions
for /f "tokens=3" %%a in ('findstr /c:"VER_FILEVERSION_STR " "%VERSION_H%" 2^>nul') do set "FILE_VER=%%~a"
for /f "tokens=3" %%a in ('findstr /c:"VER_PRODUCTVERSION_STR " "%VERSION_H%" 2^>nul') do set "PROD_VER=%%~a"

:: 去除引号
set "FILE_VER=!FILE_VER:"=!"
set "PROD_VER=!PROD_VER:"=!"

echo %GREEN%File:    %RESET%!FILE_VER!
echo %GREEN%Product: %RESET%!PROD_VER!
exit /b 0

:get_macro
:: Check if file exists
if not exist "%VERSION_H%" (
    echo %RED%Error:%RESET% version.h not found at %VERSION_H%
    exit /b 1
)

:: Get a single macro value
set "MACRO=%~2"
for /f "tokens=3,*" %%a in ('findstr /r /c:"^#define !MACRO! " "%VERSION_H%" 2^>nul') do (
    set "VALUE=%%a %%b"
    set "VALUE=!VALUE:"=!"
    echo !VALUE!
    exit /b 0
)
echo %RED%Error:%RESET% Macro "!MACRO!" not found.
exit /b 1

:: ---------------------------------------------------------------
:bump
if not exist "%VERSION_H%" (
    echo %RED%Error:%RESET% version.h not found at %VERSION_H%
    exit /b 1
)

set "COMPONENT=revision"
if not "%~2"=="" set "COMPONENT=%~2"

:: Get current versions
for /f "tokens=3" %%a in ('findstr /c:"VER_FILEVERSION_MAJOR " "%VERSION_H%" 2^>nul') do set "F_MAJ=%%a"
for /f "tokens=3" %%a in ('findstr /c:"VER_FILEVERSION_MINOR " "%VERSION_H%" 2^>nul') do set "F_MIN=%%a"
for /f "tokens=3" %%a in ('findstr /c:"VER_FILEVERSION_REVISION " "%VERSION_H%" 2^>nul') do set "F_REV=%%a"
for /f "tokens=3" %%a in ('findstr /c:"VER_PRODUCTVERSION_MAJOR " "%VERSION_H%" 2^>nul') do set "P_MAJ=%%a"
for /f "tokens=3" %%a in ('findstr /c:"VER_PRODUCTVERSION_MINOR " "%VERSION_H%" 2^>nul') do set "P_MIN=%%a"
for /f "tokens=3" %%a in ('findstr /c:"VER_PRODUCTVERSION_REVISION " "%VERSION_H%" 2^>nul') do set "P_REV=%%a"

if /i "!COMPONENT!"=="major" (
    set /a "F_MAJ+=1"
    set "F_MIN=0"
    set "F_REV=0"
    set /a "P_MAJ+=1"
    set "P_MIN=0"
    set "P_REV=0"
) else if /i "!COMPONENT!"=="minor" (
    set /a "F_MIN+=1"
    set "F_REV=0"
    set /a "P_MIN+=1"
    set "P_REV=0"
) else if /i "!COMPONENT!"=="revision" (
    set /a "F_REV+=1"
    set /a "P_REV+=1"
) else (
    echo %RED%Error:%RESET% Invalid component. Use: major, minor, or revision
    exit /b 1
)

set "FILE_VER=!F_MAJ!.!F_MIN!.!F_REV!"
set "PROD_VER=!P_MAJ!.!P_MIN!.!P_REV!"

:: Create backup
if exist "%VERSION_H%" (
    copy "%VERSION_H%" "%VERSION_H%.bak" >nul
    echo %YELLOW%Backup created:%RESET% version.h.bak
)

:: Write new version.h
call :write_version_h
echo %GREEN%Version bumped to:%RESET%
echo   File:    !FILE_VER!
echo   Product: !PROD_VER!
exit /b 0

:: ---------------------------------------------------------------
:set
if "%~2"=="" goto :usage
if "%~3"=="" goto :usage

set "FILE_VER=%~2"
set "PROD_VER=%~3"

:: Validate format x.x.x via PowerShell
for /f %%r in ('powershell -NoProfile -Command "if('!FILE_VER!' -match '^\d+\.\d+\.\d+$'){1}else{0}"') do set "VALID_F=%%r"
for /f %%r in ('powershell -NoProfile -Command "if('!PROD_VER!' -match '^\d+\.\d+\.\d+$'){1}else{0}"') do set "VALID_P=%%r"

if "!VALID_F!"=="0" (
    echo %RED%Error:%RESET% File version "!FILE_VER!" is not valid. Must be x.x.x ^(e.g. 1.2.3^)
    exit /b 1
)
if "!VALID_P!"=="0" (
    echo %RED%Error:%RESET% Product version "!PROD_VER!" is not valid. Must be x.x.x ^(e.g. 1.0.0^)
    exit /b 1
)

:: Parse components
for /f "tokens=1-3 delims=." %%a in ("!FILE_VER!") do (
    set "F_MAJ=%%a"
    set "F_MIN=%%b"
    set "F_REV=%%c"
)
for /f "tokens=1-3 delims=." %%a in ("!PROD_VER!") do (
    set "P_MAJ=%%a"
    set "P_MIN=%%b"
    set "P_REV=%%c"
)

:: Validate numeric ranges
if !F_MAJ! gtr 65535 (
    echo %RED%Error:%RESET% Major version cannot exceed 65535
    exit /b 1
)
if !F_MIN! gtr 65535 (
    echo %RED%Error:%RESET% Minor version cannot exceed 65535
    exit /b 1
)
if !F_REV! gtr 65535 (
    echo %RED%Error:%RESET% Revision cannot exceed 65535
    exit /b 1
)

:: Create backup if file exists
if exist "%VERSION_H%" (
    copy "%VERSION_H%" "%VERSION_H%.bak" >nul
    echo %YELLOW%Backup created:%RESET% version.h.bak
)

call :write_version_h
echo %GREEN%Done.%RESET% Written to version.h
echo   File:    !FILE_VER!
echo   Product: !PROD_VER!
exit /b 0

:: ---------------------------------------------------------------
:write_version_h
chcp 65001 >nul
:: Write version.h (使用 > 覆盖，避免重复内容)
> "%VERSION_H%" echo #pragma once
>> "%VERSION_H%" echo.
>> "%VERSION_H%" echo.
>> "%VERSION_H%" echo // 版本号定义
>> "%VERSION_H%" echo #define VER_FILEVERSION_MAJOR           !F_MAJ!
>> "%VERSION_H%" echo #define VER_FILEVERSION_MINOR           !F_MIN!
>> "%VERSION_H%" echo #define VER_FILEVERSION_REVISION        !F_REV!
>> "%VERSION_H%" echo.
>> "%VERSION_H%" echo #define VER_PRODUCTVERSION_MAJOR        !P_MAJ!
>> "%VERSION_H%" echo #define VER_PRODUCTVERSION_MINOR        !P_MIN!
>> "%VERSION_H%" echo #define VER_PRODUCTVERSION_REVISION     !P_REV!
>> "%VERSION_H%" echo.
>> "%VERSION_H%" echo // 字符串版本号
>> "%VERSION_H%" echo #define VER_FILEVERSION_STR             "!FILE_VER!"
>> "%VERSION_H%" echo #define VER_PRODUCTVERSION_STR          "!PROD_VER!"

chcp 936 >nul
exit /b 0


