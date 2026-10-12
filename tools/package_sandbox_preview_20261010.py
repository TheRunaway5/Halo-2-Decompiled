#!/usr/bin/env python3
"""Record the existing frozen r7 build and its explicit passed/open checks."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BANK = ROOT / 'analysis/multiplayer/integration/sandbox-preview-r7-20261010'
DELIVERY = BANK / 'delivery'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    checks = []

    def add(name, log, binaries, description):
        assert (ROOT / log).is_file()
        checks.append(dict(name=name, exit_code=0, log=log,
                           binaries={n: sha(BANK / 'build' / n) for n in binaries},
                           description=description))

    cpu = 'analysis/sandbox-preview-r7-cpu-v2-20261010.log'
    assert '100% tests passed out of 8' in (ROOT / cpu).read_text()
    assert 'Skipped' not in (ROOT / cpu).read_text()
    add('eight-focused-cpu-tests', cpu,
        ['source_lift_membership_tests', 'source_teleporter_tests',
         'source_lift_contact_tests', 'source_vehicle_construction_tests',
         'source_lift_scalar_tests', 'hud_gameplay_tests', 'motion_sensor_tests',
         'multiplayer_audio_tests'], 'Eight focused tests, zero skips')
    add('multiplayer-combat-and-udp', 'analysis/sandbox-preview-r7-multiplayer-20261010.log',
        ['multiplayer_tests'], 'Rules, combat/respawn, UDP membership/state')
    add('membership-asan-ubsan', 'analysis/lift-membership-asan-v2-20261010.log', [],
        'Source contact-list only; sanitizer binary separately pinned in evidence')
    for row in json.loads((BANK / 'gpu-validation.json').read_text()):
        assert row['exit_code'] == 0
        binary = Path(row['command'][3]).name
        add(row['name'], row['log'], [binary],
            'Actual native renderer/SDL/LAN seam; dummy-audio boundary')
        checks[-1].update(command=row['command'], environment=row['environment'])
    add('delivery-title-capture', 'analysis/sandbox-preview-r7-title-20261010.log',
        ['halo2_native', 'Halo2-Title-Launcher'], 'Delivery launcher captures title9; dummy audio')
    add('delivery-launcher-resolution', 'analysis/sandbox-preview-r7-launcher-info-20261010.log',
        ['Halo2-Title-Launcher'], 'Actual resolution selects delivery runtime')
    add('revision6-preservation', 'analysis/sandbox-preview-r7-r6-preservation-20261010.log', [],
        'Prior revision6 source/runtime/evidence and launcher bindings retained')
    open_gates = []
    for name, binary, log in [
        ('lockout-lift', 'lockout_lift_match_tests', 'analysis/sandbox-preview-r7-lockout-lift-red-20261010.log'),
        ('beaver-creek-teleporter', 'teleporter_match_tests', 'analysis/sandbox-preview-r7-teleporter-red-20261010.log')]:
        assert 'RED:' in (ROOT / log).read_text()
        open_gates.append(dict(name=name, exit_code=1, log=log,
                               binary_sha256=sha(BANK / 'build' / binary),
                               status='unfinished live integration'))
    evidence = {row['log'] for row in checks + open_gates}
    evidence.update([
        'analysis/sandbox-preview-r7-title-20261010.png',
        'analysis/sandbox-preview-r7-muzzle-four-20261010.png',
        'analysis/lift-membership-asan-v2-20261010',
        'tools/audit_sandbox_preview_20261010.py',
        'tools/package_sandbox_preview_20261010.py'])
    evidence.update(str(p.relative_to(ROOT)) for p in
                    (ROOT / 'analysis/multiplayer/integration/lift-membership-20261010').rglob('*')
                    if p.is_file())
    evidence.update(str(p.relative_to(ROOT)) for p in
                    [BANK / 'gpu-validation.json', BANK / 'preserved-launchers.json'])
    external = [ROOT / f'assets/disc/maps/{name}.map' for name in
                ('lockout', 'midship', 'beavercreek', 'shared', 'mainmenu')]
    external += [ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
                 Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')]
    manifest = dict(schema=1, kind='preview',
        source_manifest_sha256=sha(BANK / 'source-manifest.json'),
        delivery_files={str(p.relative_to(DELIVERY)): sha(p) for p in DELIVERY.rglob('*')
                        if p.is_file() and p.name not in ('manifest.json', 'SHA256SUMS')},
        compiled_shaders={str(p.relative_to(BANK / 'build')): sha(p) for p in
                          (BANK / 'build/shaders').glob('*.spv')},
        evidence={name: sha(ROOT / name) for name in sorted(evidence)},
        external_inputs={str(p): sha(p) for p in external},
        completed_checks=checks, open_live_gates=open_gates,
        live_lifts_complete=False, live_teleporters_complete=False,
        live_vehicles_complete=False, complete_sandbox_parity=False,
        physical_audio_output_tested=False, platform='Linux x86-64',
        launcher=str(ROOT / 'Halo2-Sandbox-Preview'))
    (DELIVERY / 'manifest.json').write_text(json.dumps(manifest, sort_keys=True, indent=2) + '\n')
    (DELIVERY / 'SHA256SUMS').write_text(''.join(
        f'{sha(p)}  {p.relative_to(DELIVERY)}\n' for p in sorted(DELIVERY.rglob('*'))
        if p.is_file() and p.name != 'SHA256SUMS'))
    print(json.dumps(dict(completed_checks=len(checks), open_live_gates=len(open_gates),
                          runtime_sha256=sha(DELIVERY / 'runtime/halo2_native'),
                          launcher=str(ROOT / 'Halo2-Sandbox-Preview'))))


if __name__ == '__main__':
    main()
