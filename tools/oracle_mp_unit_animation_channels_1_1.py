#!/usr/bin/env python3
"""Execute complete original channel/frame/event owners on declared API inputs.

Raw channel, graph, definition, clock and event records are component inputs,
including disabled resource initialization gates. Timing, sound and effect
callbacks are an externally owned observation ABI: their return is unused.
The observer executes only RET12 and supplies no animation, IO or effect
success. Original image instructions and constants remain unchanged. This
does not establish live graph/resource/pose producers or a playable death.
"""
import argparse, hashlib, itertools, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from export_mp_ragdoll_owners import spans, sha

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==PIN
    base,stack,sp,stop,callback=0x1000000,0x2000000,0x200f000,0x3000000,0x3001000
    tags,graph,definition,channel,clock,timing,sounds,effects,srefs,erefs=[base+i*0x1000 for i in range(10)]
    rows=[];audit=[]
    values=(-2.,-0.,0.,.5,1.,6.,7.999,8.,9.,42.25,float('inf'),float('nan'))
    roots=(0x1ca090,0x1c9f50,0x1ca2b0,0x1cae40)
    for rounding,entry,variant in itertools.product(range(4),roots,range(144)):
        name=f'round{rounding}_{entry:08x}_{variant}'
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(base,0x10000);g.mem_map(stack,0x10000);g.mem_map(stop,0x2000)
        # Explicit external observer, not an original-code patch or a supplied
        # original dependency result. No source owner consumes its EAX.
        g.mem_write(callback,b'\xc2\x0c\x00')
        def put(p,v,n=4):g.mem_write(p,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
        def fw(v):return struct.unpack('<I',struct.pack('<f',v))[0]
        def pf(p,v):put(p,fw(v))
        def u(p):return int.from_bytes(g.mem_read(p,4),'little')
        put(0x4eb114,tags);put(tags+8,graph);put(graph+0x30,definition)
        put(graph+0x18,srefs);put(graph+0x20,erefs)
        put(0x518444,clock);pf(clock+4,1/30);put(0x518410,0,1);put(0x518411,0,1)
        put(channel,0x81000000);put(channel+4,0xffff)
        flags=(0,1,2,3,7,0x83,0x103,0x203,0x3003)[(variant//3)%9]
        state=(0,1,2,9)[(variant//9)%4]
        old=values[variant%len(values)];requested=values[(variant//len(values)+variant)%len(values)]
        rate=(1.,-1.,0.,.5)[(variant//5)%4]
        count=(8,42)[(variant//7)%2];loop=(0,2,5)[(variant//11)%3]
        options=flags
        put(channel+0x10,1 if variant%13==0 else 0,1);put(channel+0x11,state,1)
        put(channel+0x12,flags,2);put(channel+0x14,0xa55a,2);put(channel+0x16,0x5aa5,2)
        pf(channel+0x18,rate);pf(channel+0x1c,old)
        put(definition+0x14,count,2);put(definition+0x2e,loop,2)
        event_count=(0,1,3,4,5,8)[(variant//2)%6]
        put(definition+0x4c,event_count);put(definition+0x50,timing)
        put(definition+0x54,8);put(definition+0x58,sounds);put(definition+0x5c,8);put(definition+0x60,effects)
        for i,f in enumerate((-1,0,1,2,5,7,8,42)):
            put(timing+i*4,(1,5,8,15,16,31,32,-1)[i],2);put(timing+i*4+2,f,2)
            for p,stride in ((sounds,8),(effects,4)):
                put(p+i*stride,i if i!=6 else -1,2);put(p+i*stride+2,f,2)
            put(sounds+i*8+4,0x6000200+i)
            for p in (srefs,erefs):
                put(p+i*12+4,0x82000000+i if i!=7 else -1)
                put(p+i*12+8,(0,2,4,0x10,0x20,6,0,0x30)[i],2)
        cb=callback if variant%4 else 0;context=0x87000000+variant
        cw,mx=0x27f+(rounding<<10),0x1f80+(rounding<<13)
        put(sp,stop)
        args={0x1ca090:(channel,fw(requested),channel+0x100,cb,context),
              0x1c9f50:(fw(requested),),0x1ca2b0:(channel+0x100,cb,context),
              0x1cae40:(graph,definition,options,fw(old),fw(requested),context)}[entry]
        for i,v in enumerate(args):put(sp+4+i*4,v)
        for reg,v in ((R.UC_X86_REG_ESP,sp),(R.UC_X86_REG_EAX,channel),
                      (R.UC_X86_REG_EBX,cb),(R.UC_X86_REG_FPCW,cw),(R.UC_X86_REG_MXCSR,mx),
                      (R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_FPSW,0)):
            g.reg_write(reg,v)
        initial={};current={};writes=[];calls=[];instruction=0
        def touch(p,raw):
            for i,v in enumerate(raw):
                if p+i not in current:initial[p+i]=current[p+i]=v
                assert current[p+i]==v,(name,hex(p+i))
        def code(uc,p,size,user):
            nonlocal instruction
            instruction=p
            if p==callback:
                es=uc.reg_read(R.UC_X86_REG_ESP)
                argv=[u(es+4+i*4)for i in range(3)]
                calls.append((es,argv,bytes(g.mem_read(argv[2],20))))
        def read(uc,access,p,size,value,user):touch(p,bytes(uc.mem_read(p,size)))
        def write(uc,access,p,size,value,user):
            before=bytes(uc.mem_read(p,size));touch(p,before)
            raw=(value&((1<<(8*size))-1)).to_bytes(size,'little')
            for i,v in enumerate(raw):current[p+i]=v
            if not stack<=p<stack+0x10000:writes.append((instruction,p,before,raw))
        touch(channel,bytes(g.mem_read(channel,32)))
        g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
        g.emu_start(entry,stop,count=100000)
        assert g.reg_read(R.UC_X86_REG_EIP)==stop,(name,hex(g.reg_read(R.UC_X86_REG_EIP)))
        result=g.reg_read(R.UC_X86_REG_EAX)if entry==0x1cae40 else 0
        rows.append(f'C {name} {entry:08x} {channel:08x} {graph:08x} {definition:08x} {options:08x} {fw(old):08x} {fw(requested):08x} {cb:08x} {context:08x} {sp:08x} {cw:04x} {mx:08x} {result:08x}')
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        for site,p,before,raw in writes:rows.append(f'W {site:08x} {p:08x} {before.hex()} {raw.hex()}')
        for es,argv,raw in calls:rows.append('P '+f'{es:08x} '+' '.join(f'{v:08x}'for v in argv)+' '+raw.hex())
        rows.append(f'A {channel:08x} '+bytes(g.mem_read(channel,32)).hex())
        audit.append(dict(name=name,entry=f'{entry:08x}',variant=variant,stores=len(writes),callbacks=len(calls)))
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    (a.output/'report.json').write_text(json.dumps(dict(tool_sha256=sha(__file__),source_sha256=PIN,
        fixture_sha256=sha(fixture),cases=audit,boundaries=__doc__,instruction_patches=[],
        external_observer=dict(address=f'{callback:08x}',code_hex='c20c00',return_consumed=False),
        supplied_gameplay_or_io_results=[],native_expected_data_used=False,live_gameplay_proof=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),callbacks=sum(c['callbacks']for c in audit),fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
