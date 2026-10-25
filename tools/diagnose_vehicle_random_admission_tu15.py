#!/usr/bin/env python3
"""Reproduce current ordinary-runtime Random vehicle-setting admission failures."""
import argparse
import json
import os
from pathlib import Path
import subprocess

from validate_weapon_runtime_tu15 import private_display
from weapon_test_cache import sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--maps', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.runtime = args.runtime.resolve(strict=True); args.maps = args.maps.resolve(strict=True)
    args.output.mkdir(parents=True, exist_ok=False)
    runtime_pin = sha(args.runtime); map_pin = sha(args.maps / 'coagulation.map')
    catalogue = Path(__file__).resolve().parents[1] / 'analysis/vehicles-tu15-goal-20261023/original-vehicle-setting-catalogue-v1.json'
    choices = json.loads(catalogue.read_text())['parameters']
    cases = []
    with private_display(owner) as display:
        environment = dict(os.environ, DISPLAY=display, SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy',
            XDG_CACHE_HOME=str(owner / 'graphics-cache'), HALO2_PIPELINE_CACHE_DIRECTORY=str(owner / 'pipeline-cache'))
        for name, parameter, value in (('primary_light', 76, 6), ('primary_heavy', 78, 3), ('primary_turret', 81, 5)):
            field = next(row for row in choices if row['parameter'] == parameter)
            if not any(choice['value'] == value and choice['label'] == 'Random' for choice in field['choices']):
                raise ValueError('Requested diagnostic is not an authored original Random setting')
            command = [str(args.runtime), '--multiplayer', '--map', str(args.maps / 'coagulation.map'),
                '--shared-maps', str(args.maps), '--local-players', '1', '--frames', '4', '--viewport', '800', '450',
                '--mute', '--mp-rule', str(parameter), str(value), '--mp-source-object-world', str(owner / (name + '-source'))]
            result = subprocess.run(command, env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                timeout=40, text=True)
            tail = result.stdout[-6000:]
            reproduced = result.returncode != 0 and 'Random' in tail and 'owner' in tail
            cases.append(dict(name=name, original_parameter=parameter, original_value=value, command=command,
                returncode=result.returncode, current_failure_reproduced=reproduced, tail=tail))
            print(json.dumps(dict(case=name, failure_reproduced=reproduced, returncode=result.returncode)), flush=True)
    unchanged = sha(args.runtime) == runtime_pin and sha(args.maps / 'coagulation.map') == map_pin
    report = dict(scope='Diagnostic failure reproduction only; these vehicle settings remain unsupported',
        current_failures_reproduced=all(row['current_failure_reproduced'] for row in cases), cases=cases,
        runtime=str(args.runtime), runtime_sha256=runtime_pin, map_sha256=map_pin, inputs_unchanged=unchanged,
        original_menu_catalogue_sha256=sha(catalogue), runtime_XBE_argument=False,
        full_vehicle_parity=False, owned_scratch=str(owner), retained_large_artifacts=[], tool_sha256=sha(Path(__file__)))
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if unchanged and report['current_failures_reproduced'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
