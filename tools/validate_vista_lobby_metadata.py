#!/usr/bin/env python3
"""Validate an opt-in PC lobby export against pinned original-source evidence."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

def sha(data):
    return hashlib.sha256(data).hexdigest()


def validate(source, output, fixture):
    data = source.read_bytes()
    expected = json.loads(fixture.read_text())
    actual = json.loads((output / "lobby-metadata.json").read_text())
    checks = 0

    def check(value, why):
        nonlocal checks
        checks += 1
        if not value:
            raise ValueError(why)

    def record(a, e):
        check(a["file_offset"] == e["file_offset"] and a["bytes"] == e["size"] and a["sha256"] == e["sha256"], "pinned original raw record")

    def tag(a, e):
        check(int(a["datum"], 16) == int(e["datum"], 16) and a["group"] == e["group"] and a["name"] == e["name"], "source-local tag identity")
        record(a["metadata"], e["metadata"])

    def raw_records(value):
        if isinstance(value, dict):
            if "raw_hex" in value:
                raw = bytes.fromhex(value["raw_hex"])
                p, n = value["file_offset"], value["bytes"]
                check(0 <= p <= len(data) and n <= len(data) - p, "source metadata range")
                check(len(raw) == n and raw == data[p:p + n] and sha(raw) == value["sha256"], "complete unchanged source metadata bytes")
            for child in value.values():
                raw_records(child)
        elif isinstance(value, list):
            for child in value:
                raw_records(child)

    check(expected["schema"] == "H2V_LOBBY_METADATA_SOURCE_V1", "source evidence schema")
    check(actual["schema"] == "halo2-vista-lobby-metadata-v1" and actual["complete"], "sidecar schema")
    check(sha(data) == actual["source_map"]["sha256"] == expected["source_map_sha256"], "exact original map pin")
    raw_records(actual)
    maps = expected["maps"]
    tag(actual["globals"], maps["owner"])
    for domain in ("campaign", "multiplayer"):
        check(len(actual[domain]) == len(maps[domain]), "all ordered raw map slots")
        for a, e in zip(actual[domain], maps[domain]):
            record(a["source"], e["metadata"])
            check(a["map_id"] == e["map_id"] and a["bitmap"] == int(e["preview"]["datum"], 16), "map ID and actual preview datum")
            check(a["english_name"] == e["name"] and a["english_description"] == e["description"] and a["scenario_path"] == (e["scenario_path"] or ""), "map labels/path")
            if domain == "campaign":
                check(a["campaign_id"] == e["campaign_id"], "campaign ID")
            else:
                check(all(a[k] == e[k] for k in ("sort_order", "flags", "team_counts")), "MP authored fields")
    check(len(actual["paths"]) == len(maps["campaign_paths"]), "campaign path count")
    for a, e in zip(actual["paths"], maps["campaign_paths"]):
        record(a["source"], e["metadata"])
        check(all(a[k] == e[k] for k in ("campaign_id", "map_id", "scenario_path")), "campaign path pair")
    rules = expected["rules"]
    tag(actual["ui_globals"], rules["wgtz"]["owner"])
    tag(actual["scenario"], rules["wgtz"]["scenario"])
    catalogue = rules["catalogues"][0]
    tag(actual["game_settings"], catalogue["owner"])
    check(actual["scenario_type"] == 2 and actual["shared_ui_globals"] == int(rules["wgtz"]["shared_globals"]["datum"], 16), "scenario-selected globals")
    check(actual["quick_settings_screen"] == (0xffffffff if catalogue["quick_screen"] is None else int(catalogue["quick_screen"]["datum"], 16)) and actual["variant_locale"] == int(catalogue["locale"], 16), "goof owner references")
    check(len(actual["groups"]) == len(catalogue["groups"]), "full source group count")
    for a, e in zip(actual["groups"], catalogue["groups"]):
        record(a["source"], e["metadata"])
        check(a["key"] == e["key"] and len(a["parameters"]) == len(e["parameters"]), "category key/order")
        for p, q in zip(a["parameters"], e["parameters"]):
            record(p["source"], q["reference"])
            check(p["datum"] == int(q["datum"], 16), "category source reference order")
    check(len(actual["variants"]) == len(catalogue["variants"]), "raw authored variant count")
    for a, e in zip(actual["variants"], catalogue["variants"]):
        record(a["source"], e["metadata"])
        check(a["name"] == int(e["name_sid"], 16) and a["engine"] == e["engine"] and len(a["values"]) == len(e["values"]), "variant source fields")
        for p, q in zip(a["values"], e["values"]):
            record(p["source"], q["metadata"])
            check(p["parameter"] == q["parameter"] and p["value"] == q["value"], "variant ordered values")
    check(len(actual["parameters"]) == len(rules["parameters"]), "complete sily bank")
    for a in actual["parameters"]:
        e = rules["parameters"][a["owner"]["datum"].upper()]
        tag(a["owner"], e["owner"])
        check(a["parameter"] == e["parameter"] and a["default_value"] == e["default_value"], "sily parameter/default")
        check(all(a[k] == int(e[q], 16) for k, q in (("locale", "locale"), ("title", "title_sid"), ("header", "header_sid"), ("description", "description_sid"))), "sily locale/SIDs")
        check(len(a["choices"]) == len(e["choices"]), "ordered sily choice count")
        for p, q in zip(a["choices"], e["choices"]):
            record(p["source"], q["metadata"])
            check(p["value"] == q["value"] and p["string"] == int(q["string"], 16), "sily choice source value/SID")
    locales = {int(v["owner"]["datum"], 16): v for v in actual["locales"]}
    for key, e in rules["locales"].items():
        a = locales[int(key, 16)]; tag(a["owner"], e["owner"])
        check(a["first"] == e["first"] and a["count"] == e["count"], "unic source range")
        for row in e["entries"]:
            v = actual["locale_entries"][row["ordinal"]]
            check(v["string"] == int(row["string"], 16) and v["text"] == row["text"] and v["utf16le_hex"] == row["text"].encode("utf-16-le").hex(), "source UTF8/UTF16 locale entry")
    # Every extra retained owner is checked directly against its serialized
    # range; only the 27 independently audited consumed ranges have labels above.
    for a in actual["locales"]:
        raw = bytes.fromhex(a["owner"]["metadata"]["raw_hex"])
        check((a["first"], a["count"]) == struct.unpack_from("<HH", raw, 16), "all source unic range fields")
        check(a["first"] + a["count"] <= len(actual["locale_entries"]), "all source unic range bounds")
    screens = {int(v["owner"]["datum"], 16): v for v in actual["screens"]}
    for e in rules["value_blocks"]:
        a = screens[int(e["owner"]["datum"], 16)]; tag(a["owner"], e["owner"])
        check(a["id"] == e["screen"] and a["locale"] == int(e["locale"], 16) and len(a["values"]) == len(e["blocks"]), "wgit locale/value owner")
        for p, q in zip(a["values"], e["blocks"]):
            record(p["source"], q["metadata"])
            check(p["name"] == int(q["name_sid"], 16) and len(p["strings"]) == len(q["values"]), "wgit ordered value group")
            for v, w in zip(p["strings"], q["values"]):
                record(v["source"], w["metadata"])
                check(v["string"] == int(w["string"], 16), "wgit value SID order")
    check(len(actual["previews"]) == len(expected["previews"]) == len(actual["preview_datums"]), "complete preview bank")
    decoded = 0
    for a in actual["previews"]:
        e = expected["previews"][a["owner"]["datum"].upper()]; tag(a["owner"], e["owner"])
        check(len(a["frames"]) == len(e["images"]), "preview frame count")
        for p, q in zip(a["frames"], e["images"]):
            record(p["source"], q["metadata"])
            check(all(p[k] == q[k] for k in ("width", "height", "format", "additional_mips")), "preview source shape/format/mips")
            check(p["expanded_sha256"] == q["pixels_sha256"] and p["expanded_bytes"] == q["expanded_bytes"] and p["compressed_sha256"] == q["compressed_sha256"], "preview exact expanded bytes")
            relative = Path(p["dds"])
            check(not relative.is_absolute() and ".." not in relative.parts, "safe preview asset path")
            dds = (output / relative).read_bytes()
            check(dds[:4] == b"DDS " and len(dds) == p["dds_bytes"] and sha(dds) == p["dds_sha256"], "emitted preview DDS readback")
            check(len(dds[128:]) == q["expanded_bytes"] and sha(dds[128:]) == q["pixels_sha256"], "emitted DDS source payload")
            decoded += len(dds) - 128
    return dict(complete=True, checks=checks, source_map_sha256=sha(data),
                fixture_sha256=sha(fixture.read_bytes()), sidecar_sha256=sha((output / "lobby-metadata.json").read_bytes()),
                preview_banks=len(actual["previews"]), preview_payload_bytes=decoded,
                campaign_records=len(actual["campaign"]), multiplayer_records=len(actual["multiplayer"]),
                parameters=len(actual["parameters"]), groups=len(actual["groups"]), variants=len(actual["variants"]),
                locales=len(actual["locales"]), screens=len(actual["screens"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path, help="new opt-in export directory to validate")
    parser.add_argument("--fixture", required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(validate(args.map, args.output, args.fixture), indent=2))


if __name__ == "__main__":
    main()
