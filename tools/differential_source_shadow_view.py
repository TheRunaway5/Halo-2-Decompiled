#!/usr/bin/env python3
"""Untouched320B0/1652E0 and C1D90 single-camera execution over explicit banks.

No source patches, callee-return hooks, geometry/camera replacements or guest IO.
The retained32875 bank is a separate live phase from original20 framebuffer.
"""
from pathlib import Path
import argparse,collections,hashlib,json,math,random,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x23fff00,0x3000000
CONTEXT,BOX,LIGHT,PROJECTION,OUTPUT,COUNT,CAMERA,VIEW=[BASE+i*0x1000 for i in range(8)]
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def real(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);ap.add_argument('--count',type=int,default=1024);a=ap.parse_args()
    x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    u.mem_map(BASE,0x400000);u.mem_map(STOP,4096)
    executed={};calls=collections.Counter();observation={};kind=None
    entries=[0x376f00,0x320b0,0x11fa40,0x11f9a0,0x1652e0,0xc1d90,0x141590,0x163db0,0x441b0,0x30bf0,0x1429d0,0x11f770]
    def observe(g,pc,size,_):
        if pc==STOP:g.emu_stop();return
        if pc not in executed:assert bytes(g.mem_read(pc,size))==x.read(pc,size);executed[pc]=size
        if pc in entries:calls[f'{pc:08X}']+=1
        if kind==0 and pc==0x1652e0:
            argv=words(g.mem_read(g.reg_read(UC_X86_REG_ESP)+4,36));assert argv[3]==24 and argv[7:]==[1,0]
            observation['world_endpoints']=words(g.mem_read(argv[2],288))
        if kind==0 and pc==0x322f1:observation['bounds']=words(g.mem_read(g.reg_read(UC_X86_REG_ESP)+4,24))
    u.hook_add(UC_HOOK_CODE,observe)
    def run(entry,args=()):
        u.mem_write(STACK,pack([STOP,*args]));u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.emu_start(entry,STOP+1,count=200000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP
    run(0x376f00)
    initial=words(u.mem_read(words(u.mem_read(0x4687e0,4))[0],24))
    def original(row):
        nonlocal kind
        kind=row[0];observation.clear();u.mem_write(0x47ff86,b'\1')
        for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
        for i in range(8):u.reg_write(UC_X86_REG_XMM0+i,0)
        if kind==0:
            assert len(row)==128
            u.mem_write(CONTEXT,bytes(0x200));u.mem_write(CONTEXT+4,pack(row[1:112]));u.mem_write(0x547f88,pack([CONTEXT]));u.mem_write(BOX,pack(row[112:]))
            u.reg_write(UC_X86_REG_ESI,BOX);run(0x320b0)
            return [u.reg_read(UC_X86_REG_EAX)&255,*observation['bounds'],*observation['world_endpoints']]
        assert kind==1 and len(row)==38 and row[7] in [1,2]
        u.mem_write(LIGHT,bytes(0x80));u.mem_write(LIGHT+0x28,pack(row[1:4]));u.mem_write(LIGHT+0x34,pack(row[4:7]));u.mem_write(PROJECTION,pack(row[7:]));u.mem_write(OUTPUT,b'\xdd'*(6*0x1bc));u.mem_write(COUNT,b'\xdd\xdd')
        run(0xc1d90,[LIGHT,PROJECTION,OUTPUT,COUNT])
        return [struct.unpack('<H',u.mem_read(COUNT,2))[0],int(u.mem_read(0x47ff86,1)[0]==0),*words(u.mem_read(OUTPUT,0x1bc))]
    rows=[];labels=[];retained={}
    def add(row,label):rows.append(row);labels.append(label)
    directory=ROOT/'analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl'
    manifest=json.loads((directory/'capture.json').read_text());phase=next(p for p in manifest['phases']if p['name']=='32875-secondary-admission')
    files={f['file']:f for f in phase['files']}
    def raw(suffix):
        name=phase['name']+'-'+suffix+'.bin';v=(directory/name).read_bytes();f=files[name]
        assert len(v)==f['size'] and sha(v)==f['sha256'];retained[name]={'size':len(v),'sha256':sha(v),'address':f['address']};return v
    stack=raw('stack');stack_base=int(files[phase['name']+'-stack.bin']['address'],16);sp=phase['fields']['ESP']-stack_base
    light=raw('shadow-camera');maincamera=raw('main-visibility')[4:4+0x1bc]
    box=stack[sp+0x2c:sp+0x6c];projection=stack[sp+0x6c:sp+0xe8];captured_camera=stack[sp+0xe8:sp+0xe8+0x1bc]
    assert len(box)==64 and len(projection)==124 and len(captured_camera)==444
    live_camera_row=[1,*words(light[0x28:0x34]),*words(light[0x34:0x40]),*words(projection)]
    add(live_camera_row,'literal32875 C1D90 inputs and completed camera')
    original_live=original(live_camera_row);captured_words=words(captured_camera)
    assert original_live[0]==struct.unpack_from('<H',stack,sp+0x1c)[0]==1
    live_diff=[{'word':i,'original':f'{v:08X}','captured':f'{captured_words[i]:08X}'}for i,v in enumerate(original_live[2:])if v!=captured_words[i]]
    assert not live_diff,live_diff[:8]
    add([0,*words(maincamera),*words(box)],'literal32875 main camera and completed32350 receiver box')
    for mode in [1,2]:
        for near in [0,-.5,.000001,.1]:
            for width,height in [(0,0),(0,1),(1,0),(1,1),(-1,1),(1,-1)]:
                row=live_camera_row.copy();row[7]=mode;row[17]=bits(near);row[29:31]=map(bits,[width,height])
                add(row,'source finite zero/negative near/dimension; IEEE/empty-frustum branches')
        row=live_camera_row.copy();row[7]=mode;row[22:25]=[0,0,0]
        add(row,'controlled finite zero projection ray; original IEEE/empty rectangle')
    rng=random.Random(0x320b0c1d90)
    def constructor(position,forward,up,near,far,rectangle):
        nonlocal kind
        kind=None;u.mem_write(CAMERA,bytes(0x80));u.mem_write(CAMERA+8,pack(list(map(bits,[*position,*forward,*up]))));u.mem_write(CAMERA+0x48,pack([bits(near),bits(far)]));u.mem_write(VIEW,bytes(0x80));u.mem_write(VIEW+0x68,pack(list(map(bits,rectangle))))
        u.reg_write(UC_X86_REG_EBX,CAMERA+8);u.reg_write(UC_X86_REG_ESI,OUTPUT);run(0x441b0,[VIEW]);return words(u.mem_read(OUTPUT,0x1bc))
    def transform(position,angle=0,scale=1):
        c,s=math.cos(angle),math.sin(angle);return list(map(bits,[scale,c,s,0,-s,c,0,0,0,1,*position]))
    for near in [0,.015625,.1,1]:
        camera=constructor((0,0,0),(1,0,0),(0,0,1),near,1024,(-1,1,-1,1))
        threshold=bits(real(bits(max(near,real(0x37800006))))+real(0xb8d1b717))
        for delta in range(-4,5):
            depth=real(threshold+delta)
            add([0,*camera,*transform((depth,0,0)),0,0,0],'adjacent source near front-epsilon point/degenerate box')
    camera=constructor((0,0,0),(1,0,0),(0,0,1),.1,1024,(-1,1,-1,1))
    for edge in [0xbf800000,0x3f800000]:
        for delta in range(-3,4):
            for yz in [(real(edge+delta),0),(0,real(edge+delta))]:
                add([0,*camera,*transform((1,*yz)),0,0,0],'adjacent literal screen rectangle overlap boundary')
    for near_enabled in [0,1]:
        for far_flags in [0,1,0x100]:
            c=camera.copy();c[0x6c//4]=near_enabled;c[0x84//4]=far_flags
            for center in [(-1,0,0),(1,0,0),(4096,0,0)]:
                add([0,*c,*transform(center),*map(bits,[.25,.5,.75])],'320B0 near1/far0 arguments override camera enabled bytes')
    for direction,up in [((1,0,0),(0,0,1)),((0,1,0),(0,0,1)),((0,0,-1),(0,1,0)),((.6,.8,0),(0,0,1))]:
        for near in [0,.015625,.1,1]:
            camera=constructor((0,0,0),direction,up,near,1024,(-1,1,-1,1))
            for depth in [-10,-1,0,.000001,.0001,.015625,.1,1,16,4096]:
                for transverse in [0,2,100]:
                    position=[v*depth for v in direction];position[2]+=transverse
                    add([0,*camera,*transform(position),*map(bits,[.25,.5,.75])],'source441B0 camera; front/near-straddle/offscreen/far-not-clipped')
    for i in range(a.count):
        pos=[rng.uniform(-1000,1000)for _ in range(3)];angle=rng.uniform(-math.pi,math.pi);forward=(math.cos(angle),math.sin(angle),0);up=(0,0,1)
        camera=constructor(pos,forward,up,rng.choice([0,.015625,.1,1]),rng.choice([16,1024,32768]),(-rng.uniform(.1,2),rng.uniform(.1,2),-rng.uniform(.1,2),rng.uniform(.1,2)))
        center=[pos[j]+rng.uniform(-50,50)for j in range(3)];scale=rng.choice([0,.5,1,2,-1]);extent=[rng.choice([0,.01,.5,1,10])for _ in range(3)]
        add([0,*camera,*transform(center,rng.uniform(-math.pi,math.pi),scale),*map(bits,extent)],'source441B0 camera; explicit box rotations/scales/extents')
    # Mode1 is a controlled branch, not the fixed-mode2 live shadow caller.
    for i in range(a.count):
        row=live_camera_row.copy();row[7]=1+(i%2);angle=rng.uniform(-math.pi,math.pi);d=(math.cos(angle),math.sin(angle),0);up=(0,0,1);left=(-d[1],d[0],0)
        row[1:7]=map(bits,[*d,*up]);row[14:17]=map(bits,left);row[17]=bits(rng.choice([.001,.015625,.1,1,10]));row[18:21]=map(bits,[rng.uniform(-2048,2048)for _ in range(3)])
        ray=[d[j]*rng.choice([.5,1,2])+left[j]*rng.uniform(-.25,.25)for j in range(3)];row[22:25]=map(bits,ray)
        row[29]=bits(rng.choice([.01,.5,2,10]));row[30]=bits(rng.choice([.01,.5,2,10]));row[32]=bits(rng.choice([.5,10,100,1024]))
        add(row,'single-camera mode1/2 literal completed projection/light fields')
    expected=[original(row)for row in rows]
    failures=[]
    if a.native:
        result=subprocess.run([str(a.native.resolve()),'--probe'],input=''.join(' '.join(f'{v:x}'for v in row)+'\n'for row in rows),text=True,capture_output=True)
        if result.returncode:raise RuntimeError(result.stderr)
        actual=[[int(v,16)for v in line.split()]for line in result.stdout.splitlines()];assert len(actual)==len(expected)
        for i,(v,e)in enumerate(zip(actual,expected)):
            if v!=e:failures.append({'case':i,'label':labels[i],'differences':[{'word':j,'native':f'{x:08X}','source':f'{y:08X}'}for j,(x,y)in enumerate(zip(v,e))if x!=y]})
    fixture=ROOT/'tests/fixtures/retail_source_shadow_view.tsv';fixture.write_text('# Untouched320B0/1652E0 and nonzero-mode C1D90 over explicit banks; retained32875 separate from original20.\n'+''.join(' '.join(f'{v:x}'for v in row)+' | '+' '.join(f'{v:x}'for v in out)+'\n'for row,out in zip(rows,expected)))
    functions=[(0x320b0,0x29a),(0x1652e0,0x73b),(0xc1d90,0xb18),(0x11fa40,0x239),(0x11f9a0,0x91),(0x141590,0x12b),(0x163db0,0xb11)]
    report={'executable_sha256':PIN,'source_unchanged':True,'entries':['000320B0','001652E0','000C1D90'],'cases':len(rows),'checked_words':sum(map(len,expected)),
        'volume_cases':sum(row[0]==0 for row in rows),'volume_admitted':sum(out[0]!=0 for row,out in zip(rows,expected)if row[0]==0),'volume_rejected':sum(out[0]==0 for row,out in zip(rows,expected)if row[0]==0),
        'secondary_cases':sum(row[0]==1 for row in rows),'calls':dict(calls),'executed_instructions':len(executed),'source_initial_bounds':[f'{v:08X}'for v in initial],
        'native_comparison':bool(a.native),'native_binary_sha256':sha(a.native.read_bytes())if a.native else None,'native_failures':failures[:20],'native_failed_cases':len(failures),
        'floating_control':{'oracle_FPCW':'027F','oracle_MXCSR':'00001F80','captured32875_FPCW':'023F','captured32875_MXCSR':'00001FB0','precision':'both PC53, round-to-nearest; differences are reserved/exception status bits, not arithmetic rounding'},
        'source_function_bytes':{f'{pc:08X}':{'size':size,'sha256':sha(x.read(pc,size))}for pc,size in functions},'fixture_sha256':sha(fixture.read_bytes()),
        'retained_control':{'phase':phase['name'],'tick':phase['clock_tick'],'view':phase['view_index'],'manifest_sha256':sha((directory/'capture.json').read_bytes()),'files':retained,
            'stack_ESP':f'{phase["fields"]["ESP"]:08X}','box_offset_from_ESP':'2C','projection_offset_from_ESP':'6C','secondary_camera_offset_from_ESP':'E8','completed_camera_words_exact':111,'original_count':original_live[0],'original_diagnostic':original_live[1]},
        'boundaries':['No guest IO or advancement; retained32875 OpenGL live phase is distinct from original20 framebuffer.','Volume case retained inputs follow a successful320B0 branch; only C1D90 completed camera words are directly independently observed.','Mode0 six-camera point-light branch is rejected, not replaced. Controlled mode1 cases do not prove gameplay mode1 admission.','No source patches, output hooks or callee return replacements; observers read original endpoints/bounds and verify every executed instruction against pinned XBE.','Finite explicit producer inputs only; zero near/ray and negative/empty dimensions preserve original IEEE/empty-bank behavior. Missing/unresolved fields are native rejection. No owner/registration/secondary visibility or gameplay shadow activation claim.']}
    path=ROOT/'analysis/multiplayer-shading/shadow-view-original.json';path.write_text(json.dumps(report,indent=2)+'\n');fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in ['calls','source_function_bytes','retained_control','boundaries','native_failures']},indent=2));return int(bool(failures))
if __name__=='__main__':raise SystemExit(main())
