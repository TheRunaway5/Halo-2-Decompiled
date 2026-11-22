"""Compare the compiled 2C2AD0 default with its complete pinned particle-frame body."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import resource

import native_effect_owner_replacements as owners

ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'
TEST = ROOT / 'tests/native_tu15_particle_motion_tests.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_motion.hpp'
SYSTEMS = ROOT / 'src/rendering/effects/native_tu15_particle_motion_systems.hpp'


def run_mutants(scratch, command, cases):
    changes = [
        (SYSTEMS, 'subtract(vz, impulse)', 'add(vz, impulse)'),
        (SYSTEMS, 'subtract(one, angular)', 'subtract(one, linear)'),
        (SYSTEMS, 'inactive_flags = 0x09', 'inactive_flags = 0x01'),
        (SYSTEMS, 'channel_count = 3', 'channel_count = 2'),
        (HEADER, 'save(0x2c2c50,', 'save(0x2c2c51,'),
        (HEADER, 'push(0x2c2afd, g[edx]);', 'push(0x2c2afd, g[eax]);'),
        (HEADER, '!native_required_owner_policy_detail::preserves_instruction_policy(s.services)', 'false'),
        (HEADER, 'if (s.active_mmx)', 'if (false)'),
        (HEADER, '!policy || !policy->literal_service_sites', '!s.instruction_cursor || !s.instruction_cursor->literal_service_sites'),
    ]
    # Mutants may damage loop bounds. Limit both address space and run time.
    def limit():
        resource.setrlimit(resource.RLIMIT_AS, (1 << 30, 1 << 30))
    refused = []
    for i, (header, before, after) in enumerate(changes):
        source = header.read_text()
        if source.count(before) != 1:
            raise ValueError('Mutation anchor drifted: '+before)
        include = scratch / f'mutant-{i}'
        copy = include / header.relative_to(ROOT/'src')
        copy.parent.mkdir(parents=True)
        text = source.replace(before, after)
        if before.startswith('!policy ||'):
            text = text.replace('policy->service_sites', 's.instruction_cursor->service_sites')
        copy.write_text(text)
        executable = include/'motion'
        subprocess.run([command[0], '-I'+str(include), *command[1:-1], str(executable)], check=True)
        result = subprocess.run([executable, str(cases)], capture_output=True, timeout=30, preexec_fn=limit)
        if result.returncode not in (1, 3):
            raise ValueError(f'Mutation {before} had unexpected exit {result.returncode}')
        refused.append(before)
    return refused


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--mutants', action='store_true')
    parser.add_argument('--cases', type=int, default=8000)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    text = UNIT.read_text()
    entry = '2c2ad0'
    body = owners.body(text, entry)
    if owners.digest(body) != owners.PINS[entry]:
        raise ValueError('Particle motion body differs from TU1.5 input')
    installed, entries = owners.install(text, text)
    if entry not in entries or owners.install(text, installed)[1]:
        raise ValueError('Particle motion default binding changed')
    for change in (text.replace(body[0], body[0].replace('0x2c2afdu', '0x2c2affu', 1)),
                   text.replace('case 0x52be68u:return 0x52be68u;', 'case 0x52be68u:return 0x52be6cu;', 1),
                   text.replace(owners.body(text, '24ffe0')[0], '', 1)):
        if change == text or entry in owners.admitted(change):
            raise ValueError('Particle motion accepted a changed source dependency')
    helpers = re.findall(r'^std::uint32_t (?:selected|canonical_branch)\([^\n]+', text, re.M)
    if len(helpers) != 2:
        raise ValueError('Missing particle operand or branch resolver')
    sites = body[0].count('s.services.instruction(')
    bound = owners.body(installed, entry)[0].replace('void run_2c2ad0(', 'void bound_run(', 1)
    (scratch / 'motion.inc').write_text('\n'.join([*helpers, body[0], owners.reference(body, entry, entry),
                                                     bound, f'constexpr unsigned motion_sites={sites};']))
    exe = scratch / 'motion'
    command = [args.compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off',
               '-I'+str(ROOT/'include'), '-I'+str(ROOT/'src'), '-I'+str(scratch), str(TEST), '-o', str(exe)]
    release_command = command.copy()
    if args.sanitizers:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    subprocess.run(command, check=True)
    report = dict(owned_scratch=str(scratch), compiler=args.compiler, sanitizers=args.sanitizers,
                  sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in (TEST, UNIT, HEADER, SYSTEMS, Path(__file__).resolve())}, roles={})
    for mode, switches in [('default', {}), ('reference', {'HALO2_REFERENCE_EFFECT_OWNERS':'2c2ad0'}),
                           ('census', {'HALO2_CENSUS_NATIVE_BRIDGES':'1'}),
                           ('verify', {'HALO2_VERIFY_EFFECT_OWNERS':'1'})]:
        env = {k:v for k,v in os.environ.items() if k not in
               ('HALO2_REFERENCE_EFFECT_OWNERS','HALO2_CENSUS_NATIVE_BRIDGES','HALO2_VERIFY_EFFECT_OWNERS')}
        result = subprocess.run([exe, str(args.cases)], env=env | switches, capture_output=True, text=True)
        report['roles'][mode] = dict(exit=result.returncode, output=(result.stdout+result.stderr)[-1200:])
        if result.returncode:
            break
    report['passed'] = len(report['roles']) == 4 and all(r['exit']==0 for r in report['roles'].values())
    if report['passed'] and args.mutants:
        report['rejected_mutants'] = run_mutants(scratch, release_command, args.cases)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(report['roles'])
    return int(not report['passed'])


if __name__ == '__main__':
    raise SystemExit(main())
