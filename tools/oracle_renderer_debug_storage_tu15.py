#!/usr/bin/env python3
"""Original1EFD0/1383C0/1CFE0 bitmap and linear GPU texture construction.

VirtualAlloc, GetLastError and XMemAllocDefault/XMemFreeDefault are explicit
platform boundaries with actual native64 anonymous pages. Original bitmap
initialization, mip arithmetic, SDK sizing/headers and all512 pool rows run
unchanged. No source/game result is supplied. The original constructor does
not initialize texture pixels; these cases verify that fact across fills.
"""
import argparse,ctypes,hashlib,json,mmap,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP

def run(image,fill,failure,executed):
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000)
    for section in image.sections:
        raw=image.section_bytes(section)
        if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
    cpu.mem_map(0x2000000,0x20000);cpu.mem_map(0x2100000,0x1000)
    stack,stop=0x2008000,0x2100000;cpu.mem_write(stack,struct.pack('<I',stop));cpu.reg_write(UC_X86_REG_ESP,stack)
    cpu.mem_write(0x515e90,struct.pack('<I',0xf9000001));cpu.mem_write(0x4c2ac0,bytes([0xa5])*4112)
    allocations,mappings,frees,instructions,errors=[],[],[],[],[]
    def hook(machine,pc,size,_):
        if pc in (0x33b3ac,0x2dbc2e,0x2da8b0,0x2da950):
            sp=machine.reg_read(UC_X86_REG_ESP);returned=struct.unpack('<I',machine.mem_read(sp,4))[0]
            if pc==0x2dbc2e:
                errors.append(8);machine.reg_write(UC_X86_REG_EAX,8);pop=4
            elif pc==0x2da950:
                pointer,flags=struct.unpack('<2I',machine.mem_read(sp+4,8))
                if pointer!=0x3040000 or flags!=0x24800000:raise ValueError('Unexpected source header cleanup')
                frees.append(pointer);pop=12
            else:
                index=len(allocations);expected=((116,4),(20,0x64800000),(131072,0xb7800000))
                if pc==0x33b3ac:
                    base,first,kind,flags=struct.unpack('<4I',machine.mem_read(sp+4,16))
                    if base or kind!=0x101000:raise ValueError('Unexpected original bitmap virtual allocation')
                    pop=20
                else:first,flags=struct.unpack('<2I',machine.mem_read(sp+4,8));pop=12
                if index>=3 or (first,flags)!=expected[index]:raise ValueError('Unexpected original debug texture allocation size/flags')
                row=dict(index=index,bytes=first,flags=flags,source_token=0,native_backing=None,fault_injected=index==failure)
                pointer=0
                if index!=failure:
                    count=(first+4095)&~4095;pages=mmap.mmap(-1,count,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE)
                    # VirtualAlloc provides zeroed committed pages; physical
                    # texture allocation's contents remain unspecified.
                    pages[:]=bytes([0 if pc==0x33b3ac else fill])*count
                    native=ctypes.addressof(ctypes.c_char.from_buffer(pages));pointer=0x3000000+index*0x40000
                    if native<=0xffffffff:raise ValueError('Original boundary lacks native64 pages')
                    machine.mem_map_ptr(pointer,count,3,native);machine.mem_map_ptr(pointer|0x80000000,count,3,native)
                    mappings.append((pointer,count,native,pages));row.update(source_token=pointer,native_backing=native)
                allocations.append(row);machine.reg_write(UC_X86_REG_EAX,pointer)
            machine.reg_write(UC_X86_REG_ESP,sp+pop);machine.reg_write(UC_X86_REG_EIP,returned);return
        raw=bytes(machine.mem_read(pc,size)).hex()
        if bytes.fromhex(raw)!=image.read(pc,size):raise ValueError('Original debug instruction changed')
        if executed.setdefault(f'{pc:08x}',raw)!=raw:raise ValueError('Instruction changed across cases')
        instructions.append(pc)
    cpu.hook_add(UC_HOOK_CODE,hook)
    try:
        cpu.emu_start(0x1efd0,stop,count=50000)
        if cpu.reg_read(UC_X86_REG_EIP)!=stop or cpu.reg_read(UC_X86_REG_ESP)!=stack+4 or (cpu.reg_read(UC_X86_REG_EAX)&255)!=(failure is None):raise ValueError('Original debug constructor ABI differs')
        globals=bytes(cpu.mem_read(0x4c2ac0,4112));pool=struct.pack('<2I',0xffffffff,0)*512
        if globals[16:]!=pool:raise ValueError('Original pool reset differs')
        if failure is None:
            if globals[:16]!=struct.pack('<4I',1,0x10000,1,0x3000000):raise ValueError('Original debug publication differs')
        elif globals[:16]!=(bytes([0xa5])*16 if failure==0 else bytes(16)):raise ValueError('Original debug failure publication differs')
        bitmap=bytes(cpu.mem_read(0x3000000,116)) if failure!=0 else None
        if bitmap:
            expected=bytearray(116);struct.pack_into('<IHHBBHHH',expected,0,0x6269746d,256,256,1,0,0,9,0x911)
            struct.pack_into('<I',expected,0x50,0x3040000 if failure is None else 0)
            if bitmap!=expected:raise ValueError('Original bitmap definition differs')
        header=list(struct.unpack('<5I',cpu.mem_read(0x3040000,20))) if failure is None else None
        if header and header!=[0x1040001,0x3080000,0,0x11d29,0x70ff0ff]:raise ValueError(f'Original linear A4R4G4B4 header differs {header}')
        if len(allocations)!=(3 if failure is None else failure+1) or frees!=([0x3040000] if failure==2 else []) or errors!=([8] if failure==0 else []):raise ValueError('Original failure allocation/cleanup differs')
        pixels=bytes(cpu.mem_read(0x3080000,131072)) if failure is None else None
        if pixels and pixels!=bytes([fill])*131072:raise ValueError('Original constructor initialized undefined texture contents')
        for address,count,native,pages in mappings:
            if bytes(cpu.mem_read(address,count))!=ctypes.string_at(native,count):raise ValueError('Original writes lost native allocation')
        return dict(fill=fill,failed_allocation=failure,return_bool=cpu.reg_read(UC_X86_REG_EAX)&255,bitmap_hex=bitmap.hex() if bitmap else None,header=header,global_header_hex=globals[:16].hex(),pool_sha256=hashlib.sha256(globals[16:]).hexdigest(),texture_uninitialized_matches_fill=pixels is not None,allocations=allocations,frees=frees,platform_errors=errors,original_instructions=len(instructions))
    finally:
        for address,count,native,pages in mappings:cpu.mem_unmap(address,count);cpu.mem_unmap(address|0x80000000,count);pages.close()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    if args.output.exists():raise ValueError('Preserve existing evidence')
    image=Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest()!=NEW_PIN:raise ValueError('Original executable differs')
    executed={};cases=[run(image,fill,None,executed) for fill in (0,0x5a,0xa5,0xff)]+[run(image,0,i,executed) for i in range(3)]
    args.output.mkdir(parents=True);report=dict(scope=__doc__,passed=True,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=cases,executed_instruction_bytes=executed,instruction_patches=[],supplied_game_results=[],platform_boundaries=['VirtualAlloc','GetLastError','XMemAllocDefault','XMemFreeDefault'],original_allocator_bodies_executed=False,live_gpu_or_frame_parity=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'Original debug storage: {len(cases)} cases; {len(executed)} unchanged sites')
if __name__=='__main__':main()
