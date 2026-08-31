@echo off
setlocal
if not defined Z88DK_REAL (
  echo Set Z88DK_REAL to the local z88dk installation directory.
  exit /b 1
)
if not exist "%Z88DK_REAL%\bin\zcc.exe" exit /b 1
if not exist Z:\bin\zcc.exe subst Z: "%Z88DK_REAL%"
if not exist Z:\bin\zcc.exe exit /b 1
set "PATH=Z:\bin;%PATH%"
set "ZCCCFG=Z:\lib\config"
cd /d "%~dp0"
del /q 000001.bin 2>nul
del /q 000001.NABU 2>nul
del /q 000001.nabu 2>nul
echo Building NCC-RF-260826-LV1
"Z:\bin\zcc.exe" +nabu main.c minimal_vdp_isr.asm -create-app -m -s --list -o000001.bin
if errorlevel 1 exit /b 1
if exist 000001.NABU exit /b 0
if exist 000001.nabu exit /b 0
exit /b 1
