#!/usr/bin/env python3
"""Unchanged TU1.5 helper instructions over declared component inputs."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ, UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

GP = (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
      UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)
XMM = tuple(globals()[f'UC_X86_REG_XMM{n}'] for n in range(8))
BASE, STOP = 0x70000000, 0x70008000

def packed(data):
    patches = []
    offset = 0
    while offset < len(data):
        if data[offset] == 0xa5:
            offset += 1
            continue
        start = offset
        while offset < len(data) and data[offset] != 0xa5:
            offset += 1
        patches.append(dict(offset=start, bytes=data[start:offset].hex()))
    return dict(size=len(data), fill=0xa5, patches=patches)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=Path('tests/fixtures/death_cycle_helpers_tu15.json'))
    args = ap.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    cases = []
    for entry in (0x17b0d0, 0x11e7b0, 0x4057f0):
        for variant in range(32):
            uc = Uc(UC_ARCH_X86, UC_MODE_32)
            end = max((section.va+section.vsize+4095)&~4095 for section in image.sections)
            uc.mem_map(0x10000, end-0x10000)
            for section in image.sections:
                uc.mem_write(section.va, image.section_bytes(section))
            uc.mem_map(BASE, 0x9000)
            initial = bytearray([0xa5] * 0x8000)
            def put(offset, *words):
                struct.pack_into('<' + 'I'*len(words), initial, offset, *words)
            esp = BASE + 0x7f00
            put(0x7f00, STOP, (0, 0x3f800000, 0x80000000, 0x7fc01234)[variant & 3])
            put(0x2000, 0x3f800000, 0x80000000, 0xbf800000)
            put(0x2010, 0x3e800000, 0x40000000, 0x40400000)
            gp = [BASE+0x1000, BASE+0x2000 if variant&1 else 0,
                  BASE+0x2010 if variant&2 else 0, 0x12345678,
                  esp, 0x98765432, 0xfedcba98, 0x87654321]
            globals_before = []
            if entry == 0x17b0d0:
                pointer = struct.unpack('<I', image.read(0x474c70, 4))[0]
                globals_before = [(0x474c70, image.read(0x474c70, 4)),
                                  (pointer, image.read(pointer, 12))]
            elif entry == 0x11e7b0:
                cluster = (0, 1, 31, 32, 63, 127, 255, 511)[variant & 7]
                struct.pack_into('<h', initial, 0x1004, cluster)
                put(0x3000 + 0x1138 + (cluster//32)*4,
                    (1 << (cluster&31)) if variant&8 else 0)
                uc.mem_write(0x4f3218, struct.pack('<I', BASE+0x3000))
                globals_before = [(0x4f3218, struct.pack('<I', BASE+0x3000))]
            else:
                # Declared resource/parent and queue-fence inputs for the
                # unchanged D3D SDK predicate; no GPU completion is supplied
                # to the game-frame probe or to production by this fixture.
                gp[1] = BASE+0x1000
                kind = 0x50000 if variant&1 else 0x40000
                put(0x1000, kind | (0x80000 if variant&8 else 0))
                put(0x1008, 150 if variant&4 else 0)
                put(0x1014, BASE+0x2000 if variant&2 else 0)
                put(0x2000, 0x40000 | (0x80000 if variant&16 else 0))
                put(0x2008, 50 if variant&4 else 0)
                put(0x302c, 200)
                put(0x3030, BASE+0x4000)
                put(0x4000, 100)
                uc.mem_write(0x411568, struct.pack('<I', BASE+0x3000))
                globals_before = [(0x411568, struct.pack('<I', BASE+0x3000))]
            uc.mem_write(BASE, bytes(initial))
            xmm = [[0x3e800000+variant, 0xdeadbeef, 0x12345678, 0x87654321] for _ in XMM]
            for register, value in zip(GP, gp): uc.reg_write(register, value)
            for register, words in zip(XMM, xmm): uc.reg_write(register, int.from_bytes(struct.pack('<4I', *words), 'little'))
            flags = 2 | (0x8d5 if variant&16 else 0)
            uc.reg_write(UC_X86_REG_EFLAGS, flags)
            stores = []
            def store(engine, access, address, size, value, user):
                stores.append(dict(instruction=engine.reg_read(UC_X86_REG_EIP), address=address,
                    before=bytes(engine.mem_read(address, size)).hex(),
                    after=(value & ((1 << (size*8))-1)).to_bytes(size, 'little').hex()))
            uc.hook_add(UC_HOOK_MEM_WRITE, store)
            uc.emu_start(entry, STOP, count=10000)
            assert uc.reg_read(UC_X86_REG_EIP) == STOP
            cases.append(dict(entry=entry, gp=gp, xmm=xmm, flags=flags,
                regions=[dict(address=BASE, bytes=packed(initial))] +
                    [dict(address=p, bytes=b.hex()) for p,b in globals_before],
                final_gp=[uc.reg_read(r) for r in GP],
                final_xmm=[list(struct.unpack('<4I', uc.reg_read(r).to_bytes(16,'little'))) for r in XMM],
                final_flags=uc.reg_read(UC_X86_REG_EFLAGS), stores=stores,
                final_bytes=packed(bytes(uc.mem_read(BASE, len(initial))))))
    # DBB90's physical block, stopped before the actual impulse callee or its
    # attribution successor. Declared mass/report inputs exercise original
    # normalization, suppression, owner stores and impulse-vector selection.
    # No physical callback return is supplied by this bounded comparison.
    for variant in range(64):
        uc=Uc(UC_ARCH_X86,UC_MODE_32)
        end=max((section.va+section.vsize+4095)&~4095 for section in image.sections)
        uc.mem_map(0x10000,end-0x10000)
        for section in image.sections:uc.mem_write(section.va,image.section_bytes(section))
        uc.mem_map(BASE,0x9000);initial=bytearray([0xa5]*0x8000)
        def put(offset,*words):struct.pack_into('<'+'I'*len(words),initial,offset,*words)
        def floats(offset,*values):struct.pack_into('<'+'f'*len(values),initial,offset,*values)
        body,report,definition,damage,frame=[BASE+p for p in (0x1000,0x2000,0x4000,0x5000,0x7e00)]
        put(0x1000,0x10000);initial[0x10aa]=variant&1
        put(0x1134,0x80000 if variant&16 else 0)
        floats(0x4014,0.0 if variant&2 else 1.0)
        floats(0x5040,0.0 if variant&4 else 4.0)
        put(0x2014,7);put(0x200c,0x10003,0x10007)
        put(0x2004,(0 if variant&8 else 0x100)|(0x200 if variant&32 else 0))
        floats(0x2018,.25,-.5,0.0);floats(0x2024,1,2,3);floats(0x2034,1)
        put(0x2040,0xffffffff);put(0x5014,0)
        put(0x7e10,damage,definition);put(0x7e4c,0x10000)
        put(0x6008,2)
        uc.mem_write(0x4f3218,struct.pack('<I',BASE+0x6000))
        uc.mem_write(BASE,bytes(initial))
        gp=[0,0,0,0,frame,body,0,report];xmm=[[0,0,0,0] for _ in XMM]
        for register,value in zip(GP,gp):uc.reg_write(register,value)
        uc.reg_write(UC_X86_REG_EFLAGS,2);uc.reg_write(UC_X86_REG_FPCW,0x27f)
        uc.reg_write(UC_X86_REG_MXCSR,0x1f80)
        stores=[];reads={};stopped=[]
        def store(engine,access,address,size,value,user):
            stores.append(dict(instruction=engine.reg_read(UC_X86_REG_EIP),address=address,
                before=bytes(engine.mem_read(address,size)).hex(),
                after=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
        def read(engine,access,address,size,value,user):
            if not BASE<=address<BASE+0x8000:
                for n,b in enumerate(engine.mem_read(address,size)):reads[address+n]=b
        def stop(engine,pc,size,user):
            if pc in (0xe0b70,0xb98b0,0xdbfcc):stopped.append(pc);engine.emu_stop()
        uc.hook_add(UC_HOOK_MEM_WRITE,store);uc.hook_add(UC_HOOK_MEM_READ,read);uc.hook_add(UC_HOOK_CODE,stop)
        uc.emu_start(0xdbd64,STOP,count=10000);assert len(stopped)==1
        regions=[]
        for p in sorted(reads):
            if regions and p==regions[-1][0]+len(regions[-1][1]):regions[-1][1].append(reads[p])
            else:regions.append((p,bytearray([reads[p]])))
        cases.append(dict(entry=0xdbd64,stop=stopped[0],gp=gp,xmm=xmm,flags=2,
            regions=[dict(address=BASE,bytes=packed(initial))]+
                [dict(address=p,bytes=bytes(data).hex())for p,data in regions],
            final_gp=[uc.reg_read(r)for r in GP],
            final_xmm=[list(struct.unpack('<4I',uc.reg_read(r).to_bytes(16,'little')))for r in XMM],
            final_flags=uc.reg_read(UC_X86_REG_EFLAGS),stores=stores,
            final_bytes=packed(bytes(uc.mem_read(BASE,len(initial))))))
    # Radius direction block after the closest-point service: all physical
    # headers, current bounds and closest-point/normal values are declared
    # component inputs. Both original normalization and component predicate
    # execute; no query/impulse return is manufactured by this fixture.
    for variant in range(64):
        uc=Uc(UC_ARCH_X86,UC_MODE_32)
        end=max((section.va+section.vsize+4095)&~4095 for section in image.sections)
        uc.mem_map(0x10000,end-0x10000)
        for section in image.sections:uc.mem_write(section.va,image.section_bytes(section))
        uc.mem_map(BASE,0x9000);initial=bytearray([0xa5]*0x8000)
        def put(offset,*words):struct.pack_into('<'+'I'*len(words),initial,offset,*words)
        def floats(offset,*values):struct.pack_into('<'+'f'*len(values),initial,offset,*values)
        body,event,pool,rows,physical,record,frame=[BASE+p for p in (0x1000,0x2000,0x3000,0x4000,0x5000,0x6000,0x7c00)]
        put(0x10b4,0xffffffff if variant&1 else 0x10000)
        floats(0x1030,1,2,3);floats(0x2030,.5,-.25,1)
        put(0x3044,rows);put(0x4070,physical);put(0x4074,0 if variant&2 else 1)
        put(0x5040,record);put(0x603c,record+0x100)
        initial[0x6040]=1 if variant&4 else 0
        floats(0x6170,-1,.5,2)
        offset=.01 if variant&8 else 1.0
        floats(0x7c28,.5+offset,-.25,1+(.25 if variant&16 else 0))
        floats(0x7c34,0,0,1 if variant&32 else -1)
        put(0x7c14,body)
        uc.mem_write(0x52be5c,struct.pack('<I',pool));uc.mem_write(BASE,bytes(initial))
        gp=[0,0,0,0,frame,event,event+0x30,0];xmm=[[0,0,0,0]for _ in XMM]
        for register,value in zip(GP,gp):uc.reg_write(register,value)
        uc.reg_write(UC_X86_REG_EFLAGS,2);uc.reg_write(UC_X86_REG_FPCW,0x27f);uc.reg_write(UC_X86_REG_MXCSR,0x1f80)
        stores=[];reads={};stopped=[]
        def store(engine,access,address,size,value,user):
            stores.append(dict(instruction=engine.reg_read(UC_X86_REG_EIP),address=address,
                before=bytes(engine.mem_read(address,size)).hex(),
                after=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
        def read(engine,access,address,size,value,user):
            if not BASE<=address<BASE+0x8000:
                for n,b in enumerate(engine.mem_read(address,size)):reads[address+n]=b
        def stop(engine,pc,size,user):
            if pc==0xd9ca4:stopped.append(pc);engine.emu_stop()
        uc.hook_add(UC_HOOK_MEM_WRITE,store);uc.hook_add(UC_HOOK_MEM_READ,read);uc.hook_add(UC_HOOK_CODE,stop)
        uc.emu_start(0xd9acc,STOP,count=10000);assert stopped==[0xd9ca4]
        regions=[]
        for p in sorted(reads):
            if regions and p==regions[-1][0]+len(regions[-1][1]):regions[-1][1].append(reads[p])
            else:regions.append((p,bytearray([reads[p]])))
        cases.append(dict(entry=0xd9acc,stop=0xd9ca4,gp=gp,xmm=xmm,flags=2,
            regions=[dict(address=BASE,bytes=packed(initial))]+
                [dict(address=p,bytes=bytes(data).hex())for p,data in regions],
            final_gp=[uc.reg_read(r)for r in GP],
            final_xmm=[list(struct.unpack('<4I',uc.reg_read(r).to_bytes(16,'little')))for r in XMM],
            final_flags=uc.reg_read(UC_X86_REG_EFLAGS),stores=stores,
            final_bytes=packed(bytes(uc.mem_read(BASE,len(initial))))))
    report = dict(image_sha256=NEW_PIN, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scope='Declared helper inputs; unchanged original instructions; no gameplay service returns or live ragdoll claim.', cases=cases)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, separators=(',',':'))+'\n')
    print(f'{len(cases)} unchanged TU1.5 helper cases')

if __name__ == '__main__': main()
