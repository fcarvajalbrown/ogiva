@echo off
setlocal
if "%~1"=="" (echo usage: ue-build-plugin.cmd ^<EngineRoot^> & exit /b 2)
set "PKG=%TEMP%\ogiva-pkg"
if exist "%PKG%" rmdir /s /q "%PKG%"
call "%~1\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="%~dp0..\..\plugin\Ogiva\Ogiva.uplugin" -Package="%PKG%" -TargetPlatforms=Win64 -Rocket
exit /b %ERRORLEVEL%
