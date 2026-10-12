#!/usr/bin/env python3
"""Pin the bounded original Needler constructor and its tested native delivery."""
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / 'build-unit-object-20261011'
SOURCE = ROOT / 'analysis/scoreboard-emblems/20261012/verified-source'
OVERLAY = ROOT / '.local/needler-projectile-overrides-20261012'
BUILD = ROOT / 'build-needler-projectile-20261012'
LIVE = EVIDENCE / 'needler-projectile-final-20261012'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record(path):
    return dict(path=str(path.resolve()), bytes=path.stat().st_size, sha256=sha(path))


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def main():
    results = json.loads((LIVE / 'results.json').read_text())
    assert len(results) == 6
    assert {r['case'] for r in results} == {
        'needler-ammo', 'needler-fire', 'sword-ready', 'sword-strike', 'sword-world', 'smg-dual'}
    for row in results:
        assert row['exit_code'] == 0 and row['validation_errors'] == 0
        assert '\nPASS ' in Path(row['log']).read_text()
        assert str(BUILD) in ' '.join(row['command'])

    checks = {
        'needler-projectile-focused-20261012.log': '100% tests passed',
        'needler-projectile-birth-u5-tests-20261012.log': 'PASS 200 original Needler constructor cases',
        'needler-projectile-network-20261012.log': 'PASS actual native UDP',
        'needler-projectile-ui-20261012.log': '12 actual-asset PCR panes',
    }
    for name, expected in checks.items():
        text = (EVIDENCE / name).read_text()
        assert expected in text and 'Validation Error:' not in text, name

    # Revalidate the complete immutable parent, including accepted UI files.
    prior_manifest = EVIDENCE / 'needler-scoreboard-source-manifest-20261012.json'
    parent = json.loads(prior_manifest.read_text())
    for bank in ('files', 'shared_external_source'):
        assert all(sha(SOURCE / name) == expected for name, expected in parent[bank].items())
    accepted = ROOT / 'analysis/scoreboard-emblems/20261012/owned-change-hashes.json'
    accepted_hashes = json.loads(accepted.read_text())
    assert all(sha(SOURCE / name) == expected for name, expected in accepted_hashes.items())

    paths = [p for p in sorted(OVERLAY.rglob('*'))
             if p.is_file() and p.suffix in ('.hpp', '.cpp', '.cmake')]
    assert all(p.name != 'birth_probe.cpp' for p in paths)
    patches = []
    for path in paths:
        name = path.relative_to(OVERLAY)
        if path.suffix == '.cmake':
            continue
        old = SOURCE / name
        diff = subprocess.run(['diff', '-u', str(old if old.exists() else '/dev/null'), str(path)],
                              capture_output=True, text=True)
        assert diff.returncode == 1, name
        patch = EVIDENCE / ('needler-projectile-' + str(name).replace('/', '-') + '.patch')
        patch.write_text(diff.stdout)
        patches.append(record(patch))

    # Header additions in an inherited build cache can otherwise retain old
    # dependencies. Check actual built objects, rather than assuming -I order
    # alone establishes the layouts used by the delivered binaries.
    blocks = subprocess.check_output(['ninja', '-C', str(BUILD), '-t', 'deps'], text=True).split('\n\n')
    simulation_consumers = []
    for block in blocks:
        lines = block.splitlines()
        if not lines:
            continue
        obj = lines[0].split(': #deps ')[0]
        if not (BUILD / obj).is_file():
            continue
        for name in ('simulation.hpp', 'content.hpp', 'native_weapon_projectile.hpp'):
            assert '    ' + str(SOURCE / 'include/halo2' / name) not in lines, (obj, name)
        if '    ' + str(OVERLAY / 'include/halo2/simulation.hpp') in lines:
            simulation_consumers.append(obj)
    assert len(simulation_consumers) >= 30
    cache = (BUILD / 'CMakeCache.txt').read_text()
    assert 'CMAKE_HOME_DIRECTORY:INTERNAL=' + str(SOURCE) in cache
    assert str(OVERLAY / 'overrides.cmake') in cache
    fixture = ROOT / 'tests/fixtures/needler_projectile_birth_original.tsv'
    manifest = EVIDENCE / 'needler-projectile-source-manifest-20261012.json'
    write(manifest, dict(
        parent_manifest=record(prior_manifest), parent_source=str(SOURCE),
        parent_files=parent['files'], shared_external_source=parent['shared_external_source'],
        preserved_parent=record(accepted), preserved_parent_hashes=accepted_hashes,
        source_overrides=str(OVERLAY), override_files=[record(p) for p in paths],
        owned_patches=patches, constructor_fixture=record(fixture),
        compiled_simulation_consumers=simulation_consumers,
        cmake_cache=record(BUILD / 'CMakeCache.txt')))

    originals = []
    for report_name, tool_name in (
        ('needler-projectile-birth-original-20261012.json', 'oracle_needler_projectile_birth_1_1.py'),
        ('needler-projectile-birth-u5-20261012/report.json', 'research_needler_projectile_birth_u5.py'),
        ('needler-terrain-contact-original-20261012.json', 'oracle_needler_terrain_contact_1_1.py')):
        path = ROOT / 'analysis/multiplayer/integration' / report_name
        data = json.loads(path.read_text())
        assert data['instruction_patches'] == []
        key = 'tool_sha256' if 'tool_sha256' in data else 'oracle_sha256'
        assert data[key] == sha(ROOT / 'tools' / tool_name)
        originals.append(record(path))

    launchers = {}
    for name in ('Halo2-Weapons-Launcher', 'Halo2-Weapons-Title-Launcher'):
        info = subprocess.check_output([str(ROOT / name), '--launcher-info'], text=True)
        assert 'Runtime: ' + str(BUILD / 'halo2_native') in info
        log = EVIDENCE / ('needler-projectile-delivered-' + name + '-20261012.log')
        log.write_text(info)
        launchers[name] = dict(binary=record(ROOT / name), info=record(log))

    target = EVIDENCE / 'needler-projectile-validation-20261012.json'
    write(target, dict(
        scope='Original dry free-Needler constructor fields and native flight-distance publication; local firing/Sword/SMG regression. Full original world parity is not claimed.',
        source_manifest=record(manifest), runtime=record(BUILD / 'halo2_native'),
        launchers=launchers, original_reports=originals,
        prior_weapon_checkpoint=record(EVIDENCE / 'needler-firing-validation-20261012.json'),
        live_gameplay_xbe=record(Path(next(p for p in results[0]['command'] if p.endswith('/default.xbe')))),
        checks=[record(EVIDENCE / name) for name in checks],
        live=[dict(**row, evidence=record(Path(row['log']))) for row in results],
        screenshots=[record(LIVE / name) for name in ('needler-fire.bmp', 'sword-world.bmp')],
        zero_distance_red=record(EVIDENCE / 'needler-projectile-red-20261012.log'),
        stale_abi_live_red=record(EVIDENCE / 'needler-projectile-stale-abi-red-20261012/needler-ammo.log'),
        debugger_evidence=record(EVIDENCE / 'needler-projectile-live-exception-20261012.log'),
        cache_repairs=[record(EVIDENCE / name) for name in (
            'needler-projectile-abi-cache-repair-20261012.json',
            'needler-projectile-ui-cache-repair-20261012.json')],
        tools=[record(ROOT / 'tools' / name) for name in (
            'validate_needler_runtime_20261012.py', 'record_needler_projectile_validation_20261012.py',
            'oracle_needler_projectile_birth_1_1.py', 'research_needler_projectile_birth_u5.py',
            'oracle_needler_terrain_contact_1_1.py')],
        limits=[
            'Constructor inputs use the existing native direction and explicit no inherited platform/parent/medium/speed override. Original upper creation descriptor, distribution and whole RNG ordering are not proven.',
            'Birth timer/guidance/angular fields are retained, not advanced by an original projectile update owner.',
            'The original head-on terrain-contact probe is research only. Native contact still retires the projectile; target guidance, sticking, fuse, contrail, supercombine and detonation remain open.',
            'Full original Unit/pose/third-person, admission/drop, Havok and world effect ownership remain incomplete.',
            'Sword lunge/power/blocking and looping blade sound lifecycle remain open.',
            'SDL dummy audio proves nonempty PCM admission, not physical audible output or original mixer identity.',
            'No original Xbox gameplay framebuffer or physical hardware pixel/audio comparison.',
            'Native LAN one-event publication does not retain complete offhand/coalesced effect histories.',
        ]))
    print(json.dumps(dict(report=str(target), sha256=sha(target), live_cases=len(results),
                          parent_files=len(parent['files']), simulation_consumers=len(simulation_consumers))))


if __name__ == '__main__':
    main()
