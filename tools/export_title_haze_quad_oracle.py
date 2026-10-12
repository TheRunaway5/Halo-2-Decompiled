#!/usr/bin/env python3
"""Compare native haze attributes with unchanged original 26CA0 or 27520.

No original instructions are intercepted or replaced. Positions execute the
original depth calculation and half-resolution scale, and modes 1..4 execute
the original SSE UV callback. Captured quad values are separate GPU fixtures.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_MXCSR

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--native-binary',type=Path,required=True)
    p.add_argument('--direct',action='store_true')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/title-haze-quad.json')
    a=p.parse_args();x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
    g.mem_map(0x2100000,0x100000);g.mem_map(0x3000000,4096)
    corners=((0.,0.),(0.,1.),(1.,1.),(1.,0.))
    views=((640,480),(1280,720),(1280,800),(1680,720),(1920,1080),(1000,600))
    original_frames=json.loads((ROOT/'analysis/frontend/patchy-fog-differential.json').read_text())['fixtures']
    rng=random.Random(0x26ca0);inputs=[];expected=[]
    for n in range(512):
        width,height=views[n%len(views)]
        if not a.direct:width,height=width*.5,height*.5
        if n<len(original_frames):
            f=original_frames[n];rows=[v for layer in range(4)for v in f['layers'][layer*14+6:layer*14+14]];vertical=f['vertical']
        else:rows=[rng.uniform(-32,32)for _ in range(32)];vertical=rng.uniform(-2,2)
        rows=struct.unpack('<32f',struct.pack('<32f',*rows));vertical=struct.unpack('<f',struct.pack('<f',vertical))[0]
        near,far=.015625,1024.
        distance=(rng.uniform(0,20),rng.uniform(21,100))
        if a.direct:rows=struct.unpack('<32f',struct.pack('<32f',*distance,width*.5,height*.5,*([0.]*28)))
        inputs.append(struct.pack('<37f',*rows,vertical,width,height,near,far))
        for layer in range(4):
            g.mem_write(0x4b9cac+layer*56+24,struct.pack('<8f',*rows[layer*8:layer*8+8]))
        for address,values in ((0x485ad4,(near,far)),(0x4b9d90,(.5,vertical)),(0x509400,(16777215. if a.direct else 1.,)),
            (0x48565c,(far,)),(0x4857dc,rows[:2]),(0x2108000,(0.,width if a.direct else width*2,0.,height if a.direct else height*2))):
            g.mem_write(address,struct.pack('<'+'f'*len(values),*values))
        g.mem_write(0x4b9d9c,b'\1')
        result=[]
        for mode in range(5):
            for uv in corners:
                g.mem_write(0x2108100,struct.pack('<2f',*uv));g.mem_write(0x2108200,bytes([0xcc])*16)
                g.mem_write(0x21fff00,struct.pack('<6I',0x3000000,mode,0x2108000,0x2108100,0x2108200,0))
                g.reg_write(UC_X86_REG_ESP,0x21fff00);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
                g.emu_start(0x27520 if a.direct else 0x26ca0,0x3000000,count=10000)
                assert g.reg_read(UC_X86_REG_EIP)==0x3000000 and g.reg_read(UC_X86_REG_EAX)&255==1
                assert g.reg_read(UC_X86_REG_ESP)==0x21fff00+24
                result.append(bytes(g.mem_read(0x2108200,16)))
        expected.append(b''.join(result))
    ip=a.output.with_suffix('.inputs.bin');ep=a.output.with_suffix('.original.bin');np=a.output.with_suffix('.native.bin')
    a.output.parent.mkdir(parents=True,exist_ok=True);ip.write_bytes(struct.pack('<I',len(inputs))+b''.join(inputs));ep.write_bytes(b''.join(expected))
    subprocess.run([str(a.native_binary.resolve()),*(['--direct']if a.direct else []),str(ip.resolve()),str(np.resolve())],check=True)
    original=ep.read_bytes();native=np.read_bytes();assert len(native)==len(original)
    report={'xbe_sha256':PIN,'original_entry':'00027520'if a.direct else '00026ca0','intercepted_instructions':0,'fixtures':len(inputs),
        'viewports':views,'original_bytes':len(original),'different_bytes':sum(x!=y for x,y in zip(native,original)),
        'all_attribute_bytes_exact':native==original,'input_sha256':hashlib.sha256(ip.read_bytes()).hexdigest(),
        'expected_sha256':hashlib.sha256(original).hexdigest(),'native_binary_sha256':hashlib.sha256(a.native_binary.read_bytes()).hexdigest(),
        'live_camera_and_random_history_verified':False,'complete_native_frame_parity_verified':False}
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));raise SystemExit(native!=original)

if __name__=='__main__':main()
