@echo off
setlocal
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -property installationPath`) do set "VSROOT=%%i"
if not defined VSROOT (echo Visual Studio not found & exit /b 1)
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0..\.."
cmake --preset dev || exit /b 1
cmake --build --preset dev || exit /b 1
ctest --preset dev
exit /b %ERRORLEVEL%
