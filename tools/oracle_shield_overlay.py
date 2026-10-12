#!/usr/bin/env python3
"""Run unchanged retail 4BCC0 through shield selection on original Lockout tags.

B9CE0, 3D3E0 and 4C450 execute unchanged. The explicit geometry-history global
4BA050=-1 excludes the separate previous-frame lookup. Execution stops at
4C09A before geometry-range finalization, or returns on an early rejection.
Original tag records are relocated, then selected fields are explicitly varied.
This proves selector admission/state; it is not a submitted draw or pixel proof.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct

from oracle_damage_feedback import FeedbackOracle, bits, word
from oracle_multiplayer_hud_positions import ROOT, STACK, STOP, PIN
from oracle_mp_spawn import Cache, CACHE_PIN
from unicorn.x86_const import UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX


class OverlayOracle(FeedbackOracle):
    def hook(self, uc, address, size, data):
        if address == 0x4c09a:
            self.accepted = self.read(uc.reg_read(UC_X86_REG_ESP)+0x2c, '<h')[0]
            uc.emu_stop()

    def __init__(self, cache):
        super().__init__()
        self.context = self.alloc(bytes(0x1e4))
        self.parent = self.alloc(bytes(0x400))
        self.headers = self.alloc(bytes(24))
        pool = self.r(0x4e0300)
        self.w(pool+0x44, self.headers)
        self.w(self.headers+8, self.unit)
        self.w(self.headers+20, self.parent)
        self.w(self.parent+0x14, 0xffffffff)
        page=cache.magic&~4095
        self.g.mem_map(page, ((len(cache.data)+cache.magic-page+4095)//4096)*4096)
        self.g.mem_write(cache.magic, bytes(cache.data))
        for datum, tag in cache.tags.items():
            if tag['offset'] is not None:
                self.w(self.table+(datum&65535)*16+8, tag['offset']+cache.magic)
        self.w(0x4ba050, 0xffffffff)  # no previous-frame LOD history
        self.g.mem_write(0x4ba019, b'\0')

    def select(self, values):
        (datum, recent, shield, mode, owner, force, header_hidden, object_hidden,
         parent_present, parent_header_hidden, parent_hidden, model_none,
         render_none, fp_count, cap, distance, damage_count, missing, dead,
         force_lod5) = values
        self.w(self.unit, datum)
        self.w(self.unit+4, object_hidden | force_lod5<<19)
        self.w(self.unit+0x14, 1 if parent_present else 0xffffffff)
        self.w(self.parent+4, parent_hidden)
        self.g.mem_write(self.headers+2, bytes([header_hidden<<4]))
        self.g.mem_write(self.headers+14, bytes([parent_header_hidden<<4]))
        self.w(self.unit+0xf0, shield)
        self.w(self.unit+0xf4, recent)
        self.w(self.unit+0x10a, dead<<2)
        self.w(0x4c1bd0, 0 if owner else 0xffffffff)
        self.g.mem_write(0x4c1bd4, struct.pack('<h', fp_count))
        self.w(0x4ba01c, cap)
        self.g.mem_write(self.context, b'\xa5'*0x1e4)
        self.g.mem_write(self.output, b'\xa5'*8)
        self.accepted = None
        self.prepare([0, distance, mode, self.output, force, self.output+4],
                     [(UC_X86_REG_EDI, self.context)])
        self.g.emu_start(0x4bcc0, STOP, count=200000)
        end=self.g.reg_read(UC_X86_REG_EIP)
        assert end in (0x4c09a, STOP), hex(end)
        if self.accepted is None:
            self.accepted=struct.unpack('<h',struct.pack('<H',self.g.reg_read(UC_X86_REG_EAX)&65535))[0]
        return [self.read(self.output,'<B')[0], self.read(self.output+4,'<B')[0],
                self.accepted, self.r(self.context+0x16c)&0x21,
                self.r(self.context+0x170), self.r(self.context+0x180),
                self.r(self.context+0x178), self.r(self.context+0x188)]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/damage-shields-goal-20261011')
    args=parser.parse_args(); args.output.mkdir(parents=True,exist_ok=True)
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(cache.data).hexdigest()==CACHE_PIN
    original=OverlayOracle(cache); rng=random.Random(0x4bcc0)
    scalar=[bits(x) for x in (-1.,0.,.00001,.0001,.125,.5,1.,1.5,2.,4.)]
    scalar += [0x80000000,0x7fc12345,0xffc12345,0x7f800000,0xff800000]
    rows=[]; categories=dict(damage=0,overshield=0,both=0,none=0)
    for n in range(1800):
        datum=(0xe9f2086d,0xea1b0896)[n%2]
        b=cache.tags[datum]['offset']; hdatum=cache.u(b+0x38); h=cache.tags[hdatum]['offset']
        q=cache.u(h+0x64)-cache.magic
        assert cache.u(h+0x9c)==0 and cache.u(h+0xf4)==0xffffffff
        assert cache.u(cache.tags[cache.u(h+4)]['offset']+0x74)==0  # no shadow-service call
        # Restore only the fields varied by this harness; asset files stay read-only.
        for at,size in ((b+0x38,4),(h+4,4),(h+0x60,4),(q+0x80,4),(q+0x88,4),(q+0xec,4),(q+0xf4,4)):
            original.g.mem_write(at+cache.magic,bytes(cache.data[at:at+size]))
        values=[datum,rng.choice(scalar),rng.choice(scalar),rng.choice([0,0,0,1,2,0xffffffff]),
                rng.randrange(2),rng.randrange(2),rng.randrange(8)==0,rng.randrange(8)==0,
                rng.randrange(2),rng.randrange(8)==0,rng.randrange(8)==0,
                rng.randrange(8)==0,rng.randrange(8)==0,rng.choice([-2,-1,0,1,2,4]),
                rng.choice([-1,0,1,2,4,5]),bits(rng.choice([0.,.1,1.,5.,20.,100.,1000.])),
                rng.choice([-1,0,1,1,1,2]),rng.randrange(16),rng.randrange(2),rng.randrange(8)==0]
        # Dense ordinary controls independently exercise every scalar/ref pair.
        if n<450:
            values[3]=0;values[5:8]=[0,False,False];values[8]=0;values[11:13]=[False,False]
            values[13]=1;values[14]=5;values[16]=1;values[17]=0;values[19]=0
        original.w(b+cache.magic+0x38, 0xffffffff if values[11] else hdatum)
        original.w(h+cache.magic+4, 0xffffffff if values[12] else cache.u(h+4))
        original.w(h+cache.magic+0x60, values[16])
        for i,offset in enumerate((0x80,0x88,0xec,0xf4)):
            if values[17]&(1<<i):original.w(q+cache.magic+offset,0xffffffff)
        result=original.select(values)
        flag=result[3];categories['both' if flag==0x21 else 'damage' if flag==1 else 'overshield' if flag==0x20 else 'none']+=1
        rows.append(' '.join(map(word,[n,*values,*result])))
    fixture=args.output/'shield-overlay.tsv'
    fixture.write_text('# Unchanged retail 4BCC0/B9CE0/3D3E0/4C450, original Lockout metadata\n'+'\n'.join(rows)+'\n')
    dependencies=[ROOT/'assets/disc/default.xbe',ROOT/'assets/disc/maps/lockout.map',Path(__file__).resolve(),
                  ROOT/'tools/oracle_damage_feedback.py',ROOT/'tools/oracle_multiplayer_hud_positions.py']
    report=dict(method=__doc__,cases=len(rows),categories=categories,retail_sha256=PIN,
                fixtures_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
                dependencies={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies})
    (args.output/'shield-overlay-original.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(rows),categories=categories)))


if __name__=='__main__':main()
