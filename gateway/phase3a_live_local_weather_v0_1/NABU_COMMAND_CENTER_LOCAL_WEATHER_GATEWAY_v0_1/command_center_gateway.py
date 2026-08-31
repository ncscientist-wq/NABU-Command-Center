#!/usr/bin/env python3
"""Single local supervisor for Command Center weather and NIST time resources."""

from __future__ import annotations

import argparse
import configparser
import logging
from pathlib import Path
import threading

import nist_time_gateway
import weather_gateway
import earthquake_gateway
import space_weather_gateway
import satellite_gateway
import airspace_gateway
import weather_alert_gateway
import music_gateway


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, default=Path("command_center.ini"))
    args = parser.parse_args()
    config = configparser.ConfigParser()
    if not config.read(args.config, encoding="utf-8"):
        raise FileNotFoundError(args.config)
    section = config["command_center"]
    stop = threading.Event()
    log_path = Path(section.get("time_log", "logs/command_center_time.log"))
    log_path.parent.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(level=logging.INFO, format="%(asctime)sZ %(levelname)s %(message)s",
                        handlers=[logging.FileHandler(log_path, encoding="utf-8"), logging.StreamHandler()])
    time_logger = logging.getLogger("ncc_time_maintenance")
    weather_config = Path(section.get("weather_config", "weather.ini"))
    store_path = Path(section.get("store_path", r"D:\NABU Internet Adapter\Store"))

    weather_thread = threading.Thread(
        target=weather_gateway.run, args=(weather_config, False, stop),
        name="ncc-weather", daemon=True,
    )
    time_thread = threading.Thread(
        target=nist_time_gateway.maintain_time,
        args=(store_path, section.get("nist_hostname", nist_time_gateway.NIST_HOST),
              section.getfloat("nist_timeout_seconds", 5.0),
              section.getfloat("nist_resync_seconds", 3600.0),
              section.getfloat("time_publish_seconds", 3600.0),
              section.getfloat("nist_max_holdover_seconds", 7200.0),
              time_logger, stop),
        name="ncc-time", daemon=True,
    )
    earthquake_thread = threading.Thread(
        target=earthquake_gateway.run, args=(weather_config, stop),
        name="ncc-earthquake", daemon=True,
    )
    space_weather_thread = threading.Thread(
        target=space_weather_gateway.run, args=(weather_config, stop),
        name="ncc-space-weather", daemon=True,
    )
    satellite_thread = threading.Thread(
        target=satellite_gateway.run,
        args=(store_path, stop),
        name="ncc-satellite",
        daemon=True,
    )
    local_adsb_url = section.get("local_adsb_url", "").strip() or None
    airspace_thread = threading.Thread(
        target=airspace_gateway.run,
        args=(store_path, weather_config, stop, local_adsb_url),
        name="ncc-airspace",
        daemon=True,
    )
    alert_thread = threading.Thread(
        target=weather_alert_gateway.run, args=(weather_config, stop),
        name="ncc-weather-alert", daemon=True,
    )
    music_thread = threading.Thread(
        target=music_gateway.run, args=(store_path, stop),
        name="ncc-music", daemon=True,
    )
    print("NABU Command Center integrated gateway", flush=True)
    print(f"Store path = {store_path}", flush=True)
    print("Weather, Alerts, Earthquake, Space Weather, Music, and NIST time maintenance starting", flush=True)
    weather_thread.start()
    time_thread.start()
    earthquake_thread.start()
    space_weather_thread.start()
    satellite_thread.start()
    airspace_thread.start()
    alert_thread.start()
    music_thread.start()
    try:
        while weather_thread.is_alive() and time_thread.is_alive() and earthquake_thread.is_alive() and space_weather_thread.is_alive() and satellite_thread.is_alive() and airspace_thread.is_alive() and alert_thread.is_alive() and music_thread.is_alive():
            stop.wait(0.5)
    except KeyboardInterrupt:
        print("Stopping Command Center gateway...", flush=True)
    finally:
        stop.set()
        weather_thread.join(2.0)
        time_thread.join(2.0)
        earthquake_thread.join(2.0)
        space_weather_thread.join(2.0)
        satellite_thread.join(2.0)
        airspace_thread.join(2.0)
        alert_thread.join(2.0)
        music_thread.join(2.0)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
