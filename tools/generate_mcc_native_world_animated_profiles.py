#!/usr/bin/env python3
"""Validate and compile the three finite clock-only material data identities."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIELDS = ((0, 10), (8, 2), (16, 10), (24, 6), (32, 4))


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()


def generate(raw):
    value = json.loads(raw)
    if value["schema"] != "H2_MCC_NATIVE_WORLD_ANIMATED_PROFILES_V1" or len(value["profiles"]) != 3:
        raise ValueError("Changed finite clock-only profile census")
    owners, projected = set(), []
    rules = (("transparent", "plasma_alpha", "dune", 5),
             ("transparent", "two_alpha_clouds", "dune", 5),
             ("water", "water_static", "zanzibar", 65537))
    for profile, (category, family, native_map, flags) in zip(value["profiles"], rules):
        name = "shaders\\shader_templates\\" + category + "\\" + family
        if (profile["template"] != name or profile["native_source_map"] != native_map or
                profile["native_record"]["group"] != "stem" or profile["native_record"]["name"] != name or
                profile["shader_type_flags"] != flags):
            raise ValueError("Unreviewed clock-only template/source")
        graph = profile["source_graph"]
        if hashlib.sha256(canonical(graph)).hexdigest() != profile["source_graph_sha256"]:
            raise ValueError("Changed literal complete MCC remap graph")
        if len(bytes.fromhex(graph["root_hex"])) != 96 or len(graph["definitions"]) != 3:
            raise ValueError("Changed literal template root")
        for definition in graph["definitions"]:
            if len(bytes.fromhex(definition["definition_hex"])) != 40 or len(definition["arrays"]) != 5:
                raise ValueError("Changed literal platform definition")
            for array, (field, stride) in zip(definition["arrays"], FIELDS):
                if (array["field"] != field or not 0 <= array["count"] <= 65536 or
                        len(bytes.fromhex(array["raw_hex"])) != array["count"] * stride or
                        len(array["typed_references"]) != (array["count"] if field == 16 else 0) or
                        any(len(ref) != 2 or ref[0] != "spas" for ref in array["typed_references"])):
                    raise ValueError("Changed complete typed compiled-remap bank")
        count = 20 if family == "plasma_alpha" else 10
        if len(profile["clock_overlays"]) != count or profile["source_counts"]["60"] != count:
            raise ValueError("Changed finite overlay role count")
        for i, overlay in enumerate(profile["clock_overlays"]):
            if family == "plasma_alpha":
                periods = [1090519040] if i == 0 else [1086324736] if i == 10 else [1065353216]
            elif family == "two_alpha_clouds":
                periods = [1101004800, 1103626240, 1106247680] if i == 0 else [1065353216]
            else:
                periods = [1056964608, 1062836634] if i == 0 else [1065353216]
            if (overlay["input"] != 0 or overlay["range"] != 0 or overlay["period_words"] != periods or
                    not 0 < len(bytes.fromhex(overlay["curve_hex"])) <= 4096):
                raise ValueError("Unreviewed clock input, period or curve extent")
            if family == "water_static" and i in (1, 3):
                variants = ["01000000cdcc4c3e0000803f00000000000000000000803f0000803f",
                            "01000000cdcccc3d0000803f00000000000000000000803f0000803f"]
                if overlay.get("curve_hex_variants") != variants or overlay["curve_hex"] != variants[0]:
                    raise ValueError("Changed exact Zanzibar/Uplift authored water curve variants")
            elif "curve_hex_variants" in overlay:
                raise ValueError("Curve variants outside the two authenticated water roles")
        for field, stride in ((52, 10), (68, 4), (76, 2), (84, 4), (92, 4)):
            if len(bytes.fromhex(profile["native_role_banks"][str(field)])) != profile["source_counts"][str(field)] * stride:
                raise ValueError("Changed complete authenticated animation role bank")
        if not profile["paired_proof"]["exact_required_role_banks"] or not profile["paired_proof"]["exact_required_curves"]:
            raise ValueError("Missing original role/curve proof")
        for owner in profile["needed_PC_owners"]:
            key = owner["map"], owner["datum"]
            if key in owners or owner["map"] != "needle":
                raise ValueError("Unknown or repeated required animated owner")
            owners.add(key)
        projected.append({k: profile[k] for k in ("template", "source_graph", "source_counts", "shader_type_flags", "runtime_diffuse_slot", "native_shape", "clock_overlays", "native_role_banks")})
        projected[-1]["native_record"] = {k: profile["native_record"][k] for k in ("payload", "bytes", "fnv64")}
    if len(owners) != 8:
        raise ValueError("Changed complete eight-owner source proof")
    data = canonical({"schema": value["schema"], "profiles": projected}).decode()
    return ("// Generated by tools/generate_mcc_native_world_animated_profiles.py; do not edit.\n"
            "// Paired source data SHA256: " + hashlib.sha256(raw).hexdigest() + "\n"
            'constexpr std::string_view native_world_animated_profiles=R"H2CLOCK(' + data + ')H2CLOCK";\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    text = generate((ROOT / "tools/data/mcc-native-world-animated-profiles.json").read_bytes())
    output = ROOT / "src/rendering/mcc_native_world_animated_profiles.inc"
    if args.check:
        if not output.is_file() or output.read_text() != text:
            raise ValueError("Stale finite animated material registry")
    else:
        output.write_text(text)


if __name__ == "__main__":
    main()
