# Integrated Command Center Gateway

Startup: double-click `START_COMMAND_CENTER_GATEWAY.BAT`.

One console and one Python supervisor remain running. Internally it starts:

- a ZIP-request/weather worker using `weather.ini`;
- a NIST-time maintenance worker using `command_center.ini`.

Store: `D:\NABU Internet Adapter\Store`.

The gateway creates no public listener. Weather cadence defaults to 900 seconds
for the last valid current-session ZIP request. NIST resynchronization defaults
to 3600 seconds; TIME Store publication defaults to the same 3600-second
resynchronization cadence and disciplined holdover expires
after 7200 seconds without a new accepted NIST sample.

Stop: focus the gateway console and press Ctrl+C. Both workers receive the same
stop event. Time logs are written to `logs\command_center_time.log`; weather
request/publication activity is printed in the console.

`START_ZIP_PROOF.BAT`, `START_WEATHER_GATEWAY.BAT`, and the old one-shot NIST
command remain preserved as historical/proof tools. They are not the integrated
Owner acceptance startup.
