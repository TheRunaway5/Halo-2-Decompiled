#!/usr/bin/env python3
"""Compare combiner assembly against the original Xbox 0001BA00 instructions.

All retail pass implementations and their fragment permutation choices are
covered. The report concerns assembly bytes, not frame or GPU arithmetic parity.
"""
import argparse
import ctypes
from contextlib import closing
import hashlib
import importlib.util
import json
import random
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'


def compare_pixel_remapping(guest, library):
    """Compare packed colors, fragment offsets and permutation filtering.

    The retail dirty-register masks belong to the pending upload scheduler;
    this gate compares the resulting 60-word program, not upload timing.
    """
    U32, I32, U8 = ctypes.c_uint32, ctypes.c_int32, ctypes.c_uint8
    native = ctypes.CDLL(str(library.resolve())).halo2_remap_pixel_constants
    native.argtypes = [ctypes.POINTER(U32), ctypes.POINTER(U32),
                       ctypes.POINTER(U32), U32, ctypes.POINTER(U32), U32,
                       ctypes.POINTER(I32), ctypes.POINTER(U8), ctypes.POINTER(U32)]
    native.restype = ctypes.c_int
    rng = random.Random(0x17170)
    cases, failures = 0, []
    # Every component mask, both constant banks and the final combiner,
    # followed by overlapping writes and skipped switch permutations.
    scenarios = [(fragmented, component, bank, slot)
                 for fragmented in (False, True) for component in range(8)
                 for bank in range(2) for slot in range(9)]
    scenarios += [None] * 1000
    for scenario in scenarios:
        initial = [rng.getrandbits(32) for _ in range(60)]
        source = [rng.getrandbits(32) for _ in range(256)]
        fragment_count = (1 if scenario[0] else 0) if scenario else rng.randrange(9)
        switches = [rng.randrange(8) for _ in range(8)]
        switch_indices = [rng.choice([-1] + list(range(8))) for _ in range(fragment_count)]
        fragments = [(8 << 25) | (switch & 0xffff) for switch in switch_indices]
        offsets = [rng.randrange(8) for _ in range(8)]
        if scenario:
            offsets[0] = 0
        mappings = []
        for _ in range(1 if scenario else rng.randrange(65)):
            fragment = rng.randrange(fragment_count) if fragment_count else 0
            if scenario:
                _, component, bank, slot = scenario
            else:
                component, bank = rng.randrange(256), rng.randrange(2)
                slot = rng.choice(list(range(8 - offsets[fragment])) + [8]) if fragment_count else rng.randrange(9)
            permutation = rng.randrange(8)
            if scenario and switch_indices:
                switch = switch_indices[fragment]
                permutation = switches[switch] if switch != -1 else permutation
            destination = fragment | (permutation << 3) | (slot << 6) | (bank << 10)
            mappings.append(destination | (component << 16) | (rng.randrange(256) << 24))
        guest.mem_write(0x2100000, b'\0' * 0x1800)
        guest.mem_write(0x210000c, struct.pack('<II', 0x2120000, 0x2130000))
        guest.mem_write(0x2100020, struct.pack('<II', 0x2180000, 0x2170000))
        guest.mem_write(0x2120010, struct.pack('<I', 0x2140000))
        guest.mem_write(0x2130024, struct.pack('<I', 0x2150000))
        guest.mem_write(0x2180024, struct.pack('<I', 0x2190000))
        guest.mem_write(0x21700f6, struct.pack('<H', (fragment_count << 9) | 4))
        guest.mem_write(0x2100300, struct.pack('<8i', *switches))
        guest.mem_write(0x2101424, struct.pack('<60I', *initial))
        guest.mem_write(0x2101520, bytes(offsets))
        guest.mem_write(0x2140000, struct.pack('<256I', *source))
        guest.mem_write(0x2150000, bytes(12) + struct.pack('<' + 'I' * len(mappings), *mappings))
        guest.mem_write(0x2190000, bytes(16) + struct.pack('<' + 'I' * fragment_count, *fragments))
        guest.mem_write(0x2160000, struct.pack('<H', (len(mappings) << 9) | 3))
        guest.mem_write(0x21fff00, struct.pack('<II', 0x3000000, 0x2160000))
        guest.reg_write(UC_X86_REG_EAX, 0x2100000)
        guest.reg_write(UC_X86_REG_ESP, 0x21fff00)
        guest.emu_start(0x17170, 0x3000000, count=100000)
        if guest.reg_read(UC_X86_REG_EIP) != 0x3000000:
            raise RuntimeError('retail pixel remapper did not return')
        expected = bytes(guest.mem_read(0x2101424, 240))
        out = (U32 * 60)()
        ok = native((U32 * 60)(*initial), (U32 * 256)(*source),
                    (U32 * len(mappings))(*mappings), len(mappings),
                    (U32 * fragment_count)(*fragments), fragment_count,
                    (I32 * 8)(*switches), (U8 * 8)(*offsets), out)
        cases += 1
        if not ok or bytes(out) != expected:
            failures.append({'case': cases, 'native_ok': ok, 'fragments': fragments,
                             'switches': switches, 'offsets': offsets, 'mappings': mappings,
                             'different_words': [i for i in range(60)
                                                 if bytes(out)[i*4:i*4+4] != expected[i*4:i*4+4]]})
    return {'retail_entry': '00017170', 'cases': cases,
            'bit_exact_matches': cases - len(failures), 'failures': failures[:10],
            'scope': 'packed constant values and fragment filtering; upload dirty masks excluded'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library',type=Path,default=ROOT/'build/libhalo2_kernels.so')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/rendering-audit/combiner-differential.json')
    args=parser.parse_args()
    # Reuse the bounded cache reader, including resource/tag allocation checks.
    spec=importlib.util.spec_from_file_location('audit',ROOT/'tools/audit_rendering.py')
    audit=importlib.util.module_from_spec(spec); spec.loader.exec_module(audit)
    image=(ROOT/'assets/disc/default.xbe').read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED:raise ValueError('unexpected retail XBE')
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    base=struct.unpack_from('<I',image,0x104)[0];count,table=struct.unpack_from('<II',image,0x11c)
    for i in range(count):
        _,address,virtual,offset,size=struct.unpack_from('<5I',image,table-base+i*56)
        guest.mem_write(address,image[offset:offset+size])
    guest.mem_map(0x2100000,0x100000);guest.mem_map(0x3000000,0x1000)
    native=ctypes.CDLL(str(args.library.resolve())).halo2_assemble_pixel_program
    U32=ctypes.c_uint32; U16=ctypes.c_uint16; I32=ctypes.c_int32
    native.argtypes=[ctypes.POINTER(U32),ctypes.POINTER(U32),U32,ctypes.POINTER(U16),U32,
                     ctypes.POINTER(U32),U32,ctypes.POINTER(I32),ctypes.POINTER(U32)]
    native.restype=ctypes.c_int
    seen=set();cases=0;implementations=0;failures=[]
    # The audit's parser API is intentionally private; inspect through its Map
    # abstraction rather than performing unbounded raw pointer reads here.
    for path in sorted((ROOT/'assets/disc/maps').glob('*.map')):
        with closing(audit.Cache(path)) as cache:
            for tag in cache.tags.values():
                if tag['group']!='spas':continue
                _,offset=cache.tag(tag['datum'],'spas')
                pp=cache.block(offset+28,88)
                if not pp:continue
                header=pp[0]
                im=cache.block(header,306)
                fr=cache.block(header+32,4); pe=cache.block(header+40,6); co=cache.block(header+48,32)
                perms=b''.join(cache.data[x:x+6] for x in pe)
                combs=b''.join(cache.data[x:x+32] for x in co)
                for impl in im:
                    definition=bytes(cache.data[impl+6:impl+246])
                    packed=cache.u16(impl+246); first,num=packed&511,packed>>9
                    fragments=b''.join(cache.data[x:x+4] for x in fr[first:first+num])
                    key=hashlib.sha256(definition+fragments+perms+combs).hexdigest()
                    if key in seen:continue
                    seen.add(key);implementations+=1
                    variants={tuple([0]*8)}
                    for f in range(num):
                        switch,r=struct.unpack_from('<hH',fragments,f*4)
                        if switch==-1:continue
                        for value in range(r>>9):
                            state=[0]*8;state[switch]=value;variants.add(tuple(state))
                    for switches in sorted(variants):
                        if num:
                            guest.mem_write(0x2100000,b'\0'*0x1800)
                            guest.mem_write(0x2100020,struct.pack('<II',0x2120000,0x2130000))
                            guest.mem_write(0x2100300,struct.pack('<8i',*switches))
                            guest.mem_write(0x2120024,struct.pack('<I',0x2140000))
                            guest.mem_write(0x212002c,struct.pack('<I',0x2150000))
                            guest.mem_write(0x2120034,struct.pack('<I',0x2160000))
                            guest.mem_write(0x21300de,definition[216:228])
                            guest.mem_write(0x21300f6,struct.pack('<H',num<<9))
                            guest.mem_write(0x2140000,fragments);guest.mem_write(0x2150000,perms);guest.mem_write(0x2160000,combs)
                            guest.mem_write(0x21fff00,struct.pack('<I',0x3000000))
                            guest.reg_write(UC_X86_REG_ESI,0x2100000);guest.reg_write(UC_X86_REG_ESP,0x21fff00)
                            guest.emu_start(0x1ba00,0x3000000,count=100000)
                            if guest.reg_read(UC_X86_REG_EIP)!=0x3000000:raise RuntimeError('retail assembler did not return')
                            expected=bytes(guest.mem_read(0x2101424,240))
                        else:expected=definition
                        out=(U32*60)()
                        ok=native((U32*60).from_buffer_copy(definition),(U32*num).from_buffer_copy(fragments),num,
                                  (U16*(len(perms)//2)).from_buffer_copy(perms),len(pe),
                                  (U32*(len(combs)//4)).from_buffer_copy(combs),len(co),(I32*8)(*switches),out)
                        cases+=1
                        if not ok or bytes(out)!=expected:
                            failures.append({'map':path.name,'pass':tag['name'],'switches':switches,'native_ok':ok,
                                             'different_words':[i for i in range(60) if bytes(out)[i*4:i*4+4]!=expected[i*4:i*4+4]]})
    remap=ctypes.CDLL(str(args.library.resolve())).halo2_remap_vertex_constants
    F32=ctypes.c_float
    remap.argtypes=[ctypes.POINTER(F32),ctypes.POINTER(F32),ctypes.POINTER(U32),U32,ctypes.POINTER(F32)]
    remap.restype=ctypes.c_int
    rng=random.Random(0x17000);remap_failures=[]
    remap_cases=[]
    for component in range(21):
        for bank in range(2):
            for register in range(16):
                remap_cases.append([(rng.randrange(256)<<24)|(component<<5)|(bank<<4)|register])
    remap_cases += [[(rng.randrange(256)<<24)|(rng.randrange(21)<<5)|(rng.randrange(2)<<4)|rng.randrange(16)
                    for _ in range(rng.randrange(1,64))] for _ in range(1000)]
    for mappings in remap_cases:
        initial=struct.pack('<128f',*[rng.uniform(-1000,1000) for _ in range(128)])
        source=struct.pack('<1024f',*[rng.uniform(-1000,1000) for _ in range(1024)])
        guest.mem_write(0x2100000,b'\0'*0x1800)
        guest.mem_write(0x210000c,struct.pack('<II',0x2120000,0x2130000))
        guest.mem_write(0x2120018,struct.pack('<I',0x2140000))
        guest.mem_write(0x2130024,struct.pack('<I',0x2150000))
        guest.mem_write(0x2101530,initial);guest.mem_write(0x2140000,source)
        guest.mem_write(0x2150000,struct.pack('<'+'I'*len(mappings),*mappings))
        guest.mem_write(0x2160000,struct.pack('<H',len(mappings)<<9))
        guest.mem_write(0x21fff00,struct.pack('<II',0x3000000,0x2160000))
        guest.reg_write(UC_X86_REG_EAX,0x2100000);guest.reg_write(UC_X86_REG_ESP,0x21fff00)
        guest.emu_start(0x17000,0x3000000,count=100000)
        if guest.reg_read(UC_X86_REG_EIP)!=0x3000000:raise RuntimeError('retail remapper did not return')
        expected=bytes(guest.mem_read(0x2101530,512));out=(F32*128)()
        ok=remap((F32*128).from_buffer_copy(initial),(F32*1024).from_buffer_copy(source),
                 (U32*len(mappings))(*mappings),len(mappings),out)
        if not ok or bytes(out)!=expected:remap_failures.append(mappings)
    pixel_remapping = compare_pixel_remapping(guest, args.library)
    report={'retail_entry':'0001ba00','xbe_sha256':EXPECTED,
            'native_library_sha256':hashlib.sha256(args.library.read_bytes()).hexdigest(),
            'unique_implementations':implementations,
            'cases':cases,'bit_exact_matches':cases-len(failures),'failures':failures[:20],
            'scope':'pixel program assembly; every retail pass and each individual fragment permutation',
            'vertex_remapping':{'retail_entry':'00017000','cases':len(remap_cases),
                                'bit_exact_matches':len(remap_cases)-len(remap_failures),'failures':remap_failures[:10]},
            'pixel_remapping':pixel_remapping,
            'frame_parity_verified':False}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2));return bool(failures or remap_failures or pixel_remapping['failures'])

if __name__=='__main__':raise SystemExit(main())
