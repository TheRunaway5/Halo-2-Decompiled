#!/usr/bin/env python3
"""Offline unchanged TU1.5 living-count calls, ordered reads and fault prefixes.

The declarations supply published actor/squad/group/scenario/object banks.
No instruction is patched and no dependency result is supplied. Ready HS
continuations execute the real 212010 argument owner and 211DA0 publisher.
The native game does not import or execute this reference executable.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE, STOP, STACK = 0x2000000, 0x3000000, 0x203f000
SQUADS, GROUPS, ACTORS, OBJECTS, GLOBALS, SCENARIO = [BASE+0x1000+i*0x100 for i in range(6)]
SQUAD_ROWS, GROUP_ROWS, ACTOR_ROWS = BASE+0x2000, BASE+0x3000, BASE+0x4000
OBJECT_ROWS, OBJECT_BODY, SQUAD_DEFS, STARTS = BASE+0x8000, BASE+0x9000, BASE+0xa000, BASE+0xb000
HS_POOL, HS_ROWS, HS_FRAME, HS_PARENT, HS_RESULT, SYNTAX_POOL, SYNTAX_ROWS = [BASE+0xc000+i*0x1000 for i in range(7)]
GLOBALS_AT = [0x52be7c, 0x52be80, 0x5020c8, 0x4ecbd0, 0x5020a8, 0x4ecc20]
QUERY_REGIONS = [[p, 4] for p in GLOBALS_AT]+[[BASE, 0xc000]]
ENTRIES = [0x27dc40, 0x27ca60, 0x20d070, 0x20d0d0, 0x27c880, 0x20cff0, 0x20d030,
           0x2af6a0, 0x212010, 0x211da0]


def declaration():
    writes = []
    def put(p, value, width=4):
        writes.append([p, width, value & ((1 << (8*width))-1)])
    for p, value in zip(GLOBALS_AT, [SQUADS, GROUPS, ACTORS, OBJECTS, GLOBALS, SCENARIO]):
        put(p, value)
    for pool, rows in [(SQUADS, SQUAD_ROWS), (GROUPS, GROUP_ROWS), (ACTORS, ACTOR_ROWS), (OBJECTS, OBJECT_ROWS)]:
        put(pool+0x44, rows)
    put(ACTORS+0x24, 0x888)
    put(GLOBALS+1, 1, 1)
    put(SCENARIO+0x160, 4); put(SCENARIO+0x164, SQUAD_DEFS)
    for i in range(4):
        row = GROUP_ROWS+i*0x38
        for offset in [4, 8, 12, 16]:
            put(row+offset, 0xffffffff)
        put(SQUAD_ROWS+i*0x98+0x74, 0xffff, 2)
        put(SQUAD_ROWS+i*0x98+0x68, 0xffffffff)
        put(SQUAD_ROWS+i*0x98+8, i+1, 2)
        put(SQUAD_ROWS+i*0x98+10, [3, -2, 5, 7][i], 2)
        put(SQUAD_ROWS+i*0x98+16, 0x3f800000)
        put(SQUAD_DEFS+i*0x74+0x48, 2)
        put(SQUAD_DEFS+i*0x74+0x4c, STARTS+i*0xc8)
        put(STARTS+i*0xc8, 0x1000+i)
        put(STARTS+i*0xc8+0x64, 0x2000+i)
    put(GROUP_ROWS+4, 1)
    put(GROUP_ROWS+8, 0)
    put(SQUAD_ROWS+0x74, 2, 2)
    put(GROUP_ROWS+0x38+8, 1)
    put(GROUP_ROWS+0x38+12, 2)
    put(GROUP_ROWS+0x38+16, 0)
    put(GROUP_ROWS+2*0x38+4, 3)
    put(GROUP_ROWS+2*0x38+16, 0)
    put(GROUP_ROWS+3*0x38+8, 3)
    put(GROUP_ROWS+3*0x38+16, 2)
    for i in range(4):
        put(ACTOR_ROWS+i*0x888, 0xe100+i, 2)
        put(ACTOR_ROWS+i*0x888+0x18, 0xe2000000+i)
        put(ACTOR_ROWS+i*0x888+0x20, 0xffffffff)
        put(ACTOR_ROWS+i*0x888+0x38, 0x1000+i)
        put(OBJECT_ROWS+i*12+8, OBJECT_BODY+i*0x200)
        put(OBJECT_BODY+i*0x200+0xec, 0x3f000000)
    put(SQUAD_ROWS+0x68, 0xe1000000)
    put(ACTOR_ROWS+0x20, 0xe1010001)
    return writes


def changed(writes, p, value, width=4):
    return writes+[[p, width, value & ((1 << (8*width))-1)]]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    ap.add_argument('--verify', action='store_true')
    ap.add_argument('--family', action='store_true',help='Capture swarm/nonswarm/strength siblings with the same original dependency closure')
    ap.add_argument('--fighting', action='store_true',help='Capture whole combat-count/actor-behavior closure and resumed original evaluator')
    args = ap.parse_args()
    if args.family and args.fighting:ap.error('Choose one query cohort')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    entries=ENTRIES+([0x2af7d0,0x2af830,0x2af770] if args.family else [])
    if args.fighting:
        entries=[0x27dda0,0x1ea090,0x1afbd0,0x27c880,0x20cff0,0x20d030,0x2af650,0x212010,0x211da0]
    bodies = {e: _trace(code, e, set(entries)) for e in entries}
    all_sites = set().union(*(set(b.code) for b in bodies.values()))
    visited = set()
    baseline = declaration()
    specs = []
    for ref in [0, 1, 2, 3, 0x40000000, 0x40000001, 0x40000002, 0x40000003,
                0x80000000, 0x80000001, 0x80000003, 0xc0000000, 0xc0000001,
                0xc0010000, 0xc000ffff, 0xc0040000, 0xffffffff]:
        specs.append(dict(name=f'reference-{ref:08x}', reference=ref, writes=baseline))
    for count in [-32768, -1, 0, 1, 32767]:
        for strength in [0, 0x80000000, 0x3f800000, 0x7f800000, 0xff800000, 0x7fc01234, 0x7f801234, 1, 0x7f7fffff]:
            w = changed(baseline, SQUAD_ROWS+10, count, 2)
            w = changed(w, SQUAD_ROWS+8, count, 2)
            w = changed(w, SQUAD_ROWS+16, strength)
            specs.append(dict(name=f'counter-{count}-strength-{strength:08x}', reference=0, writes=w))
    for active in [0, 1, 255]:
        w = changed(baseline, GLOBALS+1, active, 1)
        specs.append(dict(name=f'start-reference-active-{active}', reference=0xc0000000, writes=w))
    for salt in [0, 1, 0x7fff, 0x8000, 0xffff]:
        specs.append(dict(name=f'direct-salt-{salt}', reference=0x80000000,
                          writes=changed(baseline, ACTOR_ROWS, salt, 2)))
    w = changed(baseline, ACTOR_ROWS+0x38, 0x9999)
    w = changed(w, ACTOR_ROWS+0x888+0x38, 0x1000)
    specs.append(dict(name='starting-location-second-link', reference=0xc0000000, writes=w))
    specs.append(dict(name='starting-location-no-match', reference=0xc0000001, writes=w))
    for policy in [0x1f80,0x3f80,0x5f80,0x7f80,0x9fc0,0xbfc0,0xdfc0,0xffc0]:
        for strength in [1,0x7f801234,0x7f7fffff]:
            w=changed(baseline,SQUAD_ROWS+0x10,strength)
            w=changed(w,SQUAD_ROWS+8,32767,2)
            specs.append(dict(name=f'policy-{policy:04x}-strength-{strength:08x}',
                              reference=0,writes=w,initial_mxcsr=policy))
    if args.family:
        # Preserve the living-only canonical payload; sibling inputs explicitly
        # exercise their own counter fields and the strength population gate.
        specs=[s for s in specs if not s['name'].startswith('counter-') or
               s['name'].endswith(('strength-3f800000','strength-7f801234'))]
        for first,second in [(0x7fc12345,0x7fc54321),(0x7f812345,0x7fc54321),(0x7fc12345,0xff854321)]:
            w=changed(baseline,SQUAD_ROWS+0x98+0x10,first)
            w=changed(w,SQUAD_ROWS+3*0x98+0x10,second)
            specs.append(dict(name=f'paired-mean-nan-{first:08x}-{second:08x}',reference=0x40000000,writes=w))
        expanded=[]
        for query in ['swarm_count','nonswarm_count','strength']:
            for spec in specs:
                w=changed(spec['writes'],SQUAD_ROWS+0xc,-1,2)
                w=changed(w,SQUAD_ROWS+0x98+0xc,3,2)
                expanded.append({**spec,'writes':w,'query':query})
        specs=expanded

    query_regions=QUERY_REGIONS
    if args.fighting:
        clock=BASE+0xb800
        baseline=changed(baseline,0x51df54,clock)
        baseline=changed(baseline,clock+2,30,2)
        baseline=changed(baseline,SCENARIO+0x158,4)
        for index in range(4):
            row=ACTOR_ROWS+index*0x888
            for offset,value,width in [(9,1,1),(0x328,3,2),(0x306,600,2),(0x190,0xffff,2)]:
                baseline=changed(baseline,row+offset,value,width)
            baseline=changed(baseline,SQUAD_ROWS+index*0x98+0x16,[3,-2,5,7][index],2)
            baseline=changed(baseline,GROUP_ROWS+index*0x38+0x2c,[13,-4,0,32767][index],2)
        specs=[]
        def add(name,reference=0x80000000,writes=None,**extra):
            specs.append(dict(name=name,reference=reference,writes=baseline if writes is None else writes,
                              query='fighting_count',**extra))
        for ref in [0,1,2,3,4,0xffff,0x40000000,0x40000001,0x40000002,0x40000003,0x40000004,
                    0x80000000,0x80000001,0x80000003,0xc0000000,0xc0000001,0xc0010000,
                    0xc000ffff,0xc0040000,0xffffffff]:
            add(f'reference-{ref:08x}',ref)
        for count in [-32768,-1,0,1,32767]:
            for ref,at in [(0,SQUAD_ROWS+0x16),(0x40000000,GROUP_ROWS+0x2c)]:
                add(f'combat-counter-{ref:x}-{count}',ref,changed(baseline,at,count,2))
        for count in [-1,0,1,4,65536]:
            for ref,at in [(1,SCENARIO+0x160),(0x40000001,SCENARIO+0x158)]:
                add(f'scenario-bound-{ref:x}-{count}',ref,changed(baseline,at,count))
        for active in [0,1,255]:
            for awareness in [-32768,-1,0,2,3,32767]:
                w=changed(baseline,ACTOR_ROWS+9,active,1)
                add(f'actor-active-{active}-awareness-{awareness}',writes=changed(w,ACTOR_ROWS+0x328,awareness,2))
        for rate in [-32768,-1,0,1,30,32767]:
            for age in [-32768,-1,0,19,20,21,599,600,601,32767]:
                w=changed(baseline,clock+2,rate,2)
                add(f'actor-age-{age}-rate-{rate}',writes=changed(w,ACTOR_ROWS+0x306,age,2))
        for last in [-32768,-1,0,1,3,7]:
            for match in [-1,0,1,3,7,8]:
                w=changed(baseline,ACTOR_ROWS+0x190,last,2)
                # The depth word aliases one behavior record; publish it last.
                for index in range(9):w=changed(w,ACTOR_ROWS+0x90+index*0x40,0x38 if index==match else 0x37,2)
                w=changed(w,ACTOR_ROWS+0x190,last,2)
                add(f'behavior-last-{last}-match-{match}',writes=w)
        for active in [0,1,255]:
            add(f'start-reference-active-{active}',0xc0000000,changed(baseline,GLOBALS+1,active,1))
        w=changed(baseline,ACTOR_ROWS+0x38,0x9999)
        w=changed(w,ACTOR_ROWS+0x888+0x38,0x1000)
        add('starting-location-second-link',0xc0000000,w)
        add('starting-location-no-match',0xc0000001,w)
        w=changed(baseline,ACTORS+0x24,0)
        add('direct-null-datum',0x8000ffff,changed(w,ACTOR_ROWS,0xffff,2))
        for policy in [0x1f80,0x3fbf,0x5f80,0x7fbf,0x9fc0,0xbfff,0xdfc0,0xffff]:
            add(f'unchanged-fp-{policy:04x}',initial_mxcsr=policy)
        query_regions=QUERY_REGIONS+[[0x51df54,4]]

    physical = scratch/'physical-original-ai-query'
    physical.mkdir()
    observer = physical/'observer'
    subprocess.run(['c++','-std=c++20','-O2',str(Path(__file__).with_name('observe_native_campaign_ai_query_tu15.cpp')),'-o',str(observer)],check=True,capture_output=True)
    original_text = next(s for s in image.sections if s.name == '.text')
    text_file=physical/'text.bin';text_file.write_bytes(image.section_bytes(original_text))

    def hardware(g, spec, evaluator, fault_at, sites):
        assembly=[f'.section .original,"ax"\n.incbin "{text_file}"']
        sections=[f'.original 0x{original_text.va:x} : {{ *(.original) }}']
        for i, section in enumerate(s for s in image.sections if s.name != '.text'):
            bank=physical/f'section{i}.bin';bank.write_bytes(bytes(g.mem_read(section.va,section.vsize)))
            assembly.append(f'.section .source{i},"aw"\n.incbin "{bank}"')
            sections.append(f'.source{i} 0x{section.va:x} : {{ *(.source{i}) }}')
        bank=physical/'inputs.bin';bank.write_bytes(bytes(g.mem_read(BASE,0x40000)))
        assembly.append(f'.section .inputs,"aw"\n.incbin "{bank}"')
        sections.append(f'.inputs 0x{BASE:x} : {{ *(.inputs) }}')
        entry={'swarm_count':0x2af7d0,'nonswarm_count':0x2af830,'strength':0x2af770,'fighting_count':0x2af650}.get(spec.get('query'),0x2af6a0)
        direct_entry=0x27dda0 if args.fighting else 0x27dc40
        assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 ldmxcsr control
 mov ${spec['reference']},%eax
 mov ${spec['reference']},%ecx
 mov ${STACK},%esp
 int3
 jmp *entry
.balign 4
entry: .long {entry if evaluator else direct_entry}
control: .long {spec.get('initial_mxcsr',0x1f80)}
''')
        sections.append('.bridge 0x70000000 : { *(.bridge) }')
        (physical/'reference.s').write_text('\n'.join(assembly))
        (physical/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        subprocess.run(['as','--32',str(physical/'reference.s'),'-o',str(physical/'reference.o')],check=True,capture_output=True)
        subprocess.run(['ld','-melf_i386','-T',str(physical/'link.ld'),str(physical/'reference.o'),'-o',str(physical/'reference')],check=True,capture_output=True)
        pc=STOP if fault_at is None else sites[fault_at]
        occurrence=1 if fault_at is None else sites[:fault_at+1].count(pc)
        subprocess.run([str(observer),str(physical/'reference'),hex(pc),str(occurrence),hex(HS_RESULT if evaluator or spec.get('query')=='strength' else 0),str(physical/'observed.bin')],check=True,capture_output=True)
        result,mxcsr,steps=struct.unpack('<3I',(physical/'observed.bin').read_bytes())
        return dict(result=result,mxcsr=mxcsr,steps=steps)

    def execute(spec, evaluator=False, fault_at=None):
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x40000); g.mem_map(STOP, 4096)
        def put(p, value, width=4):
            g.mem_write(p, (value & ((1 << (8*width))-1)).to_bytes(width, 'little'))
        for p, width, value in spec['writes']:
            put(p, value, width)
        put(STACK, STOP)
        for i in range(3): put(STACK+4+i*4, 0)
        query=spec.get('query','living_count')
        opcode={'living_count':326,'swarm_count':329,'nonswarm_count':330,'strength':328,'fighting_count':325}[query]
        evaluator_entry={'living_count':0x2af6a0,'swarm_count':0x2af7d0,'nonswarm_count':0x2af830,'strength':0x2af770,'fighting_count':0x2af650}[query]
        definition=int.from_bytes(image.read(0x480c00+opcode*4,4),'little')
        assert int.from_bytes(image.read(definition+4,4),'little')==evaluator_entry
        assert int.from_bytes(image.read(definition,2),'little')==(6 if query=='strength' else 7)
        put(STACK+4,{'swarm_count':1,'nonswarm_count':2}.get(query,0))
        if query=='strength':put(STACK+12,HS_RESULT)
        if evaluator:
            put(0x505e5c, HS_POOL); put(HS_POOL+0x44, HS_ROWS)
            put(0x505e6c, SYNTAX_POOL); put(SYNTAX_POOL+0x44, SYNTAX_ROWS)
            put(HS_ROWS+0x10, HS_FRAME)
            put(HS_FRAME, HS_PARENT); put(HS_FRAME+4, 0)
            put(HS_FRAME+12, 0, 2)
            put(HS_FRAME+14, spec['reference']); put(HS_FRAME+18, 1, 2)
            put(HS_FRAME+20, 0xffffffff)
            put(HS_PARENT+8, HS_RESULT)
            put(SYNTAX_ROWS+2, opcode, 2); put(SYNTAX_ROWS+4, 6 if query=='strength' else 7, 2)
            put(STACK+4, opcode); put(STACK+8, 0); put(STACK+12, 0)
        # Preserve the exact declared physical input before original HS owners
        # mutate their own continuation stack.
        physical_input = [(section.va,bytes(g.mem_read(section.va,section.vsize))) for section in image.sections if section.name!='.text']
        physical_input.append((BASE,bytes(g.mem_read(BASE,0x40000))))
        reads, read_sites = [], []
        stopped = False
        def watch(_g, _access, p, width, _value, _user):
            nonlocal stopped
            if any(a <= p and p+width <= a+n for a, n in query_regions):
                reads.append([p, width, bytes(g.mem_read(p, width)).hex()])
                read_sites.append(g.reg_read(R.UC_X86_REG_EIP))
                if fault_at is not None and len(reads)-1 == fault_at:
                    stopped = True; g.emu_stop()
        g.hook_add(UC_HOOK_MEM_READ, watch)
        g.hook_add(UC_HOOK_CODE, lambda _g, pc, _n, _u: visited.add(pc))
        g.reg_write(R.UC_X86_REG_ESP, STACK)
        g.reg_write(R.UC_X86_REG_EAX, spec['reference'])
        g.reg_write(R.UC_X86_REG_ECX, spec['reference'])
        g.reg_write(R.UC_X86_REG_MXCSR, spec.get('initial_mxcsr',0x1f80))
        g.emu_start(evaluator_entry if evaluator else (0x27dda0 if args.fighting else 0x27dc40), STOP, count=10000)
        if fault_at is None:
            assert g.reg_read(R.UC_X86_REG_EIP) == STOP, (spec['name'], hex(g.reg_read(R.UC_X86_REG_EIP)))
        else:
            assert stopped
        result = int.from_bytes(g.mem_read(HS_RESULT, 4), 'little') if evaluator or query=='strength' else g.reg_read(R.UC_X86_REG_EAX)&0xffff
        for p,data in physical_input:g.mem_write(p,data)
        physical_result=hardware(g,spec,evaluator,fault_at,read_sites)
        if fault_at is None and query!='strength':assert physical_result['result']==result,(spec['name'],'physical/Unicorn count differs')
        # Paired NaN mean payloads, like status flags, are taken from unchanged
        # physical instructions rather than an emulator approximation.
        if fault_at is None:result=physical_result['result']
        return dict(reads=reads, result=result, mxcsr=physical_result['mxcsr'], fault_at=fault_at,
                    physical_original_steps=physical_result['steps'],unicorn_mxcsr=g.reg_read(R.UC_X86_REG_MXCSR))

    cases = []
    for spec in specs:
        direct = execute(spec)
        evaluator = execute(spec, evaluator=True)
        assert direct['reads'] == evaluator['reads'] and direct['result'] == evaluator['result'], spec['name']
        assert direct['mxcsr']==evaluator['mxcsr'],(spec['name'],'resumed HS wrapper changed FP status')
        cases.append(dict(**spec, **direct, evaluator_result=evaluator['result']))
    for query in (['fighting_count'] if args.fighting else ['swarm_count','nonswarm_count','strength'] if args.family else ['living_count']):
        fault_spec = dict(name='fault-prefix-snan-group',reference=0x40000000,
                          writes=changed(baseline,SQUAD_ROWS+0x98+0x10,0x7f801234))
        if args.family or args.fighting:fault_spec['query']=query
        for i in range(len(execute(fault_spec)['reads'])):
            cases.append(dict(**fault_spec, **execute(fault_spec, fault_at=i), evaluator_result=None))
    if args.fighting:
        for name in ['reference-80000000','starting-location-second-link','behavior-last-7-match-8','direct-null-datum']:
            fault_spec=next(s for s in specs if s['name']==name)
            for i in range(len(execute(fault_spec)['reads'])):
                cases.append(dict(**fault_spec,**execute(fault_spec,fault_at=i),evaluator_result=None))
    fixture = dict(scope=__doc__+(' Sibling swarm/nonswarm/strength evaluator coverage.' if args.family else ''), original_xbe_sha256=NEW_PIN, instruction_patches=[],
                   supplied_dependency_returns=[], query_regions=query_regions, cases=cases,
                   required_backend='unchanged-tu15-physical-i386-count-fp+unicorn-ordered-read-provenance',
                   bodies={f'{e:08x}': hashlib.sha256(b''.join(bytes(code.at(p).bytes) for p in sorted(b.code))).hexdigest() for e,b in bodies.items()})
    if args.family:
        fixture['function_bindings']={'ai_swarm_count':dict(opcode=329,evaluator=0x2af7d0,return_type=7),
                                      'ai_nonswarm_count':dict(opcode=330,evaluator=0x2af830,return_type=7),
                                      'ai_strength':dict(opcode=328,evaluator=0x2af770,return_type=6)}
    if args.fighting:
        fixture['scope']='Whole27DDA0 combat counts and actual27C880/1EA090/1AFBD0 dependency closure; resumed2AF650 evaluator, original reads and faults; no supplied callbacks or results.'
        fixture['function_bindings']={'ai_fighting_count':dict(opcode=325,evaluator=0x2af650,return_type=7)}
    encoded = json.dumps(fixture, sort_keys=True, separators=(',', ':'))+'\n'
    if args.verify:
        assert args.fixture.read_text() == encoded, 'Original replay differs from retained reference'
    else:
        assert not args.fixture.exists(), 'Preserve reference; use --verify'
        args.fixture.parent.mkdir(parents=True, exist_ok=True); args.fixture.write_text(encoded)
    report = dict(cases=len(cases), whole_count_and_evaluator_cases=len(specs), fault_prefixes=len(cases)-len(specs),
                  fixture_bytes=len(encoded.encode()), fixture_sha256=hashlib.sha256(encoded.encode()).hexdigest(),
                  original_sites_executed=len(visited & all_sites), original_sites_total=len(all_sites),
                  instruction_patches=[], supplied_dependency_returns=[], passed=True,
                  physical_fp_differences_from_unicorn=sum(c['mxcsr']!=c['unicorn_mxcsr'] for c in cases),
                  sse_policies=sorted(set(c.get('initial_mxcsr',0x1f80) for c in cases)),
                  source_sha256={str(p.relative_to(Path(__file__).resolve().parents[1])):hashlib.sha256(p.read_bytes()).hexdigest() for p in [Path(__file__).resolve(),Path(__file__).with_name('observe_native_campaign_ai_query_tu15.cpp').resolve()]},
                  scope=('Whole swarm/nonswarm/strength queries and resumed evaluators; no first-dispatch HS expression evaluation or arbitrary count modes/outputs qualified.' if args.family else 'Whole mode0 living-count dependency closure and resumed original evaluator; no first-dispatch HS expression evaluation or other count modes qualified.'))
    if args.fighting:
        report['scope']=fixture['scope']
        report['body_coverage']={f'{e:08x}':dict(executed=len(set(b.code)&visited),total=len(b.code),
            missing=[f'{p:08x}' for p in sorted(set(b.code)-visited)]) for e,b in bodies.items()}
    args.report.parent.mkdir(parents=True, exist_ok=True); args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
