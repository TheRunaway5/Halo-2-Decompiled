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
    for entry in (0x17b0d0, 0x11e7b0, 0x4057f0, 0x178d80, 0xd2ad0, 0x179700, 0xdcfa0, 0x1706f0, 0x1054b0, 0x118760, 0x282780):
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
            globals_before = []; helper_stop = None
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
            elif entry == 0x282780:
                # Complete reached world-removal callback. Original property
                # lookup chooses the first0x2001 key, narrows its full datum,
                # and clears bit12 only in that actual physical record.
                case=variant&7;slot=variant>>3
                put(0x7f04,BASE+0x1000)
                put(0x1004,BASE+0x2000)
                put(0x2018,2 if case==0 else 1)
                put(0x2020,0 if case==1 else BASE+0x3000)
                put(0x3030,BASE+0x4000)
                put(0x3034,(-1 if case==2 else 0 if case==3 else 4 if case==7 else 1)&0xffffffff)
                for n in range(4):put(0x4000+n*8,0x2002,0xeeee0003)
                if case in (5,6):put(0x4000,0x2001,0xffffffff if case==5 else 0xe2b40000|slot)
                if case==7:
                    put(0x4008,0x2001,0xe2b40000|slot)
                    put(0x4010,0x2001,0xe2b40000|((slot+1)&3))
                put(0x5044,BASE+0x6000)
                for n in range(4):put(0x6000+n*0xa0+4,(0x12345678,0xffffefff,0xffffffff,0x1000)[n])
                uc.mem_write(0x52be5c,struct.pack('<I',BASE+0x5000))
                globals_before=[(0x52be5c,struct.pack('<I',BASE+0x5000))]
            elif entry == 0x118760:
                # Whole nonzero-low-word bypass, or explicit stop before
                # the mode0 external geometry owner. Unused pool is absent.
                mode=(0,1,0x10000,0xffff,0x80000000,0xffff0001,0x100,0x10001)[variant&7]
                put(0x7f04,mode,0xee610000|variant)
                if (mode&0xffff)==0:
                    helper_stop=0x47860
                    put(0x1044,BASE+0x2000)
                    put(0x2000+variant*0x2bc+0xc,0x12345678)
                    uc.mem_write(0x4ecc04,struct.pack('<I',BASE+0x1000))
                    globals_before=[(0x4ecc04,struct.pack('<I',BASE+0x1000))]
            elif entry == 0x1054b0:
                # Literal current object/tag tables, signed count/index and
                # dirty per-record flags. Keep original wrapping arithmetic.
                gp[0] = 0xe2b40000|variant
                gp[1] = (-1,0,1,4,31,255,0x7fffffff,0x80000000)[variant>>2]&0xffffffff
                put(0x1044,BASE+0x2000)
                put(0x2000+variant*12+8,BASE+0x4000)
                put(0x4000,0xe75b0000|variant)
                put(0x3000+variant*16+8,BASE+0x6000)
                put(0x62d0,(-1,0,1,4)[variant&3]&0xffffffff)
                for n in range(-1,4):initial[0x41a8+n*0x34]=(variant*7+n+2)&255
                for address,value in ((0x4ecbd0,BASE+0x1000),(0x4f0414,BASE+0x3000)):
                    uc.mem_write(address,struct.pack('<I',value))
                globals_before=[(0x4ecbd0,struct.pack('<I',BASE+0x1000)),(0x4f0414,struct.pack('<I',BASE+0x3000))]
            elif entry == 0x1706f0:
                # Complete first-match lookup over authored signed bytes.
                # Preserve tag index narrowing and negative/empty counts.
                gp[0] = 0xe75b0000|variant
                gp[7] = (-128,-1,1,128)[variant>>3]&0xffffffff
                count = (-1,0,1,4,8,16,31,32)[variant&7]
                put(0x1000+variant*16+8,BASE+0x3000)
                put(0x3070,count&0xffffffff,BASE+0x4000)
                for n in range(max(0,count)):
                    initial[0x4000+n*0x10+5] = (-128,-1,1)[n%3]&255
                uc.mem_write(0x4f0414,struct.pack('<I',BASE+0x1000))
                globals_before = [(0x4f0414,struct.pack('<I',BASE+0x1000))]
            elif entry == 0xdcfa0:
                # Complete material-match mask loop, including zero/negative
                # counts, repeated matches and 16/32-bit shift boundaries.
                count = (-1,0,1,15,16,17,32,33)[variant&7]
                put(0x10e0, count&0xffffffff, BASE+0x3000)
                put(0x7f04, BASE+0x2000)
                put(0x203c, 0x12345678)
                for n in range(max(0,count)):
                    put(0x3000+n*0x14+8, 0x12345678 if (n+variant)%3 else 0x87654321)
                gp[6] = BASE+0x5000
                gp[7] = (0,0xaaaa,0xffff,0x8000)[variant>>3]
                struct.pack_into('<H',initial,0x5000,0x0101*(variant&15))
            elif entry == 0x179700:
                # Request-construction prelude only. Stop before actual
                # BAD60; no matrix, effect creation or return is supplied.
                helper_stop = 0xbad60
                gp[0] = 0xe28d001e+variant
                gp[2] = BASE+0x2000 if variant&1 else 0
                gp[6] = 0x0f000e30
                put(0x2000, gp[0], 0xffffffff, 0xffff)
                put(0x7f04, 0xe7ad0638, 0xffffffff, 0x3f800000, variant, 0)
            elif entry == 0xd2ad0:
                # Declared current source object header/body and full datum.
                # The unchanged helper resolves its actual body+10 identity.
                gp[1] = BASE+0x1000+variant*0x20
                gp[2] = ((0xe28d+variant)<<16)|variant
                put(0x2044, BASE+0x3000)
                put(0x3000+variant*12+8, BASE+0x5000+variant*0x20)
                put(0x5000+variant*0x20+0x10, (0,0xffffffff,0x12345678,gp[2])[variant&3])
                uc.mem_write(0x4ecbd0, struct.pack('<I', BASE+0x2000))
                globals_before = [(0x4ecbd0, struct.pack('<I', BASE+0x2000))]
            elif entry == 0x178d80:
                # Actual TU1.5 effect creation request initializer. Retain
                # its executable constant and dirty packet/guard bytes.
                gp[2] = BASE+0x1000+variant*0x80
                globals_before = [(0x46a0d0, image.read(0x46a0d0, 4))]
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
            if helper_stop is not None:
                def stop_before(engine, address, size, user):
                    if address == helper_stop: engine.emu_stop()
                uc.hook_add(UC_HOOK_CODE, stop_before)
            uc.emu_start(entry, STOP, count=10000)
            assert uc.reg_read(UC_X86_REG_EIP) == (helper_stop or STOP)
            cases.append(dict(entry=entry, gp=gp, xmm=xmm, flags=flags,
                regions=[dict(address=BASE, bytes=packed(initial))] +
                    [dict(address=p, bytes=b.hex()) for p,b in globals_before],
                final_gp=[uc.reg_read(r) for r in GP],
                final_xmm=[list(struct.unpack('<4I', uc.reg_read(r).to_bytes(16,'little'))) for r in XMM],
                final_flags=uc.reg_read(UC_X86_REG_EFLAGS), stores=stores,
                final_bytes=packed(bytes(uc.mem_read(BASE, len(initial))))))
            if helper_stop is not None: cases[-1]['stop'] = helper_stop
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
