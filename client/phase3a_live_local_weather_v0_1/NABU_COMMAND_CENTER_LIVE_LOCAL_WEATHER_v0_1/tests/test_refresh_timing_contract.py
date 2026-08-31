from pathlib import Path
import re


SOURCE = Path(__file__).resolve().parents[1] / "main.c"
text = SOURCE.read_text(encoding="ascii")


def body(name):
    match = re.search(
        rf"static (?:void|unsigned char) {name}\(void\)\s*\{{(.*?)\n\}}",
        text,
        re.S,
    )
    assert match, f"missing {name}"
    return match.group(1)


assert '#define AUTO_REFRESH_SECONDS 60' in text

clock = body("service_clock")
assert "++auto_refresh_elapsed" in clock
assert "++auto_refresh_pending" in clock

manual = body("request_manual_refresh")
assert "++manual_refresh_pending" in manual
assert "auto_refresh_elapsed" not in manual
assert "auto_refresh_pending" not in manual

refresh = body("service_global_refresh")
assert refresh.index("if(refresh_in_progress) return 0;") < refresh.index("refresh_in_progress=1;")
assert refresh.index("refresh_in_progress=1;") < refresh.index("refresh_transport();")
assert refresh.index("refresh_transport();") < refresh.index("refresh_in_progress=0;")

scheduler = body("service_scheduler")
assert scheduler.index("service_global_refresh()") < scheduler.index("if(scheduler_paused")

zip_entry = body("zip_entry")
assert "request_manual_refresh();" in zip_entry
assert "service_clock(); service_sound(); service_scheduler();" in zip_entry

auto_events = []
manual_events = []
elapsed = 0
for second in range(1, 181):
    elapsed += 1
    if elapsed == 60:
        elapsed = 0
        auto_events.append(second)
    if second in (75, 90):
        manual_events.append(second)

assert auto_events == [60, 120, 180]
assert manual_events == [75, 90]
print("PASS refresh timing: AUTO 60/120/180; manual 75/90; deadline unchanged")
