from pathlib import Path
import re


text = (Path(__file__).resolve().parents[1] / "main.c").read_text(encoding="ascii")


def function(name):
    match = re.search(rf"static (?:void|unsigned char) {name}\([^)]*\)\s*\{{(.*?)\n\}}", text, re.S)
    assert match, name
    return match.group(1)


parser = function("validate_airspace_record")
assert "data[0]!='A' || data[1]!='S' || data[2]!='0' || data[3]!='1'" in parser
assert "AIRSPACE_MAX_AIRCRAFT 3" in text
assert "checksum!=satellite_u16le(data+60)" in parser
assert "count>AIRSPACE_MAX_AIRCRAFT" in parser
assert parser.index("checksum!=satellite_u16le") < parser.index("airspace_sequence=data[4]")
assert parser.index("if(!callsign[i][0] || !icao[i][0]) return 0;") < parser.index("airspace_sequence=data[4]")

refresh = function("refresh_airspace")
assert "received==RECORD_LENGTH" in refresh
assert "validate_airspace_record" in refresh

rail = function("draw_airspace_rail")
assert rail.index("undraw(180,row,247,row)") < rail.index('micro_text(180,43,"STATUS")')
plot = function("draw_adsb_plot")
assert plot.index("clear_airspace_plot();") < plot.index("draw_airspace_terrain();")
assert "for (i=0;i<airspace_count;++i)" in plot
assert "airspace_x[i]" in plot and "airspace_y[i]" in plot
assert "draw_perspective_grid" not in text
terrain = function("draw_airspace_terrain")
assert "circle(" not in terrain
assert "draw(90,53,10,157)" in terrain and "draw(90,53,170,157)" in terrain
assert "draw(90,53,90,157)" in terrain
assert "draw(10,157,170,157)" in terrain
marker = function("draw_aircraft_marker")
assert "draw(x,y+3,x,ground)" in marker and "draw(x-2,ground,x+2,ground)" in marker
assert "AIRSPACE_PLOT_LEFT 8" in text and "AIRSPACE_PLOT_RIGHT 172" in text
assert "AIRSPACE_PLOT_TOP 53" in text and "AIRSPACE_PLOT_BOTTOM 158" in text
for telemetry in ('"ALT"', '"SPD"', '"HDG"'):
    assert telemetry not in plot
labels = function("draw_airspace_labels")
assert "selected_target+order" in labels
assert "airspace_labels_overlap" in labels
assert "if(visible[index])" in labels
declutter = function("airspace_declutter_markers")
assert "selected_target+order" in declutter
assert "delta_x<18 && delta_y<14" in declutter
assert "x<18 || x>162 || y<68 || y>148" in declutter
assert "draw_aircraft_marker(i,x,y,ground)" in plot
for fake in ("NCC101", "LAB204", "SKY317", "TEST42", "SIM550"):
    assert fake not in text
assert 'const char *rail_source="MOCK"' in text  # shared generic renderer remains untouched
assert 'draw_telemetry_rail("TRACK","ALT250","HDG072",5)' not in text

dispatcher = function("service_global_refresh")
assert "refresh_airspace();" in dispatcher
manual = function("request_manual_refresh")
assert "auto_refresh_elapsed" not in manual and "auto_refresh_pending" not in manual
clock = function("service_clock")
assert "AUTO_REFRESH_SECONDS" in clock and "++auto_refresh_pending" in clock

# Mirror the bounded four-candidate label contract with three clustered fixture tracks.
def place(points, selected):
    offsets = ((7, -3), (-42, -3), (-17, -12), (-17, 7))
    placed = {}
    for order in range(len(points)):
        index = (selected + order) % len(points)
        for dx, dy in offsets:
            x, y = points[index][0] + dx, points[index][1] + dy
            if x < 12 or x + 35 > 168 or y < 56 or y + 6 > 155:
                continue
            if any(not (x + 35 < ox or ox + 35 < x or y + 6 < oy or oy + 6 < y)
                   for ox, oy in placed.values()):
                continue
            placed[index] = (x, y)
            break
    return placed

placed = place(((88, 100), (91, 102), (94, 104)), 1)
assert 1 in placed
rectangles = list(placed.values())
for i, (x, y) in enumerate(rectangles):
    for ox, oy in rectangles[i + 1:]:
        assert x + 35 < ox or ox + 35 < x or y + 6 < oy or oy + 6 < y
print("PASS AS01 parser/cache, Airspace renderer, and centralized refresh contracts")
