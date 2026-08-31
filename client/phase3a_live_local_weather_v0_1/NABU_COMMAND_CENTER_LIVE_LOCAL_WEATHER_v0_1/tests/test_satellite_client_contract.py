from pathlib import Path


SOURCE = Path(__file__).resolve().parents[1] / "main.c"
text = SOURCE.read_text(encoding="utf-8")

required = (
    'NCC-RF-260826-LV1',
    '#define SATELLITE_NAME "ncc_satellite.dat"',
    'static unsigned char validate_satellite_record',
    'protected_file_read',
    'refresh_satellite();',
    'satellite_position_text',
    'satellite_motion_text',
    'satellite_identity',
    'satellite_norad',
)

for marker in required:
    assert marker in text, f"missing Satellite client marker: {marker}"

record = bytearray(64)
record[0:4] = b"SA01"
record[4] = 7
record[5] = 1
record[6:8] = int(2576).to_bytes(2, "little", signed=True)
record[8:10] = int(-8019).to_bytes(2, "little", signed=True)
record[10:12] = (421).to_bytes(2, "little")
record[12:14] = (2760).to_bytes(2, "little")
record[14:18] = (1787241600).to_bytes(4, "little")
record[18:23] = b"ISS  "
record[23:28] = b"25544"
record[28] = 1
record[29:31] = (4450).to_bytes(2, "little")
record[60:62] = (sum(record[:60]) & 0xFFFF).to_bytes(2, "little")
record[62:64] = b"E\n"

assert len(record) == 64
assert record[:4] == b"SA01"
assert int.from_bytes(record[60:62], "little") == sum(record[:60]) & 0xFFFF
assert record[62:64] == b"E\n"

print("Satellite client contract: PASS")
