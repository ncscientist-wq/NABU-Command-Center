from pathlib import Path
import re


root = Path(__file__).resolve().parents[1]
text = (root / "main.c").read_text(encoding="ascii")
map_text = (root / "000001.map").read_text(encoding="ascii", errors="replace")


def body(name):
    match = re.search(rf"static (?:void|unsigned char) {name}\([^)]*\)\s*\{{(.*?)\n\}}", text, re.S)
    assert match, name
    return match.group(1)


# Page 1: every value is an existing application state/event/counter.
live = body("draw_task_live_values")
for binding in ("current_draw_stage", "last_draw_stage", "dirty_mask", "full_draw_count",
                "dirty_draw_count", "static_draw_count", "dynamic_draw_count",
                "frame3d_count", "globe_step_count", "news_step_count"):
    assert binding in live
assert "APP RUNTIME LIVE" in text
assert "SAT FRAME" in text and "MODULE STEP" in text
assert "last_draw_stage=current_draw_stage" in body("draw_stage")
assert "count_inc(&frame3d_count)" in text
assert "count_inc(&globe_step_count)" in text
assert "count_inc(&news_step_count)" in text

# Page 2: constants, reproducible calculations, current MAP value, and explicit unavailable state.
hardware = body("draw_task_hardware")
for fact in ("Z80A", "3.58MHZ", "64K", "16K", "TMS9918A", "AY-3-8910"):
    assert fact in hardware
assert "RAM GAP" not in text
assert 'task_label(10,139,"LINK GAP")' in hardware
assert 'micro_text(76,139,"UNAVAILABLE")' in hardware
assert '"SEE MAP"' not in hardware
assert '"UNAVAILABLE"' in hardware
assert "sizeof(record_buffer)+sizeof(parse_buffer)" in hardware
assert "10*7+26*7+9*7" in hardware
assert "13056UL" in hardware
data_size = int(re.search(r"__data_compiler_size\s*= \$([0-9A-F]+)", map_text).group(1), 16)
bss_size = int(re.search(r"__bss_compiler_size\s*= \$([0-9A-F]+)", map_text).group(1), 16)
assert data_size + bss_size == 1104
assert 'task_label(10,103,"MAP D+B")' in hardware
assert 'task_value_text(88,103,"1104 B")' in hardware

# Page 3 and rail: unsupported geometry is N/A; sequence/status scopes are exact.
internals = body("draw_task_internals")
assert internals.count('"N/A"') == 2
rail = text[text.index("static void draw_task_rail"):text.index("static void draw_task_live_values")]
assert '"SCHED"' in rail and '"TIME SEQ"' in rail and '"T/W BYTES"' in rail
assert "has_last_valid" in rail and '"N/A"' in rail
for stale in ("1241 B", "212 B", "148 B", "325 B", "27982 CALC"):
    assert stale not in text
diagnostics = body("draw_diagnostics")
assert "P3A-01AR1" not in diagnostics
assert "BUILD_ID" in diagnostics

print("PASS Task Manager three-page truthfulness contract")
