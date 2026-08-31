from pathlib import Path
import re


SOURCE = Path(__file__).parents[1] / "main.c"
TEXT = SOURCE.read_text(encoding="utf-8")


def body(name: str) -> str:
    signature = f"int {name}(" if name == "main" else f"static void {name}("
    start = TEXT.index(signature)
    brace = TEXT.index("{", start)
    depth = 0
    for pos in range(brace, len(TEXT)):
        if TEXT[pos] == "{":
            depth += 1
        elif TEXT[pos] == "}":
            depth -= 1
            if depth == 0:
                return TEXT[brace + 1 : pos]
    raise AssertionError(f"unterminated {name}")


def test_splash_content_layout_and_startup_reachability():
    draw = body("draw_splash")
    main = body("main")
    for value in (
        "NABU PERSONAL COMPUTER", "// INFORMATION SYSTEM",
        "NABU", "COMMAND", "CENTER", "VERSION 1.0",
        "DEREK LEGER AKA (SUPER_DEREK)", "26 AUG 2026", "PRESS ANY KEY",
    ):
        assert value in draw
    assert main.index("ncc_install_minimal_vdp_isr();") < main.index("run_startup_splash();")
    assert main.index("run_startup_splash();") < main.index("draw_dashboard();")
    assert TEXT.count("run_startup_splash();") == 1


def test_timeout_tune_and_key_consumption_contract():
    run = body("run_startup_splash")
    assert "SPLASH_TIMEOUT_FRAMES 600U" in TEXT
    assert "elapsed<SPLASH_TIMEOUT_FRAMES" in run
    assert "clock_frame_counter" in run
    assert "while(getk()!=0) delay_loop();" in run
    assert run.index("splash_stop();") < run.index("while(getk()!=0) delay_loop();")
    assert "rn_file" not in run and "music_buffer" not in run
    notes = re.search(r"splash_note\[SPLASH_EVENT_COUNT\].*?=\s*\{(.*?)\};", TEXT, re.S).group(1)
    note_values = [int(v) for v in re.findall(r"\d+", notes)]
    assert len(note_values) == 32
    assert all(0 <= v <= 6 for v in note_values)
    assert "note_frames=15" in run and "drum_frames=3" in run
    assert run.index("splash_play(0)") < run.index("while(elapsed<SPLASH_TIMEOUT_FRAMES)")
    assert run.rstrip().endswith("splash_stop();")
    stop = body("splash_stop")
    assert "set_psg(8,0)" in stop and "set_psg(9,0)" in stop
    assert "set_psg(0,0)" in stop and "set_psg(1,0)" in stop and "set_psg(6,0)" in stop


def test_no_key_and_early_key_timing_model():
    timeout = 600

    def exit_frame(key_frame=None):
        elapsed = 0
        while elapsed < timeout:
            if key_frame is not None and elapsed >= key_frame:
                return elapsed, True
            elapsed += 1
        return elapsed, False

    assert exit_frame() == (600, False)
    assert exit_frame(0) == (0, True)
    assert exit_frame(240) == (240, True)
    assert exit_frame(599) == (599, True)


def test_splash_geometry_stays_inside_256_by_192():
    draw = body("draw_splash")
    assert "micro_hline(3,252,3)" in draw
    assert "micro_hline(3,252,188)" in draw
    assert "micro_vline(3,3,188)" in draw
    assert "micro_vline(252,3,188)" in draw
    assert "micro_text(89,181,\"PRESS ANY KEY\")" in draw
    assert "vdp_set_mode(mode_2)" in draw and "clg();" in draw
