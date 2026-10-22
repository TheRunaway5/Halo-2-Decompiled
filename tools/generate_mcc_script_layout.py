#!/usr/bin/env python3
"""Generate runtime facts from the independently audited executable profile."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def generated(profile):
    rows = profile["builtins"]
    aliases = []
    if len({r["mcc_opcode"] for r in rows}) != len(rows):
        raise ValueError("duplicate MCC builtin opcode")
    lines = ["// Generated from tools/mcc-script-layout.json.",
             "// MCC SHA256 " + profile["mcc_sha256"],
             "// Xbox TU1.5 SHA256 " + profile["xbox_tu15_sha256"],
             f"constexpr std::array<Builtin,{len(rows)}> builtins{{{{"]
    for row in sorted(rows, key=lambda r: r["mcc_opcode"]):
        target = row["xbox_opcode"]
        if target is not None and (row["return_type"] != row["xbox_return_type"] or
                                   row["arguments"] != row["xbox_arguments"]):
            raise ValueError("incompatible builtin signature: " + row["name"])
        if "semantic_alias" in row:
            # One independently verified rename, not a general name alias rule.
            if (row["mcc_opcode"], target, row["name"], row.get("xbox_name"),
                row["return_type"], row["arguments"]) != (
                    0x295, 0x28d, "player_effect_set_max_vibration",
                    "player_effect_set_max_rumble", 4, [6, 6]):
                raise ValueError("unverified semantic alias")
            evidence = row["semantic_alias"]
            if (evidence.get("kind"), evidence.get("mcc_setter"),
                evidence.get("xbox_evaluator"), evidence.get("xbox_owner_pointer"),
                evidence.get("owner_offsets")) != (
                    "authenticated_player_effect_rename", "0x18073b9d0",
                    "0x2b4a40", "0x50ebf8", ["0x220", "0x224"]):
                raise ValueError("semantic alias lost its authenticated owner evidence")
            aliases.append(row)
        elif row.get("xbox_name", row["name"]) != row["name"]:
            raise ValueError("changed builtin name lacks verified semantic ownership")
        if len(row["arguments"]) > 7:
            raise ValueError("builtin argument extent exceeds compiled schema")
        args = ",".join(map(str, row["arguments"]))
        lines.append(f'    {{{row["mcc_opcode"]},{target if target is not None else 65535},'
                     f'{row["return_type"]},{len(row["arguments"])},{row["xbox_flags"]},'
                     f'{json.dumps(row["name"])},{{{args}}}}},')
    lines += ["}};", f"constexpr std::array<SemanticAlias,{len(aliases)}> semantic_aliases{{{{"]
    for row in aliases:
        lines.append(f'    {{{row["mcc_opcode"]},{row["xbox_opcode"]},'
                     f'{json.dumps(row["name"])},{json.dumps(row["xbox_name"])}}},')
    globals_ = profile["external_globals"]
    if len({g["mcc_index"] for g in globals_}) != len(globals_):
        raise ValueError("duplicate MCC external global index")
    lines += ["}};", f"constexpr std::array<ExternalGlobal,{len(globals_)}> external_globals{{{{"]
    for row in sorted(globals_, key=lambda r: r["mcc_index"]):
        if row["mcc_type"] != row["xbox_type"]:
            raise ValueError("incompatible external global type: " + row["name"])
        lines.append(f'    {{{row["mcc_index"]},{row["xbox_index"]},{row["mcc_type"]},'
                     f'{json.dumps(row["name"])}}},')
    return "\n".join(lines + ["}};"]) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    options = parser.parse_args()
    result = generated(json.loads((ROOT / "tools/mcc-script-layout.json").read_text()))
    output = ROOT / "src/campaign/scripting/mcc_script_layout.inc"
    if options.check:
        if output.read_text() != result:
            raise SystemExit("compiled MCC script profile is stale")
    else:
        output.write_text(result)


if __name__ == "__main__":
    main()
