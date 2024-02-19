@echo off
setlocal

for %%i in ("%~dp0.") do set "SCRIPT_DIR=%%~fi"
set ROOT_DIR=%SCRIPT_DIR%\..
if not defined BUILD_DIR set BUILD_DIR=%ROOT_DIR%\build\vs2022-x64

cmake --build %BUILD_DIR% --config Release
