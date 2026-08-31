# Changelog

## 2026-08-12 - Transport Gate 0 Level-3E interrupt isolation

- Added a diagnostic-MAME-only RAM flight recorder for actual NABU interrupt mask, request, priority, Z80 INT/ack, IFF/IM/PC/SP, HCCA read, DAV, and overrun transitions.
- Reproduced a TIME payload ordinal-20 (`0x37`) 63/64 stall and verified Case D: HCCA RX asserted/enabled/selected during a vector-6 video ISR, but IFF1/IFF2 remained cleared until an 84.660 us-delayed vector-0 acknowledge; UART overrun occurred before the data read.
- Preserved production NABU source, binaries, transport, record formats, gateway, Adapter configuration, and installed MAME unchanged.

## 2026-08-12 — Transport Gate 0 Level-3D serial-rate validation

- Verified from exact diagnostic-fork source and runtime configuration that HCCA already operates at 111900 TX/RX, 8N1, no flow control; the historical baud-mismatch hypothesis is not applicable and no A/B/A configuration change was made.
- Added emulated-time-only UART completion, DAV assertion, and CPU data-register-read fields to the bounded RAM recorder.
- Reproduced four complete reads followed by a 63/64 overrun stall at the unchanged correct line rate.
- Successful bytes were serviced in 35.210–40.122 us (36.588 us median) against a 98.337 us median character interval. The failed byte was not read during its full 97.778 us window; the following character arrived at nominal timing.
- Overrun remains VERIFIED; baud mismatch as its trigger is FAILED / NOT APPLICABLE. The cause of the isolated late CPU service remains UNVERIFIED. Production NABU code remains unchanged.

## 2026-08-12 — Transport Gate 0 Level-3C UART discrimination

- Preserved Level-3B at checkpoint `4edb79be3144bb1d7c855e765ca643185f00231f` and added a fixed-size, RAM-only AY-3-1015 receive-completion recorder to the isolated diagnostic MAME fork.
- Automatically reproduced four complete 64-byte TIME reads followed by one 63/64 stalled read without changing the production NABU binary, installed MAME, Adapter configuration, gateway, or record format.
- Correlated the complete host Point-B stream, UART completions, and HCCA/client stream. Payload ordinal 5 (`0x31`, `1`) completed at the UART but was overwritten by the next byte (`0x7C`) when it completed with DAV already asserted.
- Captured the exact UART transition: DAV `1 -> 1`, overrun `0 -> 1`, framing error `0 -> 0`; exact AY-3-1015 source semantics replace the pending receive buffer on this transition.
- UART overrun is VERIFIED as the causal byte-loss mechanism for this captured failure. No production fix was made; work stops for Engineering Manager review.

## 2026-08-12 - Transport Gate 0 Level-3B low-observer host recorder

- Built a separate diagnostic MAME 0.250 NABU executable with fixed in-memory
  RX rings at socket/Bitbanger dequeue (Point A) and immediately before
  null-modem serialization (Point B); the installed MAME remained unchanged.
- Automated an orderly dump and reproduced the intermittent fixed-64 TIME read:
  one complete 66/66 transaction followed by a client-visible 65/66 stall.
- Both host points contained the complete declared-length plus 64-byte payload
  for the failing transaction. Payload ordinal 46 (`0x31`, `1`) was present at
  Points A/B but absent at HCCA ISR/client, while later bytes shifted left.
- First verified divergence is therefore between null-modem serialization
  handoff and the HCCA ISR. The root-cause branch is emulated serial/UART/interrupt
  delivery; the precise mechanism remains UNVERIFIED.
- No production NABU source, binary, protocol, Store, gateway, Adapter, UI,
  sound, music, record size, or installed-MAME change was made.

## 2026-08-12 - Transport Gate 0 Level-3A diagnostic checkpoint

- Closed the blocked elevated PktMon route and verified the MAME 0.250 host
  path from TCP 5816 Bitbanger input through null-modem serialization to HCCA.
- Added a separate Adapter diagnostic configuration enabling its existing
  bounded communication/console logs without changing the installed config.
- Production NABU source, binary, protocol, Store records, Adapter binary,
  MAME binary/configuration, gateway, UI, sound, and music remain unchanged.

## 2026-08-12 - Transport Gate 0 Level-2 discrimination

- Reproduced the intermittent fixed-64 A5 stall with targeted Level-2 automation.
- Corrected the byte-loss classification: payload ordinal 16 (`0x32`, `2`) was
  absent before the earliest observed HCCA RX ISR byte-read point; subsequent
  bytes shifted left and the terminal LF arrived as observed ordinal 63.
- Preserved the matched successful 66/66 response and failed 65/66 response,
  decoder reconciliation, bounded trace, evidence hashes, and fault-tree result.
- Root cause remains UNVERIFIED; no production source, binary, protocol, Store,
  Adapter, gateway, sound, music, UI, or record-format change was made.

## 2026-08-12 — Transport Gate 0 Level-1 automation checkpoint

- Added a host-side A3/A5/A7 protocol-boundary decoder that identifies real
  transactions by command/resource chronology rather than diagnostic `att`.
- Added verified MAME-native automated input, tiered Level 1/2/3 debugger
  instrumentation, unit tests, and bounded evidence-package generation.
- Reproduced the intermittent stall automatically at Level 1: four A5 reads
  completed, then the fifth received returned length `0040` and payload 63/64.
- First verified missing transition is response byte 66/66, payload byte 64/64.
- Root cause remains UNVERIFIED. No production source, binary, protocol, Store,
  Adapter, gateway, sound, music, or UI correction was made.

## 2026-08-11 - Phase 3A live-data correctness recovery candidate

- Proved and corrected the persisted ZIP-request lifecycle fault: the gateway
  now ignores a pre-existing request at startup and detects a later identical-byte
  rewrite by file-write signature, while the client requires a current-session token.
- Bound both Local Weather renderers to one accepted cache; removed the embedded
  `72F W8KT` / `72F WIND 8KT` production strings and weather `SOURCE MOCK` state.
- Added durable numeric weather stages before fixed-64 Store open/read operations.
- Added a verified installed TMS9918 frame-sync callback for local one-second UTC
  advancement after manual synchronization; no background NABU network polling.
- Added periodic weather maintenance, NIST-disciplined monotonic time holdover,
  and one integrated local gateway launcher. Forty-four host tests pass.
- Built `NCC-LD-260811-P3A-02C1`; MAME and physical NABU remain NOT TESTED.

## 2026-08-11 - Phase 3A-01A Atomic Clock MAME-verified freeze

- Owner verified `NCC-AC-260811-P3A-01AR1` boots and immediately repaints the
  accepted UTC on the dashboard after manual R, without opening a module.
- Clock persistence, navigation, Sound, Music Stream, and channel-A cue
  regressions passed in MAME.
- Froze the exact tested 32,201-byte byte-identical artifacts without rebuilding.
- Physical NABU is NOT TESTED and intentionally deferred.

## 2026-08-11 - Phase 3A-01A manual-R dashboard repaint correction

- Reconfirmed that the manual R handler skipped dashboard repaint after caching
  an accepted TIME record.
- Added one bounded y=13..19 header-row clear followed by the existing header
  renderer for dashboard refresh; other views retain their prior redraw path.
- Added no startup network access, polling, transport, gateway, protocol,
  graphics-layout, or sound changes.
- Built `NCC-AC-260811-P3A-01AR1`; MAME remains NOT TESTED.

## 2026-08-10 - Phase 3A-01A live Atomic Clock candidate

- Branched from the frozen Phase 2D tag and created isolated Phase 3A client and
  gateway working areas without modifying the frozen project.
- Reused the fixed-64 encoder, atomic replacement helper, and client transport
  semantics for a dedicated `ncc_time.dat` TIME record.
- Added bounded NIST SNTP acquisition and validation; 26 host tests pass.
- Published one validated stratum-1 NIST response from `132.163.96.2:123` as an
  exact 64-byte record and built `NCC-AC-260810-P3A-01A` successfully.
- MAME and physical NABU remain NOT TESTED.

## 2026-08-10 - Phase 2D complete visual shell freeze

- Owner accepted `NCC-VS-260809-P2D-A04C3R2` in MAME and authorized the
  Phase 2D v0.9 visual/application-shell freeze.
- Verified both 32,165-byte artifacts remain byte-identical with SHA-256
  `246BB94AE174F32FBC1DD94343853BD3F3A1B5AC20CABFE60891F6D20E2F0689`.
- Froze the dashboard, modules, Task Manager, persistent controls, schedulers,
  natural renderer, fixed-64 transport, navigation, and final corrections.
- Production live-data feeds belong to the next phase. Physical NABU remains
  NOT TESTED.

## 2026-08-10 - Phase 2D-A04C3R2 final defect correction

- Reserved a measured x=8..174 maximized-title region for every module and kept
  source/status information exclusively in the existing right rail.
- Established one channel-A cue stop/reset path and invoked it before synchronous
  transport refresh, before cue replacement, at completion, and on SOUND OFF.
- Preserved fixed-64 transport, Music Stream B/C behavior, schedulers, three-page
  Task Manager, and natural progressive rendering.
- Built `NCC-VS-260809-P2D-A04C3R2`; runtime remains NOT TESTED.

## 2026-08-10 - Phase 2D-A04C3R1 pre-MAME correction

- Preserved rejected pre-runtime candidate `fac071f` as evidence.
- Removed Music Stream controls, labels, state, track/step/wait telemetry, and
  scheduler-rate information from all three Task Manager pages.
- Restored a simple MODULE/TASK MGR internal field without adding new telemetry.
- Preserved the static SOUND and full-word MUSIC STREAM header states, B toggle,
  AY engine/channel ownership, music across views, mini/max schedulers, fixed-64
  transport, three-page diagnostic layout, and natural rendering.
- Built NCC-VS-260809-P2D-A04C3R1; runtime remains NOT TESTED.

## 2026-08-09 - Phase 2D-A04C3 final pre-freeze integration

- Preserved A04C2 and created the isolated v0.9 project/branch.
- Corrected Task Manager title/page/grid/rail crowding with measured 5x7 cells,
  internal provenance, independently refreshed live values, and no redundant ID.
- Added persistent full-word MUSIC STREAM header state and global B toggle.
- Added an original 16-event nonblocking AY stream on B/C while preserving UI
  cues on A and separate SOUND policy.
- Added staggered slow activity for all six dashboard panels and a faster bounded
  viewport update for every maximized module; music state persists across views.
- Preserved natural reveal, fixed-64 transport, A04C2 behavior, and other module
  architecture. Built NCC-VS-260809-P2D-A04C3; runtime remains NOT TESTED.

## 2026-08-09 - Phase 2D-A04C2 Task Manager expansion

- Preserved the exact A04C1 rollback and expanded the existing Task Manager tile
  into three bounded pages without changing the six-module dashboard.
- Added LIVE STATUS, HARDWARE / MEMORY, and VIDEO / INTERNALS pages using only
  hardware constants, measured/calculated build evidence, and live state.
- Added silent no-wrap LEFT/RIGHT page navigation, Task Manager-specific footer,
  inert ENTER behavior, and preserved ESC/M dashboard return.
- Preserved natural progressive rendering, fixed-64 transport, sound, input,
  all other A04C1 visual/resource work, and all five other modules.
- Built NCC-VS-260809-P2D-A04C2; runtime remains NOT TESTED.

## 2026-08-09 - Phase 2D-A04C1 display-transition correction

- Recorded the Owner A04C failure: BL-bit blanking produced a long black wait.
- Removed the blanking state, helpers, and all four transition calls.
- Restored natural visible Z80/TMS9918 drawing with no artificial delay.
- Preserved all other A04C source, instrumentation, resource, transport, sound,
  input, visual, creator, and legal work.
- Built NCC-VS-260809-P2D-A04C1; runtime remains NOT TESTED.

## 2026-08-09 - Phase 2D-A04C performance/polish/resource pass

- Preserved A04B and created isolated v0.8 production candidate metadata.
- Added protected viewport HUD/clipping and stronger ADS-B selection brackets.
- Added deliberate compact status text and restored ZIP/profile header visibility.
- Added actual render stages/counters and Task Manager render telemetry.
- Reused prior projected 3D endpoints without adding a vertex cache.
- Implemented verified TMS9918 BL-bit display hiding for major full transitions.
- Generated map/symbol/listing evidence and documented RAM/stack/VRAM results.
- Preserved the fixed-64 transport and A04B AY/input behavior.
- Built NCC-VS-260809-P2D-A04C; runtime remains NOT TESTED pending Owner gate.

## 2026-08-09 - Phase 2D-A04B production remediation

- Preserved A04 and created the isolated v0.7 production project.
- Added the honest ATOMIC CLOCK UNSYNC header, shared integer pseudo-3D globe,
  Satellite/ISS orbits, local/global Earthquake views, and bounded scheduler.
- Replaced RF/APRS with an actual-state NABU System/Task Manager; RF/APRS parity
  is SUPERSEDED BY OWNER.
- Added the final bounded AY cue vocabulary, including one-note successful
  navigation and silent blocked edges.
- Preserved the exact fixed-64 transport routine and A04 ADS-B implementation.
- Built NCC-VS-260809-P2D-A04B; runtime remains NOT TESTED pending one Owner gate.

## 2026-08-09 — Phase 2D-A04 production Command Center shell

- Implemented the audited V8 information architecture as a bounded native
  256x192 shell using the Owner-verified A03 palette and 5x7 font.
- Added a persistent honest header, six-story event tape, compact status/toast,
  DEMO ZIP profiles, LOCAL/GLOBAL emphasis, and consistent source/freshness state.
- Replaced all five remaining legacy detail pages and retained/enhanced ADS-B so
  all six modules now have coherent graphical views and compact telemetry rails.
- Added bounded Quake/Satellite/ADS-B target cycling and explicit lock state;
  migrated Help, Diagnostics, and Safe Idle to the same visual system.
- Preserved fixed-64 RetroNET behavior and existing AY behavior. No live feeds,
  new transport, pause scheduler, sound redesign, physical test, tag, or freeze.
- Final build passed: both 25395-byte artifacts are byte-identical with SHA-256
  `58AF203DDE5AD82F581DE9B62833B5871BCB0D5318DE0BA5C663C4C0F06EEDEA`.
- Runtime is NOT TESTED; stopped at the Owner A04 visual/function gate.

## 2026-08-09 — Phase 2D-A04 Checkpoint 0

- Recorded Owner verification of A03 high-density multicolor architecture in
  MAME 0.250 and preserved commit/artifact identity as an internal rollback.
- Audited the complete 1805-line V8 HTML/CSS/JavaScript runtime, its functions,
  data/state tables, controls, timing, six module renderers, and visible
  information classes.
- Added the explicit 117-item V8 parity matrix and calculated 256x192 screen
  layout budget before production source changes.
- No source, binary, transport, input, sound, frozen release, tag, or A03 artifact
  change was made in this documentation/evidence checkpoint.

## 2026-08-09 — Phase 2D-A A03 semantic color correction

- Preserved recoverable A01/A02 rollback points and the A02 5x7 font/layout.
- Added a centralized black-field semantic palette to dashboard and ADS-B only:
  white primary text, cyan primary vectors/secondary text, light-blue secondary
  vectors, yellow selection, green nominal, dark-yellow warning, light-red alert,
  and magenta special labels.
- Recorded the installed Graphics II 8x1 attribute-cell restriction and used
  bounded region separation, restoration redraw, and deliberate draw ordering.
- Preserved input, sound/AY, transport/protocol, fixed data, and other screens.
- z88dk compilation/linking passed. Both 19950-byte artifacts are byte-identical
  with SHA-256 `7C1953A08FF621E2967C70FA2299E6D071CBB597C9DF1F0B81BA46C96C72F51A`.
- Runtime remains NOT TESTED; stopped at Owner MAME Visual System Gate A03.

## 2026-08-07 — Phase T0 evidence audit

- Branch: `phase-transport-api-audit`
- Starting commit: `68f8b19cbb30f199c3a05b0a444455c8f7da589e`
- Files added: `docs/TRANSPORT_FINDINGS.md`, `docs/NETWORK_API_PROVENANCE.md`, `PHASE_STATE.md`, `CHANGELOG.md`
- Reason: record exact installed RetroNET declarations, implementations, version evidence, transport comparison, and unresolved semantics.
- Evidence: installed z88dk header/source/example/library plus local Adapter and MAME file metadata and hashes.
- Tests/builds/runtime: not performed; prohibited by T0 scope.
- Initial result: T0 PARTIAL; Store/mailbox provisional; T1 NOT READY.

## 2026-08-07 — Running Adapter identity follow-up

- Verified the currently operated executable as `C:\Program Files (x86)\DJ Sures\NABU Internet Adapter\NABU-Internet-Adapter-84.exe`, version `2025.02.07.00`; owner observed PID `17636`.
- Recorded executable SHA-256 `A3B845ED87A1E762E9FBC068F9D842958CC79F1C29FCFCD2E0799367CC8AF248` and installed README SHA-256 `0D4E798BC05EB041A96551321518C31F9F12F390D778037FE1087B7B208B3380`.
- The local README does not document the six audited RetroNET file commands or claim z88dk compatibility; compatibility remains UNVERIFIED.
- No build, MAME, Adapter restart, transport execution, or T1 work was performed.

## 2026-08-07 — Phase T1 Store proof source/build

- Created branch `phase-transport-proof-t1` from `5410d921a7e977a95e103e757c813d1d68bb7a13`.
- Verified Store `D:\NABU Internet Adapter\Store` and root resource `ncc_test.dat`.
- Added the bounded 64-byte read-only Store proof using `rn_fileOpen`, `rn_fileHandleRead`, and `rn_fileHandleClose` with checkpoints T01 through T12.
- Published the unchanged 64-byte valid fixture from existing gateway test tooling.
- z88dk compilation/linking passed; both artifacts are 12277 bytes with SHA-256 `193FF811B9CEBD59D69223969AF44728B1905C65B15456F29201F349C6B51847`.
- Runtime, MAME, and physical NABU tests were not performed.

## 2026-08-07 — Phase T1 stable-redraw correction

- Added one main-screen redraw flag so the screen is redrawn only after a key-visible state change.
- Preserved `R/r` as the only RetroNET refresh trigger and preserved all T01–T12, Safe Idle, Store, and protocol behavior.
- New build ID: `NCC-TP-260807-T1-02`.
- z88dk compilation/linking passed; both artifacts are 12317 bytes with SHA-256 `5DD4280281E7C9C2FAAC8A0A6445C0F391AD54A94512C9021CE58838C525A32A`.
- Runtime status for T1-02 remains NOT TESTED.

## 2026-08-07 — Verified T1-02 MAME runtime and freshness

- Owner-observed MAME 0.250 runtime verified boot, T01, stable redraw, responsive keyboard, Q Safe Idle, and M return.
- The initial deterministic 64-byte record produced sequence 1, value 42, text `GATEWAY_RECORD_01`, and T12.
- While the client remained running, the Store file was replaced with a valid 64-byte sequence-2 record: value 43, text `GATEWAY_RECORD_02`, checksum `0F6F`, SHA-256 `9E8F94DD3F34CE68832DFEABA9E63FB9085C01AF13B4868AF1801FC0BCDF37DD`.
- One subsequent R refresh displayed LIVE, sequence 2, value 43, text `GATEWAY_RECORD_02`, 64/64 bytes, and T12 without client rebuild/restart.
- MAME live receive/freshness path: VERIFIED. Physical NABU and adverse/recovery cases remain NOT TESTED.

## 2026-08-07 — T1 MAME-verified transport freeze

- Owner approved the T1 MAME-verified receive-dominant transport milestone for freeze.
- Preserved exact tracked T1 client/gateway material and recorded evidence under `frozen/transport_releases/ncc-transport-v0.1-mame-verified`.
- Created `releases/transport/NABU_COMMAND_CENTER_TRANSPORT_T1_MAME_VERIFIED_v0_1.zip`.
- ZIP size: 62858 bytes; SHA-256 `D651FB1382EDFF74A1F5703542C72C29CED631B6EC621F173463E5EC7CC83351`.
- Approved artifacts remain 12317 bytes each with SHA-256 `5DD4280281E7C9C2FAAC8A0A6445C0F391AD54A94512C9021CE58838C525A32A` and are byte-identical.
- Physical NABU remains NOT TESTED. T2 remains NOT STARTED.

## 2026-08-07 — Post-freeze physical NABU evidence

- Recorded Owner-observed physical execution of frozen build `NCC-TP-260807-T1-02`; no source change or rebuild was performed.
- Preserved 13 supplied photos/screenshots under `docs/evidence/t1_physical_nabu_2026-08-07` and documented their SHA-256 hashes in `docs/PHYSICAL_NABU_T1_EVIDENCE.md`.
- Verified physical boot, T01 READY, keyboard/main/diagnostic/Safe Idle behavior, one bounded 64/64 RetroNET Store receive, sequence 2/value 43/`GATEWAY_RECORD_02`, and T12 DISPLAY OK.
- Recorded Owner-observed `ADAPTOR FAILURE` with the physical interface disconnected and successful load after reconnection.
- Physical live Store replacement/freshness and adverse/recovery cases remain unverified. T2 remains NOT STARTED.

## 2026-08-08 — Additional post-freeze physical observations

- Recorded physical boot into NOT CONFIGURED with 0/64 bytes, first `R` transition to LIVE sequence 2, and second unchanged-record `R` transition to CACHED with 64/64 bytes and T12.
- Recorded Owner-observed Adapter serial activity opening and closing `ncc_test.dat` with Adapter handle `0` across multiple refresh attempts; no client-handle mapping is inferred.
- Recorded `STATE: INVALID` after `T` without assigning a cause.
- Added unchanged photo `1000073679.jpg` (SHA-256 `75A8A74582781FBB49C77094B9D88C8D7813262F45B9FEAED972075292ECAD48`) showing disconnected-interface `ADAPTOR FAILURE`.
- Physical live freshness after controlled in-session Store replacement remains NOT VERIFIED. Frozen release/tag remain unchanged; T2 remains NOT STARTED.

## 2026-08-08 — Phase T2A source/build preparation

- Created branch `phase-transport-reliability-v0.2` from `fa9d47d8f23fae2efc9425d23e0d1beff19feb0a`.
- Audited only the active bounded client validator, gateway failure modes/tests, acceptance plan, and protocol behavior.
- Reused existing client handling for all ten requested cases; no transport call, parser rule, protocol field, buffer size, or last-valid commit behavior changed.
- Updated the client identity to `NCC-TR-260808-T2A-01` and added a consolidated manual MAME matrix.
- Expanded gateway tests for all requested adverse fixtures and corrected a Windows-only test log-handler cleanup leak; final result 16/16 passed.
- z88dk compilation/linking passed once after final source changes. Both 12324-byte artifacts have SHA-256 `0CF1CD0C063922013732CD45457BE74781F25E00B2132D479370E50AA32138D8` and are byte-identical.
- MAME and physical NABU were not run. Frozen T1 release/tag remain unchanged.

## 2026-08-09 — T2B Adapter outage/recovery closeout

- Recorded T2A as MAME VERIFIED for all ten adverse-record cases.
- Recorded normal Store transport and cold recovery after restarting Adapter plus MAME as VERIFIED.
- Recorded Adapter disappearance during `rn_fileOpen` as FAILED at visible checkpoint `T02 OPEN START`; restarting the Adapter did not recover the active MAME/client session.
- Installed z88dk source verifies the blocking `rn_fileOpen` to `hcca_readByte` path; no supported timeout, cancellation, nonblocking, availability, or recovery mechanism was found.
- Owner decision: ACCEPT AS KNOWN LIMITATION. Custom lower-level HCCA recovery is deferred unless Derek explicitly reopens the issue.
- Documentation-only closeout: no source, binary, build, frozen release, or tag change. Physical NABU T2B NOT TESTED; T2C NOT STARTED.

## 2026-08-09 — T2C closeout and reliable 64-byte freeze decision

- Stopped variable-capacity work at Gate 1 because installed `rn_fileHandleRead` copies an Adapter-supplied returned count before caller validation and no installed evidence guarantees it is bounded by the request.
- Recorded T2C as BLOCKED / DEFERRED at the design/safety gate, not as a runtime failure.
- No V2 protocol, source change, rebuild, MAME test, physical test, RetroNET/HCCA change, or Adapter-recovery work was performed.
- Owner retained fixed 64-byte build `NCC-TR-260808-T2A-01` as the production transport foundation.
- Existing T2A MAME verification and T2B accepted limitation are preserved. Existing T1 freeze remains untouched.

## 2026-08-09 — Phase 2 native dashboard integration source/build

- Created `phase-dashboard-integration-v0.3` from tag `ncc-transport-v0.2-reliable-64-mame-verified` at `33fadc2ed6d046b4964707f9caa204a896b72981`.
- Added a new text-console native dashboard project with six deterministic mock module regions, field-specific redraw, module maximize/return, local news stepping, bounded five-digit ZIP profile entry, Safe Idle, help, diagnostics, and state-driven direct-AY cues.
- Integrated the frozen 64-byte `ncc_test.dat` RetroNET Store open/read/validate/close behavior without changing the protocol or accepted Adapter-outage limitation.
- Preserved D for diagnostics; W/S plus A provide complete two-column dashboard selection and E is the verified-letter activation key. ENTER/ESC/arrow mappings remain unverified and unused.
- Final z88dk `+nabu` compilation/linking passed. Both 13506-byte artifacts have SHA-256 `36453179CC3AE8E803703E152FFC70E97749932FEBDA88DEB50E3126958DC146` and are byte-identical.
- Runtime, MAME, Adapter recognition for this build, sprites, reset/reload, physical transfer, and physical execution remain NOT TESTED.

## 2026-08-09 — Phase 2B first graphics MAME gate

- Recorded Owner-observed MAME 0.250 verification of build `NCC-DG-260809-P2B-01`.
- The graphical 3x2 dashboard, all six native mini-graphics, visible selection highlight, S-row navigation, responsive main loop, and absence of visible corruption/lockup/redraw failure are VERIFIED.
- Recorded but did not address the unwanted double selection tone or requested future horizontal news ticker.
- Documentation-only update: no source, binary, frozen release, build, sound, input, ticker, transport, ZIP, maximized-view, live-module, or Phase 3 change.

## 2026-08-09 — Phase 2C verified input controls source/build

- Recorded Owner-observed MAME 0.250 raw `getk()` mappings: Right `F0`, Left `F1`, Up `F2`, Down `F3`, Enter `0D`, and Escape `1B`; number keys `31`-`36` were observed but not assigned.
- Implemented bounded, non-wrapping Arrow movement for the true row-major 3x2 dashboard grid, Enter module activation, Escape dashboard return, and preserved M fallback.
- Removed temporary W/S/A/E dashboard navigation without changing transport, protocol, sound/AY, ticker/news behavior, live data, or graphical maximized views.
- Built `NCC-DI-260809-P2C-01` successfully with the verified z88dk `+nabu` command. Both 17125-byte artifacts are byte-identical with SHA-256 `B366F19E326A22D6708BDFEF4C4A4CE185DDAC631C9EDF2A981A5CF98557EB80`.
- MAME was not run. Work stopped at the first Owner gate: cold boot, verify preserved graphics, press RIGHT once, and confirm EARTHQUAKE to SPACE WEATHER.

## 2026-08-09 — Phase 2C project-directory correction

- Restored the verified Phase 2B graphics project directory exactly to commit `8406c773ddff45a81a3a32cdeb2e30ee59c194fc`, including build `NCC-DG-260809-P2B-01` and its original artifacts.
- Relocated the unchanged Phase 2C production source/build/documentation tree to `client\phase2_dashboard_input_v0_5\NABU_COMMAND_CENTER_DASHBOARD_INPUT_v0_5`.
- Rebuilt `NCC-DI-260809-P2C-01` only from the new Phase 2C directory. Both 17125-byte artifacts retain SHA-256 `B366F19E326A22D6708BDFEF4C4A4CE185DDAC631C9EDF2A981A5CF98557EB80`.
- No input logic, key mapping, graphics, sound, transport, news, ZIP, protocol, or runtime scope changed. MAME was not run.

## 2026-08-09 — Phase 2C final Owner MAME input/regression gate

- Recorded Owner verification of `NCC-DI-260809-P2C-01` in MAME 0.250 on the NABU `nabupc` target.
- Verified all four bounded non-wrapping Arrow directions, the true 3x2 grid, Enter activation, Escape return, M fallback, Diagnostics, Help, Safe Idle, and their return behavior.
- Verified the preserved graphical dashboard, all six mini-panels, selection highlight, correct module activation, responsive operation, and absence of visible corruption, lockup, or wrap defects.
- Recorded one fixed-64 R refresh as VERIFIED for the observed `CACHED 64/64 T12 DISPLAY OK` case only; live freshness and Adapter-loss recovery were not newly tested.
- Recorded the unwanted navigation three-note tune and future one-beep/startup-fanfare targets as deferred without changing audio.
- Evidence/status documentation only; source, binaries, frozen files, transport, protocol, graphics, and other deferred scope remain unchanged.

## 2026-08-09 — Phase 2D-A Checkpoint A shared renderer and ADS-B build

- Created branch `phase-complete-visual-shell-v0.6` from verified commit `2140716d2a3a0ebb24d32bc3f9c930855e747d0c` and copied necessary Phase 2C working material into the new isolated visual-shell project.
- Preserved the verified Phase 2C project and frozen material unchanged.
- Added a reusable bounded detail frame, perspective grid, target-marker/leader operation, and the AIRSPACE / ADS-B stress view only.
- Implemented five deterministic MOCK contacts using Level 1 runtime integer pseudo-3D projection, altitude stems, heading vectors, bounded labels, and selected-target emphasis; no float, malloc, sprites, framebuffer, new graphics API, live source, or transport change.
- Static review verified visible coordinate bounds, one-time detail redraw, bounded tables/objects, and unchanged input, AY, fixed-64 transport, Diagnostics, Help, Safe Idle, dashboard, and ZIP source behavior.
- z88dk `+nabu` compilation/linking passed. Both 18497-byte artifacts are byte-identical with SHA-256 `8614DD6061235CEEA6FD565B9AB2C3792B2F1904C81B8BE432238EE033F01ADC`.
- Runtime is NOT TESTED. Work stopped before the remaining five graphical detail pages at required Owner MAME Smoke Gate A.

## 2026-08-09 — Phase 2D-A Owner MAME Gate A

- Recorded Owner verification of build `NCC-VS-260809-P2D-A01` in MAME 0.250.
- Dashboard load, AIRSPACE / ADS-B open, perspective grid, five contacts, altitude/depth representation, heading vectors, labels, selected target, Escape/M returns, responsiveness, and absence of observed lockup/corruption are VERIFIED.
- Recorded visual-quality direction to reduce leader/grid clutter, strengthen spatial communication, remove operational debug text, and establish a reusable high-density vector/microfont system.
- Evidence/status only; A01 source and artifacts remain unchanged as an internal verified rollback point. No tag or release freeze was created.

## 2026-08-11 — Phase 3A live-data 02C2 stability/UI experiment

- Preserved the exact failed 02C1 runtime artifacts and supporting source,
  machine-code evidence, screenshots, Adapter log, and complete gateway log.
- Localized the freeze to the installed blocking `rn_fileHandleRead()` path
  after `rn_fileOpen()` returned at checkpoint `T04 READ START`.
- Changed only integrated TIME publication cadence for Gate A: Store updates
  now occur with 3600-second NIST resynchronization instead of every 10 seconds;
  the NABU continues advancing synchronized UTC locally.
- Changed the clock renderer to update changed glyph cells only and replaced
  the fabricated maximized weather trend with the factual current observation.
- Host tests passed 44/44. Built `NCC-LD-260811-P3A-02C2`; both 37,667-byte
  artifacts are byte-identical with SHA-256
  `68247944C2FFD234FA1EA83968BD68F4AFE9900608DDB36D0657D295E21C28FD`.
- Follow-up review found no installed Adapter lock/share contract and no proven
  race-free fresh-publication mechanism. The 3600-second cadence is retained
  only as a stability experiment: later-R and cold-boot freshness are
  UNRESOLVED and can reset the client backward to an old Store snapshot.
- Exact low-level replacement causality remains INFERRED; the blocking read
  location and replacement overlap remain VERIFIED.
- MAME and physical NABU remain NOT TESTED. Final Owner MAME acceptance is
  deferred. No freeze, tag, or commit created.

## 2026-08-09 — Phase 2D-A A02 global high-density vector UI proof

- Created build `NCC-VS-260809-P2D-A02` while preserving A01 at commit `4f4e514f02d084101aef9383ac7edbbe4ce0d77a` plus Gate A evidence commit `a94334e57d4ff961d2bd944eabaf39307ff605c8`.
- Added a reusable 5x7 one-pixel font with 45 stored glyphs plus space, 325 exact table bytes, and no invented font/direct-VRAM API.
- Redesigned the main dashboard as a black-field, light-green, thin-vector 3x2 system with compact header/ticker/status/footer, one-pixel separators, expanded mini-plot height, and corner-only selection chrome.
- Refined ADS-B with a sparse nine-line horizon/range grid, five directional chevrons, stronger altitude stems, ground ticks, nearby non-overlapping labels with short leaders, selected-target emphasis, and compact operational telemetry; engineering debug text was removed.
- Preserved input, state semantics, fixed-64 transport, AY behavior, Diagnostics, Help, Safe Idle, and ZIP logic unchanged. No sprites, float, malloc, framebuffer, live data, sound work, or new graphics API was added.
- z88dk compilation/linking passed. Both 19545-byte artifacts are byte-identical with SHA-256 `CB70048BFAE1BFDD8F99426B04B8AE1FD7BAFFFC8489753D0D695F19068539BF`.
- A02 runtime/readability/visual quality are NOT TESTED. Work stopped before propagating the system at Owner MAME Visual System Gate A2.
## 2026-08-13 — Proven transport foundation rebase

- Preserved the former Command Center transport and Gate-0 diagnostic work as failed production-reliability evidence/rollback material.
- Audited interrupt ownership and selected NABU-LIB commit `c9cfc6b93290ca77cfa223367810729863cf3c9e` with the `+z80 --no-crt` target, giving NABU-LIB sole ownership of IM2, HCCA, keyboard, buffers, and interrupt-mask state.
- Reproduced the historical source with the isolated official z88dk 2.2 Windows toolchain; Derek's installed z88dk was not modified.
- Recorded the official Remote FS Test binary (SHA-256 `973CBAABEB72900CEBF70467D7B8009E452DA461C38C2393183D2A8A1DE98C78`) as a boot-verified but otherwise INCONCLUSIVE runtime control.
- Built Golden client `NCC-TF-260813-GOLDEN-01`; byte-identical 9,641-byte artifacts have SHA-256 `FA075985BB98DCAE8A4428F12FABD07139A9C4F6627F3622D986A5626CDD4ABD`.
- Ran MAME 0.250 in debugger mode with automatic `go`; one progressive run passed 20, 250, 1,000, and 5,000 bounded 64-byte reads with zero failures or freezes.
- Declared the Golden stack the MAME-verified production transport foundation. Command Center application integration remains separate work above the frozen boundary.
