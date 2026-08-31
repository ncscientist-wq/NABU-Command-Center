@echo off
setlocal
cd /d "%~dp0"

where py >nul 2>nul
if not errorlevel 1 (
    py -3 -m unittest discover -s tests -v
    set "EXIT_CODE=%ERRORLEVEL%"
    goto done
)

where python >nul 2>nul
if not errorlevel 1 (
    python -m unittest discover -s tests -v
    set "EXIT_CODE=%ERRORLEVEL%"
    goto done
)

echo ERROR: Python 3 was not found.
set "EXIT_CODE=1"

:done
pause
exit /b %EXIT_CODE%
