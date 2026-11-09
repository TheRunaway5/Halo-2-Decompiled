"""Exercise the real E3 sound resource owners linked by the current CMake cohort.

Prepare both installed campaign variants inside CTest's owned scratch. Preserve
their authored maps and run the complete descriptor/sector/encoded-byte consumer.
Each consumer reports whether it also exercises the complete native cold-cache
path. Neither mode admits a script or qualifies World initialization or missions.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess

import steam_workshop as workshop


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--test-executable', type=Path, required=True)
    parser.add_argument('--base-cache', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    catalog = workshop.scan()
    mod = next((row for row in catalog['mods'] if row['id'] == '3360515088'), None)
    if mod is None or not args.base_cache.is_dir():
        return 77
    assert not catalog['errors'] and not mod['error']
    executable = args.test_executable.resolve(strict=True)
    executable_pin = workshop.pin(executable)
    cases = []
    for name in ('earthcity_e3', 'earthcity_e3_potato'):
        source = workshop.source_owned(Path(mod['directory']), 'maps/' + name + '.map')
        pin = workshop.pin(source)
        prepared = workshop.prepare_cached(Path(mod['directory']), name, args.base_cache,
                                           owner / (name + '-prepared'))
        result_path = owner / (name + '.json')
        command = [str(executable), prepared['map'], prepared['shared_maps'],
                   prepared['prepared_root'], str(result_path)]
        result = subprocess.run(command, capture_output=True, text=True, timeout=300)
        case = dict(name=name, source=str(source), source_pin=pin, command=command,
                    exit_code=result.returncode, stdout=result.stdout[-4000:], stderr=result.stderr[-4000:])
        if result_path.is_file():
            case['consumer'] = json.loads(result_path.read_text())
        assert workshop.pin(source) == pin, 'Authored E3 source changed'
        cases.append(case)
    unchanged = workshop.pin(executable) == executable_pin
    unchanged &= all(workshop.pin(Path(case['source'])) == case['source_pin'] for case in cases)
    passed = unchanged and len(cases) == 2 and all(
        case['exit_code'] == 0 and case.get('consumer', {}).get('passed') is True for case in cases)
    cold = all(case.get('consumer', {}).get('native_cold_prediction', {}).get('passed') is True for case in cases)
    report = dict(scope=__doc__, passed=passed, cases=cases, executable=str(executable),
                  executable_pin=executable_pin, authored_inputs_and_consumer_unchanged=unchanged,
                  native_HS_executed=False, native_cache_allocation_executed=cold,
                  script_declaration_admitted=False, complete_mod_support=False,
                  owned_scratch=str(owner), retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    data = json.dumps(report, indent=2) + '\n'
    assert len(data.encode()) < 1024 * 1024
    args.output.write_text(data)
    print(json.dumps(dict(passed=passed, cases=len(cases), report=str(args.output))))
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
