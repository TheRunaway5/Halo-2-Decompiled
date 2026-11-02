"""Apply only the qualified environment material delta to the active owner.

Run sequentially through its existing run_test.py command coordinator. Preserve
unrelated Root loader changes and all unchanged frozen source. No selection,
asset, runtime or prior source archive is written.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from build_launcher_engine import source_hashes


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def inventory(pins):
    return hashlib.sha256(json.dumps(pins, sort_keys=True).encode()).hexdigest()


def replace(text, before, after):
    assert text.count(before) == 1 and after not in text
    return text.replace(before, after)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preparation", type=Path, required=True)
    parser.add_argument("--component", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--new-preparation", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    source = owner / "source"
    prep = json.loads(args.preparation.read_text())
    assert Path(prep["owned_scratch"]) == owner and Path(prep["source"]) == source
    assert prep["sealed_inventory_verified"] and not prep["overlays"]
    before = source_hashes(source)
    assert inventory(before) == prep["production_inventory_sha256"]
    component = json.loads(args.component.read_text())
    assert component["passed"] and len(component["results"]) == 3 and not Path(component["owned_scratch"]).exists()
    for row in component["results"]:
        report_path = args.component.parent / (row["compiler"] + ("-sanitizers" if row["sanitizers"] else "")) / "evidence/env-illum.json"
        report = json.loads(report_path.read_text())
        assert report["passed"] and report["authored_rows"] == 164 and report["original_paired_rows"] == 114
        assert report["guard_cases"] == 1908 and report["native_template_variants"] == 39
    for pins in (component["source_pins"], component["tool_pins"]):
        assert all(digest(Path(path)) == pin for path, pin in pins.items())
    selections = {name: (root / name).read_bytes() for name in ("gameplay-runtime-selection.json", "frontend-runtime-selection.json")}
    runtime = root / prep["selected_runtime"]
    assert digest(runtime) == prep["runtime_sha256"]
    copied = [
        "include/halo2/mcc_native_env_illum_three_channel.hpp",
        "src/rendering/mcc_native_env_illum_three_channel.cpp",
        "src/rendering/mcc_native_env_illum_three_channel_profiles.inc",
        "cmake/MccNativeEnvIllumThreeChannel.cmake",
        "tools/data/mcc-native-env-illum-three-channel-profiles.json",
        "tools/mcc_env_illum_projection.py", "tools/generate_mcc_native_env_illum_three_channel.py",
        "tools/export_mcc_native_env_illum_three_channel_fixture.py", "tools/freeze_mcc_env_illum_fixture.py",
        "tools/validate_mcc_env_illum_three_channel.py",
        "tests/mcc_native_env_illum_three_channel_tests.cpp", "tests/mcc_native_env_illum_three_channel_tests.py",
        "tests/fixtures/mcc-native-env-illum-three-channel-v1.json",
        "tests/fixtures/mcc-native-env-illum-three-channel-v1.json.zlib"]
    planned = {}
    for name in copied:
        assert not (source / name).exists()
        planned[name] = (root / name).read_bytes()
    name = "CMakeLists.txt"
    text = (source / name).read_text()
    anchor = "    src/rendering/mcc_native_world_default_roles.cpp src/rendering/mcc_native_illum_three_channel.cpp)\n"
    text = replace(text, anchor, anchor + "target_sources(halo2_cache PRIVATE src/rendering/mcc_native_env_illum_three_channel.cpp)\n")
    anchor = "    foreach(world_profile world_default_roles illum_no_specular illum_three_channel character_shared)\n"
    text = replace(text, anchor, "    include(cmake/MccNativeEnvIllumThreeChannel.cmake)\n" + anchor)
    planned[name] = text.encode()
    name = "src/rendering/mcc_native_shaders.cpp"
    text = (source / name).read_text()
    anchor = '#include "halo2/mcc_native_illum_three_channel.hpp"\n'
    text = replace(text, anchor, anchor + '#include "halo2/mcc_native_env_illum_three_channel.hpp"\n')
    root_loader = (root / name).read_text()
    start = "            if(!projected)projected=project_mcc_env_illum_three_channel("
    end = "            if(!projected)projected=project_mcc_world_default_roles("
    assert root_loader.count(start) == root_loader.count(end) == 1 and start not in text
    block = root_loader[root_loader.index(start):root_loader.index(end)]
    assert block.count("alias.address") == 1 and "retained_tag_allocations_" in block
    planned[name] = replace(text, end, block + end).encode()
    name = "tools/build_launcher_engine.py"
    text = (source / name).read_text()
    anchor = "                   '--target', 'mcc_native_world_animated_probe',\n"
    text = replace(text, anchor, anchor + "                   '--target', 'mcc_native_env_illum_three_channel_probe',\n")
    anchor = "                   'mcc_native_world_animated|mcc_native_world_animated_generator|'\n"
    text = replace(text, anchor, anchor + "                   'mcc_native_env_illum_three_channel|mcc_native_env_illum_three_channel_generator|'\n")
    planned[name] = text.encode()
    name = "tests/test_build_launcher_engine.py"
    text = (source / name).read_text()
    anchor = "                     'workshop_navigation_content','original_workshop_camera_shared']:\n"
    planned[name] = replace(text, anchor, "                     'workshop_navigation_content','original_workshop_camera_shared',\n"
        "                     'mcc_native_env_illum_three_channel','mcc_native_env_illum_three_channel_generator']:\n").encode()
    root_pins = {name: digest(root / name) for name in planned}
    changes = []
    for name, raw in planned.items():
        target = source / name
        assert target.parent.resolve().is_relative_to(source) and not target.is_symlink()
        changes.append(dict(path=name, before=digest(target) if target.exists() else None,
                            sha256=hashlib.sha256(raw).hexdigest(), bytes=len(raw)))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.with_suffix(".planned.json").write_text(json.dumps(dict(scope=__doc__, source=str(source),
        base_inventory_sha256=inventory(before), changes=changes), indent=2) + "\n")
    for name, raw in planned.items():
        target = source / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
    after = source_hashes(source)
    for row in changes:
        row["production"] = row["path"] in after
        assert digest(source / row["path"]) == row["sha256"]
    actual = {name for name in before.keys() | after.keys() if before.get(name) != after.get(name)}
    assert actual == {x["path"] for x in changes if x["production"]}
    assert all(digest(root / name) == pin for name, pin in root_pins.items())
    assert all((root / name).read_bytes() == raw for name, raw in selections.items())
    assert digest(runtime) == prep["runtime_sha256"]
    report = dict(scope=__doc__, passed=True, source=str(source), changes=changes,
        helper_sha256=digest(Path(__file__)),
        base_preparation=str(args.preparation), base_preparation_sha256=digest(args.preparation),
        production_inventory_before=inventory(before), production_inventory_sha256=inventory(after), production_files=len(after),
        component=str(args.component), component_sha256=digest(args.component),
        unrelated_Root_loader_changes_preserved=True, Root_files_unchanged=True,
        selected_runtime_unchanged=True, source_published=False, owned_scratch=str(owner), retained_large_artifacts=[])
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    fresh = dict(prep, scope=__doc__, status="declared_material_overlay_not_engine_qualified",
        sealed_inventory_verified=False, base_sealed_inventory_verified=True, overlay_inventory_verified=True,
        base_production_inventory_sha256=inventory(before), production_inventory_sha256=inventory(after), production_files=len(after),
        overlays=changes, source_refresh_receipt=str(args.output), source_refresh_sha256=digest(args.output))
    args.new_preparation.write_text(json.dumps(fresh, indent=2) + "\n")
    print(json.dumps(dict(passed=True, changed_files=len(changes), production_files=len(after),
                         production_inventory_sha256=inventory(after), selected_runtime_unchanged=True)))


if __name__ == "__main__":
    main()
