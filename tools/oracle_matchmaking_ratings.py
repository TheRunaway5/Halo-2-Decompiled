#!/usr/bin/env python3
"""Original raw/effective roster ratings, advertisement floor and search bounds.

Raw/effective controls enter unchanged retail 7E210 and stop at 7E550 after
the raw aggregates, before group-count/capacity production. Cases with a rated
seat in every declared group also execute complete 7E210 and compare the same
outputs; its observed validity is recorded, not assumed to be admission proof.
Advertisements execute 736A8..736FA, search initialization 9098B..90A5E after
allocation, and query prefixes stop before the real Live wrapper 8FA80. All
called original instructions/callees run unchanged. Inputs, tables and sessions
are declared synthetic controls, not historical service observations.
Run through tools/run_test.py; all compiler products stay in owned scratch.
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
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR)
from oracle_game_variants import ROOT, Source, VERSIONS, STOP


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if digest(ROOT / VERSIONS['retail']['path']) != VERSIONS['retail']['pin']:
        raise RuntimeError('Original executable identity differs')
    source = Source(VERSIONS['retail'])
    g = source.g
    g.mem_map(0x1100000, 0x80000)
    session, output, description, search, results, prior = (0x1100000, 0x1110000,
        0x1120000, 0x1130000, 0x1140000, 0x1150000)
    stack = 0x200E000

    def write(at, values):
        if values:
            g.mem_write(at, struct.pack('<' + 'i' * len(values), *values))

    def read(at):
        return struct.unpack('<i', g.mem_read(at, 4))[0]

    def reset(index=3, slot=1, table=None):
        source.call(0x66330)
        g.mem_write(session, bytes(0x80000))
        g.mem_write(0x551AE8, bytes(16 * 0x614))
        write(0x47D8F4, [2])
        g.mem_write(0x47D8F8, struct.pack('<IHH', 2, 16, 0))
        hopper = 0x551AE8 + slot * 0x614
        source.call(0x193C70, registers=((UC_X86_REG_EAX, index), (UC_X86_REG_EDX, hopper)))
        if table is not None:
            g.mem_write(hopper + 0x51C, bytes(table))
        return hopper

    def boundary(start, end, registers, argv=()):
        before = set(source.executed)
        write(stack, [STOP, *argv])
        g.reg_write(UC_X86_REG_ESP, stack)
        g.reg_write(UC_X86_REG_FPCW, 0x27F)
        g.reg_write(UC_X86_REG_MXCSR, 0x1F80)
        for reg, value in registers:
            g.reg_write(reg, value & 0xFFFFFFFF)
        def stop(machine, address, size, unused):
            if address == end:
                machine.emu_stop()
        hook = g.hook_add(UC_HOOK_CODE, stop)
        try:
            g.emu_start(start, STOP, count=2000000)
        finally:
            g.hook_del(hook)
        if g.reg_read(UC_X86_REG_EIP) != end:
            raise RuntimeError(('Original boundary not reached', hex(start), hex(end)))
        if end not in before:
            source.executed.discard(end)

    defaults = []
    for index in range(7):
        hopper = reset(index=index)
        defaults.append({'hopper': index, 'table': list(g.mem_read(hopper + 0x51C, 128))})
    rng = random.Random(0x7E210)
    maps = {'retail': defaults[0]['table'], 'identity': list(range(128)),
            'plus_one': [min(i + 1, 127) for i in range(128)],
            'nonmonotone': [rng.randrange(i, 128) for i in range(128)]}
    handcrafted = [([], [], []), ([20,20,40], [0,0,1], [0,1,0]),
        ([1,2,8], [0,0,0], [0,0,0]), ([1,2,8], [0,0,1], [0,0,0]),
        ([127,1], [0,0], [1,0]), ([1,127], [0,0], [0,2]),
        ([1,20], [0,0], [3,0]), ([-1,10], [0,0], [0,0]),
        ([-1,-1], [0,0], [1,2]), ([-1], [0], [0]),
        ([1,127], [0,1], [0,0]), ([-1,4,8], [0,0,0], [0,0,0]),
        ([10,90], [0,0], [4,0])]
    inputs = [(name, slot, ranks, groups, flags) for name in maps for slot in (0,1,15)
              for ranks, groups, flags in handcrafted]
    for _ in range(144):
        n = rng.randrange(17)
        groups, party = [], 0
        for i in range(n):
            if i and rng.randrange(3) == 0:
                party += 1
            groups.append(party)
        inputs.append((rng.choice(tuple(maps)), rng.choice((0,1,15)),
            [rng.randrange(-1,128) for _ in groups], groups,
            [rng.choice((0,0,0,1,2,3,4,16)) for _ in groups]))

    rosters, full_calls = [], 0
    for name, slot, ranks, groups, flags in inputs:
        hopper = reset(slot=slot, table=maps[name])
        write(session + 0x741C, [5]); write(session + 0x49C8, [slot]); write(session + 0x4994, [16])
        g.mem_write(session + 0x49FD, b'\x01')
        summary = session + 0x4A00
        group_count = max(groups) + 1 if groups else 0
        write(summary, [group_count])
        write(summary + 0x184, [len(ranks)])
        for i in range(group_count):
            write(summary + 8 + i * 8, [i + 1, 0])
            write(summary + 0x84 + i * 12, [100 + i, 100 + i, 0])
        for i, (rank, group, flags_word) in enumerate(zip(ranks, groups, flags)):
            write(summary + 0x188 + i * 12, [i + 1, 0, flags_word])
            write(summary + 0x248 + i * 4, [rank])
            write(summary + 0x288 + i * 4, [1000])
            write(summary + 0x2C8 + i * 4, [group])
        boundary(0x7E210, 0x7E550, ((UC_X86_REG_EAX, session),), (output,))
        stats = [read(output), read(output + 4), read(output + 8), read(output + 0x10)]
        raw = [read(output + 0xD4 + i * 4) for i in range(len(ranks))]
        adjusted = [read(output + 0x154 + i * 4) for i in range(len(ranks))]
        if raw != ranks:
            raise RuntimeError('Original producer changed its raw operands')
        full_valid = None
        if all(any(group == k and flags_word & 3 == 0 for group, flags_word in zip(groups, flags))
               for k in range(group_count)):
            full_valid = bool(source.call(0x7E210, (output,), registers=((UC_X86_REG_EAX, session),)) & 255)
            full_calls += 1
            if stats != [read(output), read(output + 4), read(output + 8), read(output + 0x10)] or \
               adjusted != [read(output + 0x154 + i * 4) for i in range(len(ranks))]:
                raise RuntimeError('Complete original producer differs from captured prefix')
        write(stack + 0x14, [hopper]); write(stack + 0x20, [stats[1]]); write(stack + 0x37C, [0])
        boundary(0x736A8, 0x736FB, ((UC_X86_REG_EDI, stats[2]), (UC_X86_REG_EBP, description),
            (UC_X86_REG_EAX, stats[0]), (UC_X86_REG_ESI, 4), (UC_X86_REG_EDX, 0)))
        ad = [read(description + off) for off in (0x78,0x7C,0x80,0x84)]
        rosters.append({'map':name,'catalog_index':slot,'raw':ranks,'groups':groups,
                        'flags':flags,'statistics':stats,'adjusted':adjusted,
                        'advertisement':ad,'complete_original_valid':full_valid})

    inverses = []
    for name, table in maps.items():
        hopper = reset(table=table)
        for rank in range(-1,128):
            value = source.call(0x1931A0, registers=((UC_X86_REG_ECX,rank),(UC_X86_REG_EDX,hopper)))
            inverses.append({'map':name,'rank':rank,'actual':value})
    searches = []
    pairs = ((0,0),(1,8),(20,20),(10,15),(49,50),(80,100),(126,127),(0,127),(22,40),(1,127))
    for name, table in maps.items():
        for minimum, maximum in pairs:
            hopper = reset(index=1, table=table)
            for offset, value in ((0x14,1),(0x18,2),(0x1C,(minimum+maximum)//2),
                (0x20,maximum),(0x24,minimum),(0x28,0),(0x2C,-1),(0x30,0),(0x34,0)):
                write(stack + offset, [value])
            boundary(0x9098B,0x90A5F,((UC_X86_REG_ESI,search),(UC_X86_REG_EBP,hopper)))
            init = [read(search + off) for off in (0x60,0x68,0x64,0x20,0x1C)]
            queries = []
            for radius in sorted({0,init[3]//2,init[3]}):
                for offset,value in ((0x1C,radius),(0x88,50*0x204),(0x8C,results),(0x90,prior)):
                    write(search + offset,[value])
                boundary(0x90B30,0x8FA80,((UC_X86_REG_ESI,search),))
                queries.append({'radius':radius,'minimum':read(search+0x30),'maximum':read(search+0x28)})
            searches.append({'map':name,'minimum':minimum,'maximum':maximum,
                             'initialization':init,'queries':queries})

    native, binary = scratch/'ratings.rs', scratch/'ratings'
    native.write_text('#[path=' + json.dumps(str(ROOT/'rust/online_protocol/src/matchmaking_ratings.rs')) + '] mod port;\n' + r'''
use std::io::{self,Read};
fn main(){let mut s=String::new();io::stdin().read_to_string(&mut s).unwrap();for line in s.lines(){
let v:Vec<i32>=line.split_whitespace().map(|s|s.parse().unwrap()).collect();let kind=v[0];
let table:Vec<u8>=v[v.len()-128..].iter().map(|v|*v as u8).collect();let map=port::RankMap::from_slice(&table).unwrap();
let out:Vec<i32>=match kind{
0=>{let slot=v[1]as usize;let n=v[2]as usize;let raw=&v[3..3+n];let flags:Vec<u32>=v[3+n..3+n*2].iter().map(|n|*n as u32).collect();
let stats=port::raw_statistics(raw,&flags).unwrap();let adjusted=port::adjusted_ranks(raw,&v[3+n*2..3+n*3],slot,&map).unwrap();
let mut out=vec![stats.average,stats.maximum,stats.minimum,stats.rated_count as i32,map.advertised_minimum(stats.minimum,stats.maximum).unwrap()];out.extend(adjusted);out},
1=>vec![map.inverse(v[1]).unwrap()],
2=>{let e=port::SearchEnvelope::new(v[1],v[2],&map).unwrap();let (lo,hi)=e.bounds(v[3]).unwrap();vec![e.inverse_maximum,e.mapped_minimum,e.ceiling,e.maximum_radius,e.initial_radius,lo,hi]},
3=>port::RankMap::retail_launch().values().iter().map(|n|*n as i32).collect(),_=>panic!()};println!("{:?}",out);}}
''')
    subprocess.run(['rustc','--edition=2024','--crate-name','ratings_oracle','-Awarnings','-O',str(native),'-o',str(binary)],check=True)
    commands, expected = [], []
    def add(values, output):
        commands.append(' '.join(map(str,values))); expected.append(output)
    for case in rosters:
        add([0,case['catalog_index'],len(case['raw']),*case['raw'],*case['flags'],*case['groups'],*maps[case['map']]],
            [*case['statistics'],case['advertisement'][3],*case['adjusted']])
    for case in inverses:
        add([1,case['rank'],*maps[case['map']]],[case['actual']])
    for case in searches:
        for query in case['queries']:
            add([2,case['minimum'],case['maximum'],query['radius'],*maps[case['map']]],
                [*case['initialization'],query['minimum'],query['maximum']])
    for case in defaults:
        add([3,*maps['retail']],case['table'])
    actual = subprocess.check_output([str(binary)],input=('\n'.join(commands)+'\n').encode()).decode().splitlines()
    if len(actual)!=len(expected): raise RuntimeError('Native output count differs')
    for i,(line,wanted) in enumerate(zip(actual,expected)):
        if json.loads(line)!=wanted: raise RuntimeError(('Original/Rust mismatch',i,commands[i],wanted,line))
    for at in source.executed:
        original=source.image.read(at,15)
        if bytes(g.mem_read(at,len(original)))!=original: raise RuntimeError(('Changed instruction',hex(at)))
    fixture={'schema':'H2_RETAIL_MATCHMAKING_RATINGS_ORIGINAL_V1','scope':__doc__,
        'original_sha256':VERSIONS['retail']['pin'],'maps':maps,'defaults':defaults,
        'rosters':rosters,'inverses':inverses,'searches':searches}
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text(json.dumps(fixture,indent=2)+'\n')
    report={'passed':True,'scope':__doc__,'roster_controls':len(rosters),'complete_7e210_calls':full_calls,
        'inverse_controls':len(inverses),'search_initializations':len(searches),
        'query_controls':sum(len(c['queries']) for c in searches),'native_comparisons':len(expected),
        'original_instructions':len(source.executed),'instruction_patches':[],'replaced_callees':[],
        'original_sha256':VERSIONS['retail']['pin'],'tool_sha256':digest(__file__),
        'native_source_sha256':digest(ROOT/'rust/online_protocol/src/matchmaking_ratings.rs'),
        'fixture_sha256':digest(args.fixture),'scratch':str(scratch),'scratch_retained':False,
        'full_summary_validity_and_capacity_ported':False,'persistent_result_comparison_owner_qualified':False}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
