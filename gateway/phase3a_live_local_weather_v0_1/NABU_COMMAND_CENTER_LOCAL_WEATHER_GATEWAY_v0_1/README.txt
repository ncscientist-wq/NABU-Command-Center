NABU COMMAND CENTER GATEWAY SIMULATOR
VERSION 0.1
BUILD NCC-GW-260806-1043-P01

PHASE 3A INTEGRATED LIVE-DATA STARTUP

Double-click `START_COMMAND_CENTER_GATEWAY.BAT`. It runs one local Python
supervisor with a ZIP/weather worker and a NIST-time maintenance worker. The
Store is `D:\NABU Internet Adapter\Store`; no public listener is created.
Weather is refreshed every 900 seconds for the last valid current-session ZIP.
NIST is resynchronized every 3600 seconds; between accepted NIST samples the
TIME resource is refreshed on the 3600-second NIST resynchronization cadence.
The NABU advances the accepted UTC locally between synchronizations, so the
Store resource is not replaced repeatedly while a client may have it open.
never Windows wall-clock authority. Holdover expires after 7200 seconds.
Press Ctrl+C in the gateway window to stop both workers cleanly. Time logs are
written to `logs\command_center_time.log`; weather activity remains visible in
the same console. `START_ZIP_PROOF.BAT` is historical proof tooling only.

DESTINATION

<NABU_WORKSPACE>\
NABU_COMMAND_CENTER_GATEWAY_SIM_v0_1

PURPOSE

Generate one small deterministic ncc_test.dat record and publish it by:

1. A configurable local Store directory using an atomic replacement, or
2. A loopback-only HTTP endpoint, or
3. Both.

The Store publisher is the provisional lowest-risk publisher for the first
experiment because it avoids HTTP-cache freshness ambiguity. This is an
INFERRED selection pending the exact installed RetroNET header audit and the
actual Internet Adapter Store-path/version evidence.

The gateway is complete and standard-library-only. It does not prove that the
NABU can open the file.

QUICK START

1. Copy config.example.ini to config.ini.
2. Set store_path to the actual verified Internet Adapter Store directory.
   Do not guess the path.
3. Run RUN_TESTS.BAT.
4. Run START_GATEWAY.BAT.
5. At the gateway prompt, enter r to publish a new sequence.
6. Enter h for all commands.

DEFAULT VALID RECORD

NCC|1|TEST|000001|1786038180|17|0042|GATEWAY_RECORD_01|0F6E|END

The fixture is exactly 64 bytes including the final LF when the fixed UTC and
sequence shown above are used.

FIELD ORDER

MAGIC | VERSION | TYPE | SEQUENCE | UTC | LENGTH | VALUE | TEXT | CHECK | END

LENGTH is the decimal byte count of TEXT, not the complete record size.

CHECK is the uppercase four-digit hexadecimal low 16 bits of the sum of every
ASCII byte from N in NCC through and including the separator after TEXT.

PUBLISH MODES

store:
    Writes a temporary file in the selected directory, flushes it, calls
    os.fsync(), closes it, and uses os.replace() to replace ncc_test.dat.

http:
    Serves only /ncc_test.dat. Default bind is 127.0.0.1:8765. Directory
    browsing is not implemented. Other paths return 404.

both:
    Enables both publishers.

FAILURE MODES

valid
missing
empty
truncated
bad_magic
unsupported_version
bad_length
bad_integrity
oversized
stale_timestamp
sequence_rollback
offline

INTERACTIVE COMMANDS

r
    Increment sequence, update UTC, generate, and publish.

s
    Show current state.

f MODE
    Select a failure mode; use r to publish it.

v INTEGER
    Set known integer 0..9999.

t TEXT
    Set known text; the gateway sanitizes it and fixes it to 17 characters.

a on
    Start automatic refresh.

h
    Show help.

q
    Clean shutdown.

SECURITY

- HTTP defaults to loopback.
- A non-loopback host is rejected unless --allow-non-loopback is supplied.
- No directory listing.
- No secrets.
- External APIs are not used.
- Logs contain generated public test records only.

STATUS

Gateway source: SOURCE CREATED
Gateway unit tests: see BUILD_STATE.txt
Gateway running on Derek's computer: NOT TESTED
Internet Adapter Store publication: NOT TESTED
NABU resource open/read: NOT TESTED
