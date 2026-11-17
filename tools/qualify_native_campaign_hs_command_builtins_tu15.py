#!/usr/bin/env python3
"""Owned native HS command compiler, mutation and sanitizer checks with borrowed closure."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path('src/campaign/scripting/native_campaign_hs_command_builtins_tu15.cpp')
DRIVER = Path('tests/native_campaign_hs_command_builtins_tu15_tests.cpp')
MUTANTS = (
    ('moving_field', '(moving?0x81u:0x76u)', '(moving?0x80u:0x76u)'),
    ('failsafe_field', '(moving?0x81u:0x76u)', '(moving?0x81u:0x75u)'),
    ('argument_value', 'g[0]=(g[0]&0xffffff00u)|read(g[0],1);', 'g[0]=(g[0]&0xffffff00u)|0u;'),
    ('command_sentinel', 'g[1]=read(0x50eee8u);\n            if(g[1]!=0xffffffffu) {', 'g[1]=read(0x50eee8u);\n            if(false && g[1]!=0xffffffffu) {'),
    ('sleep_duration', 'g[0]+g[2]+8u,0xfffffffeu', 'g[0]+g[2]+8u,0xffffffffu'),
    ('flag_callee_pop', 'g[6]=pop();pop();g[4]+=12u;\n    }\n    void face', 'g[6]=pop();pop();g[4]+=8u;\n    }\n    void face'),
)

FACE_MUTANTS = (
    ('face_enable_argument','g[1]=read(0x50eee8u);g[2]=(g[2]&0xffffff00u)|read(g[0],1);','g[1]=read(0x50eee8u);g[2]=(g[2]&0xffffff00u)|0u;'),
    ('face_point_argument','g[7]=read(g[0]+4u);g[6]=g[1];','g[7]=read(g[0]+8u);g[6]=g[1];'),
    ('face_primary_flag','write(entry+0x5du,g[6]+0x50u,g[0],1);','write(entry+0x5du,g[6]+0x50u,0,1);'),
    ('face_secondary_flag','write(entry+0x60u,g[6]+0x51u,g[0],1);','write(entry+0x60u,g[6]+0x51u,0,1);'),
    ('face_enable_gate','if(g[2]&0xffu) {','if(!(g[2]&0xffu)) {'),
    ('face_callee_pop','g[3]=pop();pop();g[4]+=12u;','g[3]=pop();pop();g[4]+=8u;'),
)

LOOK_MUTANTS = (
    ('looking_field','(looking?0x80u:', '(looking?0x7fu:'),
    ('look_point_kind','entry==0x2b1000u?2u:1u', 'entry==0x2b1000u?3u:1u'),
    ('look_object_kind','entry==0x2b1000u?2u:1u', 'entry==0x2b1000u?2u:3u'),
    ('look_target','write(entry+0x5bu,g[0]+0x4cu,g[2]);', 'write(entry+0x5bu,g[0]+0x4cu,0);'),
    ('aim_object_target','push(0x2b11dbu,g[7]);g[7]=read(g[0]+4u);', 'push(0x2b11dbu,g[7]);g[7]=read(g[0]+8u);'),
    ('aim_object_enabled','g[2]=(g[2]&0xffffff00u)|read(g[0],1);push(0x2b11dbu,g[7]);', 'g[2]=(g[2]&0xffffff00u)|0u;push(0x2b11dbu,g[7]);'),
    ('face_object_child','entry==0x2b1200u?0x280c90u:0x280d70u','entry==0x2b1200u?0x280c90u:0x280c90u'),
    ('face_object_primary','write(entry+0x5du,g[6]+0x50u,g[0],1);','write(entry+0x5du,g[6]+0x50u,0,1);'),
    ('face_object_secondary','write(entry+0x60u,g[6]+0x51u,g[0],1);','write(entry+0x60u,g[6]+0x51u,0,1);'),
)

GO_FACE_MUTANTS = (
    ('go_face_kind','write(0x2b0e7eu,g[0]+4u,3,2);','write(0x2b0e7eu,g[0]+4u,2,2);'),
    ('go_face_destination','write(0x2b0e84u,g[0]+0x28u,g[2]);','write(0x2b0e84u,g[0]+0x28u,0);'),
    ('go_face_facing','write(0x2b0e87u,g[0]+0x2cu,g[1]);','write(0x2b0e87u,g[0]+0x2cu,0);'),
    ('go_face_yield','write(0x2b0ea8u,g[6]+g[2]+8u,0xfffffffdu);','write(0x2b0ea8u,g[6]+g[2]+8u,0xfffffffeu);'),
    ('go_face_argument_order','g[1]=read(g[0]+4u);g[2]=read(g[0]);','g[1]=read(g[0]);g[2]=read(g[0]+4u);'),
    ('go_face_callee_pop','g[6]=pop();pop();g[4]+=12u;\n    }\n    void sleep_forever', 'g[6]=pop();pop();g[4]+=8u;\n    }\n    void sleep_forever'),
)



def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', required=True, type=Path)
    parser.add_argument('--fixture', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--baseline-only', action='store_true')
    parser.add_argument('--face', action='store_true')
    parser.add_argument('--look', action='store_true')
    parser.add_argument('--go-face', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    build = args.build.resolve()
    assert (build.parent / 'source').exists() and '.local/test-scratch/' in str(build), 'Only an owned build may supply dependencies'
    fixture = scratch / 'fixture.json'
    fixture.write_bytes(gzip.decompress(args.fixture.read_bytes()))
    link = shlex.split((build / 'CMakeFiles/native_campaign_hs_command_builtins_tu15_tests.dir/link.txt').read_text())
    link = [x for x in link if not x.startswith('-Wl,--dependency-file=')]
    for n, value in enumerate(link):
        if not value.startswith('-') and not Path(value).is_absolute() and value.endswith(('.o', '.a')):
            link[n] = str(build / value)
    source_index = next((n for n, value in enumerate(link) if value.endswith('/' + str(SOURCE) + '.o')),None)
    driver_index = next(n for n, value in enumerate(link) if value.endswith('/' + str(DRIVER) + '.o'))
    output_index = link.index('-o') + 1
    original = (ROOT / SOURCE).read_text()
    report = dict(passed=False, results=[], commands=[], source_sha256={str(p): sha(ROOT / p) for p in
                  (SOURCE, DRIVER, Path(__file__).relative_to(ROOT))}, fixture_sha256=sha(args.fixture),
                  owned_scratch=str(scratch), borrowed_owned_build=str(build), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)

    def save():
        args.report.write_text(json.dumps(report, indent=2) + '\n')

    def run(command, expected=0, env=None):
        report['commands'].append(command)
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, env=env)
        if expected == 0 and result.returncode != 0:
            report['failure'] = dict(command=command, returncode=result.returncode,
                                     stdout=result.stdout[-8000:], stderr=result.stderr[-8000:])
            save()
            raise RuntimeError(result.stdout[-8000:] + result.stderr[-8000:])
        return result

    configurations = [('baseline', None, None)]
    if not args.baseline_only:
        configurations += list(GO_FACE_MUTANTS if args.go_face else LOOK_MUTANTS if args.look else FACE_MUTANTS if args.face else MUTANTS) + [('asan_ubsan', None, None)]
    for name, old, new in configurations:
        changed = original
        if old is not None:
            assert original.count(old) == 1, (name, original.count(old))
            changed = original.replace(old, new)
        source = scratch / (name + '.cpp')
        obj = scratch / (name + '.o')
        driver = scratch / ('sanitized-driver.o' if name == 'asan_ubsan' else 'driver.o')
        probe = scratch / (name + '.probe')
        source.write_text(changed)
        sanitizer = name == 'asan_ubsan'
        options = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if sanitizer else []
        common = ['c++', '-std=c++20', '-O1' if sanitizer else '-O2', '-fno-fast-math', '-ffp-contract=off',
                  '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                  '-I' + str(ROOT / 'src/campaign/ai'), *options]
        run([*common, '-c', str(source), '-o', str(obj)])
        if not driver.exists():
            run([*common, '-c', str(ROOT / DRIVER), '-o', str(driver)])
        command = link.copy()
        command[driver_index] = str(driver)
        command[output_index] = str(probe)
        if source_index is None:
            # The runtime archive owns this production source. An explicit
            # replacement before the archive resolves its symbols, leaving
            # that archive member unextracted; every other child is borrowed.
            command.insert(1,str(obj))
        else:
            command[source_index] = str(obj)
        run(command + options)
        result = run([str(probe), str(fixture)], expected=1 if old else 0,
                     env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0:halt_on_error=1', 'UBSAN_OPTIONS': 'halt_on_error=1'})
        if old:
            assert result.returncode != 0, name + ' survived'
        report['results'].append(dict(name=name, passed=True, rejected=bool(old), returncode=result.returncode,
                                      stdout=result.stdout[-2000:], stderr=result.stderr[-2000:]))
        save()
        print(json.dumps(report['results'][-1]), flush=True)
        for generated in (probe, obj, source):
            generated.unlink()
    assert sha(ROOT / SOURCE) == report['source_sha256'][str(SOURCE)], 'HS builtin source changed during qualification'
    report['passed'] = True
    save()


if __name__ == '__main__':
    main()
