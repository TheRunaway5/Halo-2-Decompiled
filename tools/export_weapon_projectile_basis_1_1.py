#!/usr/bin/env python3
"""Untouched TU1.1 1090CD..1091F4 basis fragment; explicit finite directions.

No muzzle, distribution, birth, resource, contact or gameplay results are
supplied or established. Raw stack words are fragment inputs, not live poses.
"""
import hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def main():
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(x.data).hexdigest()=='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    stack=0x2001000;u.mem_map(0x2000000,0x20000)
    cases=[]
    for a in (-1.,-0.,0.,1.,.00001,.0001):
        for b in (-1.,-0.,0.,1.,.00001,.0001):
            for c in (-1.,-0.,0.,1.,.00001,.0001):cases.append((a,b,c))
    rng=random.Random(0x1090cd)
    cases.extend(tuple(rng.uniform(-1,1) for _ in range(3))for i in range(1000))
    rows=[]
    for f in cases:
        raw=struct.pack('<3f',*f);u.mem_write(stack+0x12c,raw)
        u.reg_write(UC_X86_REG_ESP,stack);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
        u.reg_write(UC_X86_REG_XMM7,0)
        u.emu_start(0x1090cd,0x1091f4,count=1000)
        assert u.reg_read(UC_X86_REG_EIP)==0x1091f4
        out=bytes(u.mem_read(stack+0x138,12))
        rows.append(' '.join(f'{w:08x}' for w in struct.unpack('<6I',raw+out)))
    path=ROOT/'tests/fixtures/weapon_projectile_basis_original.tsv'
    path.write_text('# forward XYZ words; original up XYZ words\n'+'\n'.join(rows)+'\n')
    report={'scope':__doc__,'cases':len(rows),'xbe_sha256':hashlib.sha256(x.data).hexdigest(),
        'fragment_sha256':hashlib.sha256(x.read(0x1090cd,0x1091f4-0x1090cd)).hexdigest(),
        'exporter_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'fixture_sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
    path.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report))
if __name__=='__main__':main()
