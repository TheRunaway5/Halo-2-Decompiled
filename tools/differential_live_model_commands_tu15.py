#!/usr/bin/env python3
"""Compare unchanged TU1.5 40E80 with fresh native live-model light commands.

Inputs are the actual live world after native model collection and the declared
typed caller's unit/camera/queue banks. No intermediate callee result is given.
All three command banks, call order and retained world bytes are compared.
This is CPU command evidence, not a rendered original-frame comparison.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from differential_physics_native_frame_tu15_20261012 import regions,segment


def pack(v):return struct.pack('<'+'I'*len(v),*v)
def words(b):return list(struct.unpack('<'+'I'*(len(b)//4),b))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--case',type=Path,required=True);a=p.parse_args()
    call=json.loads((a.case/'call.json').read_text());expected=json.loads((a.case/'commands.json').read_text())
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);g=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set()
    def mount(at,data):
        pages=[p for p in range(at&~4095,(at+len(data)+4095)&~4095,4096) if p not in mapped]
        first=0
        while first<len(pages):
            end=first+1
            while end<len(pages) and pages[end]==pages[end-1]+4096:end+=1
            g.mem_map(pages[first],(end-first)*4096);first=end
        mapped.update(pages);g.mem_write(at,data)
    for at,data in regions(a.case/'commands-before.bin'):mount(at,data)
    context,queue,streams=0x65000000,0x65003000,0x65004000
    mount(context,bytes(0x10000));mount(0x70000000,bytes(0x1000))
    word=lambda p:int.from_bytes(g.mem_read(p,4),'little')
    put=lambda p,v:g.mem_write(p,pack([v]))
    declared=[]
    def source(p,raw):
        declared.append((p,bytes(g.mem_read(p,len(raw)))));g.mem_write(p,raw)
    def source_words(p,v):source(p,pack(v))
    source_words(0x4cd3a0,[context]);put(context+0x14,queue);put(context+0x2a78,call['mode'])
    source_words(0x4c6958,[v for row in call['units'] for v in row])
    source(0x4cd3a4,bytes(call['coverage']));source(0x4cd6f6,bytes(call['override_slots']))
    source_words(0x4cda48,call['overrides'])
    for address,value in zip((0x4cd39c,0x4cd398,0x4cdd48,0x4ce1cc),call['counts']):source_words(address,[value])
    source_words(0x473644,[expected['player_multiplier']]);source_words(0x492348,expected['lod_view'])
    source(0x4c6839,bytes([int(call['modifier'])]))
    g.mem_write(context+0x25c,pack(call['arena']));g.mem_write(context+0xa5c,struct.pack('<H',call['arena_used']))
    g.mem_write(queue,pack([256,len(call['queue_after']),streams,streams+0x1000,streams+0x2000,streams+0x3000]))
    for n,(owner,flags,other,association) in enumerate(call['queue_after']):
        g.mem_write(streams+n*2,struct.pack('<H',flags));put(streams+0x1000+n*4,owner)
        put(streams+0x2000+n*4,other);g.mem_write(streams+0x3000+n*2,struct.pack('<H',association))
    for index,unit in enumerate(call['units'][:call['counts'][1]]):
        if unit[0]&0x40:put(0x4c6958+index*32+20,context+0x25c+unit[5]-call['arena_address'])
    headers=[0x4ce268+n*24 for n in range(3)]
    for p in headers:
        source(p,bytes(g.mem_read(p,24)))
        count,data=word(p),word(p+20);source(data,bytes(g.mem_read(data,count*24)))
    special=0x4d2b18+word(0x515f20)*0x408
    source(special,bytes(g.mem_read(special,0x406)))
    g.mem_write(0x70000008,segment(0,0x9b)+segment(0,0x93)+segment(0x60200000,0x93))
    g.reg_write(R.UC_X86_REG_GDTR,(0,0x70000000,0xfff,0))
    for reg,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):g.reg_write(reg,value)
    stack,stop=0x600f0000,0x60300000;put(stack,stop)
    for name,value in [('EAX',0),('ECX',0),('EDX',0),('EBX',0),('ESP',stack),('EBP',0),('ESI',0),('EDI',0)]:g.reg_write(getattr(R,'UC_X86_REG_'+name),value)
    for n in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{n}'),0)
    for reg,value in ((R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):g.reg_write(reg,value)
    points={0x40e80:0x40e90,0x41c70:0x41c80,0x44ab0:0x44ac0,0x449d0:0x449e0,0x45cd0:0x45ce0,
        0x186e70:0x184000,0x460c0:0x460d0,0x41030:0x41040,0x1bf30:0x1bf50,0x40e20:0x40e30,0x3b9c0:0x3b9d0,0x1d690:0x1d6a0}
    instructions={};count=0;calls=[];trace_commands=True;palette_stops=[]
    def original(vm,pc,n,_):
        nonlocal count
        actual=bytes(vm.mem_read(pc,n));assert actual==image.read(pc,n),f'Non-original instruction at {pc:08x}'
        instructions[pc]=actual.hex();count+=1
        if trace_commands and pc in points:calls.append(points[pc])
    g.hook_add(UC_HOOK_CODE,original)
    differences=[];error=None;compared=0
    try:
        g.emu_start(0x40e80,stop,count=10000000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
        actual=[]
        for index,p in enumerate(headers):
            h=words(g.mem_read(p,24));storage=[words(g.mem_read(h[5]+n*24,24)) for n in range(h[0])]
            h[5]=expected['banks'][index]['header'][5] # Native data identity is an explicit typed allocation.
            for row in storage[:h[1]]:
                if row[0]==0x4cbe0:row[0]=0x4cbf0
                if row[1]==0x4d0a0:row[1]=0x4d0b0
            actual.append(dict(header=h,storage=storage))
        for key,value in dict(banks=actual,calls=calls,
            special_units=[words(g.mem_read(special+n*32,32)) for n in range(32)],
            special_count=int.from_bytes(g.mem_read(special+0x404,2),'little',signed=True)).items():
            if value!=expected[key]:differences.append(dict(field=key,original=value,native=expected[key]))
        trace_commands=False
        output=0x65009000
        source_words(0x4c6834,[1]) # Declared ordinary selected geometry frame.
        g.mem_write(output,bytes(1024*24))
        g.mem_write(stack,pack([stop,headers[1],0,12,output]));g.reg_write(R.UC_X86_REG_ESP,stack)
        g.emu_start(0x3baa0,stop,count=10000000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
        for key,value in dict(selected_count=g.reg_read(R.UC_X86_REG_EAX),
            selected=[words(g.mem_read(output+n*24,24)) for n in range(1024)]).items():
            if value!=expected[key]:differences.append(dict(field=key,original=value,native=expected[key]))
        # Observe the unchanged4D820/15280 program at its first actual401410
        # GPU constant-upload boundary. No device result or return is supplied.
        for n,palette in enumerate(expected['palettes']):
            put(stack,stop);g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_EBX,0x4c6958+n*32)
            g.emu_start(0x4d820,0x401410,count=10000000)
            assert g.reg_read(R.UC_X86_REG_EIP)==0x401410
            pointer=g.reg_read(R.UC_X86_REG_EDX);start=g.reg_read(R.UC_X86_REG_ECX)
            size=word(g.reg_read(R.UC_X86_REG_ESP)+4)
            original=words(g.mem_read(pointer,size*4));native=[w for row in palette for w in row]
            palette_stops.append(dict(unit=n,constant=start,words=size))
            if start!=50 or original!=native:differences.append(dict(field='palette',unit=n,original=original,native=native))
        for p,b in reversed(declared):g.mem_write(p,b)
        for at,data in regions(a.case/'commands-after.bin'):
            if 0x60000000<=at<0x60200000:continue
            observed=bytes(g.mem_read(at,len(data)));compared+=len(data)
            if observed!=data:
                offsets=[n for n,(x,y) in enumerate(zip(observed,data)) if x!=y]
                differences.append(dict(bank=f'{at:08x}',bytes=len(offsets),
                    first=[dict(address=f'{at+n:08x}',original=observed[n],native=data[n]) for n in offsets[:16]]))
    except Exception as e:error=str(e)
    report=dict(scope=__doc__,passed=not error and not differences,error=error,original_sha256=NEW_PIN,
        original_instructions=count,unique_instructions=len(instructions),retained_bytes_compared=compared,
        original_pc=f'{g.reg_read(R.UC_X86_REG_EIP):08x}',tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        differences=differences,original_instruction_patches=[],callee_substitutions=0,
        palette_observation_boundaries=palette_stops)
    (a.case/'original-commands.json').write_text(json.dumps(report,indent=2)+'\n')
    (a.case/'original-command-instructions.json').write_text(json.dumps(instructions,sort_keys=True)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='differences'}))
    print('Differing banks/fields:',[(v.get('bank',v.get('field')),v.get('bytes')) for v in differences])
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
