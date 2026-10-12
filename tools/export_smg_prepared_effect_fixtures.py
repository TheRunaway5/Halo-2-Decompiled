#!/usr/bin/env python3
"""Export literal prepared-effect ABI observations from frozen original SMG traces.

The old effect service returns NONE explicitly. These fixtures compare its
caller, timing and inputs; they do not supply a successful native effect owner.
Original cache/executable bytes stay in ignored local assets/analysis.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def word(value):
    return f"{value & 0xffffffff:08x}"


def state(row):
    keys = ("loaded", "reserve", "magazine_mode", "magazine_ticks",
            "reload_transfer_ticks", "trigger_state", "trigger_ticks",
            "weapon_state")
    return [word(row[k]) for k in keys] + [row[k] for k in
        ("trigger_flags", "weapon_flags", "input_flags")] + [word(v) for v in
        row["hand_timers"]] + [row["barrel_state_hex"], row["seed"]]


def equipped_state(row):
    fields = [word(row[k]) for k in ("loaded", "reserve", "magazine_mode")]
    fields += [word(row[k] & 0xffff) for k in ("magazine_ticks", "reload_transfer_ticks")]
    fields += [word(row["trigger_state"]), word(row["trigger_ticks"] & 0xffff),
               word(row["weapon_state_u32"])]
    fields += [row[k] for k in ("trigger_flags", "weapon_flags", "input_flags")]
    return fields + [word(v) for v in row["unit_hand_timers"]] + [row["barrel_state_hex"], row["shared_seed"]]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if (ROOT / "analysis") not in args.output.resolve().parents:
        raise ValueError("Keep original fixtures in ignored analysis")
    suite_path = ROOT / "analysis/multiplayer/map-pack/smg-firing-suite/suite.json"
    suite = json.loads(suite_path.read_text())
    pins = {str(suite_path.relative_to(ROOT)): sha(suite_path)}
    for name, expected in (("assets/disc/maps/lockout.map", suite["cache_sha256"]),
        ("assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03", suite["xbe_sha256"])):
        assert sha(ROOT / name) == expected
        pins[name] = expected
    lines = []
    effect_count = dry_count = ticks = 0
    excluded = {}
    for case in suite["cases"]:
        path = ROOT / case["report"]
        assert sha(path) == case["sha256"]
        pins[case["report"]] = case["sha256"]
        report = json.loads(path.read_text())
        firing = bytes.fromhex(report["authored"]["firing_effect_entries"][0]["raw_bytes"])
        firing_tags = {f"{struct.unpack_from('<I', firing, offset)[0]:08x}" for offset in (8, 0x18)}
        lines.append(" ".join(["I", f"{case['name']}:mode{case['mode']}",
                               *state(report["initial_state"])]))
        for row in report["rows"]:
            observed = [event for event in report["events"] if
                event.get("tick") == row["tick"] and event.get("service") == "00107b50"]
            effects = [event for event in observed if event["registers"]["eax"] in firing_tags]
            for event in observed:
                if event not in effects:
                    key = event["registers"]["eax"] + ":" + event["phase"]
                    excluded[key] = excluded.get(key, 0) + 1
            requests = [event for event in report["events"] if
                event.get("tick") == row["tick"] and event.get("service") == "000b8a30"]
            fields = ["T", word(row["tick"]), word(row["held"]),
                      word(row["explicit_reload_command"])]
            for phase in ("before", "after", "post_dispatch"):
                fields += state(row[phase])
            fields += [word(len(requests)), word(len(effects))]
            for event in effects:
                assert event["phase"] == "update"
                fields += [event["registers"]["eax"], *event["stack_arguments"],
                           event["return_value"], *state(event["state"])]
                dry_count += event["state"]["loaded"] == 0 and event["state"]["flags"] & 0xc0 == 0xc0
            effect_count += len(effects)
            ticks += 1
            lines.append(" ".join(fields))
    equipped_path=ROOT / "analysis/multiplayer/map-pack/smg-constructor-equipped-source-fixture.json"
    pins[str(equipped_path.relative_to(ROOT))]=sha(equipped_path)
    equipped=json.loads(equipped_path.read_text())
    equipped_cases=equipped_ticks=equipped_effects=0
    for case in equipped["cases"]:
        path=ROOT / "analysis/multiplayer/map-pack/smg-constructor-equipped-suite" / (case["name"]+".json")
        assert sha(path)==case["source_sha256"]
        pins[str(path.relative_to(ROOT))]=sha(path)
        report=json.loads(path.read_text())
        lines.append(" ".join(["J",case["name"],*equipped_state(case["initial"])]))
        equipped_cases+=1
        for row in case["rows"]:
            buttons=struct.unpack_from("<H",bytes.fromhex(row["supplied_input_record_hex"]),0x1c)[0]
            assert buttons in (0,1)
            effects=[event for event in report["weapon_events"] if event.get("tick")==row["tick"] and
                event.get("service")=="00107b50" and event["registers"]["eax"] in firing_tags]
            fields=["P",word(row["tick"]),word(row["supplied_held"]),word(buttons==1),
                *equipped_state(row["phases"]["after_postpass"]),word(len(row["projectile_requests"])),word(len(effects))]
            for event in effects:
                fields += [event["registers"]["eax"],*event["stack_arguments"],event["return_value"]]
            equipped_ticks+=1;equipped_effects+=len(effects)
            lines.append(" ".join(fields))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines) + "\n")
    metadata = dict(source_pins=pins, fixture_sha256=sha(args.output),
        exporter_sha256=sha(Path(__file__)), cases=len(suite["cases"]),
        ticks=ticks, effect_calls=effect_count, dry_calls=dry_count,
        excluded_other_effect_tags=excluded,
        equipped_cases=equipped_cases,equipped_ticks=equipped_ticks,equipped_effect_calls=equipped_effects,
        scope=__doc__, expected_data="Literal original service-call inputs/state and failed return, plus complete original phase states. No native expected output.")
    args.output.with_suffix(".json").write_text(json.dumps(metadata, indent=2) + "\n")
    print(json.dumps({key: metadata[key] for key in ("cases", "ticks", "effect_calls", "dry_calls")}))


if __name__ == "__main__":
    main()
