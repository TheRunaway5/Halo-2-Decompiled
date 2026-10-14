#!/usr/bin/env python3
"""Unchanged TU1.5 136230 on a fresh native world's model receiver inputs.

The declared C++ caller supplies its camera, admitted queue and empty shared
draw state. Original model building, fade, presentation, cache and resource
owners run unchanged. Outputs are compared only after execution; no callee result
or intermediate model collector is supplied. This is CPU collection evidence,
not a rendered-frame or physical Xbox comparison.
"""
import argparse
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
    return struct.pack('<'+'I'*len(values),*values)


def words(raw):
    return list(struct.unpack('<'+'I'*(len(raw)//4),raw))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--case',type=Path,required=True)
    a=p.parse_args()
    call=json.loads((a.case/'call.json').read_text())
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT)
    guest=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set()
    banks=list(regions(a.case/'before.bin'))
    def mount(address,data):
        pages=[p for p in range(address&~4095,(address+len(data)+4095)&~4095,4096) if p not in mapped]
        first=0
        while first<len(pages):
            end=first+1
            while end<len(pages) and pages[end]==pages[end-1]+4096:end+=1
            guest.mem_map(pages[first],(end-first)*4096);first=end
        mapped.update(pages)
        guest.mem_write(address,data)
    for address,data in banks:mount(address,data)
    context,queue,streams=0x65000000,0x65003000,0x65004000
    mount(context,bytes(0x10000));mount(0x70000000,bytes(0x5000))
    word=lambda p:int.from_bytes(guest.mem_read(p,4),'little')
    put=lambda p,v:guest.mem_write(p,pack([v]))
    declared=[]
    def input_bytes(p,raw):
        declared.append((p,bytes(guest.mem_read(p,len(raw)))))
        guest.mem_write(p,raw)
    def input_words(p,values):input_bytes(p,pack(values))
    # Explicit view delivery, identical to the native caller's fields.
    input_words(0x4c6634,call['camera']);input_words(0x4c65c0,call['eye'])
    input_words(0x4c65cc,call['forward']);input_words(0x4c65ec,[call['distance_scale']])
    input_bytes(0x4c65f0,struct.pack('<4h',*call['viewport']))
    input_words(0x4c6848,[call['coverage_multiplier']])
    input_words(0x4c6710,[0]);input_words(0x4c67ac,[0xffffffff])
    input_words(0x4c681c,[0]);input_bytes(0x4c6824,b'\0')
    input_bytes(0x515ef0,b'\0\0');input_words(0x4cd3a0,[context])
    for address,size in ((0x4c6958,850*32),(0x4cd3a4,850),(0x4cd6f6,850),
                         (0x4cda48,192*4),(0x4cdd4c,32*36)):
        input_bytes(address,bytes(size))
    for address in (0x4cd39c,0x4cd398,0x4cdd48,0x4ce1cc):input_words(address,[0])
    put(0x4c6870,call['view'])
    put(0x4c683c,3 if call['players'] in (2,3) else 2 if call['players']==4 else 4)
    guest.mem_write(0x4c6839,bytes([int(call['modifier'])]))
    put(context+0x14,queue);put(context+0x2a78,call['mode'])
    guest.mem_write(context+0xa5e,struct.pack('<H',len(call['associations'])))
    for n,mask in enumerate(call['associations']):guest.mem_write(context+0xa60+n*16,pack(mask))
    guest.mem_write(queue,pack([256,len(call['queue_before']),streams,streams+0x1000,streams+0x2000,streams+0x3000]))
    for n,(owner,flags,other,association) in enumerate(call['queue_before']):
        guest.mem_write(streams+n*2,struct.pack('<H',flags));put(streams+0x1000+n*4,owner)
        put(streams+0x2000+n*4,other);guest.mem_write(streams+0x3000+n*2,struct.pack('<H',association))
    # Same declared reference-only target used by the isolated builder oracle;
    # its GPU multisample field is unused by the CPU bitmap caller.
    input_words(0x411568,[0x70002000]);put(0x70003a14,0x70004000);put(0x70004004,0x12345678)
    guest.mem_write(0x70000008,segment(0,0x9b)+segment(0,0x93)+segment(0x60200000,0x93))
    guest.reg_write(R.UC_X86_REG_GDTR,(0,0x70000000,0xfff,0))
    for reg,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),
                      (R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):guest.reg_write(reg,value)
    stack,stop=0x600f0000,0x60300000
    guest.mem_write(stack,pack([stop,call['children']]))
    for name,value in [('EAX',context),('ECX',0),('EDX',0),('EBX',0),('ESP',stack),('EBP',0),('ESI',0),('EDI',0)]:
        guest.reg_write(getattr(R,'UC_X86_REG_'+name),value)
    for n in range(8):guest.reg_write(getattr(R,f'UC_X86_REG_XMM{n}'),0)
    for reg,value in ((R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),
                      (R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):guest.reg_write(reg,value)
    instructions={};count=0
    def original(vm,pc,n,_):
        nonlocal count
        actual=bytes(vm.mem_read(pc,n))
        assert actual==image.read(pc,n),f'Non-original instruction at {pc:08x}'
        instructions[pc]=actual.hex();count+=1
    guest.hook_add(UC_HOOK_CODE,original)
    differences=[];error=None;compared=0;actual={}
    try:
        guest.emu_start(0x136230,stop,count=10000000)
        assert guest.reg_read(R.UC_X86_REG_EIP)==stop
        actual['counts']=[word(p) for p in (0x4cd39c,0x4cd398,0x4cdd48,0x4ce1cc)]
        actual['units']=[words(guest.mem_read(0x4c6958+n*32,32)) for n in range(850)]
        for unit in actual['units']:
            if unit[0]&0x40 and context+0x25c<=unit[5]<context+0xa5c:
                unit[5]=call['arena_address']+unit[5]-(context+0x25c)
        actual['coverage']=list(guest.mem_read(0x4cd3a4,850));actual['override_slots']=list(guest.mem_read(0x4cd6f6,850))
        actual['overrides']=words(guest.mem_read(0x4cda48,192*4))
        actual['sections']=[words(guest.mem_read(0x4cdd4c+n*36,36)) for n in range(32)]
        actual['arena_used']=int.from_bytes(guest.mem_read(context+0xa5c,2),'little')
        actual['arena']=words(guest.mem_read(context+0x25c,2048))
        actual['queue_after']=[[word(streams+0x1000+n*4),int.from_bytes(guest.mem_read(streams+n*2,2),'little'),
            word(streams+0x2000+n*4),int.from_bytes(guest.mem_read(streams+0x3000+n*2,2),'little')]
            for n in range(len(call['queue_before']))]
        for key,value in actual.items():
            if value!=call[key]:differences.append(dict(field=key,original=value,native=call[key]))
        # Camera/query/draw-state lives in typed native caller storage. Its
        # original-ABI projection is compared above, then restored to initial
        # bytes for the independent shared model/cache/resource comparison.
        for p,raw in reversed(declared):guest.mem_write(p,raw)
        for address,expected in regions(a.case/'after.bin'):
            if 0x60000000<=address<0x60200000 or address==0x60400000:continue
            observed=bytes(guest.mem_read(address,len(expected)));compared+=len(expected)
            if observed!=expected:
                offsets=[n for n,(x,y) in enumerate(zip(observed,expected)) if x!=y]
                differences.append(dict(bank=f'{address:08x}',bytes=len(offsets),
                    first=[dict(address=f'{address+n:08x}',original=observed[n],native=expected[n]) for n in offsets[:16]]))
    except Exception as e:error=str(e)
    report=dict(scope=__doc__,passed=not error and not differences,error=error,
        original_sha256=NEW_PIN,original_instructions=count,unique_instructions=len(instructions),
        retained_bytes_compared=compared,original_pc=f'{guest.reg_read(R.UC_X86_REG_EIP):08x}',
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        differences=differences,original_instruction_patches=[],callee_substitutions=0)
    (a.case/'original-collection.json').write_text(json.dumps(report,indent=2)+'\n')
    (a.case/'original-instructions.json').write_text(json.dumps(instructions,sort_keys=True)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='differences'}))
    print('Differing banks/fields:',[(v.get('bank',v.get('field')),v.get('bytes')) for v in differences])
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
