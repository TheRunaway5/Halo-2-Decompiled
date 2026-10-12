#!/usr/bin/env python3
"""Execute retail fog aggregation and leaf lookup without function stubs."""
import sys,json,struct,hashlib,argparse,subprocess,math
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
STATE,EYE,DIRECTION,STACK,STOP=0x2100000,0x2101000,0x2101100,0x21fff00,0x3000000
def invoke(g,address,args=(),registers=()):
    g.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));g.reg_write(UC_X86_REG_ESP,STACK)
    g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
    for reg,value in registers:g.reg_write(reg,value)
    g.emu_start(address,STOP,count=1000000)
    assert g.reg_read(UC_X86_REG_EIP)==STOP,hex(g.reg_read(UC_X86_REG_EIP))
def setup(c):
    image=(ROOT/'assets/disc/default.xbe').read_bytes()
    assert hashlib.sha256(image).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    base=struct.unpack_from('<I',image,0x104)[0];count,table=struct.unpack_from('<II',image,0x11c)
    for i in range(count):
        _,address,_,offset,size=struct.unpack_from('<5I',image,table-base+i*56)
        g.mem_write(address,image[offset:offset+size])
    g.mem_map(STATE,0x100000);g.mem_map(0x2300000,0x100000);g.mem_map(STOP,4096)
    invoke(g,0x377110)
    mapped=c.magic&~4095;size=(c.magic+len(c.data)-mapped+4095)&~4095
    g.mem_map(mapped,size);g.mem_write(c.magic,bytes(c.data))
    _,scnr=c.tag(c.scenario,'scnr');bsp=c.block(scnr+0x210,68)[0]
    file,size,address=struct.unpack_from('<3I',c.data,bsp)
    g.mem_write(address,bytes(c.data[file:file+size]));g.mem_write(0x4e0350,struct.pack('<I',c.magic+scnr))
    g.mem_write(0x4e0348,struct.pack('<I',address+16));g.mem_write(0x4e3b44,struct.pack('<I',0x2300000))
    for t in c.tags.values():g.mem_write(0x2300000+(t['datum']&65535)*16+8,struct.pack('<I',t['address']))
    matg=next(t for t in c.tags.values() if t['group']=='matg')
    r=c.block(matg['address']-c.magic+0x108,0x1c8)[0]
    g.mem_write(0x407488,struct.pack('<I',0x2350000));g.mem_write(0x2350000,struct.pack('<II',0x23c0000,0x23f0000))
    g.mem_write(0x485a80,struct.pack('<I',r+c.magic));g.mem_write(0x48565c,struct.pack('<f',1024));g.mem_write(0x485ae0,struct.pack('<f',16777215))
    # Suppress only GPU submission. None of the fog aggregation, cluster,
    # packing or texture-coordinate routines is replaced.
    stubs={0x14bc0:8,0x14f60:0,0x143c0:4,0x222d0:0,0x1cc30:0,0x1c710:4,0x3f5cf0:0,0x3f53d0:4,0x3f5590:4,0x3f54f0:4,0x3f59b0:4,0x16b10:0}
    capture=[];bloom_capture=[]
    def hook(guest,address,size,user):
        if address in stubs:
            sp=guest.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',guest.mem_read(sp,4))[0]
            guest.reg_write(UC_X86_REG_ESP,sp+4+stubs[address]);guest.reg_write(UC_X86_REG_EIP,ret)
        if address==0x1ccf0:
            capture.append(bytes(guest.mem_read(0x484f68,240)));guest.emu_stop()
        if address==0x214f0:
            # 13870/137A0 execute unchanged. Capture their blur submission.
            sp=guest.reg_read(UC_X86_REG_ESP)
            bloom_capture.append(struct.unpack('<3f',guest.mem_read(sp+4,12)))
            ret=struct.unpack('<I',guest.mem_read(sp,4))[0]
            guest.reg_write(UC_X86_REG_ESP,sp+32);guest.reg_write(UC_X86_REG_EIP,ret)
    g.hook_add(UC_HOOK_CODE,hook)
    # 22590's default cinematic timeline has amount/brightness = .5.
    timeline=bytearray(52)
    for offset in (12,16,20,24,36,40,44,48):struct.pack_into('<f',timeline,offset,.5)
    g.mem_write(0x2103000,bytes(timeline));g.mem_write(0x50934c,struct.pack('<I',0x2103000))
    g.mem_write(0x485890,bytes(4));g.mem_write(0x4858a0,bytes(8))
    return g,file,address,capture,bloom_capture
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native-binary',type=Path,required=True,help='bsp_atmosphere_tests executable')
parser.add_argument('--maps',type=Path,action='append',help='Original cache directories (repeat for DLC)')
parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/bsp-atmosphere-differential.json')
parser.add_argument('--sky-model-from-metadata',action='store_true',
    help='Run original14B360 and its authored override, rather than the historical model-control0 probe')
args=parser.parse_args()
roots=args.maps or [ROOT/'assets/disc/maps',ROOT/'assets/map-pack/maps']
paths=[p for root in roots for p in sorted(root.glob('*.map'))]
out=[]
for path in paths:
    c=Cache(path)
    if c.scenario==0xffffffff:c.close();continue
    _,scnr=c.tag(c.scenario,'scnr')
    if c.u16(scnr+0x10)!=1:c.close();continue
    g,file,address,capture,bloom_capture=setup(c);magic=address-file
    collision=c.block(file+16+0x14,64,magic)[0]
    clusters=c.block(file+16+156,176,magic);leaves=c.block(file+16+0x2c,8,magic)
    samples=[]
    for cluster,p in enumerate(clusters):
        bounds=struct.unpack_from('<6f',c.data,p+0x54)
        eye=[(bounds[i]+bounds[i+1])*.5 for i in (0,2,4)]
        for z_offset in (0,-40,40):
            eye[2]+=z_offset
            exact_eye=list(struct.unpack('<3f',struct.pack('<3f',*eye)))
            g.mem_write(EYE,struct.pack('<3f',*eye));g.mem_write(DIRECTION,struct.pack('<3f',1,0,0))
            invoke(g,0x14a280,registers=[(UC_X86_REG_ECX,address+collision-file),(UC_X86_REG_EDX,EYE),(UC_X86_REG_EAX,0)])
            leaf=g.reg_read(UC_X86_REG_EAX);actual_cluster=-1 if leaf==0xffffffff else struct.unpack_from('<h',c.data,leaves[leaf])[0]
            # Forced cluster exposes all metadata selectors even where the
            # centroid is not a point inside that cluster's nonconvex space.
            model=0;sky_control=bytes(20)
            if args.sky_model_from_metadata:
                control=0x2102800;g.mem_write(control,bytes(20))
                invoke(g,0x14b360,registers=[(UC_X86_REG_ECX,cluster),(UC_X86_REG_EDX,control),
                    (UC_X86_REG_EBX,control+4),(UC_X86_REG_EDI,control+8)])
                model=g.reg_read(UC_X86_REG_EAX)&255;sky_control=bytes(g.mem_read(control,20))
            invoke(g,0x12e5e0,(EYE,DIRECTION,model,1),[(UC_X86_REG_EAX,STATE),(UC_X86_REG_ECX,cluster)])
            if sky_control[4]:g.mem_write(STATE+4,sky_control[8:20])
            state=bytes(g.mem_read(STATE,288))
            g.mem_write(0x48574c,state);g.mem_write(0x4b9ef0,state)
            scalars=[];colors=[]
            for slot in range(24,37):
                invoke(g,0x336f0,registers=[(UC_X86_REG_EAX,slot)])
                scalars.append(struct.unpack('<f',struct.pack('<I',g.reg_read(UC_X86_REG_XMM0)&0xffffffff))[0])
            for slot in range(25,33):
                invoke(g,0x33980,registers=[(UC_X86_REG_EAX,slot),(UC_X86_REG_ESI,0x2102000)])
                colors.extend(struct.unpack('<3f',g.mem_read(0x2102000,12)))
            g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(UC_X86_REG_ESP,STACK)
            for reg,value in [(UC_X86_REG_ECX,EYE),(UC_X86_REG_EAX,DIRECTION),(UC_X86_REG_EDX,STATE)]:g.reg_write(reg,value)
            g.emu_start(0x25a10,0x25c6a,count=100000)
            assert g.reg_read(UC_X86_REG_EIP)==0x25c6a
            constants=list(struct.unpack('<24f',g.mem_read(0x4077f8,96)))
            g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(UC_X86_REG_ESP,STACK);capture.clear()
            g.emu_start(0x25e50,STOP,count=1000000)
            assert capture
            composite=list(struct.unpack('<60I',capture[-1]))
            coordinates=[]
            if composite[54]==0x2621:
                for stage in (2,3):
                    invoke(g,0x26710,(stage,0x2102000,0x2102100,0x2102200))
                    coordinates.extend(struct.unpack('<4f',g.mem_read(0x2102200,16)))
            else:coordinates=[0]*8
            # Disable only scenario screen-effect submissions that follow
            # bloom; the original bloom aggregation/query remains active.
            g.mem_write(0x4e0350,bytes(4));bloom_capture.clear()
            invoke(g,0x13870,(0,1))
            g.mem_write(0x4e0350,struct.pack('<I',c.magic+scnr))
            assert len(bloom_capture)==1,bloom_capture
            bloom_frame=[bloom_capture[0][1],bloom_capture[0][2],struct.unpack_from('<f',state,0xec)[0]]
            samples.append({'eye':exact_eye,'input_cluster':cluster,'actual_cluster':actual_cluster,
                'sky_model_control':model,'authored_clear_override':sky_control[4],
                'classification':struct.unpack_from('<I',state)[0],
                'primary':list(struct.unpack_from('<6f',state,0x1c)),
                'secondary':list(struct.unpack_from('<6f',state,0x34)),
                'planar':list(struct.unpack_from('<9f',state,0xa0)),
                'patchy':list(struct.unpack_from('<10f',state,0x70)),
                'patchy_datum':hex(struct.unpack_from('<I',state,0x6c)[0]),
                'planar_datum':hex(struct.unpack_from('<I',state,0x9c)[0]),
                'camera':list(struct.unpack_from('<3f',state,0x108)),
                'transition':struct.unpack_from('<f',state,0x118)[0],
                'sky_rgb':list(struct.unpack_from('<3f',state,0x5c)),'sky_alpha':struct.unpack_from('<f',state,0x68)[0],
                'plane_only':state[0x114],'bloom':list(struct.unpack_from('<4f',state,0xe0)),
                'raw_hex':state.hex(),'extern_scalars':scalars,'extern_colors':colors,'constants':constants,
                'composite':composite,'coordinates':coordinates,'bloom_frame':bloom_frame})
            words=(len(clusters)+31)//32
            pvs_address=c.u32(file+16+0x58)
            potential=list(struct.unpack('<'+'I'*words,g.mem_read(pvs_address+cluster*words*4,words*4)))
            masks=[potential]
            g.mem_write(0x4f93a4,struct.pack('<I',0x2104000))
            portal_count=c.u32(file+16+0x5c)
            for open_mask in (0,0xffffffff,0xaaaaaaaa):
                g.mem_write(0x2104000,struct.pack('<'+'I'*((portal_count+31)//32),*([open_mask]*((portal_count+31)//32))))
                invoke(g,0x16de20,(address+16,0x2105000),[(UC_X86_REG_EAX,cluster)])
                masks.append(list(struct.unpack('<'+'I'*words,g.mem_read(0x2105000,words*4))))
            samples[-1]['pvs_masks']=masks
            eye[2]-=z_offset
    out.append({'map':path.name,'samples':samples})
    print(path.name,'samples',len(samples),'types',sorted({s['classification'] for s in samples}),flush=True)
    c.close()

args.output.parent.mkdir(parents=True,exist_ok=True)
fixtures=args.output.with_suffix('.fixtures.json')
inputs=args.output.with_suffix('.inputs.txt')
fixtures.write_text(json.dumps({'maps':out,'aggregation_stubs':[],
    'compositor_stubs':'GPU submission only','framebuffer_parity':False},indent=2)+'\n')
inputs.write_text(''.join(' '.join([m['map'],*map(str,s['eye']),str(s['input_cluster'])])+'\n'
    for m in out for s in m['samples']))
command=[str(args.native_binary.resolve()),str(roots[0].resolve()),str(inputs.resolve())]
if len(roots)>1:command.append(str(roots[1].resolve()))
native=subprocess.check_output(command,cwd=ROOT,text=True).splitlines()
summary={'maps':len(out),'fixtures':len(native),'state_mismatches':[],
    'sky_model_control_source':'original14B360' if args.sky_model_from_metadata else 'explicit historical zero control',
    'compositor_mismatches':[],'lookup_mismatches':[],'bloom_mismatches':[],'pvs_mismatches':[],
    'aggregation_stubs':[],'compositor_stubs':'GPU submission only','framebuffer_parity':False,
    'executable_sha256':hashlib.sha256((ROOT/'assets/disc/default.xbe').read_bytes()).hexdigest(),
    'state_tolerance':{'absolute':2e-5,'relative':1e-6}}
rows=iter(native)
for m in out:
    for s in m['samples']:
        row=next(rows).split()
        state=s['primary']+s['secondary']+s['planar'][:6]+s['patchy']+s['camera']+\
            [s['transition'],s['plane_only']]+s['bloom']+s['sky_rgb']+[s['sky_alpha']]
        expected=state+s['extern_scalars']+s['extern_colors']+s['constants']
        actual=list(map(float,row[4:106]))
        context={'map':m['map'],'cluster':s['input_cluster'],'eye':s['eye']}
        if tuple(map(int,row[2:4]))!=(s['actual_cluster'],s['classification']):
            summary['state_mismatches'].append({**context,'field':'cluster/classification',
                'original':[s['actual_cluster'],s['classification']],'native':row[2:4]})
        for index,(a,b) in enumerate(zip(expected,actual)):
            if not math.isclose(a,b,rel_tol=1e-6,abs_tol=2e-5):
                summary['state_mismatches'].append({**context,'field':index,'original':a,'native':b})
        for index,(a,b) in enumerate(zip(s['composite'],map(int,row[106:166]))):
            if a!=b:summary['compositor_mismatches'].append({**context,'word':index,'original':a,'native':b})
        for index,(a,b) in enumerate(zip(s['coordinates'],map(float,row[166:174]))):
            if not math.isclose(a,b,rel_tol=1e-6,abs_tol=2e-5):
                summary['lookup_mismatches'].append({**context,'component':index,'original':a,'native':b})
        for index,(a,b) in enumerate(zip(s['bloom_frame'],map(float,row[174:177]))):
            if not math.isclose(a,b,rel_tol=1e-6,abs_tol=2e-5):
                summary['bloom_mismatches'].append({**context,'component':index,'original':a,'native':b})
        for index,(a,b) in enumerate(zip(sum(s['pvs_masks'],[]),map(int,row[177:]))):
            if a!=b:summary['pvs_mismatches'].append({**context,'word':index,'original':a,'native':b})
summary['passed']=not any(summary[key] for key in ('state_mismatches','compositor_mismatches','lookup_mismatches','bloom_mismatches','pvs_mismatches'))
args.output.write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps({key:summary[key] for key in ('maps','fixtures','passed','framebuffer_parity')}))
raise SystemExit(0 if summary['passed'] else 1)
