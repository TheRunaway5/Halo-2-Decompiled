#!/usr/bin/env python3
"""Unchanged D1E10 object-light combination region, with explicit service outputs."""
from pathlib import Path
import hashlib,json,random,struct,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import PIN,words,packed,fw
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
FRAME,OUT,DEFAULT=0x20010000,0x20020000,0x20021000
def main():
    x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000);u.mem_map(0x20000000,0x100000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    verified=set()
    def code(g,pc,size,_):
        assert bytes(g.mem_read(pc,size))==x.read(pc,size);verified.add(pc)
        if pc==0xd292e:g.emu_stop()
    u.hook_add(UC_HOOK_CODE,code)
    rows=[];actual=json.loads((ROOT/'analysis/multiplayer-shading/object-prt-snapshot-original.json').read_text())
    for r in actual['records']:
        m=r['light_mix'];rows.append([int(m['prt_valid']),int(m['baked_valid']),*m['prt_incident'],*m['prt_color'],*m['baked_incident'],*m['baked_color'],*m['default_incident']])
    rng=random.Random(0xd247d)
    for i in range(2048):
        rows.append([i&1,(i>>1)&1,*[fw(rng.uniform(-2,2))for _ in range(12)],fw(0),fw(0),fw(-1)])
    # Adjacent luminance and direction normalization thresholds, with exactly
    # opposed weighted directions and source zero-vector handling.
    for color in [0.,.00009999,.0001,.00010001,.2,1.]:
        for length in [0.,.00009999,.0001,.00010001,1.,2.]:
            for opposite in [False,True]:
                rows.append([1,1,*map(fw,[length,0,0,color,color,color,-length if opposite else 0,length,0,color,color,color,0,0,-1])])
    expected=[]
    for index,row in enumerate(rows):
        p,b,*v=row
        u.mem_write(FRAME,bytes(0x200));u.mem_write(FRAME+0x13,bytes([p]));u.mem_write(FRAME+0x58,packed(v[:3]));u.mem_write(FRAME+0x28,packed(v[3:6]));u.mem_write(FRAME+0x34,packed(v[6:9]));u.mem_write(FRAME+0x14,packed(v[9:12]));u.mem_write(DEFAULT,packed(v[12:15]));u.mem_write(0x4687b0,packed([DEFAULT]));u.mem_write(FRAME+0xa8,packed([OUT]));u.mem_write(OUT,packed([0xcdcdcdcd]*5))
        for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
        u.reg_write(UC_X86_REG_EAX,b);u.reg_write(UC_X86_REG_ESP,FRAME);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.emu_start(0xd247d,0xd2934,count=20000);assert u.reg_read(UC_X86_REG_EIP)==0xd292e and u.reg_read(UC_X86_REG_ESP)==FRAME
        out=words(bytes(u.mem_read(OUT,12)))+[struct.unpack('<I',bytes(u.mem_read(OUT+16,4)))[0]]
        if index<8:assert out==actual['records'][index]['light_mix']['output_words']
        expected.append(out)
    fixture=ROOT/'tests/fixtures/retail_source_object_light_mix.tsv';fixture.write_text('# Unchanged D247D..D292E arithmetic plus24590/30BF0/131ED0; supplied service outputs. First8 corroborated wholeD1E10 inputs/results.\n'+''.join(' '.join(f'{v:x}'for v in row)+' | '+' '.join(f'{v:x}'for v in out)+'\n'for row,out in zip(rows,expected)))
    report={'source_xbe_sha256':PIN,'source_unchanged':True,'cases':len(rows),'checked_words':len(rows)*4,'actual_whole_D1E10_controls':8,'instructions_verified':len(verified),'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'boundary':'Combination region executes unchanged; prior PRT/radiance/incident service outputs are explicit inputs. No native live cache, resident geometry or query scheduling producer.'}
    (ROOT/'analysis/multiplayer-shading/object-light-mix-original.json').write_text(json.dumps(report,indent=2)+'\n');fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
