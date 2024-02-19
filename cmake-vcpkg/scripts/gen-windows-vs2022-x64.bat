@echo off
setlocal

for %%i in ("%~dp0.") do set "SCRIPT_DIR=%%~fi"
set ROOT_DIR=%SCRIPT_DIR%\..
if not defined BUILD_DIR set BUILD_DIR=%ROOT_DIR%\build\vs2022-x64

cmake ^
	-S %ROOT_DIR% ^
	-B %BUILD_DIR% ^
	-DCMAKE_INSTALL_PREFIX=%BUILD_DIR%\install ^
	-G "Visual Studio 17 2022" ^
	-A x64
