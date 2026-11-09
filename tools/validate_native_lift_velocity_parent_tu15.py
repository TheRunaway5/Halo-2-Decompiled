#!/usr/bin/env python3
"""Check the staged 1DA750 velocity handoffs against physical TU1.5 fixtures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PARENT = ROOT / 'analysis/physics/native-phantom-body-velocity-parent-stage-20261103/staged'
VELOCITY = ROOT / 'analysis/physics/native-phantom-body-velocity-qualified-20261103/source'
FROZEN = ROOT / 'analysis/physics/native-accepted-body-contact-world-cohort1482-v4-test-closure-20261103/source-manifest.json'
FROZEN_PIN = '0c20914f37d6160ab3a7d7e8d7581d948cccddf6c3a4c277276a6404687e8308'
LIBRARIES = (
    'physics_vehicle_environment_state', 'physics_vehicle_constraint_cleanup', 'physics_vehicle_contact_state',
    'physics_vehicle_motion_math', 'physics_vehicle_motion', 'physics_vehicle_update', 'physics_vehicle_runtime',
    'native_lift_tu15', 'native_vehicle_rate_consumer', 'native_vehicle_flip', 'native_object_velocity', 'native_pair_consumer',
    'native_array_reserve', 'native_partition_metadata',
    'native_camera_projection', 'native_shape_bounds',
)


def sha(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def replace(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


def driver():
    test = (ROOT / 'tests/native_lift_scalar_parent_tu15_tests.cpp').read_text()
    test = '#include "halo2/native_tu15_object_activation_residuals.hpp"\n' \
           '#include "halo2/native_tu15_contact_candidate_private_data.hpp"\n' + test
    test = replace(test, 'struct Bank final:halo2::SourcePhysicsContactMemory {',
                   'struct Bank final:halo2::SourcePhysicsContactMemory,halo2::NativeTu15RetainedCounterStorage {')
    test = replace(test, '\n};\nhalo2::SourcePhysicsContactRuntimeAbi abi', r'''
  halo2::NativeTu15RetainedCounterUpdate adjust_counter(std::uint32_t p,unsigned width,std::uint32_t adjustment)override{
    check(width==2||width==4,"Invalid fixture counter width");
    std::array<std::byte,4> before{};
    for(unsigned i=0;i<width;++i)check(data.contains(p+i),"Unbacked fixture counter");
    read(p,std::span(before).first(width));
    const auto result=halo2::native_tu15_adjust_counter_value(before,width,adjustment);
    write(p,std::span(result.written).first(width));return result;
  }
};
halo2::SourcePhysicsContactRuntimeAbi abi''')
    test = replace(test, '  for(const auto& row:fixture.at("cases")){',
                   '  std::array<std::uint64_t,2> velocity_calls{};\n  for(const auto& row:fixture.at("cases")){')
    test = replace(test, '    halo2::NativeLiftScalarConsumerScopeTu15 scope', r'''
    std::size_t delivery_index{};
    policy.deliver_velocity=[&](auto &current,const auto &request,auto &numeric,const auto &observer){
      check(&current==&memory,label+" caller passed a different bank");
      const auto &deliveries=row.at("velocity_deliveries");
      check(delivery_index<deliveries.size(),label+" unexpected native velocity delivery");
      const auto &expected=deliveries.at(delivery_index++);
      const bool angular=request.phase==halo2::NativeTu15LiftVelocityPhase::set_angular;
      check(expected.at("callsite")== (angular?0x1db64fu:0x1db370u) &&
            expected.at("component")==request.component && expected.at("slot")==request.slot &&
            expected.at("vector")==request.caller_vector,label+" native velocity arguments differ");
      std::array<std::byte,12> source_words{};
      current.read(request.caller_vector,source_words);
      check(expected.at("vector_hex")==hex(source_words),label+" native velocity input differs");
      constexpr auto base=0x62000000u;
      const std::array<halo2::NativeTu15ContactPrivateRange,1> owned{{{base,0x900u}}};
      halo2::NativeTu15ContactCandidatePrivateData view(current,owned,{});
      halo2::native_tu15_deliver_lift_body_velocity(view,request,numeric,
          {base+0x20u,base+0x24u,base+0x40u,base+0x80u,0x800u},{},observer);
    };
    halo2::NativeLiftScalarConsumerScopeTu15 scope''')
    test = replace(test, '    const auto& expected_stores=row.at("semantic_writes");', r'''
    const auto expected_velocity=row.at("velocity_calls").get<std::array<std::uint64_t,2>>();
    check(delivery_index==row.at("velocity_deliveries").size() &&
          owned_activity.velocity_attempts==expected_velocity && owned_activity.velocity_completed==expected_velocity &&
          owned_activity.velocity_observed_stores==expected_velocity && !owned_activity.velocity_refusals,
          label+" exact native velocity activity differs");
    for(unsigned i=0;i<2;++i)velocity_calls[i]+=expected_velocity[i];
    const auto& expected_stores=row.at("semantic_writes");''')
    test = replace(test, '  for(auto count:calls)check(count,"An original1DA750 scalar callsite was not exercised");',
                   '  check(velocity_calls[0]>0 && velocity_calls[1]>0,"Both original velocity phases required");')
    return replace(test, '{"native_scalar_calls",calls},',
                   '{"native_scalar_calls",calls},{"native_velocity_calls",velocity_calls},')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True, help='Verified current 1482 source owner')
    parser.add_argument('--build', type=Path, required=True, help='Read-only archives from the same active 1482 owner')
    parser.add_argument('--fixture', type=Path, help='Fixture beneath this scratch directory')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    source, build, output = args.source.resolve(), args.build.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    report = {'passed': False, 'owned_scratch': str(scratch), 'retained_large_artifacts': [], 'results': []}
    try:
        assert source.parent == build.parent and source != scratch
        assert sha(FROZEN) == FROZEN_PIN
        pins = json.loads(FROZEN.read_text())['source_files_sha256']
        assert all(sha(source / name) == pin for name, pin in pins.items())
        manifest = {}
        archive = scratch / 'libraries'
        archive.mkdir()
        for name in LIBRARIES:
            basename = f'libhalo2_{name}.a'
            original, private = build / basename, archive / basename
            pin = sha(original)
            shutil.copyfile(original, private)
            assert sha(private) == sha(original) == pin
            manifest[basename] = pin
        report['verified_source_manifest_sha256'] = FROZEN_PIN
        report['read_only_archive_sha256'] = manifest
        fixture = args.fixture.resolve() if args.fixture else scratch / 'lift-velocity-parent.json'
        assert fixture.is_relative_to(scratch)
        if not fixture.exists():
            generator = ROOT / 'tools/generate_native_lift_velocity_parent_fixture_tu15.py'
            subprocess.run([sys.executable, str(generator), '--report-output', str(output / 'fixture-provenance.json')],
                           check=True)
        report['fixture_sha256'] = sha(fixture)
        test = scratch / 'parent.cpp'
        test.write_text(driver())
        force = scratch / 'force.cpp'
        force_source = PARENT / 'src/physics/physics_phantom_force_tu15_generated.cpp'
        force.write_text(replace(force_source.read_text(),
                                 '"physics/vehicles/physics_vehicle_motion_math_internal.hpp"',
                                 '"physics_vehicle_motion_math_internal.hpp"'))
        inputs = (test, force, PARENT / 'src/physics/native_tu15_object_activation_residuals.cpp',
                  VELOCITY / 'src/physics/native_tu15_phantom_body_velocity.cpp',
                  VELOCITY / 'src/physics/native_tu15_island_activation.cpp', source / 'src/native_lift_scalar_consumer_tu15.cpp',
                  source / 'src/physics/native_tu15_array_growth.cpp', source / 'src/physics/native_tu15_array_allocator.cpp')
        report['compiled_source_sha256'] = {str(p): sha(p) for p in inputs}
        for compiler in ('g++', 'clang++'):
            binary = scratch / compiler
            command = [compiler, '-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
                       '-ffunction-sections', '-fdata-sections', f'-I{PARENT / "include"}', f'-I{VELOCITY / "include"}',
                       f'-I{source / "include"}', f'-I{source / "src"}', f'-I{ROOT / "src/physics"}', *map(str, inputs),
                       '-Wl,--gc-sections', '-Wl,--start-group', *map(str, sorted(archive.glob('*.a'))),
                       '-Wl,--end-group', '-pthread', '-o', str(binary)]
            compiled = subprocess.run(command, capture_output=True, text=True)
            (output / f'{compiler}-build.log').write_text(compiled.stdout + compiled.stderr)
            assert compiled.returncode == 0, compiled.stderr[-7000:]
            result = subprocess.run([str(binary), str(fixture)], capture_output=True, text=True)
            (output / f'{compiler}-run.log').write_text(result.stdout + result.stderr)
            assert result.returncode == 0, result.stderr[-5000:]
            report['results'].append({'compiler': compiler, 'passed': True, 'detail': json.loads(result.stdout)})
        report['passed'] = True
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(
            p.stat().st_blocks * 512 for p in scratch.rglob('*') if p.is_file())
        (output / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
