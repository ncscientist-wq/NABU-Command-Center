from pathlib import Path


TEXT = (Path(__file__).parents[1] / "main.c").read_text(encoding="utf-8")


def weather_body() -> str:
    start = TEXT.index("static void draw_weather_detail(void)")
    brace = TEXT.index("{", start)
    depth = 0
    for pos in range(brace, len(TEXT)):
        if TEXT[pos] == "{":
            depth += 1
        elif TEXT[pos] == "}":
            depth -= 1
            if depth == 0:
                return TEXT[brace + 1 : pos]
    raise AssertionError("unterminated draw_weather_detail")


def bounded_weather_format(place: str, temp: str, condition: str):
    title = list("WEATHER " + "\0" * 20)
    metric1 = ["\0"] * 13
    i = 0
    j = 8
    while i < 19 and j < len(title) - 1 and place[i:i + 1]:
        title[j] = place[i]
        i += 1
        j += 1
    title[j] = "\0"

    i = j = 0
    while i < 3 and j < len(metric1) - 1 and temp[i:i + 1]:
        metric1[j] = temp[i]
        i += 1
        j += 1
    if j < len(metric1) - 1:
        metric1[j] = "F"
        j += 1
    if j < len(metric1) - 1:
        metric1[j] = " "
        j += 1
    i = 0
    while i < 8 and j < len(metric1) - 1 and condition[i:i + 1]:
        metric1[j] = condition[i]
        i += 1
        j += 1
    metric1[j] = "\0"
    return "".join(title).split("\0", 1)[0], "".join(metric1).split("\0", 1)[0], j


def test_gilbert_live_weather_max_width_is_bounded_and_terminated():
    body = weather_body()
    assert body.index("i=0; j=0;") < body.index("weather_temp[i]")
    assert "i<3 && j<sizeof(metric1)-1 && weather_temp[i]" in body
    assert "i<8 && j<sizeof(metric1)-1 && weather_condition[i]" in body
    assert "i<19 && j<sizeof(title)-1 && place[i]" in body
    assert "metric1[j]=0;" in body and "title[j]=0;" in body

    title, metric1, terminator = bounded_weather_format(
        "GILBERT AZ", "999", "OVERCAST"
    )
    assert title == "WEATHER GILBERT AZ"
    assert metric1 == "999F OVERCAS"
    assert terminator == 12


def test_metric2_fixed_fields_fit_declared_capacity():
    body = weather_body()
    assert "char metric1[13], metric2[13], title[28];" in body
    assert "metric2[10]=0;" in body
    assert "weather_wind[2]" in body and "weather_pressure[3]" in body
