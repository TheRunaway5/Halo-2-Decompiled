#!/usr/bin/env python3
"""Compare the pure mode3 builder with complete unchanged retail3A5C0.

Owned explicit inputs restore actual captured SDK banks. Every executed
instruction must match the immutable XBE. Real installation callees execute;
no service, raster, shader, instruction or original emulator is replaced.
"""
import argparse
import collections
import hashlib
import json
import random
import struct
import subprocess
import sys
from pathlib import Path
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words
from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
BANK, DEPTH, STACK, STOP = 0x2100000, 0x2100100, 0x22fff00, 0x3000000
POISON = [0xc0de0000+i for i in range(60)]
RAM_PIN = 'f76d0569dedcd2e94d3f2ee4c67a97db47cefea848105a8b24fe08342daf24cd'

def packed(v): return struct.pack('<'+'I'*len(v), *v)
def fw(v): return uint(struct.pack('<f', v))
def f32(v): return struct.unpack('<f', struct.pack('<f', v))[0]
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def cases(snapshot):
    ref = ROOT/'analysis/multiplayer-shading/weather-plate-original-reference/event-215778'
    raw = words((ref/'source-mode3-input52.bin').read_bytes())
    table=uint(snapshot.read(0x4e3b44,4));root=uint(snapshot.read(table+(0xe77d0608&65535)*16+8,4))
    plate=uint(snapshot.read(root+12,4))
    baseline = [*raw, uint(snapshot.read(plate+0x80,4)), uint(snapshot.read(plate+0x84,4)), uint(snapshot.read(0x485658,4)),
        uint(snapshot.read(0x48565c,4)), 3, 0, 0, 1, 1, 1, 0x023f, 0x1fa0]
    result, labels, assets = [], [], []
    def add(label, start=10., end=30., far=1024., near=1./64., flags=(0,0,1),
            compositor=None, record_flag=None, depth=True, skip=1, cw=0x023f, mx=0x1f80):
        rec = list(raw if compositor is None else compositor)
        if record_flag is not None: rec[0] = record_flag
        result.append([*rec, fw(start), fw(end), fw(near), fw(far), 3, *flags, skip, int(depth), cw, mx])
        labels.append(label)
    result.append(baseline); labels.append('actual-prior391F0-plate-with-actual-near-far')
    add('actual-compositor-null-depth', depth=False)
    for name in ['lockout','warlock']:
        path = ROOT/('assets/disc/maps' if name=='lockout' else 'assets/map-pack/maps')/(name+'.map')
        c = Cache(path)
        try:
            entries=[]
            for datum,tag in c.tags.items():
                if tag['group']!='weat': continue
                _, at = c.tag(datum,'weat')
                for ordinal, offset in enumerate(c.block(at+8,936)):
                    bank = c.data[offset:offset+936]
                    floats = lambda off,count: struct.unpack_from('<'+'f'*count,bank,off)
                    start,end = floats(0x80,2); alpha = floats(0x88,3); tint = floats(0x98,3)
                    entries.append({'datum':f'{datum:08X}','ordinal':ordinal,'plate_sha256':hashlib.sha256(bank).hexdigest(),
                        'start_end': [start,end], 'authored_opacity':alpha,'authored_first_tint':tint})
                    for factor in [0.,.25,.5,1.]:
                        # Explicit caller37A60-shaped inputs; this selected
                        # factor is diagnostic, not historical intensity.
                        opacity=[max(0.,min(1.,f32(v*factor))) for v in alpha]
                        clamped_tint=[max(0.,min(1.,v)) for v in tint]
                        rec=words(struct.pack('<I12f',1,0.,opacity[0],0.,0.,opacity[1],0.,0.,opacity[2],0.,*clamped_tint))
                        add(f'authored-{name}-{ordinal}-explicit-factor-{factor}',start,end,compositor=rec)
            assets.append({'map':name,'path':str(path.relative_to(ROOT)),'cache_sha256':sha(path),'plates':entries})
        finally: c.close()
    for power in range(8):
        for a in [0., 1./65536, 255./65536, 256./65536, .5, 1., 1.5, 32766.]:
            for delta in [-1./65536, 0., 1./65536]:
                add(f'power{power}-start{a}-span-delta{delta}',a*1024,(a+2.**(-power)+delta)*1024,
                    record_flag=0x100 if power%2 else 0x101)
    for start,end in [(10.,10.0625),(10.,10.046875),(1.,1.03125),(-10.,10.),(10.,10.001),
            (10.,10.),(30.,10.),(0.,1.),(-1.,-.5),(10.,4096.)]:
        for bits in range(8):
            flags=tuple(1 if bits&(1<<i) else 0 for i in range(3))
            add(f'anchor-control-{start}-{end}-{bits}',start,end,flags=flags)
            add(f'anchor-low-byte-control-{start}-{end}-{bits}',start,end,
                flags=tuple(0x100|v for v in flags),depth=False,skip=0x101,cw=0x027f,mx=0x1fa0)
    for near in [-1.,-0.,0.,1./64.,6.0234375,10.,10000.]:
        add(f'optional-near-clamp-{near}',10.,10.046875,near=near,flags=(1,0,1))
    for flag in [0,1,0x100,0x101,0xffff0000,0xffffffff]:
        add(f'record-low-byte-{flag:08X}',record_flag=flag)
    colors=[-.5,.5,1.5,2.5,-1.5,-2.5,1.,0.,-0.,257.,-257.,8421504.,-8421504.,
        f32(.5/255),f32(1.5/255),f32(127.5/255),struct.unpack('<f',packed([1]))[0]]
    for i,value in enumerate(colors):
        rec=[0x100 if i%2 else 0x101]+[fw(colors[(i+j)%len(colors)]) for j in range(12)]
        add(f'raw-nearest-even-wrapped-color-{i}',compositor=rec)
    rng=random.Random(0x3a5c0)
    for i in range(2048):
        far=rng.choice([16.,32.,256.,1024.,8192.]); a=rng.uniform(-1.,16.)
        span=rng.choice([0.,-1.,1./131072,1./65536,1./32768,.001,.0078125,.125,1.,4.,4096.])
        values=[rng.choice(colors) if i%7==0 else rng.uniform(-257.,257.) for _ in range(12)]
        rec=[rng.getrandbits(32)]+[fw(v) for v in values]
        flags=tuple(rng.choice([0,1,0x100,0x101,0xffffffff]) for _ in range(3))
        add(f'bounded-finite-{i}',f32(a*far),f32((a+span)*far),far=far,
            near=rng.choice([0.,1./64.,10.,1000.]),flags=flags,compositor=rec,
            depth=i%3!=0,skip=rng.choice([1,0x101,0xffffffff]),cw=rng.choice([0x023f,0x027f]),
            mx=rng.choice([0x1f80,0x1fa0]))
    return result,labels,assets

class Original:
    def __init__(self,snapshot,image):
        self.snapshot,self.image=snapshot,image
        self.g=Uc(UC_ARCH_X86,UC_MODE_32);self.g.mem_map(0x10000,0x600000)
        for section in image.sections:self.g.mem_write(section.va,image.section_bytes(section))
        self.pages={at:snapshot.read(at,4096) for at in [*range(0x404000,0x40a000,4096),0x484000,0x485000]}
        self.g.mem_map(BANK,0x200000);self.g.mem_map(STOP,4096)
        self.verified={};self.entries=collections.Counter();self.classes=collections.Counter();self.power=None;self.written_pages=set()
        self.g.hook_add(UC_HOOK_CODE,self.code);self.g.hook_add(UC_HOOK_MEM_UNMAPPED,self.invalid)
        self.g.hook_add(UC_HOOK_MEM_WRITE,self.write)
    def code(self,g,pc,size,_):
        if pc==STOP:return
        if bytes(g.mem_read(pc,size))!=self.image.read(pc,size):
            raise RuntimeError(f'changed or non-XBE original instruction {pc:08X}')
        self.verified[pc]=size
        if pc in [0x3a5c0,0x131ed0,0x1ccf0,0x3fa3d0]:self.entries[pc]+=1
        if pc==0x3aae7:self.power=g.reg_read(UC_X86_REG_EBX)
    def write(self,g,access,address,size,value,_):
        for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):
            if not BANK<=at<BANK+0x200000:self.written_pages.add(at)
    def invalid(self,g,access,address,size,value,_):
        for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):
            raw=self.snapshot.read(at,4096);g.mem_map(at,4096);g.mem_write(at,raw);self.pages[at]=raw
        return True
    def run(self,row):
        g=self.g;self.power=None
        # Reset every captured SDK/command page for an independent call.
        for at,raw in self.pages.items():g.mem_write(at,raw)
        g.mem_write(0x484f68,packed(POISON));g.mem_write(BANK,packed(row[:13]));g.mem_write(DEPTH,packed([0xdeadbeef]))
        g.mem_write(0x485658,packed([row[15]]));g.mem_write(0x48565c,packed([row[16]]))
        args=[BANK,row[13],row[14],row[17],*row[18:22],DEPTH if row[22] else 0]
        g.mem_write(STACK,packed([STOP,*args]));g.reg_write(UC_X86_REG_ESP,STACK)
        for register in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:g.reg_write(register,0)
        g.reg_write(UC_X86_REG_EFLAGS,2);g.reg_write(UC_X86_REG_FPCW,row[23]);g.reg_write(UC_X86_REG_MXCSR,row[24])
        g.emu_start(0x3a5c0,STOP,count=100000)
        if g.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('complete source call did not return')
        success=int((g.reg_read(UC_X86_REG_EAX)&255)!=0)
        program=words(g.mem_read(0x484f68,240));depth=uint(g.mem_read(DEPTH,4))
        if not success and (program!=POISON or depth!=0xdeadbeef):raise RuntimeError('source false return mutated old banks')
        if not row[22] and depth!=0xdeadbeef:raise RuntimeError('source null pointer wrote optional output')
        if success:
            if self.power not in range(8):raise RuntimeError('unexpected mode3 original class')
            self.classes[self.power]+=1
        return [success,success*row[22],depth,*program]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--write-fixture',type=Path)
    parser.add_argument('--snapshot',type=Path,default=ROOT/'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry')
    args=parser.parse_args();snapshot=Snapshot(args.snapshot);image=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(image.data).hexdigest()!=PIN:raise RuntimeError('unsupported original executable')
    if hashlib.sha256(snapshot.ram).hexdigest()!=RAM_PIN:raise RuntimeError('unsupported immutable SDK/reference RAM')
    rows,labels,assets=cases(snapshot);oracle=Original(snapshot,image)
    expected=[oracle.run(row) for row in rows]
    actual_retained=words((ROOT/'analysis/multiplayer-shading/weather-plate-original-reference/event-215778/retained-pre391F0-pixel-definition.bin').read_bytes())
    if actual_retained!=words(snapshot.read(0x484f68,240)):raise RuntimeError('retained exported definition differs from actual RAM bank')
    if expected[0][3:]!=actual_retained or expected[0][2]!=fw(10):raise RuntimeError('actual single-baseline60words/depth changed')
    encoded=[' '.join(f'{v:08x}' for v in row) for row in rows]
    run=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True)
    if run.returncode:raise RuntimeError(f'native plate probe failed after {len(run.stdout.splitlines())} rows: {run.stderr.strip()}')
    native=[[int(v,16)for v in line.split()]for line in run.stdout.splitlines()]
    if len(native)!=len(expected) or any(len(v)!=63 for v in native):raise RuntimeError('native plate probe lost complete outputs')
    failures=[{'case':i,'label':labels[i],'differences':[{'output_word':j,'original':f'{x:08X}','native':f'{y:08X}'}for j,(x,y)in enumerate(zip(a,b))if x!=y]}
        for i,(a,b)in enumerate(zip(expected,native))if a!=b]
    successes=sum(v[0]for v in expected)
    if set(oracle.classes)!=set(range(8)):raise RuntimeError('missing original mode3 reciprocal class')
    if oracle.written_pages-set(oracle.pages):raise RuntimeError('original call wrote an unreset external page')
    if args.write_fixture and not failures:
        args.write_fixture.parent.mkdir(parents=True,exist_ok=True)
        args.write_fixture.write_text('# Complete unchanged retail3A5C0 mode3, source XBE '+PIN+'\n'+
            '# Input25words: record13,start/end/near/far,mode,center/end/start/skip,depth-request,FCW,MXCSR. Output63: success,depth-present,depthword,60words. False/null retain C0DE0000+i/DEADBEEF.\n'+
            '\n'.join(x+' | '+' '.join(f'{v:08x}'for v in y)for x,y in zip(encoded,expected))+'\n')
    report={'scope':'Pure mode3 program/depth production, complete unchanged original SDK installation executed on owned captured banks; no runtime/scene/history/GPU activation.',
        'source_xbe_sha256':PIN,'source_RAM_sha256':hashlib.sha256(snapshot.ram).hexdigest(),
        'native_binary_sha256':sha(args.binary),'cases':len(rows),'exact_cases':len(rows)-len(failures),
        'compared_words':len(rows)*63,'source_successes':successes,'source_false_preserved_old_banks':len(rows)-successes,
        'optional_outputs':sum(v[1]for v in expected),'null_depth_pointer_cases':sum(not row[22]for row in rows),
        'actual_baseline_near_far_words':[f'{v:08X}'for v in rows[0][15:17]],'actual_baseline_all60words_equal':True,
        'actual_baseline_depth_word':f'{expected[0][2]:08X}','verified_original_instructions':len(oracle.verified),
        'source_entries':{f'{k:08X}':v for k,v in sorted(oracle.entries.items())},
        'source_reciprocal_classes':dict(sorted(oracle.classes.items())),
        'distinct_successful60word_programs':len({tuple(v[3:])for v in expected if v[0]}),
        'restored_SDK_program_global_command_pages':{f'{k:08X}':hashlib.sha256(v).hexdigest()for k,v in sorted(oracle.pages.items())},
        'written_external_pages_all_restored_each_case':[f'{v:08X}'for v in sorted(oracle.written_pages)],
        'source_instruction_patches':[],'services_replaced':[],'authored_assets':assets,'case_labels':labels,
        'source_inputs':{str(p.relative_to(ROOT)):sha(p)for p in [ROOT/'assets/disc/default.xbe',args.snapshot/'export.json',
            args.snapshot/'exports/physical-ram.bin',ROOT/'analysis/multiplayer-shading/weather-plate-original-reference/pixel-program-original-proof.json',
            ROOT/'analysis/multiplayer-shading/weather-plate-original-reference/event-215778/source-mode3-input52.bin',
            ROOT/'analysis/multiplayer-shading/weather-plate-original-reference/event-215778/retained-pre391F0-pixel-definition.bin']},
        'failures':failures[:12],'native_inputs':{str(p.relative_to(ROOT)):sha(p)for p in [ROOT/'include/halo2/source_weather_plate_program.hpp',ROOT/'src/rendering/weather/source_weather_plate_program.cpp',ROOT/'tests/source_weather_plate_program_tests.cpp',Path(__file__).resolve()]}}
    if args.write_fixture and not failures:report['fixture']={'path':str(args.write_fixture),'sha256':sha(args.write_fixture)}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in ['case_labels','authored_assets','restored_SDK_program_global_command_pages','native_inputs','source_inputs']},indent=2))
    return bool(failures)
if __name__=='__main__':raise SystemExit(main())
