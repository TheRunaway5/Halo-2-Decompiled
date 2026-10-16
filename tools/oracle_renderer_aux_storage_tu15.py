#!/usr/bin/env python3
"""Execute unchanged Xbox TU1.5 1D0C0 fallback texture initialization.

Only XMemAllocDefault/XMemFreeDefault are platform adapters. Each allocation
has actual native64 anonymous pages, including low and KSEG aliases used by the
SDK. The original SDK texture sizing, data lookup, fresh-resource wait and
all game texel stores execute unchanged. No texture result is supplied.
This is original CPU construction evidence, not an Xbox GPU frame capture.
"""
import argparse
import ctypes
import hashlib
import json
import mmap
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP


def run(image,fill,failure,executed):
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000)
    for section in image.sections:
        raw=image.section_bytes(section)
        if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
    cpu.mem_map(0x2000000,0x20000);cpu.mem_map(0x2100000,0x1000)
    stack,stop=0x2008000,0x2100000
    cpu.mem_write(stack,struct.pack('<I',stop));cpu.reg_write(UC_X86_REG_ESP,stack)
    # Original cold resource headers are not submitted, so BlockOnResource
    # reads their real zero lock and never polls the device. A nonzero SDK
    # context pointer exercises that branch without inventing device results.
    cpu.mem_write(0x411568,struct.pack('<I',0x2000000))
    allocations,mappings,frees,instructions,stores=[],[],[],[],[]
    def word(address):return struct.unpack('<I',cpu.mem_read(address,4))[0]
    def hook(machine,pc,size,_):
        if pc in (0x2da8b0,0x2da950):
            sp=machine.reg_read(UC_X86_REG_ESP)
            returned,first,flags=struct.unpack('<3I',machine.mem_read(sp,12))
            if pc==0x2da950:
                if flags!=0x24800000 or not any(row['source_token']==first for row in allocations):raise ValueError('Unowned header free')
                frees.append(first)
            else:
                index=len(allocations)
                expected=((20,0x64800000),(32,0xb7800000),(20,0x64800000),(128,0xb7800000),(20,0x64800000),(768,0xb7800000))
                if index>=len(expected) or (first,flags)!=expected[index]:raise ValueError('Unexpected texture allocation size/flags')
                row=dict(index=index,bytes=first,flags=flags,fault_injected=index==failure,source_token=0,native_backing=None)
                if index==failure:pointer=0
                else:
                    count=(first+4095)&~4095
                    pages=mmap.mmap(-1,count,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE)
                    pages[:]=bytes([fill])*count
                    native=ctypes.addressof(ctypes.c_char.from_buffer(pages))
                    if native<=0xffffffff:raise ValueError('Allocation lacks native64 pages')
                    pointer=0x3000000+index*0x10000
                    machine.mem_map_ptr(pointer,count,3,native)
                    machine.mem_map_ptr(pointer|0x80000000,count,3,native)
                    mappings.append((pointer,count,native,pages));row.update(source_token=pointer,native_backing=native)
                allocations.append(row);machine.reg_write(UC_X86_REG_EAX,pointer)
            machine.reg_write(UC_X86_REG_ESP,sp+12);machine.reg_write(UC_X86_REG_EIP,returned);return
        raw=bytes(machine.mem_read(pc,size)).hex()
        if bytes.fromhex(raw)!=image.read(pc,size):raise ValueError('Original instruction changed')
        if executed.setdefault(f'{pc:08x}',raw)!=raw:raise ValueError('Instruction identity changed across cases')
        instructions.append(pc)
    def write(machine,access,address,size,value,_):
        if address>=0x83000000:stores.append(dict(instruction=machine.reg_read(UC_X86_REG_EIP),address=address,size=size,value=value))
    cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,write)
    try:
        cpu.emu_start(0x1d0c0,stop,count=50000)
        if cpu.reg_read(UC_X86_REG_EIP)!=stop or cpu.reg_read(UC_X86_REG_ESP)!=stack+4:raise ValueError('Original fallback constructor ABI differs')
        returned=cpu.reg_read(UC_X86_REG_EAX)&255
        if returned!=(failure is None):raise ValueError('Original allocation failure/success differs')
        cells=struct.unpack('<3I',cpu.mem_read(0x492304,12))
        headers={f'{0x492304+i*4:08x}':list(struct.unpack('<5I',cpu.mem_read(cell,20))) for i,cell in enumerate(cells) if cell}
        for address,count,native,pages in mappings:
            if bytes(cpu.mem_read(address,count))!=ctypes.string_at(native,count) or bytes(cpu.mem_read(address|0x80000000,count))!=ctypes.string_at(native,count):raise ValueError('Original alias writes did not reach native pages')
        data={str(row['index']):bytes(cpu.mem_read(row['source_token'],row['bytes'])).hex() for row in allocations if row['flags']==0xb7800000 and row['source_token']}
        expected_formats=(0x2210429,0x22210439,0x221042d)
        for i,cell in enumerate(cells):
            present=failure is None or failure>=i*2+2
            if bool(cell)!=present:raise ValueError('Original failure publication prefix differs')
            if cell and headers[f'{0x492304+i*4:08x}']!=[0x1040001,0x3010000+i*0x20000,0,expected_formats[i],0]:raise ValueError('Original texture header/layout differs')
        if failure is not None:
            if len(allocations)!=failure+1 or stores:raise ValueError('Original allocation failure performed texel stores or continued allocating')
            if frees!=([0x3000000+(failure-1)*0x10000] if failure&1 else []):raise ValueError('Original failure cleanup differs')
            if any(bytes.fromhex(value)!=bytes([fill])*len(bytes.fromhex(value)) for value in data.values()):raise ValueError('Original early return initialized texels')
            rgba=[]
        else:
            if len(stores)!=176 or frees:raise ValueError('Original successful constructor store count/cleanup differs')
            expected_words=struct.pack('<2H',0x0f00,0xf0f0)
            if bytes.fromhex(data['1'])!=expected_words*8 or bytes.fromhex(data['3'])!=expected_words*32:raise ValueError('Original 2D/volume texel words differ')
            cube=bytes.fromhex(data['5'])
            for face in range(6):
                if cube[face*128:face*128+32]!=expected_words*8 or cube[face*128+32:(face+1)*128]!=bytes([fill])*96:raise ValueError('Original cube face pitch/texel words/padding differs')
            # Independent dimension-mask Morton decode. The low address bit
            # is X0 for both original2D and volume swizzles. Preserve every
            # original alpha nibble; do not turn transparent red opaque.
            def decode(raw,depth):
                decoded=[]
                for z in range(depth):
                    for y in range(4):
                        for x in range(4):
                            address=0;destination=1
                            for bit in (1,2):
                                for coordinate,extent in ((x,4),(y,4),(z,depth)):
                                    if bit<extent:
                                        if coordinate&bit:address|=destination
                                        destination<<=1
                            p=struct.unpack_from('<H',raw,address*2)[0]
                            value=[((p>>8)&15)*17,((p>>4)&15)*17,(p&15)*17,(p>>12)*17]
                            if value!=([0,255,0,255] if x&1 else [255,0,0,0]):raise ValueError('Original Morton/color decode differs')
                            decoded+=value
                return bytes(decoded).hex()
            rgba=[decode(bytes.fromhex(data['1']),1),decode(bytes.fromhex(data['3']),4),''.join(decode(cube[face*128:face*128+32],1) for face in range(6))]
        return dict(fill=fill,failed_allocation=failure,return_bool=returned,publications=list(cells),headers=headers,allocations=allocations,frees=frees,data_hex=data,linear_rgba_hex=rgba,texel_stores=stores,original_instructions=len(instructions))
    finally:
        for address,count,native,pages in mappings:
            cpu.mem_unmap(address,count);cpu.mem_unmap(address|0x80000000,count);pages.close()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    if args.output.exists():raise ValueError('Preserve existing evidence; select a new output')
    image=Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest()!=NEW_PIN:raise ValueError('Original executable differs')
    executed={};cases=[run(image,fill,None,executed) for fill in (0,0x5a,0xa5,0xff)]+[run(image,0,i,executed) for i in range(6)]
    args.output.mkdir(parents=True)
    report=dict(scope=__doc__,passed=True,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),engine_format9_d3d=struct.unpack('<I',image.read(0x45c488+9*4,4))[0],cases=cases,executed_instruction_bytes=executed,instruction_patches=[],supplied_game_results=[],allocation_platform_boundaries=['XMemAllocDefault','XMemFreeDefault'],original_allocator_body_executed=False,live_gpu_or_frame_parity=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'Original auxiliary textures: {len(cases)} cases; {len(executed)} unchanged sites')
if __name__=='__main__':main()
