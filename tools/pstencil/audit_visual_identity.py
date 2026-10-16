#!/usr/bin/env python3
"""Read-only evidence inventory for the May 2003 material identity audit.

Loose-tag path strings are recorded as bytes with offsets, not decoded shader
execution. Retail object/material links use the existing version-8 cache reader
and the field offsets owned by src/model.cpp and src/model_selection.cpp.
No material parameters, historical aliases, or runtime selection are inferred.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from audit_rendering import Cache, file_sha

BUILD_SHA = "5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18"
PATH_STRING = re.compile(rb"(?:shaders|objects|scenarios|rasterizer)\\[a-zA-Z0-9_ '()\\.-]+\x00")
DEFAULT_MAPS = ("coagulation", "lockout", "03b_newmombasa", "05a_deltaapproach", "07a_highcharity")
SAMPLE_DIRS = (
    "objects/vehicles/warthog", "objects/vehicles/ghost", "objects/vehicles/banshee",
    "objects/vehicles/wraith", "objects/vehicles/phantom", "objects/vehicles/pelican",
    "objects/vehicles/shadow", "objects/vehicles/creep", "objects/vehicles/scorpion",
    "objects/weapons/rifle/smg", "objects/weapons/rifle/smg_dual",
    "objects/weapons/rifle/covenant_carbine", "objects/weapons/rifle/plasma_rifle",
    "objects/weapons/melee/energy_blade", "objects/weapons/support_low/brute_shot",
    "objects/weapons/support_high/fuel_rod_gun",
    "scenarios/objects/human/residential/vending_machine",
    "scenarios/objects/covenant/military/cov_storage",
)


def loose_file(path: Path, tags: Path) -> dict:
    data = path.read_bytes()
    return dict(path=path.relative_to(tags).as_posix(), bytes=len(data),
                sha256=hashlib.sha256(data).hexdigest(),
                embedded_paths=[dict(file_offset=f"0x{m.start():x}", path=m.group()[:-1].decode("ascii"))
                                for m in PATH_STRING.finditer(data)])


def retail_material(cache: Cache, datum: int) -> dict:
    tag, p = cache.tag(datum, "shad")
    records = []
    for pp in cache.block(p + 32, 124):
        template, stem = cache.tag(cache.u32(pp), "stem")
        bitmaps = []
        for entry in cache.block(pp + 4, 12):
            bitmap = cache.tags.get(cache.u32(entry))
            bitmaps.append(dict(file_offset=f"0x{entry:x}", datum=f"{cache.u32(entry):08x}",
                                name=bitmap["name"] if bitmap else None,
                                bitmap_index=cache.u32(entry + 4)))
        # Store literal template layer/pass references without assigning names to
        # bitmap slots or claiming a selected LOD/implementation.
        definitions = []
        for definition in cache.block(stem + 88, 40):
            passes = []
            for entry in cache.block(definition + 16, 10):
                pass_datum = cache.u32(entry + 4)
                pass_tag, _ = cache.tag(pass_datum, "spas")
                passes.append(dict(file_offset=f"0x{entry:x}", datum=f"{pass_datum:08x}", name=pass_tag["name"]))
            definitions.append(dict(lods=[dict(file_offset=f"0x{entry:x}", available_layers=f"0x{cache.u32(entry + 2):x}")
                                          for entry in cache.block(definition, 10)], passes=passes))
        records.append(dict(file_offset=f"0x{pp:x}", template=template["name"], bitmaps=bitmaps, definitions=definitions))
    return dict(datum=f"{datum:08x}", name=tag["name"], file_offset=f"0x{p:x}",
                specular_selector=cache.u16(p + 0x3e),
                brightness_bytes=cache.data[p + 0x44:p + 0x4c].hex(), postprocess=records)


def retail_cache(path: Path, full: bool = False) -> dict:
    cache = Cache(path)
    try:
        objects, shaders = [], {}
        for datum, tag in cache.tags.items():
            groups = {"bipd", "vehi", "weap", "eqip", "garb", "proj", "scen", "mach", "ctrl", "lifi", "ssce", "bloc", "crea"} if full else {"vehi", "weap", "scen", "bloc", "mach", "eqip"}
            if tag["group"] not in groups or not tag["address"]:
                continue
            # All weapons/vehicles; props only from the cited sample families.
            if not full and tag["group"] not in {"vehi", "weap"} and not any(
                    key in tag["name"] for key in ("cov_storage", "vending_machine", "crate", "street_light", "scarab")):
                continue
            p = cache.pointer(tag["address"], cache.magic, tag["size"])
            hlmt = cache.u32(p + 0x38)
            if hlmt == 0xffffffff:
                objects.append(dict(group=tag["group"], name=tag["name"], model=None))
                continue
            model_tag, model = cache.tag(hlmt, "hlmt")
            mode = cache.u32(model + 4)
            if mode == 0xffffffff:
                objects.append(dict(group=tag["group"], name=tag["name"], model=model_tag["name"], render_model=None))
                continue
            render_tag, render = cache.tag(mode, "mode")
            materials = []
            for entry in cache.block(render + 0x60, 32):
                shader = cache.u32(entry + 12)
                if shader == 0xffffffff:
                    materials.append(None)
                    continue
                key = f"{shader:08x}"
                if key not in shaders:
                    shaders[key] = retail_material(cache, shader)
                materials.append(dict(shader=key, file_offset=f"0x{entry:x}"))
            objects.append(dict(group=tag["group"], name=tag["name"], datum=f"{datum:08x}",
                                file_offset=f"0x{p:x}", model=model_tag["name"],
                                render_model=render_tag["name"], render_model_file_offset=f"0x{render:x}", materials=materials))
        if full:
            for datum, tag in cache.tags.items():
                key=f"{datum:08x}"
                if tag["group"]=="shad" and tag["address"] and key not in shaders:
                    shaders[key]=retail_material(cache,datum)
        return dict(path=str(path), sha256=file_sha(path), objects=objects, shaders=shaders)
    finally:
        cache.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=Path("/mnt/SSD4TB/Leaks/Halo 2/2003"))
    parser.add_argument("--maps", type=Path, default=ROOT / "assets/disc/maps")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--full", action="store_true", help="Inventory every current primary map/object/material and every loose2003 object/material reference")
    args = parser.parse_args()
    if args.output.exists():
        parser.error("output exists; supply a new evidence file")
    xbe = args.build / "game/halo2debug.xbe"
    if file_sha(xbe) != BUILD_SHA:
        parser.error("the May 2003 reference XBE hash differs")
    tags = args.build / "tools/tags"
    paths = set((tags / "shaders").rglob("*.shader_template"))
    paths.update((tags / "shaders").rglob("*.shader_pass"))
    paths.update((tags / "shaders").rglob("*.shader_light_response"))
    shader_users = []
    for path in sorted(tags.rglob("*.shader")):
        row = loose_file(path, tags)
        templates = [entry for entry in row["embedded_paths"] if entry["path"].startswith("shaders\\shader_templates\\")]
        if templates:
            shader_users.append(dict(path=row["path"], sha256=row["sha256"], template_paths=templates))
    for directory in SAMPLE_DIRS:
        paths.update(p for p in (tags / directory).rglob("*") if p.is_file() and p.suffix in {
            ".vehicle", ".weapon", ".model", ".render_model", ".shader", ".shader_stencilshadow", ".scenery"})
    if args.full:
        paths.update(p for p in tags.rglob('*') if p.is_file() and p.suffix in {
            '.biped','.vehicle','.weapon','.equipment','.garbage','.projectile','.scenery',
            '.device_machine','.device_control','.device_light_fixture','.sound_scenery','.crate','.creature',
            '.model','.render_model','.shader','.shader_stencilshadow'})
    map_paths = [args.maps/f'{name}.map' for name in DEFAULT_MAPS]
    if args.full:
        map_paths=[p for p in sorted(args.maps.glob('*.map')) if p.stem not in {'mainmenu','shared','single_player_shared'}]
    report = dict(schema="pstencil-visual-identity-evidence-v1", reference_xbe=str(xbe), reference_sha256=BUILD_SHA,
                  loose_tag_root=str(tags), loose_files=[loose_file(p, tags) for p in sorted(paths)],
                  loose_shader_template_users=shader_users,
                  current_caches=[retail_cache(path,args.full) for path in map_paths],
                  boundaries=["Loose embedded paths are byte evidence; no loose tag serializer/schema was emulated.",
                              "Current object -> model -> render model -> material links are resident metadata, not live instance/part/LOD admission.",
                              "Bitmap names/slots do not establish texture semantics; authored pass remaps must own every implementation binding.",
                              "No alias or numeric material parameter is inferred; no original framebuffer was captured."])
    if args.full:
        reference_names={str(path.relative_to(tags).with_suffix('')).replace('/','\\') for path in paths}
        report['coverage_scope']='All supplied primary current maps, all resident object/material metadata; not all live parts/LODs or framebuffer validation.'
        report['literal_path_matches']={}
        for family, rows in [('objects',[o for c in report['current_caches'] for o in c['objects']]),
                             ('shaders',[s for c in report['current_caches'] for s in c['shaders'].values()])]:
            names=sorted({row['name'] for row in rows})
            matched=[name for name in names if name in reference_names]
            report['literal_path_matches'][family]=dict(unique_current=len(names),matched=matched,
                no_literal_reference=[name for name in names if name not in reference_names],
                boundary='Exact path only; matching paths do not prove unchanged values, equivalent templates or runtime appearance. No match does not prove historical absence.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(f"{len(report['loose_files'])} loose files; {len(shader_users)} shader template users; {len(report['current_caches'])} current caches; {args.output}")


if __name__ == "__main__":
    main()
