#!/usr/bin/env python3
"""Execute the original title fog aggregation against its unmodified cache.

This audits the fog inputs and admission state, not final framebuffer parity.
No fog initialization, cluster traversal, tag query or arithmetic is stubbed.
"""
import csv
import ctypes
import hashlib
import json
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE, EYE, DIRECTION, STACK, STOP = 0x2100000, 0x2101000, 0x2101100, 0x21fff00, 0x3000000


def main():
    image = (ROOT / 'assets/disc/default.xbe').read_bytes()
    assert hashlib.sha256(image).hexdigest() == EXPECTED
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    base = struct.unpack_from('<I', image, 0x104)[0]
    count, table = struct.unpack_from('<II', image, 0x11c)
    for i in range(count):
        _, address, _, offset, size = struct.unpack_from('<5I', image, table-base+i*56)
        guest.mem_write(address, image[offset:offset+size])
    guest.mem_map(STATE, 0x100000)
    guest.mem_map(0x2300000, 0x100000)
    guest.mem_map(STOP, 4096)
    # Original static initializer: the zero-spread sky fallback is 1, not
    # the executable's pre-initialization BSS value (which would divide by 0).
    guest.mem_write(STACK, struct.pack('<I', STOP))
    guest.reg_write(UC_X86_REG_ESP, STACK)
    guest.emu_start(0x377110, STOP, count=100)
    cache = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
    mapped = cache.magic & ~4095
    size = (cache.magic+len(cache.data)-mapped+4095) & ~4095
    guest.mem_map(mapped, size)
    guest.mem_write(cache.magic, bytes(cache.data))
    _, scenario = cache.tag(cache.scenario, 'scnr')
    bsp = cache.block(scenario+0x210, 68)[0]
    file, size, address = struct.unpack_from('<3I', cache.data, bsp)
    guest.mem_write(address, bytes(cache.data[file:file+size]))
    guest.mem_write(0x4e0350, struct.pack('<I', cache.magic+scenario))
    guest.mem_write(0x4e0348, struct.pack('<I', address+16))
    collision=cache.block(file+16+0x14,64,address-file)[0]
    guest.mem_write(0x4e3b44, struct.pack('<I', 0x2300000))
    for tag in cache.tags.values():
        guest.mem_write(0x2300000+(tag['datum'] & 65535)*16+8, struct.pack('<I', tag['address']))
    clusters = cache.block(file+16+156, 176, address-file)
    print('cluster sky/fog selectors:', sorted(set(cache.data[p+0x6c:p+0x72].hex() for p in clusters)))
    samples = []
    native=ctypes.CDLL(str(ROOT/'build-frontend/libhalo2_kernels.so')).halo2_menu_fog_externs
    native.argtypes=[ctypes.POINTER(ctypes.c_float)]*4;native.restype=ctypes.c_int
    _,sky_root=cache.tag(next(t['datum']for t in cache.tags.values()if t['group']=='sky 'and t['name'].endswith('main_menu')),'sky ')
    primary=struct.unpack_from('<6f',cache.data,cache.block(sky_root+0x48,24)[0])
    plane=cache.block(file+16+0x64,24,address-file)[0]
    plane_values=struct.unpack_from('<4f',cache.data,plane+4)
    palette=cache.block(scenario+0x348,16)[cache.u16(plane)]
    fog_datum=cache.u32(palette+8);_,fog=cache.tag(fog_datum,'fog ')
    fog_values=struct.unpack_from('<9f',cache.data,fog+12)
    parameters=(*primary,*plane_values,*fog_values[6:9],*fog_values[:3],fog_values[5])
    failures=[];admissions=[];extern_cases=0
    aggregate_differences=[];actual_clusters=[]
    _,pass_root=cache.tag(0xe1b90044,'spas')
    postprocess=cache.block(pass_root+28,88)[0]
    implementation=cache.block(postprocess,306)[0]
    initial=struct.unpack_from('<60I',cache.data,implementation+6)
    packed=cache.u16(implementation+0x122)
    mappings=cache.block(postprocess+56,4)[packed&511:(packed&511)+(packed>>9)]
    mapping_words=[cache.u32(p)for p in mappings]
    program_native=ctypes.CDLL(str(ROOT/'build-frontend/libhalo2_kernels.so')).halo2_menu_planar_fog_program
    F,U=ctypes.c_float,ctypes.c_uint32
    program_native.argtypes=[ctypes.POINTER(F),ctypes.POINTER(F),ctypes.POINTER(U),ctypes.POINTER(U),U,ctypes.POINTER(U)]
    program_native.restype=ctypes.c_int
    def invoke(entry,args=(),registers=()):
        guest.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        guest.reg_write(UC_X86_REG_ESP,STACK)
        guest.reg_write(UC_X86_REG_MXCSR,0x1f80);guest.reg_write(UC_X86_REG_FPCW,0x027f)
        for register,value in registers:guest.reg_write(register,value)
        guest.emu_start(entry,STOP,count=1000000)
        assert guest.reg_read(UC_X86_REG_EIP)==STOP,hex(guest.reg_read(UC_X86_REG_EIP))
    with (ROOT / 'analysis/frontend/native-camera-trace.csv').open() as trace:
        rows=[row for row in csv.DictReader(trace)if int(row['tick'])in(1,200,350,800,1100,2100)]
        for index,height in enumerate((210,215.82756,221.82756,225,240)):
            rows.append({**rows[0],'tick':str(-1-index),'z':str(height)})
        for row in rows:
            eye = [float(row[k]) for k in ('x', 'y', 'z')]
            direction = [float(row[k]) for k in ('forward_x', 'forward_y', 'forward_z')]
            guest.mem_write(EYE, struct.pack('<3f', *eye))
            guest.mem_write(DIRECTION, struct.pack('<3f', *direction))
            invoke(0x14a280,registers=[(UC_X86_REG_ECX,address+collision-file),(UC_X86_REG_EDX,EYE),(UC_X86_REG_EAX,0)])
            leaf=guest.reg_read(UC_X86_REG_EAX)
            leaves=cache.block(file+16+0x2c,8,address-file)
            actual_cluster=-1 if leaf==0xffffffff else struct.unpack_from('<h',cache.data,leaves[leaf])[0]
            actual_clusters.append({'tick':int(row['tick']),'leaf':leaf,'cluster':actual_cluster})
            baseline=None;actual_state=None
            for cluster in range(len(clusters)):
                invoke(0x12e5e0,(EYE,DIRECTION,0,1),[(UC_X86_REG_EAX,STATE),(UC_X86_REG_ECX,cluster)])
                state=bytes(guest.mem_read(STATE,288))
                # Aggregate metadata can differ; compare the actual fog banks
                # and admission fields rather than unrelated cluster indices.
                fields=state[0x1c:0x4c]+state[0x70:0x9c]+state[0xa0:0xc8]+state[0xf4:0x120]
                if baseline is None:baseline=fields
                if fields!=baseline:
                    aggregate_differences.append({'tick':int(row['tick']),'cluster':cluster,
                        'different_offsets':[i for i in range(288)if state[i]!=baseline_state[i]],
                        'primary':struct.unpack_from('<6f',state,0x1c),'patchy':struct.unpack_from('<10f',state,0x70)})
                else:baseline_state=state
                if cluster==actual_cluster:actual_state=state
                guest.mem_write(0x48574c,state);guest.mem_write(0x4b9ef0,state)
                scalars=(ctypes.c_float*43)();colors=(ctypes.c_float*168)()
                assert native((ctypes.c_float*17)(*parameters),(ctypes.c_float*3)(*eye),scalars,colors)
                for index in range(24,37):
                    invoke(0x336f0,registers=[(UC_X86_REG_EAX,index)])
                    actual=struct.pack('<I',guest.reg_read(UC_X86_REG_XMM0)&0xffffffff)
                    if actual!=bytes(scalars)[index*4:index*4+4]:failures.append((row['tick'],cluster,'scalar',index))
                for index in range(25,33):
                    invoke(0x33980,registers=[(UC_X86_REG_EAX,index),(UC_X86_REG_ESI,0x2102000)])
                    if bytes(guest.mem_read(0x2102000,12))!=bytes(colors)[index*16:index*16+12]:
                        failures.append((row['tick'],cluster,'color',index))
                # Bind the actual implementation through original 19CA0.
                guest.mem_write(0x2140000,bytes(0x1600))
                guest.mem_write(0x2140020,struct.pack('<II',0x2150000,0x2160000))
                guest.mem_write(0x2150024,struct.pack('<I',0x2170000))
                guest.mem_write(0x215003c,struct.pack('<I',0x2180000))
                guest.mem_write(0x2180000,struct.pack('<'+'I'*len(mapping_words),*mapping_words))
                guest.mem_write(0x2190000,struct.pack('<H',len(mapping_words)<<9))
                for index in range(24,37):
                    invoke(0x336f0,registers=[(UC_X86_REG_EAX,index)])
                    guest.mem_write(0x214005c+index*4,struct.pack('<I',guest.reg_read(UC_X86_REG_XMM0)&0xffffffff))
                for index in range(25,33):
                    invoke(0x33980,registers=[(UC_X86_REG_EAX,index),(UC_X86_REG_ESI,0x2140108+index*12)])
                guest.mem_write(0x2141424,struct.pack('<60I',*initial))
                invoke(0x19ca0,(0x2190000,),[(UC_X86_REG_ESI,0x2140000)])
                output=(U*60)()
                assert program_native((F*17)(*parameters),(F*3)(*eye),(U*60)(*initial),
                    (U*len(mapping_words))(*mapping_words),len(mapping_words),output)
                if bytes(output)!=bytes(guest.mem_read(0x2141424,240)):
                    failures.append((row['tick'],cluster,'pixel_program'))
                extern_cases+=1
                # 134390 sets visibility flag 400 using this exact palette
                # query. 1332F0 then admits layer 16 without flag 100.
                invoke(0x16e210,registers=[(UC_X86_REG_ECX,cluster),(UC_X86_REG_EDI,fog_datum)])
                assert guest.reg_read(UC_X86_REG_EAX)&255==1,cluster
                invoke(0x1332f0,(0x2103000,),[(UC_X86_REG_EDX,0xf400)])
                flags=guest.reg_read(UC_X86_REG_EAX)
                classification=struct.unpack_from('<I',state)[0]
                if classification==3:
                    assert flags&0x80 and not(flags&0x100) and not(flags&0x4000),hex(flags)
                else:assert not(flags&0x80),hex(flags)
                assert state[0x114]==0
                admissions.append({'tick':int(row['tick']),'cluster':cluster,'render_flags':flags,'implementation':0 if classification==3 else -1})
            if actual_state is None:
                invoke(0x12e5e0,(EYE,DIRECTION,0,1),[(UC_X86_REG_EAX,STATE),(UC_X86_REG_ECX,0xffffffff)])
                actual_state=bytes(guest.mem_read(STATE,288))
            state=actual_state
            sample = {'tick': int(row['tick']), 'cluster':actual_cluster,'classification': struct.unpack_from('<I', state)[0],
                      'primary': struct.unpack_from('<6f', state, 0x1c),
                      'secondary': struct.unpack_from('<6f', state, 0x34),
                      'patchy': struct.unpack_from('<10f', state, 0x70),
                      'planar': struct.unpack_from('<9f', state, 0xa0),
                      'camera_state': struct.unpack_from('<3f', state, 0x108),
                      'plane_only': state[0x114],
                      'transition': struct.unpack_from('<f', state, 0x118)[0],
                      'interpolation': struct.unpack_from('<f', state, 0x11c)[0]}
            samples.append(sample)
            print(json.dumps(sample))
    report = {'xbe_sha256': EXPECTED, 'cache_sha256': hashlib.sha256(cache.data).hexdigest(),
              'entries': ['0012e5e0','0016e210','001332f0','000336f0','00033980'], 'initializers': ['00377110'],
              'clusters':len(clusters),'extern_cases':extern_cases,'extern_failures':failures[:10],
              'camera_clusters':actual_clusters,'aggregate_differences_for_other_cluster_inputs':aggregate_differences,
              'stubbed_functions': [],'framebuffer_parity_verified':False,'samples':samples,'admissions':admissions}
    (ROOT / 'analysis/frontend/aggregated-menu-fog-state.json').write_text(json.dumps(report, indent=2)+'\n')
    assert not failures,failures[:10]


if __name__ == '__main__':
    main()
