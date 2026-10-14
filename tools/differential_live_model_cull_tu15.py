#!/usr/bin/env python3
"""Unchanged TU1.5 44710/45550 on genuinely produced native model queries.

Each case supplies its own complete native INPUT world, source-collected units
and declared sphere. Labeled isolated cases explicitly request subpart culling
on those native units; they do not establish original live attribution. Authored geometry and current posed/inverse-bind nodes
are read directly from that world. No original instructions or callees are
replaced. All unit/mask words, call order and retained world bytes are checked.
"""
import argparse
import collections
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from differential_physics_native_frame_tu15_20261012 import regions, segment


def pack(values):
    return struct.pack('<'+'I'*len(values), *values)


def words(raw):
    return list(struct.unpack('<'+'I'*(len(raw)//4), raw))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compare(path, image):
    case=json.loads(path.read_text())
    source=path.with_name(path.stem+'-inputs.bin')
    banks=list(regions(source))
    guest=Uc(UC_ARCH_X86, UC_MODE_32)
    mapped=set()
    def mount(at, data):
        pages=[p for p in range(at&~4095,(at+len(data)+4095)&~4095,4096) if p not in mapped]
        first=0
        while first<len(pages):
            end=first+1
            while end<len(pages) and pages[end]==pages[end-1]+4096:end+=1
            guest.mem_map(pages[first],(end-first)*4096)
            first=end
        mapped.update(pages)
        guest.mem_write(at,data)
    for at,data in banks:mount(at,data)
    context=0x65000000
    mount(context,bytes(0x4000))
    mount(0x70000000,bytes(4096))
    declared=[]
    def project(at, data):
        declared.append((at,bytes(guest.mem_read(at,len(data)))))
        guest.mem_write(at,data)
    project(0x4cd3a0,pack([context]))
    project(0x4cd39c,pack([case['prefix']]))
    project(0x4cd398,pack([case['total']]))
    units=[row.copy() for row in case['before']['units']]
    for row in units[:case['total']]:
        if row[0]&0x40:
            assert case['arena_address']<=row[5]<case['arena_address']+2048
            row[5]=context+0x25c+row[5]-case['arena_address']
    project(0x4c6958,pack([v for row in units for v in row]))
    guest.mem_write(context+0x25c,pack(case['before']['arena']))
    guest.mem_write(context+0xa5c,struct.pack('<H',case['before']['arena_used']))
    guest.mem_write(context+0x2a68,pack([*case['center'],case['radius'],1,1]))
    guest.mem_write(0x70000008,segment(0,0x9b)+segment(0,0x93)+segment(0x60200000,0x93))
    guest.reg_write(R.UC_X86_REG_GDTR,(0,0x70000000,0xfff,0))
    for reg,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),
                      (R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):
        guest.reg_write(reg,value)
    stack,stop=0x600f0000,0x60300000
    guest.mem_write(stack,pack([stop]))
    for name in ('EAX','ECX','EDX','EBX','EBP','ESI','EDI'):
        guest.reg_write(getattr(R,'UC_X86_REG_'+name),0)
    guest.reg_write(R.UC_X86_REG_ESP,stack)
    for n in range(8):guest.reg_write(getattr(R,f'UC_X86_REG_XMM{n}'),0)
    for reg,value in ((R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),
                      (R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):
        guest.reg_write(reg,value)
    trace=[]
    instructions={}
    calls=collections.Counter()
    count=0
    def original(vm,pc,size,_):
        nonlocal count
        actual=bytes(vm.mem_read(pc,size))
        assert actual==image.read(pc,size),f'Non-original instruction at {pc:08x}'
        instructions[pc]=actual.hex()
        count+=1
        if pc in (0x44710,0x45550,0xbac30,0x145560,0x145200,0x2e7c40,0x458c0):
            calls[f'{pc:08x}']+=1
        if pc==0x45550:
            pointer,radius=words(vm.mem_read(vm.reg_read(R.UC_X86_REG_ESP)+4,8))
            trace.append([vm.reg_read(R.UC_X86_REG_ECX)&65535,*words(vm.mem_read(pointer,12)),radius])
    guest.hook_add(UC_HOOK_CODE,original)
    differences=[]
    error=None
    compared=0
    try:
        guest.emu_start(0x44710,stop,count=10000000)
        assert guest.reg_read(R.UC_X86_REG_EIP)==stop
        actual_units=[words(guest.mem_read(0x4c6958+n*32,32)) for n in range(850)]
        for row in actual_units[:case['total']]:
            if row[0]&0x40:row[5]=case['arena_address']+row[5]-context-0x25c
        actual=dict(units=actual_units,arena=words(guest.mem_read(context+0x25c,2048)),
                    arena_used=int.from_bytes(guest.mem_read(context+0xa5c,2),'little'))
        for field,value in actual.items():
            if value!=case['after'][field]:
                differences.append(dict(field=field,original=value,native=case['after'][field]))
        if trace!=case['calls']:differences.append(dict(field='calls',original=trace,native=case['calls']))
        for at,data in reversed(declared):guest.mem_write(at,data)
        for at,expected in banks:
            if 0x60000000<=at<0x60200000:continue  # Actual original caller stack.
            actual=bytes(guest.mem_read(at,len(expected)))
            compared+=len(expected)
            if actual!=expected:
                differences.append(dict(field='retained_world',address=f'{at:08x}',
                    offsets=[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:32]))
    except Exception as e:error=str(e)
    return dict(case=path.name,declared_subpart_request=case['declared_subpart_request'],input_sha256=sha(source),case_sha256=sha(path),
        instructions=count,unique_instructions=len(instructions),calls=dict(calls),
        retained_bytes=compared,error=error,differences=differences,passed=error is None and not differences)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',type=Path,required=True)
    args=parser.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT)
    cases=sorted(args.case.glob('cull-[0-9]*.json'))
    assert cases,'No genuine native cull cases'
    results=[compare(path,image) for path in cases]
    active=sum(row['calls'].get('00045550',0) for row in results)
    posed=sum(row['calls'].get('000bac30',0) for row in results)
    report=dict(executable_sha256=NEW_PIN,source_unchanged=True,tool_sha256=sha(Path(__file__)),
        cases=results,subpart_cull_calls=active,world_node_reads=posed,all_cases_match=all(row['passed'] for row in results),
        passed=all(row['passed'] for row in results) and active>0 and posed>0,
        boundary=f"{sum(not row['declared_subpart_request'] for row in results)} actual native light queries plus {sum(row['declared_subpart_request'] for row in results)} explicitly declared subpart-cull requests on their actual model geometry and pose. Exact CPU evidence, not original live attribution or matched pixels.")
    (args.case/'original-cull.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({**report,'cases':[{k:v for k,v in r.items() if k!='differences'} for r in results]},indent=2))
    return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
