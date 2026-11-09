#!/usr/bin/env python3
"""Physical current-source native marker/velocity handoffs across authored force families."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

import generate_native_lift_authored_parent_fixture_tu15 as generator
import validate_native_lift_marker_velocity_parent_tu15 as legacy
import stage_native_lift_body_velocity_port_tu15 as staging

ROOT = generator.ROOT
LIBRARIES = tuple(name for name in legacy.base.LIBRARIES if name != 'native_partition_metadata') + (
    'native_array_storage', 'native_weapon_effect_dispatch_tu15', 'native_tu15_render_ownership')


def once(text, old, new):
    return legacy.base.replace(text, old, new)


def driver():
    text = legacy.driver()
    text = once(text, '  std::set<std::uint32_t> *marker_writes{};',
                '  std::set<std::uint32_t> *marker_writes{};\n  mutable J *delivery_reads{};')
    old = '  void read(std::uint32_t p,std::span<std::byte> out)const override{for(auto& b:out){const auto at=data.find(p);check(at!=data.end(),"Missing parent retained read at "+address(p));b=at->second;++p;}}'
    new = '  void read(std::uint32_t p,std::span<std::byte> out)const override{const auto first=p;for(auto& b:out){const auto at=data.find(p);check(at!=data.end(),"Missing parent retained read at "+address(p));b=at->second;++p;}if(delivery_reads)delivery_reads->push_back(J::array({first,hex(out)}));}'
    text = once(text, old, new)
    text = once(text, '  for(const auto& row:fixture.at("cases")){',
                '  for(const auto& row:fixture.at("cases"))for(bool reads:{false,true}){')
    text = once(text, 'Bank memory(row.at("initial_banks"));',
                'Bank memory(row.at("initial_banks"));J actual_delivery_reads=J::array();')
    text = once(text, 'services.instruction=[&](auto pc){phase=pc;};',
                'services.instruction=[&](auto pc){phase=pc;if(reads&&(pc==0x1db370u||pc==0x1db64fu)){actual_delivery_reads=J::array();memory.delivery_reads=&actual_delivery_reads;}};')
    text = once(text, '''      std::array<std::byte,12> source_words{};
      current.read(request.caller_vector,source_words);
      check(expected.at("vector_hex")==hex(source_words),label+" native velocity input differs");
''', '')
    text = once(text, '''          {base+0x20u,base+0x24u,base+0x40u,base+0x80u,0x800u},{},observer);
''', '''          {base+0x20u,base+0x24u,base+0x40u,base+0x80u,0x800u},{},observer);
      memory.delivery_reads=nullptr;
      if(reads){J wanted=J::array();for(const auto&r:expected.at("retained_reads"))wanted.push_back(J::array({r.at("address"),r.at("raw")}));check(actual_delivery_reads==wanted,label+" native delivery retained read order differs: "+actual_delivery_reads.dump()+" expected "+wanted.dump());}
''')
    text = once(text, 'owned_activity.velocity_observed_stores==expected_velocity',
                'owned_activity.velocity_observed_stores==(reads?std::array<std::uint64_t,2>{}:row.at("velocity_store_calls").get<std::array<std::uint64_t,2>>())')
    callback = '[&](const auto& event){stores.push_back(J{{"instruction",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});}'
    text = once(text, 'fp,services,' + callback,
                'fp,services,reads?halo2::SourcePhysicsContactRecordObserver{}:halo2::SourcePhysicsContactRecordObserver(' + callback + ')')
    text = once(text, '    const auto& expected_stores=row.at("semantic_writes");',
                '    if(!reads){const auto& expected_stores=row.at("semantic_writes");')
    text = once(text, '    std::set<std::uint32_t> private_final;',
                '    }\n    std::set<std::uint32_t> private_final;')
    text = once(text, 'int main(int argc,char**argv)try{',
                '#include "native_lift_authored_delivery_model_tu15.hpp"\nint main(int argc,char**argv)try{')
    text = once(text, '  std::cout<<J{{"passed",true},',
                '  const auto delivery_models=run_authored_delivery_models(fixture);\n  std::cout<<J{{"native_delivery_models",delivery_models},{"passed",true},')
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--build', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--quick', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.source, args.build, args.output = (p.resolve() for p in (args.source, args.build, args.output))
    args.output.mkdir(parents=True, exist_ok=False)
    report = dict(passed=False, scope=__doc__, full_parent_native=False,
                  generated_parent_preserved=True, live_admission=False, root_modified=False,
                  owned_scratch=str(scratch), retained_large_artifacts=[],
                  validator_sha256=generator.lift.sha(Path(__file__)))
    archive_pins = {}
    pins = {}
    tool_paths = (Path(__file__), Path(generator.__file__), Path(legacy.__file__), Path(legacy.base.__file__))
    tool_pins = {str(path): generator.lift.sha(path) for path in tool_paths}
    report['tool_source_sha256'] = tool_pins
    try:
        manifest = json.loads((args.source.parent / 'staging/source-pins.json').read_text())
        assert manifest['source_only'] and not manifest['live_qualified']
        assert all(generator.lift.sha(args.source / name) == value for name, value in manifest['candidate_source_sha256'].items())
        report['staging_manifest_sha256'] = generator.lift.sha(args.source.parent / 'staging/source-pins.json')
        copies = scratch / 'libraries'
        copies.mkdir()
        archives = []
        borrowed = []
        for name in LIBRARIES:
            origin = args.build / ('libhalo2_' + name + '.a')
            destination = copies / origin.name
            before = generator.lift.sha(origin)
            shutil.copyfile(origin, destination)
            assert generator.lift.sha(destination) == generator.lift.sha(origin) == before
            archive_pins[str(destination)] = before
            borrowed.append(dict(origin=str(origin), copied_to=str(destination), sha256=before,
                                 origin_verified_before_after_copy=True, bytes=destination.stat().st_size))
            archives.append(destination)
        report['read_only_archive_copies'] = borrowed
        rows = generator.controls(args.quick)
        fixture = generator.physical_fixture(rows, scratch, report)
        fixture_path = scratch / 'authored-parent.json'
        fixture_path.write_text(json.dumps(fixture, separators=(',', ':')) + '\n')
        report.update(fixture_sha256=generator.lift.sha(fixture_path), declared_cases=len(rows),
                      authored_families=len({row['authored_family'] for row in rows}),
                      body_types=sorted({row['root_type'] for row in rows}),
                      body_branches=dict(generator.collections.Counter(row['body_branch'] for row in rows)),
                      full_original_returns=len(fixture['cases']))
        cpp = scratch / 'parent.cpp'
        cpp.write_text(driver())
        force = scratch / 'force.cpp'
        original_force = args.source / 'src/physics/physics_phantom_force_tu15_generated.cpp'
        force.write_bytes(original_force.read_bytes())
        inputs = [cpp, force, args.source / 'src/physics/native_tu15_object_activation_residuals.cpp',
                  args.source / 'src/physics/native_tu15_phantom_body_velocity.cpp',
                  args.source / 'src/physics/native_tu15_island_activation.cpp',
                  ROOT / 'src/physics/native_lift_scalar_consumer_tu15.cpp',
                  ROOT / 'src/physics/native_tu15_array_growth.cpp', ROOT / 'src/physics/native_tu15_array_allocator.cpp',
                  ROOT / 'src/campaign/ai/native_campaign_object_marker_request_tu15.cpp',
                  ROOT / 'src/campaign/ai/native_campaign_object_markers_tu15.cpp',
                  ROOT / 'src/campaign/ai/native_campaign_object_motion_refs_tu15.cpp',
                  ROOT / 'src/core/native_tu15_data_array_allocate.cpp',
                  ROOT / 'src/core/native_tu15_data_array_delete.cpp',
                  ROOT / 'src/core/native_tu15_data_array_iteration.cpp']
        pins = {str(path): generator.lift.sha(path) for path in inputs}
        report['compiled_input_sha256'] = pins
        # Compile a coherent private header/source cohort. Concurrent work in
        # ROOT after this copy cannot replace a header beneath the compiler.
        frozen = scratch / 'compile-source'
        header_pins = {}
        headers = [p for directory in (ROOT / 'include', ROOT / 'src') for p in directory.rglob('*')
                   if p.is_file() and (directory == ROOT / 'include' or p.suffix in ('.hpp', '.h', '.inc', '.def', '.inl', '.ipp', '.tcc'))]
        root_inputs = [p for p in inputs if p.is_relative_to(ROOT) and not p.is_relative_to(args.source)]
        for path in (*headers, *root_inputs, ROOT / 'tools/native_lift_authored_delivery_model_tu15.hpp'):
            name = path.relative_to(ROOT)
            before = generator.lift.sha(path)
            copied = frozen / name
            copied.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, copied)
            assert generator.lift.sha(copied) == generator.lift.sha(path) == before
            header_pins[str(name)] = before
        assert all(generator.lift.sha(ROOT / name) == value for name, value in header_pins.items())
        (args.output / 'frozen-current-inputs.json').write_text(json.dumps(header_pins, indent=2) + '\n')
        report['frozen_current_inputs'] = dict(path='frozen-current-inputs.json', sha256=generator.lift.sha(args.output / 'frozen-current-inputs.json'), files=len(header_pins))
        for name, value in manifest['candidate_source_sha256'].items():
            copied = frozen / name
            copied.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(args.source / name, copied)
            assert generator.lift.sha(copied) == value
        # Test driver stays in this same owner. All production units and
        # their neighboring quoted includes now come from the frozen tree.
        inputs = [path if path in (cpp, force) else frozen / (path.relative_to(args.source) if path.is_relative_to(args.source) else path.relative_to(ROOT)) for path in inputs]
        extra = ROOT / 'tools/native_lift_authored_delivery_model_tu15.hpp'
        report['delivery_model_sha256'] = generator.lift.sha(extra)
        report['results'] = []
        for compiler in ('g++', 'clang++'):
            binary = scratch / compiler
            command = [compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
                       '-ffunction-sections', '-fdata-sections', '-I' + str(frozen / 'include'),
                       '-I' + str(frozen / 'src'), '-I' + str(frozen / 'src/physics'), '-I' + str(frozen / 'tools'),
                       *map(str, inputs), '-Wl,--gc-sections', '-Wl,--start-group', *map(str, archives),
                       '-Wl,--end-group', '-pthread', '-o', str(binary)]
            p = subprocess.run(command, capture_output=True, text=True)
            (args.output / (compiler + '-build.log')).write_text(p.stdout + p.stderr)
            assert p.returncode == 0, p.stderr[-7000:]
            p = subprocess.run([str(binary), str(fixture_path)], capture_output=True, text=True)
            (args.output / (compiler + '-run.log')).write_text(p.stdout + p.stderr)
            assert p.returncode == 0, p.stderr[-7000:]
            report['results'].append(dict(compiler=compiler, passed=True, detail=json.loads(p.stdout), command=command))
        assert all(generator.lift.sha(Path(name)) == value for name, value in archive_pins.items())
        assert all(generator.lift.sha(Path(name)) == value for name, value in pins.items())
        assert all(generator.lift.sha(args.source / name) == value for name, value in manifest['candidate_source_sha256'].items())
        assert generator.lift.sha(extra) == report['delivery_model_sha256']
        assert all(generator.lift.sha(Path(name)) == value for name, value in tool_pins.items())
        report['passed'] = True
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(path.stat().st_blocks * 512 for path in scratch.rglob('*') if path.is_file())
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
