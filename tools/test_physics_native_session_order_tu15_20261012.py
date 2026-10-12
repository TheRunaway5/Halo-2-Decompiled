#!/usr/bin/env python3
"""Check session-before-world order using the immutable fresh physics objects.

Only the diagnostic test's initialization order changes. All native runtime
objects and headers remain the pinned source-v2 cohort. This does not exercise
the complete Match constructor, player birth or populated-world physics.
"""
import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PARENT = ROOT/'analysis/multiplayer/sandbox-source/physics-native-frame-integration-20261012'


def sha(p):
    h = hashlib.sha256()
    with Path(p).open('rb') as f:
        while b := f.read(1 << 20):
            h.update(b)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    a.output = a.output.resolve()
    a.output.mkdir(parents=True, exist_ok=False)
    manifest = json.loads((PARENT/'source-manifest-v2.json').read_text())
    for name, pin in {**manifest['headers'], **manifest['files']}.items():
        assert sha(PARENT/'source-v2'/name) == pin, name
    old = (PARENT/'source-v2/tests/physics_native_frame_tu15_tests.cpp').read_text()
    before = '''            world.initialize_map();
            rejected=false;
            try {world.step_physics();}catch(const halo2::FormatError&) {rejected=true;}
            check(rejected,"Physics frame admitted a missing source session");
            world.configure_session(halo2::source_game_variant_default(0),1,30,0x12345678u);'''
    after = '''            world.configure_session(halo2::source_game_variant_default(0),1,30,0x12345678u);
            rejected=false;
            try {world.step_physics();}catch(const halo2::FormatError&) {rejected=true;}
            check(rejected,"Physics frame admitted a missing source world");
            world.initialize_map();'''
    assert old.count(before) == 1
    source = a.output/'session_order.cpp'
    source.write_text(old.replace(before, after))
    result = dict(scope=__doc__, tool_sha256=sha(__file__),
                  parent_manifest_sha256=sha(PARENT/'source-manifest-v2.json'),
                  diagnostic_source_sha256=sha(source),
                  source_changes='Session configuration precedes world construction; rejection checks the missing world',
                  all_other_compiled_sources_unchanged=True, builds={})
    xbe = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
    assert sha(xbe) == '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    cache = PARENT/'source-v2/reference-libraries/libhalo2_cache.a'
    assert sha(cache) == manifest['archives']['libhalo2_cache.a']['sha256']
    for mode in ('release', 'sanitizer'):
        b = PARENT/f'build-{mode}-v2'
        objects = sorted((b/'CMakeFiles/physics_native_frame_tu15_tests.dir/src').glob('*.cpp.o'))
        assert len(objects) == 43
        pins = {str(p): sha(p) for p in objects}
        commands = json.loads((b/'compile_commands.json').read_text())
        assert len(commands) == 44
        for row in commands:
            p = Path(row['file'])
            assert p.is_relative_to(PARENT/'source-v2')
            assert sha(p) == manifest['files'][str(p.relative_to(PARENT/'source-v2'))]
            if mode == 'sanitizer':
                assert '-fsanitize=address,undefined' in row['command']
        binary = a.output/f'frame-session-{mode}'
        flags = ['-std=c++20', '-O0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math']
        if mode == 'sanitizer':
            flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        command = ['c++', *flags, '-I'+str(PARENT/'source-v2/include'), str(source),
                   *map(str, objects), str(cache), '-pthread', '-lz', '-o', str(binary)]
        with (a.output/f'compile-{mode}.log').open('w') as log:
            subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
        env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1')
        cases = []
        for map_name in ('lockout', 'coagulation'):
            map_path = ROOT/f'assets/disc/maps/{map_name}.map'
            cmd = [str(binary), str(xbe), str(map_path)]
            if mode == 'release':
                cmd.append(str(a.output/f'{map_name}-frames'))
            log = a.output/f'{mode}-{map_name}.log'
            with log.open('w') as out:
                subprocess.run(cmd, env=env, check=True, stdout=out, stderr=subprocess.STDOUT, timeout=120)
            assert '120 source frames, 20880 ordered writes' in log.read_text()
            cases.append(dict(map=map_name, map_sha256=sha(map_path), passed=True, frames=120, stores=20880, log_sha256=sha(log)))
        assert pins == {str(p): sha(p) for p in objects}, 'Pinned physics objects changed'
        result['builds'][mode] = dict(binary_sha256=sha(binary), runtime_objects=pins,
                                     compile_command=command, compile_commands_sha256=sha(b/'compile_commands.json'),
                                     cases=cases, cache_decoder_instrumented=False)
    result.update(bios_runtime_required=False, full_multiplayer_physics_parity=False,
                  full_match_initialization_tested=False, populated_world_tested=False)
    (a.output/'report.json').write_text(json.dumps(result, sort_keys=True, indent=2)+'\n')
    print('PASS session-before-world: both maps, Release and address/undefined/leak checks')


if __name__ == '__main__':
    main()
