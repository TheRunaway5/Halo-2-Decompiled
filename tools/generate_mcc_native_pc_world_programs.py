#!/usr/bin/env python3
"""Export finite original programs for the additional paired PC world profiles.

This imports no material instances, images, globals or StringIDs. Cooked
parameter conversion remains a separate source-proved admission step.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from native_program_storage_graph import storage_graph

ROOT = Path(__file__).resolve().parents[1]
SOURCES = {
    "coagulation": "f523c80325d5fe3f8bff7d4f3768a3b97fb2d9904a9fa455f307757e7436917e",
    "01b_spacestation": "db415f7973f0cc7a966148db98fd2837c8dfd3c4c320e534eec00ae7e11beb41",
    "03a_oldmombasa": "a249dafbf4bf99413cc4470c65e83e66ba1c1c94d8fc5bf4deffb4870e53f519",
    "dune": "d4ba6bf2e48b7f27be3059b7eb90dd8ca12e093330450d44dafcb4b0f66f56e8",
    "03b_newmombasa": "fb031f9ef910595180983caa9d29843d0c47c52a63bb7968781e474b11b5fd2a",
    "04a_gasgiant": "2d486204932531c726ea6db76d9f4a2e367a470cc9be9ae8c9c7d03da8dff9aa",
    "05a_deltaapproach": "473cef43ccbcbf7a598821bf1083b556a786018bbe5a2b74ca1b85a066e699e0",
    "deltatap": "b1dc10c8f04fdfcaa6523baa2bbed8f74995fb2b3d77ea5085a86032c821b450",
    "turf": "c3b6a581743e04a9b95bf33838c76216a5cc82597a65a68f89e613aca2a5b745",
    "zanzibar": "836dc371006640ddaf22b931904c88ee1a9602702fa69c9216b72100ac6cdb84",
}
ROOTS = {
    "needle": [
        ("coagulation", "opaque", "tex_bump_env_four_change_color"),
        ("coagulation", "opaque", "emblem_overlay_simple"),
        ("01b_spacestation", "opaque", "tex_bump_env_combined"),
        ("01b_spacestation", "opaque", "tex_bump_env_illum_combined"),
        ("01b_spacestation", "opaque", "tex_bump_illum"),
        ("01b_spacestation", "opaque", "tex_bump_alpha_test_single_pass"),
        ("dune", "transparent", "one_alpha_env_illum"),
        ("dune", "water", "water_edge_blend"),
        ("dune", "transparent", "plasma_alpha"),
        ("dune", "transparent", "two_alpha_clouds"),
        ("04a_gasgiant", "opaque", "tex_bump_illum_3_channel"),
        ("03a_oldmombasa", "opaque", "tex_bump_three_detail_blend"),
        ("zanzibar", "water", "water_static"),
    ],
    "street_sweeper": [
        ("coagulation", "opaque", "tex_bump_env_four_change_color"),
        ("coagulation", "opaque", "emblem_overlay_simple"),
        ("01b_spacestation", "opaque", "overlay"),
        ("01b_spacestation", "opaque", "tex_alpha_test"),
        ("01b_spacestation", "opaque", "tex_bump_alpha_test"),
        ("01b_spacestation", "opaque", "tex_bump_alpha_test_single_pass"),
        ("01b_spacestation", "opaque", "tex_bump_env_alpha_test_combined"),
        ("01b_spacestation", "opaque", "tex_bump_env_combined"),
        ("01b_spacestation", "opaque", "tex_bump_illum"),
        ("03a_oldmombasa", "transparent", "one_alpha_env"),
        ("03b_newmombasa", "opaque", "decal_simple"),
        ("05a_deltaapproach", "opaque", "tex_bump_detail_keep"),
        ("turf", "opaque", "tex_bump_illum_no_specular"),
        ("deltatap", "opaque", "tex_env"),
    ],
}


def require(value, message):
    if not value:
        raise ValueError("PC world program closure: " + message)


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()


def digest(value):
    return hashlib.sha256(value).hexdigest()


def identity(record):
    return record["group"], record["name"]


def generate(cache):
    native = Path(cache) / "native-shaders"
    libraries = {}
    for name, expected in SOURCES.items():
        library = json.loads((native / name / "library.json").read_bytes())
        require(digest(canonical(library)) == expected,
                "complete original source library differs: " + name)
        records = {identity(r): r for r in library["shader_tags"]}
        require(len(records) == len(library["shader_tags"]), "ambiguous source identity")
        libraries[name] = records

    def payload(source, record):
        filename = record["payload"]
        require(len(filename) == 68 and filename.endswith(".bin") and
                all(c in "0123456789abcdef" for c in filename[:64]), "invalid payload identity")
        data = (native / source / "payloads" / filename).read_bytes()
        require(len(data) == record["bytes"] and digest(data) + ".bin" == filename,
                "changed complete original source payload")
        return data

    def normalized(source, record):
        return storage_graph(record, payload(source, record))

    hud = json.loads((ROOT / "tools/data/mcc-native-hud-bootstrap.json").read_bytes())
    sky = json.loads((ROOT / "tools/data/mcc-native-pc-sky-closure.json").read_bytes())
    world = json.loads((ROOT / "tools/data/mcc-native-pc-world-closure.json").read_bytes())
    rows = []
    for sky_map in sky["maps"]:
        name = sky_map["map"]
        require(name in ROOTS, "unreviewed PC source map")
        base_records = [(hud["source_map"], r) for r in hud["records"]]
        base_records += [(sky_map["source_map"], r) for r in sky_map["records"]]
        base_records += [(world["source_map"], r) for r in world["records"]]
        base = {identity(r): (source, r) for source, r in base_records}
        require(len(base) == len(base_records), "ambiguous preceding HUD/sky/world bank")
        bank = {"schema": "H2_PRESERVED_NATIVE_SHADERS_V1", "map": name,
                "shader_tags": [r for _, r in base_records]}
        additions, equivalences = {}, {}
        roots = []
        pending = []
        for source, family, leaf in ROOTS[name]:
            key = ("stem", "shaders\\shader_templates\\" + family + "\\" + leaf)
            roots.append({"source_map": source, "group": key[0], "name": key[1]})
            pending.append((source, key))
        visited = set()
        while pending:
            source, key = pending.pop()
            if (source, key) in visited:
                continue
            visited.add((source, key))
            require(key[0] in ("stem", "spas", "vrtx") and key in libraries[source],
                    "dependency escapes the original typed program bank")
            record = libraries[source][key]
            require(not record["strings"], "program closure would change the StringID namespace")
            body = normalized(source, record)
            previous = base.get(key, additions.get(key))
            if previous:
                retained = normalized(*previous)
                require(body["graph"] == retained["graph"], "incompatible complete typed original program identity: " + repr(key))
                equivalences[(source, key)] = {"source_map": source, "group": key[0], "name": key[1],
                    "candidate_payload": record["payload"], "retained_payload": previous[1]["payload"],
                    "typed_complete_graph_sha256": body["graph_sha256"],
                    "candidate_storage": body["storage"], "retained_storage": retained["storage"]}
            else:
                additions[key] = source, record
            for _, target in record["references"]:
                require(target[0] in ("stem", "spas", "vrtx"),
                        "program closure unexpectedly requires an image or material")
                pending.append((source, tuple(target)))
        all_keys = set(base) | set(additions)
        for source, record in additions.values():
            require(all(tuple(target) in all_keys for _, target in record["references"]),
                    "incomplete staged program dependencies")
        rows.append({"map": name, "bytes": sky_map["bytes"], "sha256": sky_map["sha256"],
            "bootstrap_canonical_sha256": digest(canonical(bank)), "roots": roots,
            "sources": {source: SOURCES[source] for source, _, _ in ROOTS[name]},
            "records": [{"source_map": source, "record": record}
                        for key, (source, record) in sorted(additions.items())],
            "equivalent_existing_identities": [value for key, value in sorted(equivalences.items())]})
    require({row["map"] for row in rows} == set(ROOTS), "incomplete PC map census")
    return {"schema": "H2_MCC_NATIVE_PC_WORLD_PROGRAMS_V1", "maps": rows,
        "scope": "Finite additional TU1.5 stem/spas/vrtx programs only; cooked material and GPU admission are separate",
        "overlap_proof": "Every complete typed root/array word, all seven vertex classes, opaque code/declarations and typed references match; allocation order and bounded original alignment padding differ only as explicitly retained in each storage receipt"}


def render(policy):
    data = json.dumps(policy, indent=2) + "\n"
    runtime = json.loads(json.dumps(policy))
    for row in runtime["maps"]:
        for proof in row["equivalent_existing_identities"]:
            for field in ("candidate_storage", "retained_storage"):
                proof[field + "_sha256"] = digest(canonical(proof.pop(field)))
    encoded = canonical(runtime).decode()
    generated = ("// Generated by tools/generate_mcc_native_pc_world_programs.py; do not edit.\n"
        "// Data SHA256: " + digest(data.encode()) + "\n"
        'constexpr char native_pc_world_programs_storage[]=R"H2PCPROGS('
        + encoded + ')H2PCPROGS";\n'
        'constexpr std::string_view native_pc_world_programs_policy{native_pc_world_programs_storage,sizeof(native_pc_world_programs_storage)-1};\n')
    return data, generated


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    policy = generate(args.cache)
    data, generated = render(policy)
    for path, text in ((ROOT / "tools/data/mcc-native-pc-world-programs.json", data),
                       (ROOT / "src/runtime/mcc_native_pc_world_programs.inc", generated)):
        if args.check:
            require(path.is_file() and path.read_text() == text, "stale generated source: " + str(path))
        else:
            path.write_text(text)
    print("Authenticated additional PC world programs:",
          ", ".join(f"{r['map']}={len(r['records'])}" for r in policy["maps"]))


if __name__ == "__main__":
    main()
