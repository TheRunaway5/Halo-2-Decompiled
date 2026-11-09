#!/usr/bin/env python3
"""Qualify the isolated current-source lift velocity/counter port, without live admission."""
import argparse
import difflib
import hashlib
import inspect
import json
import os
from pathlib import Path
import subprocess
import sys

import validate_native_phantom_body_velocity_counter_tu15 as counter
import validate_native_phantom_body_velocity_tu15 as body

ROOT = body.ROOT
QUALIFIED = ROOT / 'analysis/physics/native-phantom-body-velocity-qualified-20261103/source'
PARENT = ROOT / 'analysis/physics/native-phantom-body-velocity-parent-stage-v2-20261103/staged'
NEW = ('include/halo2/native_tu15_phantom_body_velocity.hpp',
       'include/halo2/native_tu15_retained_counter_update.hpp',
       'src/physics/native_tu15_phantom_body_velocity.cpp')
CORE = ('include/halo2/source_runtime_memory.hpp', 'src/runtime/source_runtime_memory.cpp',
        'src/physics/native_tu15_island_activation.cpp')
VIEWS = ('include/halo2/native_tu15_contact_candidate_private_data.hpp',
         'src/physics/physics_game_arena_native.cpp')
CONTEXT = ('src/runtime/source_runtime_write_protection.inc',
           'src/runtime/source_runtime_host_physical_memory.cpp',
           'src/runtime/source_runtime_host_storage.cpp',
           'src/platform/native_synchronization.cpp', 'src/platform/native_threads.cpp',
           'include/halo2/native_tu15_retained_store_observer.hpp',
           'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',
           'tests/native_tu15_array_growth_tests.cpp')
EXPECTED_CURRENT = {
    'include/halo2/source_runtime_memory.hpp': '82057bed1632ed9ffa97f4bc69b82704b91b629b142f309183ff3cbd717f7cf9',
    'src/runtime/source_runtime_memory.cpp': '793445346ceb67969a1f8637943f826974ff2cf97a5a9a7dce68d934da802bdf',
    'src/physics/native_tu15_island_activation.cpp': '3ee17c6125b2b98b97f98ec52ea102524c77cda515e7c8520db9974a68aee0cd',
    'include/halo2/native_tu15_contact_candidate_private_data.hpp': '8f37457f1d9a579ae3d6f5f4d385d3507b26a8ab0f012a5fc38a66312be2f7d1',
    'src/physics/physics_game_arena_native.cpp': 'd4e440320c4ea62c303908bd7a96e613257065feaeecc952ecf90d0a431b2f4e',
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


def prepare(stage, output):
    """Apply only the independently proved additions to current source bytes."""
    pins = {name: sha(ROOT / name) for name in CORE + VIEWS + CONTEXT}
    assert all(pins[name] == value for name, value in EXPECTED_CURRENT.items()), 'Current source differs from reviewed port base'
    manifest = json.loads((QUALIFIED.parent / 'source-pins.json').read_text())
    assert manifest['qualified'] and manifest['source_original'] == '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    assert all(sha(QUALIFIED / name) == value for name, value in manifest['source_sha256'].items())
    parent = json.loads((PARENT.parent / 'source-pins.json').read_text())
    assert parent['qualified'] and not parent['live_qualified']
    assert all(sha(PARENT / name) == parent['source_sha256'][name] for name in VIEWS)
    diffs = []
    for name in NEW + CORE + VIEWS:
        original = (ROOT / name).read_text() if name not in NEW else ''
        path = (PARENT if name in VIEWS else QUALIFIED) / name
        candidate = path.read_text()
        # A current source change must be reviewed, never replaced with a stale
        # whole-file snapshot. Every old line in the accepted hunk must match.
        if name in NEW:
            assert not (ROOT / name).exists(), 'Port already present: ' + name
        else:
            delta = list(difflib.ndiff(original.splitlines(keepends=True), candidate.splitlines(keepends=True)))
            assert not any(line.startswith('- ') for line in delta if line[2:].strip()
                           and not any(key in line for key in (
                               'class SourceRuntimeMemory final',
                               'public NativeTu15RetainedStoreObservationDomain',
                               'struct SharedMemory final',
                               'm.word(0x316d6bu,', 'm.word(0x316d8au,',
                               'm.word(0x31663au,'))), 'Unreviewed source drift: ' + name
        target = stage / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(candidate)
        diffs.extend(difflib.unified_diff(original.splitlines(keepends=True), candidate.splitlines(keepends=True),
                                         fromfile='current/' + name, tofile='candidate/' + name))
    # The qualified write-policy implementation is still identical. Preserve
    # the current owner rather than copying a different guard implementation.
    name = 'src/runtime/source_runtime_write_protection.inc'
    assert sha(ROOT / name) == sha(QUALIFIED / name)
    (stage / name).write_bytes((ROOT / name).read_bytes())
    (output / 'candidate.diff').write_text(''.join(diffs))
    return pins


CONTROL_DRIVER = r'''
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/native_tu15_retained_counter_update.hpp"
#include "halo2/native_tu15_contact_candidate_private_data.hpp"
#include "halo2/physics_game_arena_native.hpp"
#include <atomic>
#include <cstring>
#include <iostream>
#include <thread>
using namespace halo2;
void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
unsigned word(SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);unsigned n{};std::memcpy(&n,b.data(),4);return n;}
struct Unknown final:SourcePhysicsContactMemory {
 mutable unsigned reads{};unsigned writes{};
 void read(std::uint32_t,std::span<std::byte>)const override{++reads;throw std::runtime_error("Unexpected unknown read");}
 void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw std::runtime_error("Unexpected unknown write");}
};
int main(int argc,char**argv)try{
 check(argc==2,"Supply owned scratch directory");
 constexpr unsigned p=0x100000u,private_first=0x200000u;
 SourceRuntimeMemory memory;std::array<std::byte,4> initial{};memory.mount(p,initial,SourceRuntimeMemory::Access::ReadWrite,"declared retained counter");
 unsigned guards{},audits{},events{};bool refuse{};
 source_runtime_register_write_guard(memory,&guards,[&](auto address,auto bytes){check(address==p&&bytes==4,"Forwarded whole operand differs");++guards;if(refuse)throw std::runtime_error("Declared policy refusal");});
 source_runtime_register_write_observer(memory,&audits,[&](auto,auto){++audits;});
 const std::array owned{NativeTu15ContactPrivateRange{private_first,16}};
 NativeTu15ContactCandidatePrivateData view(memory,owned,{});
 const SourcePhysicsContactRecordObserver observed=[&](const auto&e){++events;check(e.address==p&&e.instruction==0x316d6eu&&word(memory,p)==1,"Event must follow exact committed counter");};
 try{native_tu15_adjust_retained_counter(memory,0x316d6eu,p,4,1,{});}
 catch(const std::runtime_error&e){std::cerr<<"CURRENT_STORAGE_RED: "<<e.what()<<'\n';return 31;}
 check(word(memory,p)==1&&guards==1&&audits==1,"Ordinary storage did not commit once");
 auto*storage=dynamic_cast<NativeTu15RetainedCounterStorage*>(&memory);check(storage,"Qualified storage capability absent");
 storage->adjust_counter(p,4,0xffffffffu);guards=audits=0;
 try{native_tu15_adjust_retained_counter(view,0x316d6eu,p,4,1,observed);}
 catch(const std::runtime_error&e){std::cerr<<"CURRENT_PRIVATE_VIEW_RED: "<<e.what()<<'\n';return 32;}
 check(word(memory,p)==1&&guards==1&&audits==1&&events==1,"Private view repeated policy or observer");
 const auto reject=[&](auto operation,const char*message){bool rejected{};try{operation();}catch(const std::runtime_error&){rejected=true;}check(rejected,message);};
 refuse=true;reject([&]{native_tu15_adjust_retained_counter(view,0x316d6eu,p,4,1,observed);},"Policy refusal absent");
 check(word(memory,p)==1&&guards==2&&audits==1&&events==1,"Refused view update committed or notified");
 refuse=false;
 for(auto [address,width]:{std::pair{private_first,4u},std::pair{private_first-1,2u},std::pair{private_first+15,2u}})
   reject([&]{native_tu15_adjust_retained_counter(view,0x316d6eu,address,width,1,{});},"Private/crossing counter incorrectly admitted");
 check(guards==2&&audits==1&&events==1,"Private refusal touched retained policy");
 Unknown unknown;reject([&]{native_tu15_adjust_retained_counter(unknown,0x316d6eu,p,4,1,{});},"Unknown storage admitted");
 NativeTu15ContactCandidatePrivateData unavailable(unknown,owned,{});
 reject([&]{native_tu15_adjust_retained_counter(unavailable,0x316d6eu,p,4,1,{});},"Unknown outer storage admitted");
 check(!unknown.reads&&!unknown.writes,"Unknown storage accessed DATA before refusal");
 source_runtime_unregister_write_observer(memory,&audits);source_runtime_unregister_write_guard(memory,&guards);
 // Use the real public Arena owner. No startup program or source worker is
 // launched, and its shared memory forwarding executes under its own mutex.
 SourcePhysicsGameArenaNative arena(memory,{std::filesystem::path(argv[1]),0x300000u,{},{}});
 auto&shared=arena.memory();auto*serialized=dynamic_cast<NativeTu15RetainedCounterStorage*>(&shared);
 if(!serialized){std::cerr<<"CURRENT_ARENA_RED: missing checked counter storage\n";return 33;}
 native_tu15_adjust_retained_counter(shared,0x316d6eu,p,4,0xffffffffu,{});
 check(word(shared,p)==0,"Arena did not forward retained update");
 std::thread first([&]{for(unsigned n=0;n<2000;++n)serialized->adjust_counter(p,4,1);});
 std::thread second([&]{for(unsigned n=0;n<2000;++n)serialized->adjust_counter(p,4,1);});
 first.join();second.join();check(word(shared,p)==4000,"Arena RMW lost an update");
 std::cout<<"ordinary/private/unknown/Arena forwarding: GREEN; 4000 serialized updates\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''


def controls(stage, scratch, output):
    cpp = scratch / 'storage-controls.cpp'
    cpp.write_text(CONTROL_DRIVER)
    sources = ('src/runtime/source_runtime_memory.cpp', 'src/physics/physics_game_arena_native.cpp',
               'src/runtime/source_runtime_host_physical_memory.cpp', 'src/runtime/source_runtime_host_storage.cpp',
               'src/platform/native_synchronization.cpp', 'src/platform/native_threads.cpp')
    results = []
    for compiler in ('g++', 'clang++'):
        for variant in ('current', 'storage-only', 'storage-private', 'candidate'):
            include = scratch / (compiler + '-' + variant + '-include')
            include.mkdir()
            # A distinct header selection tests each actually missing adapter.
            names = NEW[:2] + (('include/halo2/source_runtime_memory.hpp',) if variant != 'current' else ())
            if variant in ('storage-private', 'candidate'):
                names += ('include/halo2/native_tu15_contact_candidate_private_data.hpp',)
            for name in names:
                target = include / Path(name).relative_to('include')
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes((stage / name).read_bytes())
            units = [str((stage if ((name.endswith('source_runtime_memory.cpp') and variant != 'current') or
                                    (name.endswith('physics_game_arena_native.cpp') and variant == 'candidate')) else ROOT) / name)
                     for name in sources]
            binary = scratch / (compiler + '-' + variant)
            cmd = [compiler, '-std=c++20', '-O2', '-pthread', '-ffunction-sections', '-fdata-sections',
                   '-Wl,--gc-sections', '-I' + str(include), '-I' + str(ROOT / 'include'),
                   '-I' + str(ROOT / 'src'), str(cpp), *units, '-o', str(binary)]
            p = subprocess.run(cmd, capture_output=True, text=True)
            (output / (binary.name + '-compile.log')).write_text(p.stdout + p.stderr)
            assert p.returncode == 0, p.stderr[-5000:]
            p = subprocess.run([str(binary), str(scratch)], capture_output=True, text=True)
            (output / (binary.name + '.log')).write_text(p.stdout + p.stderr)
            expected = {'current': (31, 'CURRENT_STORAGE_RED'), 'storage-only': (32, 'CURRENT_PRIVATE_VIEW_RED'),
                        'storage-private': (33, 'CURRENT_ARENA_RED'), 'candidate': (0, 'GREEN')}[variant]
            assert p.returncode == expected[0] and expected[1] in p.stdout + p.stderr, (variant, p.returncode, p.stderr)
            results.append(dict(compiler=compiler, variant=variant, returncode=p.returncode,
                                expected=expected[1], command=cmd))
    return results


def current_original(image, banks, rows, scratch):
    # The current physical runner has an explicit ordinary entry-register
    # input API. Use it instead of the old validator's textual assembly splice.
    original = inspect.getsource(body.base.original)
    assert "initial_registers=case.get('input_registers',{})" in original
    for row in rows:
        row['input_registers'] = {name: row[name] for name in ('eax', 'edx', 'esi')}
    environment = dict(body.base.__dict__, OWNERS=body.OWNERS)
    exec(inspect.getsource(body.base.metadata), environment)
    exec(original, environment)
    return environment['original'](image, banks, rows, scratch)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    phases = parser.add_mutually_exclusive_group()
    phases.add_argument('--storage-only', action='store_true')
    phases.add_argument('--physical-only', action='store_true')
    parser.add_argument('--storage-receipt', type=Path)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    stage = scratch / 'source'
    stage.mkdir()
    report = dict(passed=False, scope=__doc__, live_admission=False, source_changes_in_root=False,
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    pins = {}
    try:
        pins = prepare(stage, args.output)
        report['current_source_sha256'] = pins
        report['candidate_source_sha256'] = {name: sha(stage / name) for name in NEW + CORE + VIEWS}
        if args.physical_only:
            assert args.storage_receipt, 'Supply the existing exact source storage receipt'
            previous = json.loads(args.storage_receipt.read_text())
            assert previous['passed'] and previous['current_source_sha256'] == pins
            assert previous['candidate_source_sha256'] == report['candidate_source_sha256']
            report['storage_controls_receipt'] = {'path': str(args.storage_receipt), 'sha256': sha(args.storage_receipt)}
        else:
            report['storage_controls'] = controls(stage, scratch, args.output)
        if not args.storage_only:
            # Reuse the independently reviewed physical instruction/read/store
            # oracle, with current source plus only the minimal counter port.
            text = inspect.getsource(counter.main)
            text = once(text, "    text = inspect.getsource(base.main)", "    text = inspect.getsource(base.main)")
            body.original = current_original
            environment = dict(counter.__dict__, STAGE=stage)
            exec(text, environment)
            argv = sys.argv
            try:
                sys.argv = [__file__, '--output', str(args.output / 'physical')]
                environment['main']()
            finally:
                sys.argv = argv
            report['physical'] = json.loads((args.output / 'physical/report.json').read_text())
            assert report['physical']['passed']
        assert all(sha(ROOT / name) == value for name, value in pins.items()), 'Current input source changed during qualification'
        report['passed'] = True
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(p.stat().st_blocks * 512 for p in scratch.rglob('*') if p.is_file())
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
