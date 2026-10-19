#!/usr/bin/env python3
"""Admit selected prepared MCC inputs for the native source-weapon diagnostic.

Run through tools/run_test.py. Native saves/results/logs belong to its scratch;
only an explicitly requested bounded JSON receipt may survive outside scratch.
This runner never imports assets, starts an Xbox runtime, or supplies gameplay
results. A nonzero native result remains a failed composition check.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from mcc_content import directory
from weapon_test_cache import mcc_runtime_inputs

MODES = ('direct', 'match', 'charge', 'charge-instant', 'impact', 'unmapped', 'network')


def pin(path):
    path = Path(path)
    with path.open('rb') as stream:
        stat = os.fstat(stream.fileno())
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
        after = os.fstat(stream.fileno())
    current = path.stat()
    identity = lambda value: (value.st_dev, value.st_ino, value.st_size,
                              value.st_mtime_ns, value.st_ctime_ns)
    if identity(stat) != identity(after) or identity(stat) != identity(current):
        raise ValueError('MCC weapon input changed during admission')
    return dict(path=str(path), bytes=stat.st_size, sha256=digest,
                device=stat.st_dev, inode=stat.st_ino)


def selected_inputs(root, requested=None):
    root = Path(root).resolve(strict=True)
    registry = root / 'assets/mcc-source.tsv'
    if registry.stat().st_size > 4096:
        raise ValueError('MCC source registry is excessive')
    rows = registry.read_text().splitlines()
    if (len(rows) != 2 or rows[0] != 'H2_MCC_ASSET_SOURCE_V1' or
            not rows[1].startswith('cache\t') or
            any(character in rows[1][6:] for character in '\t\r\n\0')):
        raise ValueError('Invalid selected MCC source registry')
    path = Path(rows[1][6:])
    if not str(path) or str(path) == '.':
        raise ValueError('Selected MCC source is empty')
    if not path.is_absolute():
        path = root / path
    if path.is_symlink():
        raise ValueError('Selected MCC import must have a direct directory owner')
    cache = path.resolve(strict=True)
    if cache.parent != (root / 'assets/imported').resolve(strict=True) or cache.name.startswith('.'):
        raise ValueError('Weapon effects require the selected owned prepared MCC cache')
    if requested is not None and directory(requested).parent != cache:
        raise ValueError('Requested MCC input is not the selected prepared cache')
    provenance = cache / 'provenance.json'
    if provenance.is_symlink() or provenance.stat().st_size > 1024 * 1024:
        raise ValueError('Prepared MCC provenance has no bounded direct owner')
    preparation = json.loads(provenance.read_text())
    if (preparation.get('schema') != 'H2_MCC_TU15_PREPARATION_V1' or
            preparation.get('external_installation_requirement') != 'Halo MCC only'):
        raise ValueError('Weapon effects require MCC-only preparation provenance')
    maps = cache / 'halo2/h2_maps_win64_dx11'
    if maps.is_symlink() or maps.resolve(strict=True) != maps:
        raise ValueError('Prepared MCC map directory has no direct owner')
    for name in ('01b_spacestation.map', 'lockout.map', 'mainmenu.map', 'shared.map',
                 'single_player_shared.map'):
        path = maps / name
        if path.is_symlink() or path.resolve(strict=True).parent != maps:
            raise ValueError('Prepared MCC map has no direct file owner')
    checked, receipt = mcc_runtime_inputs(cache)
    if checked.resolve(strict=True) != maps:
        raise ValueError('MCC input resolver selected a different maps directory')
    receipt.update(cache=str(cache), registry=pin(registry), provenance=pin(provenance),
                   import_or_conversion_performed=False)
    return maps, receipt


def native_command(runtime, maps, output, mode, weapon=None):
    if mode not in MODES:
        raise ValueError('Unknown source weapon diagnostic mode')
    if weapon is None:
        weapon = 'authored-plasma-pistol' if mode in ('charge', 'charge-instant') else '5'
    if weapon == 'authored-plasma-pistol':
        if mode not in ('charge', 'charge-instant'):
            raise ValueError('Authored plasma pistol requires a charge diagnostic')
    elif not weapon.isdecimal() or (int(weapon) != 0 and not 3 <= int(weapon) <= 20):
        raise ValueError('Select an original authored starting-weapon option')
    command = [str(runtime), str(maps / 'lockout.map'), str(maps), '--mcc', str(output), weapon]
    if mode != 'direct':
        command.append(mode)
    return command


def admission_root(root, prepared, scratch):
    if prepared is None:
        return Path(root).resolve(strict=True)
    candidate=Path(prepared)
    if candidate.is_symlink():
        raise ValueError('Prepared MCC admission root requires a direct scratch owner')
    candidate=candidate.resolve(strict=True)
    if not candidate.is_relative_to(Path(scratch).resolve(strict=True)):
        raise ValueError('Prepared MCC admission root must remain in HALO2_TEST_SCRATCH')
    return candidate


def result_matches_mode(native, mode):
    accepted = (native.get('schema') == 'H2_MCC_SOURCE_WEAPON_EFFECTS_V1' and
        native.get('passed') is True and native.get('runtime_cache_source') == 'MCC' and
        native.get('original_executable_runtime_input') is False and native.get('mode') == mode)
    if mode != 'charge-instant':
        return accepted
    allocations = native.get('charge_positive_allocations', 0)
    return bool(accepted and native.get('instant_charge_required') is True and
        native.get('charge_metadata', {}).get('equivalent_to_original') is True and
        native.get('charge_instance', 0xffffffff) != 0xffffffff and
        isinstance(allocations, int) and allocations > 0 and
        native.get('charge_damage_returns') == allocations and
        0 < native.get('charge_returned_none', 0) <= allocations and
        all(native.get(field) is True for field in ('charge_retired_before_none',
            'charge_player_response', 'charge_held', 'charge_ready', 'charge_released',
            'charged_projectile_observed')) and native.get('actual_effect_frames') == 240 and
        native.get('source_battery_spent', 0) > 0)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--mcc-root', type=Path, help='Must resolve to the selected prepared MCC cache')
    parser.add_argument('--prepared-root', type=Path,
        help='Optional scratch-owned root with its own real prepared MCC source registry/provenance')
    parser.add_argument('--runtime', type=Path, required=True, help='Actual compiled source weapon diagnostic')
    parser.add_argument('--mode', choices=MODES, default='direct')
    parser.add_argument('--weapon', help='Original authored option, or authored-plasma-pistol for charge')
    parser.add_argument('--report-output', type=Path, help='Explicit bounded JSON receipt outside scratch')
    parser.add_argument('--timeout', type=int, default=120)
    args = parser.parse_args(argv)
    report = dict(schema='H2_MCC_SOURCE_WEAPON_EFFECTS_RUN_V1', passed=False,
        runtime_cache_source='MCC', original_executable_runtime_input=False,
        full_weapon_parity=False, retained_large_artifacts=[])
    status = 1
    try:
        scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
        report['scratch'] = str(scratch)
        if args.runtime.is_symlink() or not os.access(args.runtime, os.X_OK):
            raise ValueError('Source weapon diagnostic requires a direct executable owner')
        runtime = args.runtime.resolve(strict=True)
        if not 1 <= args.timeout <= 240:
            raise ValueError('Source weapon diagnostic timeout must be bounded to 1..240 seconds')
        selected_root=admission_root(args.root,args.prepared_root,scratch)
        maps, inputs = selected_inputs(selected_root, args.mcc_root)
        report['admission_root']=str(selected_root)
        report.update(inputs=inputs, runtime=pin(runtime), mode=args.mode)
        output = scratch / ('source-weapon-effects-' + args.mode)
        output.mkdir(exist_ok=False)
        command = native_command(runtime, maps, output, args.mode, args.weapon)
        report['command'] = command
        env = {key: value for key, value in os.environ.items()
               if not key.startswith(('HALO2_', 'H2_DISCORD_', 'LD_PRELOAD'))}
        env.update(HALO2_TEST_SCRATCH=str(scratch), TMPDIR=str(scratch), TMP=str(scratch),
                   TEMP=str(scratch), PYTHONDONTWRITEBYTECODE='1',
                   XDG_CONFIG_HOME=str(output / 'config'), XDG_DATA_HOME=str(output / 'data'),
                   XDG_STATE_HOME=str(output / 'state'), XDG_CACHE_HOME=str(output / 'cache'))
        with (output / 'native.log').open('w') as log:
            try:
                result = subprocess.run(command, cwd=args.root.resolve(strict=True), env=env,
                    stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
                    timeout=args.timeout)
                status = result.returncode
            except subprocess.TimeoutExpired:
                report['timeout'] = True
                status = 124
        text = (output / 'native.log').read_text(errors='replace')
        report.update(native_exit_code=status, native_log_tail=text[-16000:])
        if (output / 'authored-charge.json').is_file():
            if (output / 'authored-charge.json').stat().st_size > 65536:
                raise ValueError('Native charge metadata exceeds its bounded schema')
            report['effective_charge_metadata'] = json.loads((output / 'authored-charge.json').read_text())
        if (output / 'result.json').is_file():
            if (output / 'result.json').stat().st_size > 65536:
                raise ValueError('Native source weapon result exceeds its bounded schema')
            report['native_result'] = json.loads((output / 'result.json').read_text())
        _, after = selected_inputs(selected_root, args.mcc_root)
        report['runtime_and_inputs_unchanged'] = (report['runtime'] == pin(runtime) and inputs == after)
        native = report.get('native_result', {})
        report['passed'] = (status == 0 and report['runtime_and_inputs_unchanged'] and
            result_matches_mode(native, args.mode))
        if not report['passed'] and status == 0:
            status = 1
    except (OSError, ValueError, KeyError, RuntimeError, subprocess.SubprocessError) as error:
        report['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        report['scratch_removed_by'] = 'tools/run_test.py after this process exits'
        encoded = json.dumps(report, indent=2) + '\n'
        if len(encoded.encode()) > 1024 * 1024:
            raise RuntimeError('MCC source weapon receipt exceeds the small-report limit')
        if args.report_output:
            destination = args.report_output.absolute()
            if destination.suffix != '.json' or destination.is_symlink():
                raise ValueError('Retained source weapon receipt must be a direct JSON file')
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_text(encoded)
        print(encoded, end='')
    return 0 if report['passed'] else status or 1


if __name__ == '__main__':
    raise SystemExit(main())
