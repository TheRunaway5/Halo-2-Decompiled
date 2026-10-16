#!/usr/bin/env python3
"""Recover successive missing AI owners from genuine source-world captures.

Each step executes unchanged original code and independent host hardware,
lowers whole reached routines, then continues the same declared fixture.
Object-update failures, original faults and hardware mismatches stop recovery.
This does not certify the resulting module or change the delivered runtime.
"""
import argparse
import json
import shutil
import subprocess
from pathlib import Path

from audit_rendering import file_sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--base', type=Path, default=ROOT / 'analysis/weapons-tu15-goal-20261013')
    ap.add_argument('--case', required=True)
    ap.add_argument('--generation', required=True)
    ap.add_argument('--next-case', type=int, required=True)
    ap.add_argument('--next-generation', type=int, required=True)
    ap.add_argument('--steps', type=int, default=3)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    base = a.base.resolve()
    out = a.output.resolve()
    assert not out.exists() and 1 <= a.steps <= 8
    out.mkdir(parents=True)
    python = ROOT / '.tools/python/bin/python'
    case, generation = base / a.case, base / a.generation
    records = []

    def run(command, log):
        with log.open('w') as stream:
            subprocess.run(command, cwd=ROOT, stdout=stream,
                           stderr=subprocess.STDOUT, check=True, timeout=240)

    for index in range(a.steps):
        build = json.loads((case / 'build.json').read_text())
        if build['returncode'] == 0:
            print('Fixture completed:', case.name, flush=True)
            break
        failure = (case / 'native.log').read_text().splitlines()[-1]
        if 'live actual root 1d0670' not in failure:
            print('Non-AI continuation requires inspection:', failure, flush=True)
            break
        failed = [p.parent for p in case.glob('ai-frame*/native.json')
                  if not json.loads(p.read_text())['completed']]
        assert len(failed) == 1
        frame = failed[0]
        step = out / f'step{index + 1}'
        step.mkdir()
        number = a.next_generation + index
        original = base / f'hunter-close-ai-recovery-v{number}-original'
        assert not original.exists()
        commands = [[str(python), str(ROOT / 'tools/probe_original_charge_particle_tu15.py'),
                     '--case', str(frame), '--output', str(original),
                     '--entry', '0x1d0670', '--phase', 'before'],
                    [str(python), str(ROOT / 'tools/verify_original_particle_hardware_tu15.py'),
                     '--case', str(original)]]
        for ordinal, command in enumerate(commands):
            run(command, step / f'original{ordinal}.log')
        report = json.loads((original / 'report.json').read_text())
        hardware = json.loads((original / 'hardware/report.json').read_text())
        assert report['completed'] and not report['instruction_patches']
        assert not report['supplied_game_services']
        assert hardware['emulator_retained_state_matches_hardware']
        next_generation = base / f'ai-frame-native-functions-v{number}'
        assert not next_generation.exists()
        previous = json.loads((generation / 'generation.json').read_text())
        command = [str(python), str(ROOT / 'tools/generate_ai_frame_tu15.py')]
        for reference in previous['references']:
            command += ['--reference', str(Path(reference['report']).parent)]
        command += ['--reference', str(original), '--output',
                    str(next_generation / 'generated.cpp'), '--directory', str(next_generation)]
        commands.append(command)
        run(command, step / 'generation.log')
        generated = json.loads((next_generation / 'generation.json').read_text())
        assert generated['generated_cpp_sha256'] == file_sha(next_generation / 'generated.cpp')
        next_case = base / f'hunter-ai-forward-native-v{a.next_case + index}'
        assert not next_case.exists()
        next_case.mkdir()
        shutil.copytree(case / 'source', next_case / 'source')
        shutil.copy2(next_generation / 'generated.cpp', next_case / 'source/ai_frame.cpp')
        script = (case / 'build-and-capture.py').read_text()
        old_cache = next(line for line in script.splitlines() if 'cache=base/' in line)
        replacement = old_cache[:old_cache.index('cache=base/')] + f"cache=base/'{case.name}'" + old_cache[old_cache.index(';cache_report'):]
        script = script.replace(old_cache, replacement)
        (next_case / 'build-and-capture.py').write_text(script)
        for name in ('generate-transition.py', 'generation.json', 'bodies.json', 'scope.json'):
            shutil.copy2(case / name, next_case / name)
        command = [str(python), str(next_case / 'build-and-capture.py')]
        commands.append(command)
        run(command, step / 'native.log')
        result = json.loads((next_case / 'build.json').read_text())
        record = dict(input_case=case.name, frame=frame.name,
                      original=original.name, original_report_sha256=file_sha(original / 'report.json'),
                      hardware_report_sha256=file_sha(original / 'hardware/report.json'),
                      generation=next_generation.name, generation_sha256=file_sha(next_generation / 'generation.json'),
                      whole_bodies=len(generated['complete_native_entries']),
                      lowered_instructions=len(generated['lowered_instructions']),
                      native_case=next_case.name, native_build_sha256=file_sha(next_case / 'build.json'),
                      native_returncode=result['returncode'], commands=commands)
        records.append(record)
        (out / 'report.json').write_text(json.dumps(dict(scope=__doc__,
            tool_sha256=file_sha(Path(__file__)), steps=records,
            delivered_runtime_changed=False, full_weapon_parity=False), indent=2) + '\n')
        print(json.dumps({k: record[k] for k in ('frame', 'generation', 'whole_bodies', 'native_case', 'native_returncode')}), flush=True)
        case, generation = next_case, next_generation


if __name__ == '__main__':
    main()
