#!/usr/bin/env python3
"""Replay unchanged retail 71730 assignment decisions and hopper getters.

Starts at 71B1E with declared synthetic ready-summary/register/stack inputs;
stops before launch publication (71CAD) or failure/session transition (71C73).
All called getters, count/weighted kernels and their callees run unchanged.
This does NOT execute upstream session/readiness/7E210 producers or downstream
71FF0 launch, does not replay Live, and does not certify the whole 71730 owner.
Run through tools/run_test.py. All compiler outputs stay in its owned scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess

from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDI,
                              UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW,
                              UC_X86_REG_MXCSR)
from oracle_game_variants import ROOT, Source, VERSIONS, STOP


GETTERS = (0x193440, 0x193370, 0x1934D0, 0x1933D0, 0x1933A0, 0x193400, 0x193470, 0x1934B0)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if digest(ROOT / VERSIONS['retail']['path']) != VERSIONS['retail']['pin']:
        raise RuntimeError('Original retail executable identity differs')
    source = Source(VERSIONS['retail'])
    machine = source.g
    hopper, stack = 0x1000000, 0x200E000
    stops, calls = [], []

    def read(at):
        return struct.unpack('<i', machine.mem_read(at, 4))[0]

    def write(at, values):
        if values:
            machine.mem_write(at, struct.pack('<' + 'i' * len(values), *values))

    def observe(g, address, size, unused):
        if address in (0x71CAD, 0x71C73):
            stops.append(address)
            g.emu_stop()
        if address in (0x91E30, 0x91FC0):
            sp = g.reg_read(UC_X86_REG_ESP)
            caller = read(sp)
            if caller in (0x71B6D, 0x71BC1, 0x71C11, 0x71C65):
                calls.append({'owner': hex(address), 'preserve_parties': bool(read(sp + 24))})

    machine.hook_add(UC_HOOK_CODE, observe)
    inputs = []
    sizes_cases = ([1] * 8, [5, 3], [3, 3, 2], [4, 3, 1], [1] * 12, [12], [3] * 4, [])
    for kind in range(1, 6):
        for sizes in sizes_cases:
            ids = [i for i, size in enumerate(sizes) for _ in range(size)]
            for flag in (False, True):
                inputs.append(([kind, 2, 3, 4, 0, int(flag), 3, 4], ids,
                               [24, 24, 22, 22, 22, 22, 20, 20] if len(ids) == 8 else [i % 128 for i in range(len(ids))]))
    rng = random.Random(0x71730)
    for _ in range(256):
        kind = rng.randrange(1, 6)
        maximum = rng.randrange(1, 9)
        minimum = rng.randrange(1, maximum + 1)
        count = rng.randrange(0, 17)
        ids = []
        party = 0
        for i in range(count):
            if i and rng.randrange(3) == 0:
                party += 1
            ids.append(party)
        inputs.append(([kind, rng.randrange(1, 5), minimum, maximum,
                        rng.choice((0, 1, 2, 0x100, 0x101)) if kind == 3 else rng.randrange(4),
                        rng.randrange(2), minimum, maximum], ids,
                       [rng.randrange(-1, 128) for _ in ids]))
    cases = []
    for fields, ids, weights in inputs:
        kind, teams, minimum, maximum, imbalance, flag, flag_minimum, flag_maximum = fields
        machine.mem_write(hopper, bytes(0x614))
        write(hopper, [kind])
        write(hopper + 0x5EC, [teams, minimum, maximum, imbalance])
        machine.mem_write(hopper + 0x604, bytes([flag]))
        write(hopper + 0x608, [flag_minimum, flag_maximum])
        getters = [source.call(address, registers=((UC_X86_REG_EAX, hopper), (UC_X86_REG_ECX, hopper)))
                   for address in GETTERS]
        machine.mem_write(stack, bytes(0x500))
        machine.mem_write(stack + 0x11, b'\x01\x00')
        write(stack + 0x9C, [len(ids)])
        write(stack + 0x164, weights)
        write(stack + 0x224, ids)
        write(stack + 0x50, [777] * 16)
        machine.reg_write(UC_X86_REG_ESP, stack)
        machine.reg_write(UC_X86_REG_EDI, hopper)
        machine.reg_write(UC_X86_REG_FPCW, 0x27F)
        machine.reg_write(UC_X86_REG_MXCSR, 0x1F80)
        stops.clear()
        calls.clear()
        machine.emu_start(0x71B1E, STOP, count=5000000)
        if len(stops) != 1 or machine.reg_read(UC_X86_REG_EIP) != stops[0]:
            raise RuntimeError(('Original assignment did not reach declared boundary', fields, ids))
        output = list(struct.unpack('<16i', machine.mem_read(stack + 0x50, 64)))
        success = stops[0] == 0x71CAD
        if not success and output != [777] * 16:
            raise RuntimeError(('Original failure changed output', fields, output))
        if output[len(ids):] != [777] * (16 - len(ids)):
            raise RuntimeError('Original assignment wrote outside roster')
        weighted = [c for c in calls if c['owner'] == '0x91fc0']
        path = -1 if not success else 0 if kind in (1, 2) else 1 if weighted[-1]['preserve_parties'] else 2
        cases.append({'fields': fields, 'parties': ids, 'raw_weights': weights,
                      'getters': getters, 'assignments': output[:len(ids)] if success else None,
                      'path': path, 'calls': calls.copy(), 'stop': hex(stops[0])})

    native = scratch / 'assignment.rs'
    binary = scratch / 'assignment'
    native.write_text('#[path=' + json.dumps(str(ROOT / 'rust/online_protocol/src/team_balance.rs')) + '] mod team_balance;\n'
                      '#[path=' + json.dumps(str(ROOT / 'rust/online_protocol/src/team_assignment.rs')) + '] mod team_assignment;\n' + r'''
use std::io::{self,Read};
fn main(){let mut s=String::new();io::stdin().read_to_string(&mut s).unwrap();for line in s.lines(){
let v:Vec<i32>=line.split_whitespace().map(|v|v.parse().unwrap()).collect();let n=v[8]as usize;
let h=team_assignment::Hopper{kind:v[0].try_into().unwrap(),width:v[1],height:v[2],depth:v[3],
imbalance:v[4],flag_enabled:v[5]!=0,flag_minimum:v[6],flag_maximum:v[7]};let (flag,min,max)=h.flags();
let getters=[h.team_count(),h.maximum_team_size(),i32::from(flag),min,max,h.allowed_imbalance(),
i32::from(h.rank_visible()),i32::from(h.is_team())];
let a=team_assignment::assign(h,&v[9..9+n],&v[9+n..9+n*2]);
if let Some(a)=a{println!("{} | {:?} | {:?}",a.path as i32,a.teams,getters);}
else{println!("-1 | [] | {:?}",getters);}}}
''')
    subprocess.run(['rustc', '--edition=2024', '--crate-name', 'assignment_oracle', '-Awarnings',
                    '-O', str(native), '-o', str(binary)], check=True)
    lines = [' '.join(map(str, [*c['fields'], len(c['parties']), *c['parties'], *c['raw_weights']])) for c in cases]
    actual = subprocess.check_output([str(binary)], input=('\n'.join(lines) + '\n').encode()).decode().splitlines()
    if len(actual) != len(cases):
        raise RuntimeError('Native output count differs')
    for case, line in zip(cases, actual):
        path, assignments, getters = line.split('|')
        if int(path) != case['path'] or json.loads(getters) != case['getters'] or \
                (json.loads(assignments) if int(path) != -1 else None) != case['assignments']:
            raise RuntimeError(('Original/Rust assignment mismatch', case, line))
    # Every executed original address retains the authenticated original bytes.
    for at in source.executed:
        expected = source.image.read(at, 15)
        if bytes(machine.mem_read(at, len(expected))) != expected:
            raise RuntimeError(('Changed original instruction bytes', hex(at)))
    fixture = {'schema': 'H2_RETAIL_ASSIGNMENT_CORRIDOR_V1', 'scope': __doc__,
               'original_sha256': VERSIONS['retail']['pin'], 'entry': '0x71b1e',
               'stops': ['0x71cad', '0x71c73'], 'getter_addresses': list(map(hex, GETTERS)), 'cases': cases}
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(json.dumps(fixture, indent=2) + '\n')
    report = {'passed': True, 'scope': __doc__, 'cases': len(cases),
              'original_assignment_decisions': len(cases), 'original_getter_calls': len(cases) * len(GETTERS),
              'original_instructions': len(source.executed), 'instruction_patches': [], 'replaced_callees': [],
              'original_sha256': VERSIONS['retail']['pin'], 'tool_sha256': digest(__file__),
              'native_source_sha256': {name: digest(ROOT / ('rust/online_protocol/src/' + name + '.rs'))
                                       for name in ('team_assignment', 'team_balance')},
              'fixture_sha256': digest(args.fixture), 'scratch': str(scratch), 'scratch_retained': False,
              'whole_71730_or_session_flow_qualified': False}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
