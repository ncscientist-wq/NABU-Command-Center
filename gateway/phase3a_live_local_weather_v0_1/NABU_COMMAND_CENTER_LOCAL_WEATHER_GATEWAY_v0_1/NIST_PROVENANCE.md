# NIST / NTP Provenance — Phase 3A-01A

- Date checked: 2026-08-10 America/Phoenix / 2026-08-11 UTC
- Source: NIST Internet Time Service
- Official service: https://www.nist.gov/pml/time-and-frequency-division/time-distribution/internet-time-service-its
- Official servers/status: https://tf.nist.gov/tf-cgi/servers.cgi/en-en/
- Protocol authority: RFC 5905, https://www.rfc-editor.org/rfc/rfc5905.html
- Configured hostname: `time.nist.gov` (NIST-recommended global round-robin name)
- Responding address: `132.163.96.2`
- Response source: `132.163.96.2:123`
- Protocol/port: SNTP-compatible NTP packet over UDP/123
- Timeout: 5 seconds
- Request: 48-byte VN4 client-mode packet with a nonzero transmit timestamp
- Accepted response: 48 bytes, server mode 4, version 3 or 4, LI not 3,
  stratum 1..15, matching originate timestamp, nonzero transmit timestamp
- KoD: stratum 0 rejected; RATE/DENY/RSTR are never rapidly retried
- UTC authority: server transmit timestamp at bytes 40..47, converted from the
  1900 NTP epoch to Unix UTC seconds; host clock is not the authoritative value
- Authentication: ordinary public NIST NTP; NOT cryptographically authenticated
- NIST rate rule: never query more frequently than once every four seconds
- Prior Phase 3A-01A behavior: one-shot/manual acquisition only

## Integrated maintenance

The integrated gateway performs an authoritative NIST transaction every 3600
seconds by default. The integrated gateway publishes TIME on that same
3600-second resynchronization cadence. Between accepted transactions it derives UTC
from elapsed `time.monotonic()` since the last NIST sample. This is labeled
as NIST-disciplined monotonic holdover, not host wall-clock
authority. The default holdover expires at 7200 seconds; after expiry, TIME
publication stops until a later valid NIST synchronization. Valid publications
advance sequence and use atomic Store replacement. Failed NIST acquisitions do
not advance sequence or replace the last valid resource.
- Host system clock modification: NO
- Failure: no record publication and no sequence advancement

Live accepted response: VN3, mode 4, LI 0, stratum 1, 48 bytes; UTC
`2026-08-11T06:26:24+00:00`; exact minute payload `2026-08-11T06:26Z`.
