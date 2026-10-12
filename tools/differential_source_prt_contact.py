#!/usr/bin/env python3
"""Execute unchanged retail241C0 over actual and controlled explicit cache banks."""
from pathlib import Path
import collections, hashlib, json, random, struct, subprocess, sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,words,packed,fw
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
STACK,STOP,POINTS,INCIDENT,COLOR=0x200fe000,0x200ff000,0x20000100,0x20000200,0x20000220

def main():
    x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
    snapshot=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000);u.mem_map(0x20000000,0x100000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    verified=set()
    def observe(g,pc,size,_):
        if pc==STOP:g.emu_stop();return
        assert bytes(g.mem_read(pc,size))==x.read(pc,size)
        verified.add(pc)
    u.hook_add(UC_HOOK_CODE,observe)
    count=struct.unpack('<i',snapshot.read(0x5093e8,4))[0]
    banks=[(count,snapshot.read(0x4b89b0,count*104),snapshot.read(0x487b1c,0x27000))]
    rng=random.Random(0x241c0)
    def controlled(token=6400,duplicate=False,active=1):
        row=[0xe0010020,0,token]+[j*320 for j in range(16)]+[0x02020202]*4+[fw(.31),fw(-.17),fw(.8)]
        later=row.copy();later[2]=token+1280;later[23:]=[fw(-.9),fw(.7),fw(.1)]
        arena=packed([fw(rng.uniform(-1.5,1.5)) for _ in range(16000)])
        return active,packed(row+(later if duplicate else [])),arena
    banks.extend([controlled(),controlled(0x10000000+6400,True,2),controlled(active=0)])
    rows=[];labels=[]
    actual=json.loads((ROOT/'analysis/multiplayer-shading/object-prt-snapshot-original.json').read_text())
    short_rows=[]
    for record in actual['records']:
        for call in record['prt_calls']:
            rows.append([0,call['owner'],call['section'],call['palette'],*[v for p in call['points']for v in p],*call['fractions']])
            labels.append('actual D1E10 invocation over diagnostic source14A8E0 contact')
            if call['encoded_short_points']:
                short_rows.extend((raw,decoded)for raw,decoded in zip(call['encoded_short_points'],call['points']))
    for bank in [1,2]:
        for section in range(16):
            for palette in [-2,-1,0,1,2,255]:
                points=[[fw(j+.875),*[fw(rng.uniform(-1,1))for _ in range(4)]]for j in [0,5,9]]
                rows.append([bank,0xe0010020,section,palette&0xffffffff,*[v for p in points for v in p],fw(.2),fw(.7)])
                labels.append('exact fullkey, node selection, negative selector and cluster truncation')
    for i in range(1024):
        bank=1 if i%2 else 2;points=[[fw(rng.uniform(0,9.99)),*[fw(rng.uniform(-1,1))for _ in range(4)]]for _ in range(3)]
        rows.append([bank,0xe0010020,rng.randrange(16),rng.randrange(4),*[v for p in points for v in p],fw(rng.uniform(-1,2)),fw(rng.uniform(-1,2))]);labels.append('controlled current cache arithmetic and unclipped input fractions')
    # Missing owner does not consume invalid section/palette/points or arena.
    for bank,owner in [(0,0x12340010),(1,0xe0020020),(3,0xe0010020)]:
        rows.append([bank,owner,0xffffffff,0xffffffff,*[0x7fc00000]*17]);labels.append('full salt mismatch or zero active prefix leaves outputs untouched')
    expected=[]
    for row in rows:
        bank,owner,section,palette,*rest=row;active,records,arena=banks[bank]
        u.mem_write(0x5093e8,packed([active]));u.mem_write(0x4b89b0,records);u.mem_write(0x487b1c,arena)
        u.mem_write(POINTS,packed(rest[:15]));u.mem_write(INCIDENT,packed([0xcdcdcdcd]*3));u.mem_write(COLOR,packed([0xcdcdcdcd]*3))
        u.mem_write(STACK,packed([STOP,section,POINTS,POINTS+20,POINTS+40,*rest[15:17],COLOR]))
        for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
        u.reg_write(UC_X86_REG_EAX,INCIDENT);u.reg_write(UC_X86_REG_ECX,palette);u.reg_write(UC_X86_REG_EDX,owner)
        u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.emu_start(0x241c0,STOP+1,count=10000);assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==STACK+32
        expected.append([u.reg_read(UC_X86_REG_EAX)&255,*words(bytes(u.mem_read(INCIDENT,12))),*words(bytes(u.mem_read(COLOR,12)))])
    data=bytearray(b'H2PRTCO1'+packed([len(banks)]))
    for active,records,arena in banks:data.extend(packed([active,len(records)//104,len(arena)]));data.extend(records);data.extend(arena)
    fixture=ROOT/'tests/fixtures/retail_source_prt_contact.tsv';bankfile=fixture.with_suffix('.bin')
    bankfile.write_bytes(data);fixture.write_text('# Actual and controlled unchanged241C0 cache banks. No cache producer replacement.\n'+''.join(' '.join(f'{v:x}'for v in row)+' | '+' '.join(f'{v:x}'for v in out)+'\n'for row,out in zip(rows,expected)))
    short_fixture=ROOT/'tests/fixtures/retail_source_prt_contact_short.tsv'
    short_fixture.write_text('# Literal original D1E10 CPU declaration57 inputs/outputs, same diagnostic contacts.\n'+''.join(' '.join(f'{v&65535:x}'for v in raw)+' | '+' '.join(f'{v:x}'for v in out)+'\n'for raw,out in short_rows))
    report={'source_xbe_sha256':PIN,'source_unchanged':True,'cases':len(rows),'actual_D1E10_cases':8,'checked_words':len(rows)*7,'instructions_verified':len(verified),
        'source_function_sha256':hashlib.sha256(x.read(0x241c0,706)).hexdigest(),'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'bank_sha256':hashlib.sha256(data).hexdigest(),
        'snapshot_ram_sha256':hashlib.sha256(snapshot.ram).hexdigest(),'snapshot_tick':snapshot.report['clock_tick'],'actual_cache_count':count,'actual_cpu_short_points':len(short_rows),'short_fixture_sha256':hashlib.sha256(short_fixture.read_bytes()).hexdigest(),
        'boundary':'Actual diagnostic source contacts and literal current cache; controlled rows explicitly supplied. No native cache creation, registration or game query scheduling.'}
    (ROOT/'analysis/multiplayer-shading/prt-contact-original.json').write_text(json.dumps(report,indent=2)+'\n');fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
