#!/usr/bin/env python3
"""Bounded original channel constructor after authenticated SDK storage.

Starts38EEF1 from an explicitly declared caller, then38F104, stopping before
the first38EFEA channel-reset MMIO store38F007. Codec/init/interrupt setup from
38F45F is not executed or supplied. Existing source objects/descriptors come
from the already-authenticated complete SDK storage prefix, never preset bytes.
"""
import argparse,ctypes,hashlib,json,mmap,struct,threading
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_sound_sdk_native_prefix_tu15 import GP,FS,GDT,POOL,STOP,segment,pool_layout,REGIONS
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

STACK=0x2018000
def run(image,storage,fill,index,flags,executed):
    cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    for address,size in [(0x10000,0x600000),(0x2000000,0x20000),(STOP,4096),(0x80000000,4096),(FS,4096),(GDT,4096)]:cpu.mem_map(address,size)
    for section in image.sections:
        raw=image.section_bytes(section)
        if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
    for row in storage['regions']:cpu.mem_write(row['address'],bytes.fromhex(row['hex']))
    cpu.mem_write(0x2010000,bytes([fill])*0x10000)
    cpu.mem_write(GDT+8,segment(0,0x9b)+segment(0,0x93)+segment(FS,0x93));cpu.reg_write(R.UC_X86_REG_GDTR,(0,GDT,4095,0))
    for reg,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(reg,value)
    mappings=[];raw=mmap.mmap(-1,0x10000,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE)
    native=ctypes.addressof(ctypes.c_char.from_buffer(raw));assert native>0xffffffff;cpu.mem_map_ptr(POOL,0x10000,3,native);mappings.append((POOL,0x10000,raw))
    for row in storage['allocations']:cpu.mem_write(row['source'],bytes.fromhex(row['hex']))
    for row in storage['dma_allocations']:
        page=mmap.mmap(-1,row['capacity'],flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE)
        page[:]=bytes.fromhex(row['hex']);pointer=ctypes.addressof(ctypes.c_char.from_buffer(page));assert pointer>0xffffffff
        cpu.mem_map_ptr(row['source'],row['capacity'],3,pointer);mappings.append((row['source'],row['capacity'],page))
    cursor=max(r['source']+r['bytes']for r in storage['allocations']);controller=storage['frontier']['source_subobject']+0x14
    assert bytes(cpu.mem_read(controller,8))==bytes(8)
    gp=[0x21000000+i*0x111111 for i in range(8)];gp[1]=controller;gp[4]=STACK
    for reg,value in zip(GP,gp):cpu.reg_write(reg,value)
    for reg,value in [(R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)]:cpu.reg_write(reg,value)
    lock=threading.RLock();lock.acquire();depth=1;exclusion=threading.RLock();level=0
    instructions=[];stores=[];platform=[];physical=[];raw_allocations=[]
    def word(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
    def hook(m,pc,size,_):
        nonlocal cursor,depth,level
        sp=m.reg_read(R.UC_X86_REG_ESP);returned=word(sp)
        if pc in [0x800000a0,0x800000a1]:
            cl=m.reg_read(R.UC_X86_REG_ECX)&255
            if pc==0x800000a0:
                assert returned==0x383d8e and cl==2 and level==0;exclusion.acquire();level=2;m.reg_write(R.UC_X86_REG_EAX,m.reg_read(R.UC_X86_REG_EAX)&0xffffff00)
            else:assert returned==0x383da3 and cl==0 and level==2;level=0;exclusion.release()
            m.mem_write(FS+0x24,bytes([level]));pop=4;value=None
        elif pc in [0x80000115,0x80000126,0x8000000f,0x80000017]:
            if pc==0x80000115:
                assert returned==0x383f98 and word(sp+4)==0x390bf8;lock.acquire();depth+=1;pop=8;value=None
            elif pc==0x80000126:
                assert returned in [0x3878b6,0x3842d0,0x384317]and word(sp+4)==0x390bf8 and depth;lock.release();depth-=1;pop=8;value=None
            elif pc==0x8000000f:
                request,tag=word(sp+4),word(sp+8);assert returned==0x38797b and request==0x30 and tag==0x626f5344
                extent,offset,capacity,alignment=pool_layout(request);base=(cursor+alignment-1)&~(alignment-1);assert base+extent<=POOL+0x10000
                m.mem_write(base,bytes(extent));m.mem_write(base+3,bytes([extent>>5]));value=base+offset;cursor=base+extent;pop=12
                raw_allocations.append(dict(source=base,bytes=extent,pointer=value,requested=request,capacity=capacity))
            else:
                assert returned==0x387988;row=next(r for r in raw_allocations if r['pointer']==word(sp+4));value=row['capacity'];pop=8
        elif pc in [0x800000a6,0x800000b4,0x800000ad]:
            if pc==0x800000a6:
                request,lowest,highest,alignment,protection=[word(sp+i*4)for i in range(1,6)];parent=word(sp+28)
                assert returned==0x3879e2 and parent in [0x38f13f,0x38f17a]and request==(0x100 if parent==0x38f13f else 0x2000)and not lowest and highest==0xffffffff and alignment==0x1000 and protection==4
                capacity=(request+4095)&~4095;address=0x06000000+sum(r['capacity']for r in physical);source=address|0x80000000
                page=mmap.mmap(-1,capacity,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,prot=mmap.PROT_READ|mmap.PROT_WRITE);page[:]=bytes([0xa5])*capacity
                pointer=ctypes.addressof(ctypes.c_char.from_buffer(page));assert pointer>0xffffffff;m.mem_map_ptr(source,capacity,3,pointer);mappings.append((source,capacity,page))
                physical.append(dict(source=source,physical=address,requested=request,capacity=capacity,channel=m.reg_read(R.UC_X86_REG_ESI),index=index,field=0x18 if parent==0x38f13f else 0x20));value=source;pop=24
            else:
                row=next(r for r in physical if r['source']==word(sp+4));pop=8
                if pc==0x800000b4:assert returned==0x3879ef and word(sp+12)in [0x38f13f,0x38f17a];value=row['capacity']
                else:assert returned==0x38f15e;value=row['physical']
        else:
            assert image.read(pc,size)==bytes(m.mem_read(pc,size));assert executed.setdefault(f'{pc:08x}',image.read(pc,size).hex())==image.read(pc,size).hex();instructions.append(pc);return
        platform.append(dict(target=pc,returned=returned,result=value,stack_cleanup=pop,depth=depth,level=level))
        if value is not None:m.reg_write(R.UC_X86_REG_EAX,value)
        m.reg_write(R.UC_X86_REG_ESP,sp+pop);m.reg_write(R.UC_X86_REG_EIP,returned)
    def store(m,kind,address,size,value,_):stores.append([m.reg_read(R.UC_X86_REG_EIP),address,size,value&((1<<(8*size))-1)])
    cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
    try:
        cpu.mem_write(STACK,struct.pack('<3I',STOP,index,0));cpu.emu_start(0x38eef1,STOP,count=10000);assert cpu.reg_read(R.UC_X86_REG_EIP)==STOP and not level and depth==1
        channel=word(controller+index*4);assert not raw_allocations and word(channel)==index and word(channel+8)==controller
        # Genuine387A32 now allocates from the previously constructed shared
        # small-block pool. Its split/list/refcount CPU graph is retained.
        assert any(r['source']<=channel and channel+0x30<=r['source']+r['capacity']for r in storage['dma_allocations'])
        constructor=dict(instructions=instructions.copy(),source_stores=stores.copy(),returned_gp=[cpu.reg_read(r)for r in GP],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS))
        instructions.clear();stores.clear();cpu.mem_write(STACK,struct.pack('<4I',STOP,flags,0,0));cpu.reg_write(R.UC_X86_REG_ECX,channel);cpu.reg_write(R.UC_X86_REG_ESP,STACK)
        input_gp=[cpu.reg_read(r)for r in GP];input_flags=cpu.reg_read(R.UC_X86_REG_EFLAGS)
        cpu.emu_start(0x38f104,0x38f007,count=100000);assert cpu.reg_read(R.UC_X86_REG_EIP)==0x38f007 and level==2 and depth==1
        assert word(cpu.reg_read(R.UC_X86_REG_ESP)+16)==0x38f195
        assert len(physical)==(2 if flags&1 else 1)and word(channel+0x18)==physical[0]['source']and word(channel+0x1c)==physical[0]['physical']
        for row in physical:assert bytes(cpu.mem_read(row['source'],row['capacity']))==bytes(row['capacity'])
        return dict(fill=fill,index=index,flags=flags,initial_gp=gp,constructor=constructor,channel=channel,controller=controller,allocation_input_gp=input_gp,allocation_input_eflags=input_flags,instructions=instructions,source_stores=stores,returned_gp=[cpu.reg_read(r)for r in GP],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),physical_allocations=[dict(r,hex=bytes(cpu.mem_read(r['source'],r['capacity'])).hex())for r in physical],raw_allocations=[dict(r,hex=bytes(cpu.mem_read(r['source'],r['bytes'])).hex())for r in raw_allocations],retained_raw_hex=[dict(source=r['source'],bytes=r['bytes'],hex=bytes(cpu.mem_read(r['source'],r['bytes'])).hex())for r in storage['allocations']],retained_dma_hex=[dict(source=r['source'],bytes=r['capacity'],hex=bytes(cpu.mem_read(r['source'],r['capacity'])).hex())for r in storage['dma_allocations']],platform=platform,stack_hex=bytes(cpu.mem_read(0x2010000,0x10000)).hex(),controller_hex=bytes(cpu.mem_read(controller,8)).hex(),raw_statistics=word(0x390bc0),physical_statistics=word(0x390bc4),pool_statistics=word(0x390bcc),fs_level=level,first_unsupported_instruction=0x38f007,first_unsupported_address=0xfec0011b if index==0 else 0xfec0017b,channel_constructor_returned=False)
    finally:
        if level:exclusion.release()
        while depth:lock.release();depth-=1
        for source,size,page in reversed(mappings):cpu.mem_unmap(source,size);page.close()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--storage-reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    assert image.read(0x390b94,8)==struct.pack('<2I',0x10,0x70)and image.read(0x4212f8,4)==struct.pack('<I',0x383dec)
    storage=json.loads(a.storage_reference.read_text());assert storage['passed']and storage['original_xbe_sha256']==NEW_PIN and not storage['instruction_patches']and not storage['supplied_game_or_sdk_results']
    baseline=storage['cases'][0];assert baseline['through_storage']and len(baseline['dma_allocations'])==16
    executed={};cases=[run(image,baseline,fill,index,flags,executed)for fill,index,flags in [(0,0,0),(0x5a,1,0),(0xa5,0,1),(0xa5,1,1)]]
    a.output.mkdir(parents=True);report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,storage_reference_sha256=hashlib.sha256(a.storage_reference.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=cases,executed_instruction_bytes=executed,instruction_patches=[],supplied_game_or_codec_results=[],whole38f45f_executed=False,channel_reset_store_applied=False,channel_ready=False,producer_ready=False,full_startup_completed=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'{len(cases)} bounded channel constructors; {len(executed)} unchanged sites; stopped before38F007 reset command')
if __name__=='__main__':main()
