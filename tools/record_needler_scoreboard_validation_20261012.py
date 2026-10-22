#!/usr/bin/env python3
"""Record the tested weapon delivery and its preserved scoreboard source."""
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / 'build-unit-object-20261011'
SOURCE = ROOT / 'analysis/scoreboard-emblems/20261012/verified-source'
OVERLAY = ROOT / '.local/needler-scoreboard-overrides-20261012'
BUILD = ROOT / 'build-needler-scoreboard-20261012'
LIVE = EVIDENCE / 'needler-scoreboard-final-20261012'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record(path):
    return dict(path=str(path.resolve()), bytes=path.stat().st_size, sha256=sha(path))


def write(path, data):
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n')


def main():
    results = json.loads((LIVE / 'results.json').read_text())
    assert {r['case'] for r in results} == {
        'needler-ammo', 'needler-fire', 'sword-ready', 'sword-strike', 'sword-world', 'smg-dual'}
    assert len(results) == 6
    for row in results:
        assert row['exit_code'] == 0 and row['validation_errors'] == 0
        assert '\nPASS ' in Path(row['log']).read_text()
        assert str(BUILD) in ' '.join(row['command'])

    log_expectations = {
        'needler-scoreboard-focused-20261012.log': '100% tests passed',
        'needler-scoreboard-network-20261012.log': 'PASS actual native UDP',
        'needler-scoreboard-ui-20261012.log': '27 connection cases match unchanged Vista instructions',
        'needler-scoreboard-ui-gpu-20261012.log': 'PASS ',
    }
    for name, expected in log_expectations.items():
        text = (EVIDENCE / name).read_text()
        assert expected in text and 'Validation Error:' not in text

    # The delivered menu and scoreboard remain the accepted parent's exact code.
    accepted = ROOT / 'analysis/scoreboard-emblems/20261012/owned-change-hashes.json'
    preserved = json.loads(accepted.read_text())
    for name, expected in preserved.items():
        assert sha(SOURCE / name) == expected

    # Needler and Sword owners are the same files already exercised in the
    # preceding independent MCC checkpoint. Only the SMG fix is overlaid here.
    previous = ROOT / '.local/needler-mcc-source-20261012'
    unchanged = [
        'include/halo2/content.hpp', 'include/halo2/fixed_effect_event.hpp',
        'include/halo2/muzzle_effects.hpp', 'include/halo2/effect_sound.hpp',
        'src/rendering/effects/muzzle_effects.cpp', 'src/rendering/effects/effect_sound.cpp',
        'tests/needler_firing_tests.cpp', 'tests/native_covenant_weapon_runtime_tests.cpp',
        'tests/fixtures/needler_firing_effects_original.tsv',
        'tests/fixtures/weapon_fixed_events_original.tsv',
    ]
    for name in unchanged:
        assert sha(SOURCE / name) == sha(previous / name)

    files = {}
    for directory in ('include', 'src', 'shaders', 'cmake', 'tests', 'tools', 'resources'):
        for path in sorted((SOURCE / directory).rglob('*')):
            if path.is_file() and not path.is_symlink() and '__pycache__' not in path.parts:
                files[str(path.relative_to(SOURCE))] = sha(path)
    files['CMakeLists.txt'] = sha(SOURCE / 'CMakeLists.txt')
    external = {}
    for path in sorted((SOURCE / 'external/imgui').rglob('*')):
        if path.is_file() and path.suffix in ('.cpp', '.h', '.inl'):
            external[str(path.relative_to(SOURCE))] = sha(path)
    overlay_files = {}
    patches = []
    for name in ('src/gameplay/simulation.cpp', 'src/gameplay/multiplayer/multiplayer_runtime.cpp',
                 'tests/native_weapon_runtime_tests.cpp', 'tests/muzzle_effects_tests.cpp'):
        overlay_files[name] = sha(OVERLAY / name)
        diff = subprocess.run(['diff', '-u', str(SOURCE / name), str(OVERLAY / name)],
                              capture_output=True, text=True)
        assert diff.returncode == 1
        patch = EVIDENCE / ('needler-scoreboard-' + name.replace('/', '-') + '.patch')
        patch.write_text(diff.stdout)
        patches.append(record(patch))
    overlay_files['overrides.cmake'] = sha(OVERLAY / 'overrides.cmake')
    fixture = OVERLAY / 'tests/fixtures/weapon_fixed_events_original.tsv'
    assert sha(fixture) == sha(SOURCE / 'tests/fixtures/weapon_fixed_events_original.tsv')
    cache = (BUILD / 'CMakeCache.txt').read_text()
    assert 'CMAKE_HOME_DIRECTORY:INTERNAL=' + str(SOURCE) in cache
    assert str(OVERLAY / 'overrides.cmake') in cache
    manifest = EVIDENCE / 'needler-scoreboard-source-manifest-20261012.json'
    write(manifest, dict(source=str(SOURCE), files=files, shared_external_source=external,
                         source_overrides=str(OVERLAY), override_files=overlay_files,
                         fixed_event_fixture=record(fixture), owned_patches=patches,
                         preserved_parent=record(accepted), preserved_parent_hashes=preserved,
                         unchanged_weapon_files=unchanged, cmake_cache=record(BUILD / 'CMakeCache.txt')))
    assert all(sha(SOURCE / name) == expected for name, expected in files.items())
    assert all(sha(SOURCE / name) == expected for name, expected in external.items())

    launchers = {}
    for name in ('Halo2-Weapons-Launcher', 'Halo2-Weapons-Title-Launcher'):
        info = subprocess.run([str(ROOT / name), '--launcher-info'],
                              check=True, capture_output=True, text=True).stdout
        assert 'Runtime: ' + str(BUILD / 'halo2_native') in info
        log = EVIDENCE / ('needler-scoreboard-delivered-' + name + '-20261012.log')
        log.write_text(info)
        launchers[name] = dict(binary=record(ROOT / name), info=record(log))
    shared_launchers = {}
    for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
        info = subprocess.run([str(ROOT / name), '--launcher-info'],
                              check=True, capture_output=True, text=True).stdout
        log = EVIDENCE / ('needler-scoreboard-shared-' + name + '-20261012.log')
        log.write_text(info)
        shared_launchers[name] = dict(binary=record(ROOT / name), info=record(log))

    originals = []
    for name, tool, key in (
            ('weapon-fixed-events-20261012.json', 'oracle_weapon_fixed_events_1_1.py', 'tool_sha256'),
            ('needler-firing-effects-original-20261012.json', 'oracle_needler_firing_effects_1_1.py', 'oracle_sha256')):
        report = ROOT / 'analysis/multiplayer/integration' / name
        assert json.loads(report.read_text())[key] == sha(ROOT / 'tools' / tool)
        originals.append(record(report))

    target = EVIDENCE / 'needler-firing-validation-20261012.json'
    write(target, dict(
        scope='Normal dry local Needler firing, Sword ready/strike/world presentation and dual SMG regression. Full original game parity is not claimed.',
        source_manifest=record(manifest), runtime=record(BUILD / 'halo2_native'),
        launchers=launchers, original_reports=originals,
        independently_selected_shared_launchers=shared_launchers,
        live_gameplay_xbe=record(Path(next(p for p in results[0]['command'] if p.endswith('/default.xbe')))),
        checks=[record(EVIDENCE / name) for name in log_expectations],
        prior_focused_checks=[record(EVIDENCE / name) for name in (
            'needler-focused-regression-20261012.log', 'needler-ammunition-movement-final-20261012.log')],
        live=[dict(**row, evidence=record(Path(row['log']))) for row in results],
        screenshots=[record(LIVE / name) for name in ('needler-fire.bmp', 'sword-world.bmp')],
        rejected_capture=record(EVIDENCE / 'needler-firing-capture-20261012.log'),
        smg_duplicate_sound_red=record(EVIDENCE / 'smg-duplicate-sound-red-20261012.log'),
        tools=[record(ROOT / 'tools' / name) for name in (
            'validate_needler_runtime_20261012.py', 'record_needler_scoreboard_validation_20261012.py')],
        assets=[record(ROOT / name) for name in (
            'assets/disc/maps/lockout.map', 'assets/disc/maps/shared.map',
            'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
            'assets/mcc-classic-hud/manifest.json', 'assets/mcc-classic-menus/manifest.json')],
        limits=[
            'Dummy SDL accepted PCM is not physical audible output or original mixer identity.',
            'No original Xbox gameplay framebuffer or physical GPU comparison.',
            'Full Unit/pose/Havok, original allocation and attached/world effect ownership remain open.',
            'Fluid/media transitions, multi-user camera/world placement and render-independent emission remain open.',
            'Needler homing, attachment, contrail, impact/supercombine/detonation remain open.',
            'Sword lunge/power/blocking and looping blade sound lifecycle remain open.',
            'Native LAN retains one effect event per player; offhand and coalesced event histories are incomplete.',
        ]))
    print(json.dumps(dict(report=str(target), sha256=sha(target),
                          source_files=len(files), live_cases=len(results))))


if __name__ == '__main__':
    main()
