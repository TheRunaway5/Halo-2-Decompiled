#!/usr/bin/env python3
"""Unchanged1B0A0→143C0→137550→12310/12CCF0 bitmap requests.

The actual resident cache is an explicit already-loaded future-deadline input,
so12310 completes without allocation/IO services. Separate cache fixtures run
the hardware partition lookup and stop at the actual allocation request entry.
No original calls or math are replaced. Async cache history remains explicit.
"""
import argparse,hashlib,json,math,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,CONTEXT,STACK,STOP=0x2100000,0x2120000,0x22fef00,0x3000000
TABLE,TAGS,RECORDS,GLOBALS=BASE,BASE+0x1000,BASE+0x3000,BASE+0x10000
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(f):return words(struct.pack('<f',f))[0]
def real(x):return struct.unpack('<f',pack([x]))[0]
def signed(x):return struct.unpack('<i',pack([x]))[0]
def digest(data):return hashlib.sha256(data).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--binary',type=Path,required=True)
    ap.add_argument('--count',type=int,default=4096);a=ap.parse_args()
    x=Xbe(ROOT/'assets/disc/default.xbe');assert digest(x.data)==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for section in x.sections:u.mem_write(section.va,x.section_bytes(section))
    u.mem_map(BASE,0x200000);u.mem_map(STOP,0x1000)
    u.mem_write(0x4e3b44,pack([TABLE]));u.mem_write(0x485a80,pack([GLOBALS]))
    default=0xf0000005;u.mem_write(GLOBALS+0x64,pack([default]));u.mem_write(0x4e6488,pack([17]))
    for index in range(1,6):
        tag=TAGS+index*0x100;records=RECORDS+index*0x1000
        u.mem_write(TABLE+index*16,pack([0x6269746d,0xf0000000+index,tag,0x4c]))
        u.mem_write(tag+0x44,pack([8,records]))
        for image in range(8):
            r=records+image*0x74;u.mem_write(r+4,struct.pack('<HH',64>>min(image,5),32>>min(image,4)))
            u.mem_write(r+0x50,pack([0x4000000+index*0x100+image]));u.mem_write(r+0x70,pack([1000]))
    seen={};requests=[];phase=['stage'];needs_request=[False]
    entry_counts={p:0 for p in [0x1b0a0,0x143c0,0x137550,0x12310,0xd15e0,0x12ccf0,0x12ccb0,0x12c960,0x12ce00]}
    request_flags=[3]
    def observe(emu,pc,size,_):
        if pc==STOP:emu.emu_stop();return
        assert bytes(emu.mem_read(pc,size))==x.read(pc,size),(hex(pc),'changed original instruction')
        seen[pc]=size
        if pc in entry_counts:entry_counts[pc]+=1
        if pc==0x12ce00 and phase[0]=='peek':
            #The original12310/D15E0/12CCF0 prefix has returned0. Stop at
            #the actual resource request entry, before fallback/allocation;
            #no success or source resource result is supplied.
            esp=emu.reg_read(UC_X86_REG_ESP)
            assert words(emu.mem_read(esp+8,4))[0]==request_flags[0]
            needs_request[0]=True;emu.emu_stop();return
        if pc==0x143c0:
            esp=emu.reg_read(UC_X86_REG_ESP);stage=words(emu.mem_read(esp+4,4))[0]
            requests.extend([stage,emu.reg_read(UC_X86_REG_EAX),emu.reg_read(UC_X86_REG_ECX)&65535,
                             emu.reg_read(UC_X86_REG_XMM2)&0xffffffff])
    u.hook_add(UC_HOOK_CODE,observe)
    def original(row):
        requests.clear();u.mem_write(CONTEXT,bytes(0x1800));u.mem_write(CONTEXT+0x2c,pack(row[:1]))
        u.mem_write(CONTEXT+0x1420,pack(row[2:3]));u.mem_write(0x4ba019,bytes([bool(row[1])]))
        for index in range(4):
            datum,count,flags,frame,scalar,previous=row[3+index*6:9+index*6]
            stage=CONTEXT+0x1320+index*64
            u.mem_write(stage,pack([datum,flags,0,0,previous,scalar,frame]))
            u.mem_write(TAGS+(index+1)*0x100+0x44,pack([count]))
        u.mem_write(STACK,pack([STOP,CONTEXT]));u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0)
        u.emu_start(0x1b0a0,STOP,count=1000000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP,'original request did not finish'
        minima=[words(u.mem_read(CONTEXT+0x1330+i*64,4))[0]for i in range(4)]
        return minima+[len(requests)//4]+requests.copy()
    rng=random.Random(0x1b0a0143c0);rows=[];labels=[]
    def add(adjust,modifier,mask,stages,label):
        rows.append([bits(adjust),modifier,mask,*sum(stages,[])]);labels.append(label)
    def stage(index,count=8,flags=0,frame=0,scalar=2.882718086242676,previous=3,none=False):
        return [0xffffffff if none else 0xf0000001+index,count,flags,bits(frame),bits(scalar),bits(previous)]
    #Literal8787 authored postprocess bitmap scalars, distinct from any fitted
    #captured mip selection. Clock/coverage is an explicit input of each case.
    scalars=[2.882718086242676,3.363171100616455,3.8436241149902344,3.8436241149902344]
    for adjust in [-20,-4,-3,-2,-1,-.5,0,.5,1,2,4,20,float('inf'),-float('inf')]:
        for modifier in [0,1]:
            add(adjust,modifier,15,[stage(i,scalar=v)for i,v in enumerate(scalars)],'literal8787 scalar bank/explicit coverage')
    for mask in range(16):
        for force in [0,1]:
            add(-1,0,mask,[stage(i,flags=force,none=i==2)for i in range(4)],'enabled/force/NONE ordered stage controls')
    frames=[0,-0.,.0000999999975,.0001,-.0001,.5,-.5,1,-1,7.9999995,-7.9999995,1e10,-1e10]
    for frame in frames:
        for previous in [0,1,2,3]:
            add(-1,0,15,[stage(i,count=(i%3)+1,frame=frame,previous=previous)for i in range(4)],'x87 wrap/negative floor/future minimum')
    for boundary in [0,1,2,3,4]:
        #Adjacent float32 scalar inputs before the actual SUBSS .5. Taking
        #nextafter in Python double and then narrowing would erase the edge.
        b=bits(float(boundary)+.5)
        for scalar in [real(b-1),real(b),real(b+1)]:
            add(0,0,15,[stage(i,scalar=scalar,previous=3)for i in range(4)],'request clamp/floor adjacent')
    for _ in range(a.count):
        add(rng.uniform(-10,10),rng.randrange(2),rng.randrange(16),[
            stage(i,count=rng.randrange(1,9),flags=rng.randrange(4),frame=rng.choice(frames),
                  scalar=rng.uniform(-8,8),previous=rng.randrange(4),none=rng.randrange(9)==0)
            for i in range(4)],'declared finite metadata/context control')
    outputs=[original(row)for row in rows]
    #Persistent minimum is per binding context. Increasing a later request
    #does not issue another service call or restore the old bucket.
    for series in [[0,-1,-2,-3,0],[2,1,0,-1,-2,1]]:
        stages=[stage(i,scalar=v)for i,v in enumerate(scalars)]
        for adjust in series:
            add(adjust,0,15,stages,'unchanged successive minimum-state control');out=original(rows[-1]);outputs.append(out)
            stages=[s[:-1]+[out[i]]for i,s in enumerate(stages)]
    request_case_count=len(rows)
    #Genuine12310→12CCF0 hardware-cache lookup. Actual pool/handle/LRU rows
    #are explicit caller state, including ready/pending/missing partitions.
    RECORD=BASE+0x19000;POOL=BASE+0x18000;IMAGES=BASE+0x1a000
    CACHE=BASE+0x1b000;LRUPOOL=BASE+0x1c000;LRU=BASE+0x1d000
    u.mem_write(0x4e6454,pack([POOL]));u.mem_write(POOL+0x44,pack([IMAGES]))
    u.mem_write(0x4e6464,pack([CACHE]));u.mem_write(CACHE+0x64,pack([LRUPOOL]))
    u.mem_write(LRUPOOL+0x44,pack([LRU]));u.mem_write(0x4e647b,b'\0')
    def original_peek(row):
        phase[0]='peek';needs_request[0]=False
        request,flags,quality,bias,clock,use,bitmapflags,bytebias,cutoff,cached,until,software=row[:12]
        assert flags in [0,3] and (len(row)==29 or not software or not(bitmapflags&2) or (cached and signed(until)>signed(clock)))
        request_flags[0]=flags
        u.mem_write(RECORD,bytes(0x74));u.mem_write(RECORD+0xf,bytes([bitmapflags]))
        u.mem_write(RECORD+0x16,bytes([bytebias]));u.mem_write(RECORD+0x50,pack([cached]))
        u.mem_write(RECORD+0x58,pack([cutoff]));u.mem_write(RECORD+0x70,pack([until]))
        if len(row)==29:
            category,control,pending,software_token,software_enabled=row[24:]
            assert software_token==RECORD+0x5c
            u.mem_write(RECORD+0x17,bytes([category]));u.mem_write(RECORD+0x5c,pack([control]))
            u.mem_write(RECORD+0x64,pack([pending]));u.mem_write(0x4e6468,bytes([bool(software_enabled)]))
        u.mem_write(0x468841,bytes([bool(quality)]));u.mem_write(0x4e647c,pack([bias]))
        u.mem_write(0x4e647b,bytes([bool(software)]))
        u.mem_write(0x4e6488,pack([clock]));u.mem_write(CACHE+0x38,pack([use]))
        for i in range(3):
            handle,token,ready,last=row[12+i*4:16+i*4]
            u.mem_write(RECORD+0x28+i*4,pack([handle]));u.mem_write(IMAGES+(i+1)*40+3,bytes([bool(ready)]))
            u.mem_write(LRU+(i+1)*24+0x14,pack([last]));assert token==IMAGES+(i+1)*40+0x14
        u.mem_write(STACK,pack([STOP]));u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_EAX,RECORD);u.reg_write(UC_X86_REG_XMM2,request)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0)
        u.emu_start(0xd15e0 if flags==0 else 0x12310,STOP,count=1000000)
        assert u.reg_read(UC_X86_REG_EIP)==(0x12ce00 if needs_request[0]else STOP)
        token=0 if needs_request[0]else u.reg_read(UC_X86_REG_EAX)
        return [token,*words(u.mem_read(RECORD+0x50,4)),*words(u.mem_read(RECORD+0x70,4)),
                *[words(u.mem_read(LRU+(i+1)*24+0x14,4))[0]for i in range(3)],int(needs_request[0])]
    def peek(request=0,quality=0,bias=0,clock=17,use=91,flags=2,bytebias=0,cutoff=5,
             cached=0,until=0,ready=(1,1,1),present=(1,1,1),software=0,request_mode=3,
             software_state=None,label='declared source cached partition state'):
        row=[bits(request),request_mode,quality,bits(bias),clock&0xffffffff,use&0xffffffff,flags,bytebias,bits(cutoff),cached,until&0xffffffff,software]
        for i in range(3):row.extend([0xe0000001+i if present[i]else 0xffffffff,IMAGES+(i+1)*40+0x14,ready[i],123+i])
        if software_state is not None:row.extend(software_state)
        rows.append(row);labels.append(label);outputs.append(original_peek(row))
    for request in [-2,0,.9999999403953552,1,1.0000001192092896,1.9999998807907104,2,2.000000238418579,4,5,float('inf'),-float('inf')]:
        for quality in [0,1]:
            for cached,until in [(0,0),(0x4100000,16),(0x4100000,17),(0x4100000,18)]:
                peek(request,quality,.25,cached=cached,until=until,bytebias=7,label='source deadline/partition/bias edge')
    for flags in [0,2]:
        for ready in [(0,0,0),(0,1,0),(1,0,1)]:
            for present in [(0,0,0),(1,1,1),(1,0,1)]:
                for request in [0,1,2]:peek(request,flags=flags,ready=ready,present=present,label='pending/missing/direct hardware-image state')
    for flags in [0,2]:
        for cached in [0,0x4100000]:
            for until in [16,17,18]:
                if flags&2 and not(cached and until>17):continue
                peek(flags=flags,cached=cached,until=until,software=1,label='software mode bypassed by literal direct/cached path')
    for clock in [0x7ffffffe,0x7fffffff,0x80000000,0xfffffffe,0xffffffff]:
        peek(clock=clock,request=0,until=clock,label='source signed deadline/add2 overflow control')
    for quality in [0,1]:
        for cached,until in [(0,0),(0x4100000,16),(0x4100000,17),(0x4100000,18)]:
            for ready in [(0,0,0),(1,1,1)]:
                for present in [(0,0,0),(1,1,1)]:
                    peek(0,quality,.25,cached=cached,until=until,ready=ready,present=present,
                         request_mode=0,label='actual D30D0 zero-request/D15E0 prefix')
    for category in [0,1,2,3,11,12,14,16,17,18,255]:
        eligible=1<=category<=11 or category in [17,18]
        for enabled in [0,1]:
            for control in [0,0x40001]:
                for pending in [0,64]:
                    if eligible and enabled and not control and pending:continue #12C820 construction, not supplied.
                    peek(0,software=1,request_mode=0,
                         software_state=[category,control,pending,RECORD+0x5c,enabled],
                         label='complete12CCB0/12C960 existing inline-header admission')
    for _ in range(a.count//2):
        peek(rng.uniform(-4,8),rng.randrange(2),rng.uniform(-2,2),clock=rng.randrange(-10,25),
            use=rng.randrange(-20,500),flags=rng.choice([0,2]),bytebias=rng.randrange(256),cutoff=rng.uniform(-1,8),
            cached=rng.choice([0,0x4100000]),until=rng.randrange(-10,25),
            ready=tuple(rng.randrange(2)for _ in range(3)),present=tuple(rng.randrange(2)for _ in range(3)))
    stream=''.join(' '.join(f'{v:x}'for v in row)+'\n'for row in rows)
    result=subprocess.run([str(a.binary.resolve()),'--probe'],input=stream,text=True,capture_output=True)
    if result.returncode:raise RuntimeError(result.stderr)
    native=[[int(v,16)for v in line.split()]for line in result.stdout.splitlines()]
    assert len(native)==len(outputs)
    for i,(actual,expected)in enumerate(zip(native,outputs)):
        assert actual==expected,(i,labels[i],rows[i],actual,expected)
    fixture=ROOT/'tests/fixtures/retail_source_bitmap_residency.tsv'
    fixture.write_text('# Untouched1B0A0->143C0->137550->12310 and12310/D15E0->12CCF0 hardware cache; actual12CE00 entry stops\n'+''.join(
        ' '.join(f'{v:08x}'for v in row)+' | '+' '.join(f'{v:08x}'for v in out)+'\n'for row,out in zip(rows,outputs)))
    report=dict(passed=True,xbe_sha256=PIN,cases=len(rows),checked_words=sum(map(len,outputs)),
        ordered_calls=sum(out[4]for out in outputs[:request_case_count]),entry_counts={f'{p:08x}':n for p,n in entry_counts.items()},
        request_cases=request_case_count,peek_cases=len(rows)-request_case_count,
        cpu_peek_cases=sum(len(row)in[24,29] and row[1]==0 for row in rows),
        software_peek_cases=sum(len(row)==29 for row in rows),
        source_instruction_count=len(seen),source_instructions_sha256=digest(b''.join(pack([pc])+x.read(pc,n)for pc,n in sorted(seen.items()))),
        fpcw='027F',mxcsr='1F80',fixture_sha256=digest(fixture.read_bytes()),
        native_sha256=digest(a.binary.read_bytes()),labels={label:labels.count(label)for label in set(labels)},
        scope='Finite authored frame/scalar, literal initial3 or generated0..2 minimum, explicit context+2C/4BA019/active mask. Original143C0→12310 completes against explicit already-loaded future-deadline cache rows. Independent12310/D15E0→12CCF0 cases execute hardware-cache deadline/bias/partition/LRU paths plus complete12CCB0/12C960 existing software-header selection. Stop at actual12CE00 resource-request entry before any fallback/allocation result. No services/math replaced. This does not produce live allocation/deadline/residency history, physical mip uploads, source coverage log arithmetic,12C820 header initialization or original8787 mip choices.')
    path=ROOT/'analysis/multiplayer-shading/source-bitmap-residency-original.json';path.write_text(json.dumps(report,indent=2)+'\n')
    fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'{len(rows)} cases/{report["ordered_calls"]} ordered requests/{report["checked_words"]} words exact')
if __name__=='__main__':main()
