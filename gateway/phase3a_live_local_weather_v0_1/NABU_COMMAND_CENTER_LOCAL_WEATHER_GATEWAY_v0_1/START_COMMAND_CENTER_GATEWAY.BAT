@echo off
setlocal
cd /d "%~dp0"

where py >nul 2>nul
if not errorlevel 1 (
    py -3 -c "import importlib.metadata as m; assert m.version('mido') == '1.3.3'; import mido" >nul 2>nul
    if errorlevel 1 (
        echo Required Gateway dependency missing or wrong version: mido==1.3.3
        echo Install once with: py -3 -m pip install -r requirements-music.txt
        goto failed
    )
    py -3 command_center_gateway.py --config command_center.ini
    goto done
)

python -c "import importlib.metadata as m; assert m.version('mido') == '1.3.3'; import mido" >nul 2>nul
if errorlevel 1 (
    echo Required Gateway dependency missing or wrong version: mido==1.3.3
    echo Install once with: python -m pip install -r requirements-music.txt
    goto failed
)
python command_center_gateway.py --config command_center.ini
goto done

:failed
exit /b 1

:done
if errorlevel 1 pause
endlocal
