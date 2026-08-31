# v1.0 protocol/resource source index

The authoritative v1.0 client validators are in:

`client/phase3a_live_local_weather_v0_1/NABU_COMMAND_CENTER_LIVE_LOCAL_WEATHER_v0_1/main.c`

The authoritative Gateway encoders/publishers are in:

`gateway/phase3a_live_local_weather_v0_1/NABU_COMMAND_CENTER_LOCAL_WEATHER_GATEWAY_v0_1/`

Current resources are `ncc_zip.req` (17 bytes), 64-byte `ncc_time.dat`, `ncc_weather.dat`, `ncc_location.dat`, `ncc_wxhist.dat`, `ncc_alert.dat`, `ncc_quake.dat`, `ncc_space.dat`, `ncc_satellite.dat`, and `ncc_airspace.dat`, plus 512-byte `ncc_music.dat`.

Reconstruction must use the exact current validators and encoders for field widths, checksum coverage, terminal bytes, token/ZIP matching, sequence rules, and last-known-good behavior. Historical protocol documents do not override the v1.0 source.
