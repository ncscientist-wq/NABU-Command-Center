# Network API Provenance

## Installed z88dk API evidence

The declarations below were directly inspected in installed `arch/nabu/retronet.h`; implementations were directly inspected under installed `libsrc/target/nabu/retronet`. Handle type is `uint8_t`.

| Function | Declaration/verified behavior | Implementation evidence |
|---|---|---|
| `rn_fileOpen` | `uint8_t rn_fileOpen(uint8_t filenameLen, char *filename, uint16_t fileFlag, uint8_t fileHandle) __smallc;` Modes: read-only `0`, read/write `1`; `0xff` requests automatic assignment. Exact failure value UNVERIFIED. | Command `0xA3`; `rn_fileOpen.c` SHA-256 `7DED0250C1DD1702927226742A89179D660AF1E75C487193575818289D88B5CE` |
| `rn_fileHandleClose` | `void rn_fileHandleClose(uint8_t fileHandle) __smallc;` Releases handle and waits for write completion; no result value. | Command `0xA7`; SHA-256 `DFEB32A04AC785A13281CB762103722178E116C1EB191086A4D1E30725CD9F84` |
| `rn_fileHandleSize` | `int32_t rn_fileHandleSize(uint8_t fileHandle) __smallc;` URL may return `-1` when not downloaded; local missing-file behavior requires care. | Command `0xA4`; SHA-256 `58D554998BBB8F8326DBAA510814A94DB9B1FB9C9C6AC689542A515028A5CC42` |
| `rn_fileHandleRead` | `uint16_t rn_fileHandleRead(uint8_t fileHandle, uint8_t *buffer, uint16_t bufferOffset, uint32_t readOffset, uint16_t readLength) __smallc;` Returns bytes read; zero is error or EOF. Wrapper trusts returned count. | Command `0xA5`; SHA-256 `1CBA04F4E6277CD883AEA48697AE0A3BCB5438DBF85898CB8C7589CF71ED3E2E` |
| `rn_fileHandleReadSeq` | `uint16_t rn_fileHandleReadSeq(uint8_t fileHandle, uint8_t *buffer, uint16_t bufferOffset, uint16_t readLength) __smallc;` Uses server-maintained position; short/zero return does not fully distinguish error from EOF. | Command `0xB5`; SHA-256 `45D93CEF2CC2CA16E2DE07DD86F7052DAB975076E1B80F974BA2796EBCABC93F` |
| `rn_fileHandleSeek` | `uint32_t rn_fileHandleSeek(uint8_t fileHandle, int32_t offset, uint8_t seekOption) __smallc;` `SET=1`, `CUR=2`, `END=3`; result clamped to bounds. Error sentinel UNVERIFIED. | Command `0xB6`; SHA-256 `2C695CD1D96B9A90EF99104D80F3AD7E41DAC6A4B272DD1916D5C24153F54926` |

The linked symbols are present in installed `nabu_int.lib` (SHA-256 `E0D10821182A921AFE78E798313A71B9364ED7ABEBEF68BA168D010BDC8B8D39`). The wrappers use HCCA block operations; no inspected wrapper supplies timeout or cancellation semantics.

## Adapter provenance

- Running identity supplied from owner-observed Windows process evidence:
  - path: `C:\Program Files (x86)\DJ Sures\NABU Internet Adapter\NABU-Internet-Adapter-84.exe`
  - version: `2025.02.07.00`
  - observed PID: `17636`
- Direct installed-file verification:
  - company: `DJ Sures`
  - file/product version: `2025.02.07.00`
  - SHA-256: `A3B845ED87A1E762E9FBC068F9D842958CC79F1C29FCFCD2E0799367CC8AF248`
- Version-adjacent `README.TXT` SHA-256: `0D4E798BC05EB041A96551321518C31F9F12F390D778037FE1087B7B208B3380`.

The installed README contains no RetroNET file-command list, protocol/API version, z88dk compatibility matrix, or behavior contract for the six audited APIs. Its reference to a TCP RetroNET Server is specifically for CP/M Redirect and does not prove file-command compatibility. Consequently:

- running Adapter identity/version: **VERIFIED**
- compatibility with all six installed z88dk APIs: **UNVERIFIED**
- compilation/linking in this project: **NOT TESTED**
- MAME runtime: **NOT TESTED**
- physical NABU runtime: **NOT TESTED**

## Phase T1 call binding

The bounded proof binds the installed declarations as follows:

```c
rn_fileOpen(12, "ncc_test.dat", OPEN_FILE_FLAG_READONLY, 0xff);
rn_fileHandleRead(file_handle, record_buffer, 0, 0, 64);
rn_fileHandleClose(file_handle);
```

The filename-without-drive form maps to the configured Store root according to
the installed header. Compilation and linking against installed `nabu_int.lib`
passed. Runtime command compatibility remains **NOT TESTED**.

## Phase T1-02 runtime provenance update

Owner-observed MAME 0.250 runtime with Adapter `2025.02.07.00` verified the
exact bounded `rn_fileOpen` / `rn_fileHandleRead` / `rn_fileHandleClose`
sequence and one live Store replacement followed by a successful R refresh.
Compilation/linking and these three calls are therefore verified for this T1
scope. Other audited APIs and adverse/recovery semantics remain unverified.
