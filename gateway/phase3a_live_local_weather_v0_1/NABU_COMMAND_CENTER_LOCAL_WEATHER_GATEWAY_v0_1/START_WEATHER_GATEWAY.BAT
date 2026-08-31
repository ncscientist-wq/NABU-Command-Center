@echo off
cd /d "%~dp0"
python weather_gateway.py --config weather.ini
