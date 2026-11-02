#!/usr/bin/env python3
"""Stage native2815F0 at the World listener callback, with honest diagnostics."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
from prepare_native_discrete_pair_world_binding_tu15 import once, sha

ROOT = Path(__file__).resolve().parents[1]
FIELDS = dict(attempts='attempts', calls='completed', refusals='refusals',
              deferred_bodies='listener.deferred_bodies', array_growths='listener.array_growths',
              property_queries='listener.property_queries', motion_publications='listener.component_motion_publications',
              unbound_services='listener.unbound_services')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source = args.source_root.resolve()
    proof_path = ROOT / 'analysis/physics/native-body-deactivation-listener-production-20261102-final/report.json'
    proof = json.loads(proof_path.read_text())
    assert proof['passed'] and proof['production_binding'] and proof['source_snapshot_unchanged']
    for name, digest in proof['source_hashes'].items():
        assert sha(ROOT / name) == digest, name
    args.output.mkdir(parents=True, exist_ok=False)
    names = ['include/halo2/native_tu15_world_integration.hpp',
             'include/halo2/native_tu15_physics_frame_residuals.hpp',
             'src/physics/native_tu15_world_integration_residuals.cpp',
             'src/physics/physics_live_world_tu15.cpp', 'tests/physics_multiplayer_acceptance.cpp',
             'cmake/PhysicsLiveWorldTu15.cmake']
    paths = {name: source / name for name in names}
    getter = names[3]
    if not paths[getter].exists():
        paths[getter] = source / 'src/physics_live_world_tu15.cpp'
    base = {name: path.read_text() for name, path in paths.items()}
    staged = dict(base)
    name = names[0]
    staged[name] = once(staged[name], '#include "halo2/native_tu15_pair_island_wake_binding.hpp"',
                        '#include "halo2/native_tu15_pair_island_wake_binding.hpp"\n#include "halo2/native_tu15_body_deactivation_binding.hpp"')
    staged[name] = once(staged[name], '  NativeTu15PairIslandWakeActivity pair_island_wake;',
                        '  NativeTu15PairIslandWakeActivity pair_island_wake;\n  NativeTu15BodyDeactivationBindingActivity body_deactivation;')
    name = names[1]
    anchor = '  std::uint64_t native_pair_island_wake_attempts{}'
    at = staged[name].index(anchor)
    staged[name] = staged[name][:at] + '  std::uint64_t ' + ','.join('native_body_deactivation_' + field + '{}' for field in FIELDS) + ';\n' + staged[name][at:]
    name = names[2]
    old = '  transition_services.body_deactivated=[&](auto listener,auto owner,auto body,auto&){const std::array args{body.identity};call(owner.identity,args,listener.identity,4);};'
    new = '''  transition_services.body_deactivated=[&](auto listener,auto owner,auto body,auto&current){
    if(owner.identity==0x2815f0u){
      native_tu15_notify_bound_body_deactivation(m,listener,body,current,arena_storage,observe,
        stats?&stats->body_deactivation:nullptr);return;
    }
    const std::array args{body.identity};call(owner.identity,args,listener.identity,4);
  };'''
    staged[name] = once(staged[name], old, new)
    assignments = ''.join('    result.native_body_deactivation_' + field + '=world_integration.body_deactivation.' + member + ';\n' for field, member in FIELDS.items())
    staged[getter] = once(staged[getter], '    result.native_pair_island_wake_attempts=', assignments + '    result.native_pair_island_wake_attempts=')
    name = names[4]
    row = '        {"native_pair_island_wake_attempts",native_frames.native_pair_island_wake_attempts},'
    assert staged[name].count(row) == 2
    rows = ''.join('        {"native_body_deactivation_' + field + '",native_frames.native_body_deactivation_' + field + '},\n' for field in FIELDS)
    staged[name] = staged[name].replace(row, rows + row)
    gate = '''  if(std::getenv("HALO2_REQUIRE_NATIVE_BODY_DEACTIVATION")) {
    const bool native_body_sleep=native_frames.native_body_deactivation_calls>0&&
      native_frames.native_body_deactivation_attempts==native_frames.native_body_deactivation_calls&&
      native_frames.native_body_deactivation_refusals==0&&native_frames.native_body_deactivation_unbound_services==0&&
      native_frames.native_body_deactivation_property_queries>0&&!native_frames.residual_owner_calls.contains(0x2815f0u);
    report["native_body_deactivation_gate"]=native_body_sleep;passed&=native_body_sleep;
  }
'''
    anchor = '  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_CREATION_QUEUES")) {'
    staged[name] = once(staged[name], anchor, gate + anchor)
    staged[names[5]] += '\n# Native2815F0 body-deactivation listener; selected motion child remains explicit.\ntarget_sources(halo2_native_pair_consumer PRIVATE src/physics/native_tu15_body_deactivation_listener.cpp)\n'
    new_files = ['include/halo2/native_tu15_body_deactivation_binding.hpp',
                 'include/halo2/native_tu15_body_deactivation_listener.hpp',
                 'src/physics/native_tu15_body_deactivation_listener.cpp']
    for name in new_files:
        staged[name] = (ROOT / name).read_text()
    inherited = ['tests/native_tu15_world_residuals_tests.cpp',
                 'tests/helpers/native_world_convex_factory_prefix_tu15.hpp',
                 'tests/helpers/native_world_island_merge_prefix_tu15.hpp',
                 'tests/helpers/native_contact_process_binding_model_tu15.hpp']
    for name in inherited:
        staged[name] = (source / name).read_text()
    delta = []
    for name, text in staged.items():
        target = args.output / 'staged' / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)
        if name in base:
            delta.extend(difflib.unified_diff(base[name].splitlines(True), text.splitlines(True), fromfile='a/' + name, tofile='b/' + name))
    (args.output / 'delta.patch').write_text(''.join(delta))
    (args.output / 'source-pins.json').write_text(json.dumps(dict(qualified=False, production_applied=False,
        live_qualified=False, physical_proof=str(proof_path.relative_to(ROOT)), physical_proof_sha256=sha(proof_path),
        base_paths={name: str(path) for name, path in paths.items()},
        base_hashes={name: hashlib.sha256(text.encode()).hexdigest() for name, text in base.items()},
        staged_hashes={name: hashlib.sha256(text.encode()).hexdigest() for name, text in staged.items()},
        delta_files=names, new_files=new_files, inherited_files=inherited,
        scope='Sole qualified native2815F0 binding at genuine World body listener; native allocation services, explicit native1D7260 refusal before execution. Eight Getter/twoJSON/positive-gate fields; all other World routes retained.'), indent=2) + '\n')
    print(json.dumps(dict(staged_files=len(staged), physical_proof_passed=True)))


if __name__ == '__main__':
    main()
