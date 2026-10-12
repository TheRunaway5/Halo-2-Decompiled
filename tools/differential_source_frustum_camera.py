#!/usr/bin/env python3
"""Compare complete native frustum banks with untouched retail camera/frustum instructions."""
import argparse,hashlib,json,math,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x21fff00,0x3000000
CAMERA,RECTANGLE,BANK,OUTPUT,PROJECTION,LAYOUT,AUXILIARY,SCENARIO=[BASE+i*0x1000 for i in range(8)]
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def real(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def packed(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer-shading/source_frustum_camera_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/source-frustum-camera-differential.json')
    p.add_argument('--write-fixture',type=Path)
    p.add_argument('--count',type=int,default=4096)
    p.add_argument('--projection-count',type=int,default=4096)
    a=p.parse_args();xbe=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(xbe.data).hexdigest()!=PIN:raise RuntimeError('unsupported original executable')
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in xbe.sections:u.mem_write(s.va,xbe.section_bytes(s))
    u.mem_map(BASE,0x100000);u.mem_map(STOP,0x1000)
    entries={entry:0 for entry in [0x376f00,0x441b0,0x30bf0,0x141590,0x163db0,0x1429d0,0x11f770,0x2f970,0x2fd90,0x2fb70]}
    def trace(_u,address,_size,_data):
        if address in entries:entries[address]+=1
    u.hook_add(UC_HOOK_CODE,trace)
    def run(at,args=()):
        u.mem_write(STACK,packed([STOP,*args]));u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.emu_start(at,STOP,count=100000)
        if u.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('original frustum routine did not return')
    run(0x376f00)
    def original(row):
        u.mem_write(0x47ff86,b'\1')
        if row[0] in [2,3]:
            u.mem_write(CAMERA,bytes(0x74));u.mem_write(CAMERA+0x28,packed(row[1:2]))
            u.mem_write(CAMERA+0x30,struct.pack('<8H',*[v&65535 for v in row[2:10]]))
            u.mem_write(CAMERA+0x58,bytes([bool(row[10])]))
            u.mem_write(CAMERA+0x5c,packed(row[11:14]))
            u.mem_write(CAMERA+0x68,bytes([bool(row[14])]))
            u.mem_write(CAMERA+0x6c,packed(row[15:17]))
            u.mem_write(0x485ac2,bytes([bool(row[17])]))
            u.mem_write(0x4e6948,packed([LAYOUT if row[18] else 0]))
            u.mem_write(LAYOUT,bytes(16));u.mem_write(LAYOUT+1,bytes([row[19]&255]))
            u.mem_write(LAYOUT+2,struct.pack('<H',row[20]&65535));u.mem_write(LAYOUT+8,packed(row[21:22]))
            u.mem_write(0x510c50,packed([AUXILIARY if row[22] else 0]));u.mem_write(AUXILIARY+5,bytes([row[23]&255]))
            u.mem_write(0x4e0350,packed([SCENARIO if row[24] else 0]));u.mem_write(SCENARIO+0x10,struct.pack('<H',row[25]&65535))
            u.reg_write(UC_X86_REG_EAX,CAMERA);u.reg_write(UC_X86_REG_ESI,RECTANGLE);run(0x2f970)
            normalized=words(u.mem_read(RECTANGLE,16));modifier=u.mem_read(0x4ba019,1)[0]
            u.reg_write(UC_X86_REG_ECX,CAMERA);u.reg_write(UC_X86_REG_EDX,RECTANGLE if row[26] else 0)
            u.reg_write(UC_X86_REG_EAX,OUTPUT);run(0x2fb70)
            program=words(u.mem_read(OUTPUT,64));expected=[*normalized,*program,modifier]
            if row[0]==2:return expected
            u.mem_write(CAMERA,packed(row[27:36]));u.mem_write(CAMERA+0x40,packed(row[36:38]))
            u.reg_write(UC_X86_REG_ESI,CAMERA);u.reg_write(UC_X86_REG_EDX,RECTANGLE if row[26] else 0)
            u.reg_write(UC_X86_REG_EBX,PROJECTION);run(0x2fd90)
            if words(u.mem_read(PROJECTION+0x68,16))!=program[12:16]:raise RuntimeError('source2FD90 rectangle forwarding changed')
            u.reg_write(UC_X86_REG_EBX,CAMERA);u.reg_write(UC_X86_REG_ESI,BANK)
            run(0x441b0,[PROJECTION])
            return [*expected,*words(u.mem_read(BANK,0x1bc)),int(u.mem_read(0x47ff86,1)[0]==0)]
        if row[0]==0:
            u.mem_write(CAMERA,b'\0'*0x80)
            # EBX points at camera.position; the separate stack argument is
            # the 2FD90 view/projection bank, whose clip rectangle is at+68.
            u.mem_write(CAMERA+8,packed(row[1:10]))
            u.mem_write(CAMERA+0x48,packed(row[10:12]))
            u.mem_write(CAMERA+0x68,packed(row[12:16]))
            u.reg_write(UC_X86_REG_EBX,CAMERA+8);u.reg_write(UC_X86_REG_ESI,BANK)
            u.mem_write(BANK,b'\xff'*0x1bc);run(0x441b0,[CAMERA])
            return words(u.mem_read(BANK,0x1bc))+[int(u.mem_read(0x47ff86,1)[0]==0)]
        u.mem_write(BANK,packed(row[1:112]));u.mem_write(RECTANGLE,packed(row[112:116]))
        u.mem_write(OUTPUT,packed(row[117:]));u.reg_write(UC_X86_REG_ECX,RECTANGLE)
        u.reg_write(UC_X86_REG_EDI,OUTPUT);run(0x163db0,[BANK,row[116]])
        return [u.reg_read(UC_X86_REG_EAX)&255,int(u.mem_read(0x47ff86,1)[0]==0),*words(u.mem_read(OUTPUT,0x108))]

    rng=random.Random(0x441b0163db0);rows=[];labels=[]
    def ctor(pos,forward,up,near=.015625,far=1024,rectangle=(-1,1,-1,1),label='constructor'):
        rows.append([0,*map(bits,[*pos,*forward,*up,near,far,*rectangle])]);labels.append(label)
    # Literal captured camera records. Execute the actual source projection
    # helpers to obtain 441B0's separate rectangle; it is not a camera field.
    captured=[]
    for name in ['zanzibar-live-view.bin','ivory-live-view.bin']:
        path=ROOT/'analysis/multiplayer-shading/original-xemu'/name
        if not path.exists():continue
        raw=path.read_bytes()
        # Retained view snapshots place camera.position at+0xC; corroborated
        # independently in prove-zanzibar20-camera.py against actual GPU c0..11.
        c=raw[0xc:0x80]
        if len(c)!=0x74:raise RuntimeError('incomplete source camera snapshot')
        u.mem_write(CAMERA,c)
        for address,size in [(0x4e6948,4),(0x510c50,4),(0x485ac2,1),(0x4e0350,4),(0x404fe8,1)]:u.mem_write(address,bytes(size))
        u.reg_write(UC_X86_REG_EAX,CAMERA);u.reg_write(UC_X86_REG_ESI,RECTANGLE);run(0x2f970)
        u.reg_write(UC_X86_REG_ESI,CAMERA);u.reg_write(UC_X86_REG_EDX,RECTANGLE);u.reg_write(UC_X86_REG_EBX,PROJECTION);run(0x2fd90)
        rectangle=words(u.mem_read(PROJECTION+0x68,16))
        row=[0,*words(c[:0x24]),*words(c[0x40:0x48]),*rectangle]
        rows.append(row);labels.append(name);captured.append({'file':str(path.relative_to(ROOT)),'sha256':hashlib.sha256(raw).hexdigest(),
            'camera_position_offset':'0xC','rectangle_producer':'untouched2F970/2FD90/2FB70; standard view globals explicit zero',
            'input_words':[f'{v:08X}' for v in row[1:]]})
    for forward,up in [((1,0,0),(0,0,1)),((0,1,0),(0,0,1)),((0,0,1),(0,1,0)),((.6,.8,0),(0,0,1)),((1,-0.,0),(0,-0.,1))]:
        for pos in [(0,0,0),(10,20,30),(10000,-10000,10000)]:
            for rectangle in [(-1,1,-1,1),(-.532049,.532049,-.399037,.399037),(0,1,0,1),(0,0,-1,1),(-1,1,1,0)]:
                ctor(pos,forward,up,rectangle=rectangle,label='axes,empty,signedzero,cancellation')
    # Adjacent threshold lengths and signed-zero basis components.
    for threshold in [0,0x38d1b717,0x3f800000]:
        for delta in [-2,-1,0,1,2]:
            value=real(max(0,threshold+delta))
            ctor((0,0,0),(value,0,0),(0,value,0),label='normalization adjacent threshold')
    for i in range(a.count):
        pos=[rng.uniform(-32768,32768) for _ in range(3)]
        forward=[rng.uniform(-1.5,1.5) for _ in range(3)];up=[rng.uniform(-1.5,1.5) for _ in range(3)]
        lo=[rng.uniform(-4,0) for _ in range(2)];hi=[rng.uniform(0,4) for _ in range(2)]
        ctor(pos,forward,up,near=rng.choice([0,.015625,.1]),far=rng.choice([1,16,256,1024,32768]),rectangle=(lo[0],hi[0],lo[1],hi[1]),label='random explicit camera')
    expected=[original(row) for row in rows]
    constructor_cases=len(rows)
    # Exercise standalone clipped rectangles with retained arbitrary bytes,
    # non-unit transform scale, original near-plane-disabled branch and empties.
    for i in range(a.count):
        bank=expected[i%constructor_cases][:111].copy()
        bank[0x38//4]=bits(rng.choice([0,-2,-.5,.5,1,2,4]))
        bank[0x6c//4]=rng.randrange(2)
        rectangle=[rng.uniform(-2,0),rng.uniform(0,2),rng.uniform(-2,0),rng.uniform(0,2)]
        if i%17==0:rectangle[1]=rectangle[0]
        if i%19==0:rectangle[3]=rectangle[2]
        previous=[rng.getrandbits(32) for _ in range(66)]
        row=[1,*bank,*map(bits,rectangle),rng.getrandbits(32),*previous]
        rows.append(row);labels.append('retained clipped frustum');expected.append(original(row))
    clipped_cases=a.count
    # Full2F970→2FB70 and actual2FD90→441B0 callers use identical explicit
    # feature/context state. Branch combinations include both active camera
    # features (58 takes precedence), auxiliary mode, suppression and nullEDX.
    projection_start=len(rows)
    def projection_case(mode,angle,outer,safe,features=None,label='projection explicit branches',camera=None):
        # feature fields: override,centerX,centerY,scale,offsetEnabled,offsetXY,
        # wide,layoutPresent,layoutFlag,layoutIndex,layoutMode,auxPresent,auxFlag,
        # scenarioPresent,scenarioType,useBounds.
        features=features or [0,bits(0),bits(0),bits(1),0,bits(0),bits(0),0,0,0,65535,0,0,0,1,1,1]
        row=[mode,bits(angle),*[v&65535 for v in [*outer,*safe]],*features]
        if mode==3:row.extend(map(bits,camera or [10,20,30,.6,.8,0,0,0,1,.015625,1024]))
        rows.append(row);labels.append(label);expected.append(original(row))
    for name in ['zanzibar-live-view.bin','ivory-live-view.bin']:
        path=ROOT/'analysis/multiplayer-shading/original-xemu'/name
        if not path.exists():continue
        c=path.read_bytes()[0xc:0x80]
        camera=[*struct.unpack('<9f',c[:0x24]),*struct.unpack('<2f',c[0x40:0x48])]
        projection_case(3,struct.unpack_from('<f',c,0x28)[0],struct.unpack_from('<4h',c,0x30),
            struct.unpack_from('<4h',c,0x38),label='retained source camera complete pipeline',camera=camera)
    for i in range(256):
        outer=[0,0,480,640] if i%4==0 else [0,0,240,320]
        safe=[outer[0]+18,outer[1]+24,outer[2]-18,outer[3]-24]
        features=[i&1,bits(-.2),bits(.3),bits(.8),(i>>1)&1,bits(7),bits(-9),
            (i>>2)&1,(i>>3)&1,255 if (i>>4)&1 else 0,3 if (i>>5)&1 else 65535,
            3 if (i>>6)&1 else 2,(i>>7)&1,255,1,i%3,(i>>2)&1]
        projection_case(3 if i&1 else 2,.7592033743858337,outer,safe,features)
    for i in range(a.projection_count):
        top,left=rng.randrange(-1024,1024),rng.randrange(-1024,1024)
        height,width=rng.randrange(32,4096),rng.randrange(32,4096)
        outer=[top,left,top+height,left+width]
        border_y,border_x=rng.randrange(height//4),rng.randrange(width//4)
        safe=[top+border_y,left+border_x,top+height-border_y,left+width-border_x]
        features=[rng.randrange(2),bits(rng.uniform(-.8,.8)),bits(rng.uniform(-.8,.8)),bits(rng.uniform(.1,2)),
            rng.randrange(2),bits(rng.uniform(-320,320)),bits(rng.uniform(-240,240)),
            rng.randrange(2),rng.randrange(2),rng.choice([0,1,255]),rng.choice([65535,0,1]),rng.choice([0,2,3]),
            rng.randrange(2),rng.choice([0,1,255]),rng.randrange(2),rng.randrange(3),rng.randrange(2)]
        projection_case(3 if i%2 else 2,rng.uniform(.0001,3.13),outer,safe,features)
    # Adjacent literal angles independently exercise the x87 tangent spill.
    for angle_bits in [0x38d1b717,0x3f425b27,0x3f9c61aa,0x4047ae14]:
        for delta in range(-4,5):projection_case(3,real(angle_bits+delta),[0,0,480,640],[36,48,444,592],label='projection adjacent angle')
    encoded=[f'{len(row):x} '+' '.join(f'{v:08x}' for v in row) for row in rows]
    native=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',capture_output=True,text=True,check=True)
    actual=[[int(v,16) for v in line.split()] for line in native.stdout.splitlines()]
    if len(actual)!=len(rows):raise RuntimeError('native frustum probe lost cases')
    failures=[]
    for i,(row,source,host) in enumerate(zip(rows,expected,actual)):
        if source!=host:
            differences=[{'word':j,'source':f'{s:08x}','native':f'{n:08x}'} for j,(s,n) in enumerate(zip(source,host)) if s!=n]
            failures.append({'case':i,'label':labels[i],'input':encoded[i],'differences':differences[:20]})
    if a.write_fixture and not failures:
        chosen=sorted(set([*range(min(100,constructor_cases)),
            *range(projection_start,min(projection_start+258,len(rows))),
            *range(max(projection_start,len(rows)-36),len(rows)),
            *range(0,len(rows),max(1,len(rows)//256))]))
        lines=['# Complete untouched retail441B0 and163DB0 camera/frustum banks. XBE '+PIN,
            '# Hexadecimal inputcount,inputwords,outputwords. Mode0=constructor;mode1=clipped retained frustum.']
        lines.extend(encoded[i]+' '+' '.join(f'{v:08x}' for v in expected[i]) for i in chosen)
        a.write_fixture.write_text('\n'.join(lines)+'\n')
    report={'original_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(a.binary.read_bytes()).hexdigest(),
        'source_input_snapshots':captured,'x87_control_word':'027F','mxcsr':'1F80','untouched_entries':{f'{k:08X}':v for k,v in entries.items()},
        'constructor_cases':constructor_cases,'clipped_rectangle_cases':clipped_cases,
        'projection_pipeline_cases':len(rows)-projection_start,'compared_words':sum(map(len,expected)),
        'bit_exact_cases':len(rows)-len(failures),'failures':failures[:12],
        'scope':'Complete0x1BC/0x108 byte banks plus163DB0 return/diagnostic. Explicit source camera fields; no native viewport inference, portal recursion or whole framebuffer parity.'}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
