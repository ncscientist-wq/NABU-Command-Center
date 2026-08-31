@echo off
setlocal
cd /d "%~dp0"

where py >nul 2>nul
if not errorlevel 1 (
    py -3 gateway.py --config config.ini
    set "EXIT_CODE=%ERRORLEVEL%"
    goto done
)

where python >nul 2>nul
if not errorlevel 1 (
    python gateway.py --config config.ini
    set "EXIT_CODE=%ERRORLEVEL%"
    goto done
)

echo.
echo ERROR: Python 3 was not found.
echo Install or select a verified Python 3 environment.
echo.
set "EXIT_CODE=1"

:done
echo.
echo Gateway exited with code %EXIT_CODE%.
pause
exit /b %EXIT_CODE%
