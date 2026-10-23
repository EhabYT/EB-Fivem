@echo off
setlocal EnableExtensions EnableDelayedExpansion
REM ============================================================================
REM  EBengin UI - command line build
REM
REM    build.bat                 Release x64   (the only config that makes sense:
REM                                             FiveM / GTA V are 64-bit)
REM    build.bat Debug           Debug   x64
REM    build.bat Release Win32   Release x86   (won't inject into FiveM)
REM    build.bat Clean           deletes x64\ and Debug\ intermediate output
REM
REM  Needs: Visual Studio 2019 or 2022 with the "Desktop development with C++"
REM  workload. No DirectX SDK install required - d3d11.h / dxgi.h come from the
REM  Windows 10/11 SDK that ships with VS.
REM ============================================================================

set "CONFIG=Release"
set "PLATFORM=x64"
set "SOLUTION=EBengin UI.sln"

if /i "%~1"=="Clean" (
    if exist "%~dp0x64"     rd /s /q "%~dp0x64"
    if exist "%~dp0Debug"   rd /s /q "%~dp0Debug"
    if exist "%~dp0Release" rd /s /q "%~dp0Release"
    echo Cleaned intermediate output.
    exit /b 0
)
if /i not "%~1"=="" set "CONFIG=%~1"
if /i not "%~2"=="" set "PLATFORM=%~2"

if /i not "%CONFIG%"=="Release" if /i not "%CONFIG%"=="Debug" (
    echo Unknown configuration "%CONFIG%" - use Release or Debug.
    exit /b 1
)
if /i not "%PLATFORM%"=="x64" if /i not "%PLATFORM%"=="Win32" (
    echo Unknown platform "%PLATFORM%" - use x64 or Win32.
    exit /b 1
)

REM ---- locate MSBuild -------------------------------------------------------
set "MSBUILD="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * ^
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
        -property installationPath`) do set "VSROOT=%%i"
    if defined VSROOT if exist "!VSROOT!\MSBuild\Current\Bin\MSBuild.exe" ^
        set "MSBUILD=!VSROOT!\MSBuild\Current\Bin\MSBuild.exe"
)
if not defined MSBUILD (
    where msbuild >nul 2>nul && set "MSBUILD=msbuild"
)
if not defined MSBUILD (
    echo.
    echo  MSBuild was not found.
    echo  Install Visual Studio 2019/2022 and tick the
    echo  "Desktop development with C++" workload, then re-run this script.
    echo.
    exit /b 1
)

echo.
echo  MSBuild   : %MSBUILD%
echo  Target    : %CONFIG%^|%PLATFORM%
echo.

"%MSBUILD%" "%~dp0%SOLUTION%" ^
    /t:Rebuild ^
    /p:Configuration=%CONFIG% ^
    /p:Platform=%PLATFORM% ^
    /m ^
    /nologo ^
    /clp:Summary;Verbosity=minimal
if errorlevel 1 (
    echo.
    echo  BUILD FAILED - scroll up for the first error.
    exit /b 1
)

set "OUT=%~dp0%PLATFORM%\%CONFIG%\EBengin UI.dll"
if not exist "%OUT%" set "OUT=%~dp0%PLATFORM%\%CONFIG%\ImGui DirectX 11 Kiero Hook.dll"

echo.
echo  BUILD OK
echo  DLL   : %OUT%
if exist "%~dp0EBengin.png" (
    copy /y "%~dp0EBengin.png" "%~dp0%PLATFORM%\%CONFIG%\EBengin.png" >nul
    echo  Logo  : copied EBengin.png next to the DLL ^(the menu loads it from there^)
) else (
    echo  Logo  : EBengin.png not found - the menu falls back to a text logo
)
echo.
endlocal
exit /b 0
