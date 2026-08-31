# NABU Command Center v1.0

NABU Command Center is a live information and diagnostics application for the NABU Personal Computer, created by Derek Leger (Super_Derek).

This is the sanitized publication repository derived from licensed release commit `75390dc68b2b2dfa3143d8e08d48dfb9fc946fe3`. It contains the production NABU client, production Gateway, focused tests and build scripts, release documentation, and the exact owner-accepted v1.0 artifacts.

## Quick Start — Running NABU Command Center

1. Use the accepted boot image at [`releases/v1.0/artifacts/000001.NABU`](releases/v1.0/artifacts/000001.NABU).
2. In [`gateway/phase3a_live_local_weather_v0_1/NABU_COMMAND_CENTER_LOCAL_WEATHER_GATEWAY_v0_1`](gateway/phase3a_live_local_weather_v0_1/NABU_COMMAND_CENTER_LOCAL_WEATHER_GATEWAY_v0_1), copy `config.example.ini` to `command_center.ini`. Set `store_path` to the Store directory used by your NABU Internet Adapter.
3. Install the required Gateway dependency once: `py -3 -m pip install -r requirements-music.txt` (or `python -m pip install -r requirements-music.txt`).
4. Start the production Gateway by double-clicking `START_COMMAND_CENTER_GATEWAY.BAT`. Keep it running; it publishes the live service files into the Internet Adapter Store.
5. Use the NABU Internet Adapter to make `000001.NABU` available as the Command Center boot image. Start the NABU or emulator, select Command Center, and boot it.
6. A successful connection reaches the Command Center dashboard with `NET LIVE`; live modules then consume the companion Gateway data.

For complete setup, controls, and operating guidance, see the [NABU Command Center v1.0 User Manual and Operator Guide](docs/user/NABU_Command_Center_v1.0_User_Manual_and_Operator_Guide.docx).

## Accepted artifact

- Build ID: `NCC-SPLASH-260826-LV2`
- `000001.NABU`: 59,501 bytes
- SHA-256: `5D27DFBB3ED8DCD96AF828CECC7550EF546B07C10509E20A71FECFCF0CC89087`
- MAME execution: owner verified
- Physical NABU execution and GUI/runtime operation: owner verified

The authoritative copies are in [`releases/v1.0/artifacts`](releases/v1.0/artifacts).

## Source layout

- `client/phase3a_live_local_weather_v0_1/NABU_COMMAND_CENTER_LIVE_LOCAL_WEATHER_v0_1/` — production NABU client, build scripts, and focused tests.
- `gateway/phase3a_live_local_weather_v0_1/NABU_COMMAND_CENTER_LOCAL_WEATHER_GATEWAY_v0_1/` — production Gateway, example configuration, launchers, data, and tests.
- `docs/` — release-appropriate engineering, user, roadmap, protocol, and module documentation.
- `releases/v1.0/` — accepted artifacts, manifests, and build evidence.

Use `config.example.ini` as the configuration template. Local configuration, caches, logs, credentials, historical scratch material, and private evidence are intentionally excluded.

## License and provenance

Derek-owned NABU Command Center material is licensed under the [MIT License](LICENSE), Copyright (c) 2026 Derek Leger. Third-party boundaries and attribution are documented in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). NABU-LIB is referenced for provenance but is not redistributed; see [NABU_LIB_REFERENCE.md](NABU_LIB_REFERENCE.md).

See [PUBLICATION_PROVENANCE.md](PUBLICATION_PROVENANCE.md) for the relationship between this sanitized repository and the private frozen engineering repository.
