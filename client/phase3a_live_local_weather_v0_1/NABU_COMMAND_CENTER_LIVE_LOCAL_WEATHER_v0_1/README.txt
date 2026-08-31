NABU COMMAND CENTER - PHASE 3A-01A LIVE ATOMIC CLOCK FIRST PROOF

Creator: Derek Leger
Copyright (c) 2026 Derek Leger. All rights reserved.
Product Version: UNASSIGNED
Working Project Version: v0.9
Build ID: NCC-AC-260811-P3A-01AR1

A04C3R2 is the frozen Phase 2D Complete Visual Shell. Owner MAME acceptance is
VERIFIED. Physical NABU acceptance is NOT TESTED. A04C3R1 remains preserved at
commit 04e66adf33c68d31bb384a652d956e5831b15186.

All maximized modules now reserve x=8..174 for their full title and keep
source/status information in the right rail. UI cues are explicitly stopped
before synchronous refresh, before replacement, and at natural completion so
AY channel A always reaches its defined silent state.

The three-page Task Manager uses measured fixed columns, a dedicated scope/
PAGE row, and an INTERNAL or CONST/MAP rail. Its static labels are constructed
on page entry; Page 1 and Page 3 values update independently at MAX cadence.
Music Stream controls, state, counters, track information, and scheduler-rate
information are intentionally absent from all Task Manager pages.

MUSIC STREAM is an original compact local AY event stream, not Internet audio.
B toggles it globally; cold boot defaults OFF. Music owns AY B/C while existing
SOUND-controlled cues retain A. OFF silences only music voices; ON resumes at
the next retained stream event.

Dashboard animation is staggered one mini-module activity lane every 900
scheduler iterations. Maximized module viewports update every 300 iterations.
No measured frame-rate claim is made. Static frames, headers, and rails remain
stable; no display blanking or artificial rendering delay is used.

The fixed-64 RetroNET Store transport function remains unchanged.

Build:
Z:\bin\zcc.exe +nabu main.c -create-app -o000001.bin

Runtime status: MAME VERIFIED. Physical NABU: NOT TESTED — intentionally deferred.

This isolated candidate preserves the Phase 2D shell and fixed-64 RetroNET
transport while binding manual R refresh to `ncc_time.dat`. Before acceptance
the header reads ATOMIC CLOCK UNSYNC. A valid TIME record displays its bounded
`YYYY-MM-DDTHH:MMZ` payload in the existing header geometry.

Freeze boundary: this milestone freezes the visual/application shell.
Production live-data feeds belong to the next phase.
