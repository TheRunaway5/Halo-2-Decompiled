#!/usr/bin/env python3
"""Stage only the qualified native281530 World route and honest diagnostics."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIELDS = ("attempts", "calls", "refusals", "unbound_filters", "unbound_motion_types",
          "payload_filters", "motion_queries")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def once(text, old, new):
    assert text.count(old) == 1, old[:140]
    return text.replace(old, new)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    args.source_root = args.source_root.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    proof_path = ROOT / "analysis/physics/native-discrete-pair-binding-original-20261102-first/report.json"
    proof = json.loads(proof_path.read_text())
    assert proof["passed"] and proof["current_sources_unchanged"]
    for name, digest in proof["source_hashes"].items():
        if (ROOT / name).is_relative_to(Path(proof["owned_scratch"])):
            continue  # Removed owned diagnostic source, hashed in its proof.
        assert sha(ROOT / name) == digest, name
    files = ["include/halo2/native_tu15_world_integration.hpp",
             "include/halo2/native_tu15_physics_frame_residuals.hpp",
             "src/physics/native_tu15_world_integration_residuals.cpp",
             "src/physics/physics_live_world_tu15.cpp", "tests/physics_multiplayer_acceptance.cpp"]
    paths = {name: args.source_root / name for name in files}
    getter_name = "src/physics/physics_live_world_tu15.cpp"
    if not paths[getter_name].exists():
        paths[getter_name] = args.source_root / "src/physics_live_world_tu15.cpp"
    base = {name: path.read_text() for name, path in paths.items()}
    staged = dict(base)
    name = files[0]
    staged[name] = once(staged[name], '#include "halo2/native_tu15_pair_island_wake_binding.hpp"',
                        '#include "halo2/native_tu15_pair_island_wake_binding.hpp"\n#include "halo2/native_tu15_discrete_pair_binding.hpp"')
    staged[name] = once(staged[name], '  NativeTu15PairIslandWakeActivity pair_island_wake;',
                        '  NativeTu15PairIslandWakeActivity pair_island_wake;\n  NativeTu15DiscretePairBindingActivity pair_discrete_query;')
    name = files[1]
    anchor = '  std::uint64_t native_pair_island_wake_attempts{}'
    at = staged[name].index(anchor)
    staged[name] = staged[name][:at] + '  std::uint64_t ' + ','.join('native_pair_discrete_' + field + '{}' for field in FIELDS) + ';\n' + staged[name][at:]
    name = files[2]
    old = '''create.agents.test_discrete_pair=[&](auto dispatcher,auto target,auto result,auto first,auto second,auto&){
      if(target.identity!=0x281530u)unqualified_pair_service("Native TU1.5 pair create requires qualified discrete owner");
      const std::array args{result,first.identity,second.identity};call(target.identity,args,dispatcher,12);return state.gp[0];
    };'''
    new = '''create.agents.test_discrete_pair=[&](auto dispatcher,auto target,auto result,auto first,auto second,auto&current){
      if(target.identity!=0x281530u)unqualified_pair_service("Native TU1.5 pair create requires qualified discrete owner");
      return native_tu15_test_bound_discrete_pair(m,dispatcher,result,first,second,current,observe,
        stats?&stats->pair_discrete_query:nullptr);
    };'''
    staged[name] = once(staged[name], old, new)
    name = files[3]
    assignments = dict(attempts="attempts", calls="completed", refusals="refusals",
                       unbound_filters="unbound_filters", unbound_motion_types="unbound_motion_types",
                       payload_filters="payload.calls", motion_queries="payload.motion_queries")
    getter = ''.join('    result.native_pair_discrete_' + field + '=world_integration.pair_discrete_query.' + assignments[field] + ';\n' for field in FIELDS)
    staged[name] = once(staged[name], '    result.native_pair_island_wake_attempts=', getter + '    result.native_pair_island_wake_attempts=')
    name = files[4]
    row = '        {"native_pair_island_wake_attempts",native_frames.native_pair_island_wake_attempts},'
    assert staged[name].count(row) == 2
    rows = ''.join('        {"native_pair_discrete_' + field + '",native_frames.native_pair_discrete_' + field + '},\n' for field in FIELDS)
    staged[name] = staged[name].replace(row, rows + row)
    gate = '''  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_DISCRETE_QUERY")) {
    const bool native_discrete=native_frames.native_pair_discrete_calls>0&&
      native_frames.native_pair_discrete_attempts==native_frames.native_pair_discrete_calls&&
      native_frames.native_pair_discrete_refusals==0&&native_frames.native_pair_discrete_unbound_filters==0&&
      native_frames.native_pair_discrete_unbound_motion_types==0&&native_frames.native_pair_discrete_payload_filters>0&&
      !native_frames.residual_owner_calls.contains(0x281530u);
    report["native_pair_discrete_query_gate"]=native_discrete;passed&=native_discrete;
  }
'''
    staged[name] = once(staged[name], '  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_ISLAND_WAKE")) {',
                        gate + '  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_ISLAND_WAKE")) {')
    helper = "include/halo2/native_tu15_discrete_pair_binding.hpp"
    staged[helper] = (ROOT / helper).read_text()
    # Inherit exact coherent-model inputs from this frozen source. They are
    # never overwritten by a broader Root source or a different fixture version.
    inherited = ["cmake/PhysicsLiveWorldTu15.cmake", "tests/native_tu15_world_residuals_tests.cpp",
                 "tests/helpers/native_world_convex_factory_prefix_tu15.hpp",
                 "tests/helpers/native_world_island_merge_prefix_tu15.hpp",
                 "tests/helpers/native_contact_process_binding_model_tu15.hpp"]
    for name in inherited:
        staged[name] = (args.source_root / name).read_text()
    delta = []
    for name, content in staged.items():
        target = args.output / "staged" / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content)
        if name in base:
            delta.extend(difflib.unified_diff(base[name].splitlines(True), content.splitlines(True),
                                             fromfile="a/" + name, tofile="b/" + name))
    (args.output / "delta.patch").write_text(''.join(delta))
    pins = dict(qualified=False, production_applied=False, live_qualified=False,
                source_root=str(args.source_root), physical_proof=str(proof_path.relative_to(ROOT)),
                physical_proof_sha256=sha(proof_path),
                base_paths={name: str(path) for name, path in paths.items()},
                base_hashes={name: hashlib.sha256(content.encode()).hexdigest() for name, content in base.items()},
                staged_hashes={name: hashlib.sha256(content.encode()).hexdigest() for name, content in staged.items()},
                inherited_files=inherited, new_files=[helper], delta_files=files,
                scope="Sole native281530/281F10/1D7530 World pair callback; seven Getter/twoJSON/positive-gate fields. Known native motion getters; actual unknown filters/motion types count and refuse, no residual execution or GP result transport.")
    (args.output / "source-pins.json").write_text(json.dumps(pins, indent=2) + "\n")
    print(json.dumps(dict(staged_files=len(staged), physical_proof_passed=True)))


if __name__ == "__main__":
    main()
