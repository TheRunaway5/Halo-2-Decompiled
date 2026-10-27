#!/usr/bin/env python3
"""Run ordinary submitted-frame smokes for every authenticated multiplayer map."""
import argparse
import hashlib
import json
import os
from pathlib import Path

from build_launcher_engine import digest, publication_identity, smoke
from validate_weapon_runtime_tu15 import private_display


def shader_inputs(runtime):
    directory = runtime.parent / 'shaders'
    files = sorted(path for path in directory.rglob('*.spv') if path.is_file())
    if not files:
        raise ValueError('Runtime has no adjacent shader inputs')
    pins = [(path.relative_to(directory).as_posix(), path.stat().st_size, digest(path))
            for path in files]
    return dict(files=len(pins), bytes=sum(row[1] for row in pins),
                sha256=hashlib.sha256(json.dumps(pins).encode()).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--inventory', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--resume-report', type=Path,
                        help='Continue only unexecuted cases from an identical-input report')
    parser.add_argument('--timeout', type=float, default=180)
    args = parser.parse_args()
    root = args.root.resolve(strict=True)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    runtime = args.runtime.resolve(strict=True)
    data = args.inventory.read_bytes()
    inventory = json.loads(data)
    if (inventory.get('schema') != 'HALO2_NATIVE_RENDERER_SCENARIO_INVENTORY_V1'
            or inventory.get('passed') is not True):
        raise ValueError('A complete successful Cache scenario inventory is required')
    prepared = Path(inventory['prepared_root']).resolve(strict=True)
    rows = inventory['maps']
    paths = {Path(row['path']).resolve(strict=True) for row in rows}
    if (len(paths) != len(rows)
            or paths != {p.resolve(strict=True) for p in prepared.rglob('*.map')}):
        raise ValueError('Scenario inventory does not cover the complete prepared map set')
    for row in rows:
        if Path(row['basename']).name != row['basename'] or row.get('error'):
            raise ValueError('Invalid or unresolved scenario inventory row')
        path = Path(row['path']).resolve(strict=True)
        shared = Path(row['shared_directory']).resolve(strict=True)
        if not path.is_relative_to(prepared) or not shared.is_relative_to(prepared):
            raise ValueError('Scenario input path escapes the prepared map owner')
        if path.name != row['basename']:
            raise ValueError('Scenario inventory basename differs from its input')
    selected = [row for row in rows if row['scenario_present'] and row['scenario_type'] == 1]
    if not selected:
        raise ValueError('Scenario inventory contains no authored multiplayer maps')
    publication = ('Halo2-Launcher', 'Halo2-Title-Launcher', 'gameplay-runtime-selection.json',
                   'frontend-runtime-selection.json', 'assets/mcc-source.tsv')
    before = {name: publication_identity(root / name) for name in publication}
    report = dict(schema='HALO2_NATIVE_RENDERER_MULTIPLAYER_MAP_SMOKES_V1', passed=False,
                  scope='Every authored multiplayer scenario: ordinary four-frame runtime startup; '
                        'campaign BSPs and shader parity remain separate GPU gates',
                  runtime=str(runtime), runtime_sha256=digest(runtime),
                  shader_inputs=shader_inputs(runtime),
                  helper_source_sha256=digest(Path(__file__)),
                  prepared_root=str(prepared), inventory_sha256=hashlib.sha256(data).hexdigest(),
                  gpu_cache_policy='Shared within this owned cohort; each map starts a new process',
                  selected_maps=[row['basename'] for row in selected],
                  cases=[dict(map=row['basename'], status='not-executed', passed=False)
                         for row in selected],
                  limits=['No complete match, playthrough or physical Xbox framebuffer parity claim'])
    if args.resume_report:
        if args.resume_report.resolve() == args.report.resolve():
            raise ValueError('Keep the original report when continuing an interrupted cohort')
        previous_bytes = args.resume_report.read_bytes()
        previous = json.loads(previous_bytes)
        for field in ('schema', 'runtime', 'runtime_sha256', 'inventory_sha256',
                      'prepared_root', 'selected_maps'):
            if previous.get(field) != report[field]:
                raise ValueError('Interrupted cohort input differs: ' + field)
        if 'shader_inputs' in previous and previous['shader_inputs'] != report['shader_inputs']:
            raise ValueError('Interrupted cohort shader inputs differ')
        if len(previous['cases']) != len(selected):
            raise ValueError('Interrupted cohort case ledger is incomplete')
        for index, (row, case) in enumerate(zip(selected, previous['cases'])):
            if case['map'] != row['basename']:
                raise ValueError('Interrupted cohort case order differs')
            if case['status'] == 'not-executed':
                continue
            command = [str(runtime), '--multiplayer', '--map', row['path'],
                       '--shared-maps', row['shared_directory'], '--local-players', '1',
                       '--frames', '4', '--mute', '--viewport', '640', '480']
            if (case['status'] not in ('passed', 'failed') or case['command'] != command
                    or case['input_sha256'] != row['sha256']):
                raise ValueError('Interrupted cohort case provenance differs')
            if case['passed'] and not (case['status'] == 'passed' and case['exit_code'] == 0
                    and case['submitted_frames'] == 4 and case['simulation_ticks'] > 0
                    and case['local_players'] == case['peak_players'] == 1
                    and case['session_joined'] is True):
                raise ValueError('Interrupted cohort contains an invalid positive result')
            if digest(Path(row['path'])) != case['input_sha256']:
                raise ValueError('Completed map input changed since interrupted cohort')
            report['cases'][index] = case
        report['resumed_from'] = dict(path=str(args.resume_report),
            sha256=hashlib.sha256(previous_bytes).hexdigest(),
            helper_source_sha256=previous.get('helper_source_sha256'),
            shader_inputs_pinned='shader_inputs' in previous,
            gpu_cache_policy=previous.get('gpu_cache_policy'),
            completed_cases=sum(case['status'] != 'not-executed' for case in report['cases']))
    args.report.parent.mkdir(parents=True, exist_ok=True)

    def save():
        args.report.write_text(json.dumps(report, indent=2) + '\n')

    save()
    try:
        for name, variable in (('xdg', 'XDG_RUNTIME_DIR'), ('config', 'XDG_CONFIG_HOME'),
                               ('data', 'XDG_DATA_HOME'), ('cache', 'XDG_CACHE_HOME')):
            directory = scratch / name
            directory.mkdir(mode=0o700)
            os.environ[variable] = str(directory)
        shared_caches = {}
        for name in ('vulkan-cache', 'mesa-cache', 'nvidia-cache'):
            directory = scratch / ('shared-' + name)
            directory.mkdir()
            shared_caches[name] = directory
        with private_display(scratch) as display:
            os.environ.update(DISPLAY=display, SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy')
            for index, row in enumerate(selected):
                if report['cases'][index]['status'] != 'not-executed':
                    continue
                path = Path(row['path'])
                if digest(path) != row['sha256']:
                    raise ValueError('Map input changed since scenario inventory: ' + row['basename'])
                # An absolute map argument retains the custom map's Mods path;
                # its shared resource directory remains its separate owner.
                case_scratch = scratch / f'case-{index}'
                case_scratch.mkdir()
                for name, directory in shared_caches.items():
                    (case_scratch / name).symlink_to(directory, target_is_directory=True)
                case = smoke(runtime, Path(row['shared_directory']), str(path), 1, case_scratch, args.timeout,
                             root=root, video_driver='x11')
                case['map'] = row['basename']
                case['source'] = row['source']
                case['input_sha256'] = row['sha256']
                case['status'] = 'passed' if case['passed'] else 'failed'
                report['cases'][index] = case
                save()
        report['passed'] = all(case['passed'] for case in report['cases'])
    except Exception as error:
        report['error'] = str(error)
    finally:
        report['runtime_unchanged'] = digest(runtime) == report['runtime_sha256']
        report['shader_inputs_unchanged'] = shader_inputs(runtime) == report['shader_inputs']
        report['publication_unchanged'] = all(
            publication_identity(root / name) == identity for name, identity in before.items())
        report['passed'] &= (report['runtime_unchanged'] and report['shader_inputs_unchanged']
                             and report['publication_unchanged'])
        save()
    print(f"{sum(case['passed'] for case in report['cases'])}/{len(selected)} multiplayer maps passed; {args.report}")
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
