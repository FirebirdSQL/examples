@echo off

for %%i in ("%~dp0.") do (
	if not defined BUILD_DIR (
		set "PATH=%%~fi\..\build\vs2022-x64\vcpkg_installed\x64-windows\debug\bin;%PATH%"
	) else (
		set "PATH=%BUILD_DIR%\vcpkg_installed\x64-windows\debug\bin;%PATH%"
	)
)
