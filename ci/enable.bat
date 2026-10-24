@echo off
setlocal EnableExtensions

rem Copies the staged workflow into .github\workflows\ and stages it.
rem It deliberately does NOT commit and does NOT push: that commit has to be
rem made by an account holding the "workflows" permission, which the token
rem that produced this PR does not have.  See ci\README.md.

set "REPO=%~dp0.."
set "SRC=%REPO%\ci\build.yml"
set "DST=%REPO%\.github\workflows\build.yml"

if not exist "%SRC%" (
    echo [enable] source not found: %SRC%
    echo [enable] run this script from a checkout of the repository.
    exit /b 1
)

if not exist "%REPO%\.github\workflows" mkdir "%REPO%\.github\workflows"

copy /Y "%SRC%" "%DST%" >nul
if errorlevel 1 (
    echo [enable] copy failed.
    exit /b 1
)
echo [enable] copied ci\build.yml -^> .github\workflows\build.yml

git -C "%REPO%" add -- .github/workflows/build.yml
if errorlevel 1 (
    echo [enable] git add failed - is this a git repository, and is git on PATH?
    exit /b 1
)
echo [enable] staged.

echo.
echo [enable] Now commit and push with your own credentials:
echo.
echo     git commit -m "ci: enable the Windows build workflow"
echo     git push origin arena/57915bfa-eb-fivem
echo.
echo [enable] Then start the first run from the Actions tab (workflow_dispatch).

endlocal
exit /b 0
