#!/usr/bin/env python3
"""Compare native sampler-state decoding with retail 000189A0 instructions."""
import argparse, ctypes, hashlib, json, random, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EDX, UC_X86_REG_ESP

ROOT=Path(__file__).resolve().parents[1]
EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--library',type=Path,default=ROOT/'build-appearance/libhalo2_kernels.so')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/appearance/texture-states.json')
    a=p.parse_args()
    image=(ROOT/'assets/disc/default.xbe').read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED:raise ValueError('unsupported executable')
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    for i in range(count):
        _,address,_,offset,size=struct.unpack_from('<5I',image,table-base+i*56)
        u.mem_write(address,image[offset:offset+size])
    u.mem_map(0x2100000,0x100000);u.mem_map(0x3000000,0x1000)
    def word(at,value):u.mem_write(at,struct.pack('<I',value))
    word(0x2100020,0x2101000);word(0x210101c,0x2102000)
    word(0x407488,0x2103000)
    word(0x2103004,0x21f0000)
    native=ctypes.CDLL(str(a.library.resolve())).halo2_decode_texture_state
    native.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32)]
    rng=random.Random(0x189a0)
    cases=[bytes.fromhex('7654ba9801030400d4c3b2a198765432abcdef8910325476')]
    # Include every authored state found in the bounded Cairo rendering audit.
    audit=ROOT/'analysis/appearance/assets/01b_spacestation.json'
    if audit.exists():
        data=json.loads(audit.read_text())
        cases+=sorted({bytes.fromhex(b) for x in data['passes'].values()
            for pp in x['postprocess'] for b in pp['blocks']['texture_states']})
    cases += [rng.randbytes(24) for _ in range(4096)]
    failures=[]
    for n,record in enumerate(cases):
        # Random first index also exercises the retail packed range addressing.
        first=rng.randrange(8);word(0x2104000,512|first)
        u.mem_write(0x2102000+first*24,record)
        word(0x2103000,0x2140000)
        u.mem_write(0x21fff00,struct.pack('<II',0x3000000,0x2104000))
        u.reg_write(UC_X86_REG_EDX,0x2100000);u.reg_write(UC_X86_REG_ESP,0x21fff00)
        u.emu_start(0x189a0,0x3000000,count=10000)
        expected=list(struct.unpack('<12I',u.mem_read(0x404dd0,48)))
        expected+=list(struct.unpack('<2I',u.mem_read(0x404e44,8)))
        output=(ctypes.c_uint32*14)()
        if native((ctypes.c_uint8*24).from_buffer_copy(record),output)!=1:
            raise RuntimeError('native decoder rejected a fixed-size record')
        if list(output)!=expected:failures.append({'case':n,'input':record.hex(),'native':list(output),'retail':expected})
    report={'retail_xbe_sha256':EXPECTED,
        'native_library_sha256':hashlib.sha256(a.library.read_bytes()).hexdigest(),
        'function':'000189A0','cases':len(cases),
        'passed':len(cases)-len(failures),'failures':failures,
        'scope':'all 14 state values against original instructions; host filtering precision and complete frames unverified'}
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(f"Texture states: {report['passed']}/{report['cases']} exact")
    return bool(failures)

if __name__=='__main__':raise SystemExit(main())
