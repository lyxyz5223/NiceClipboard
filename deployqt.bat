@echo off
setlocal enabledelayedexpansion


set PROJECT_NAME=NiceClipboard

windeployqt .\pack\x64\


for /f "delims=" %%i in ('version.bat get VER_PRODUCTVERSION_STR') do (
    set "VER_PRODUCTVERSION_STR=%%i"
    :: 去除字符串中的所有空格
    set "VER_PRODUCTVERSION_STR=!VER_PRODUCTVERSION_STR: =!"
)

echo Version: [!VER_PRODUCTVERSION_STR!]

:: Copy the files from x64 to the main pack directory
copy .\pack\x64\!PROJECT_NAME!.exe .\pack\!PROJECT_NAME!_v!VER_PRODUCTVERSION_STR!_x64.exe

:: Compress the files in the x64 directory into a zip file
powershell Compress-Archive -Path ".\pack\x64\*" -DestinationPath ".\pack\!PROJECT_NAME!_v!VER_PRODUCTVERSION_STR!_x64.zip" -Force

