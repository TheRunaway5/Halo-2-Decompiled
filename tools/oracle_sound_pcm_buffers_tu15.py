#!/usr/bin/env python3
"""Execute original38EF59/EDF4 from saved genuine SDK/channel constructors.

Only default38E484 analog/nonencodedSPDIF frames are declared. Saved input
bytes come from the unchanged original complete SDK+38EEF1/38F104 proof.
No codec/start/DSP program or20xx scheduler result is supplied.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_sound_sdk_native_prefix_tu15 import GP,FS,GDT,POOL,STOP,segment
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

STACK=0x2018000
def run(image,storage,channel,executed):
    cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    for address,size in [(0x10000,0x600000),(0x2000000,0x20000),(STOP,4096),(0x80000000,4096),(FS,4096),(GDT,4096),(0xfec00000,4096)]:cpu.mem_map(address,size)
    for section in image.sections:
        raw=image.section_bytes(section)
        if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
    for row in storage['regions']:cpu.mem_write(row['address'],bytes.fromhex(row['hex']))
    cpu.mem_map(POOL,0x10000)
    for row in channel['retained_raw_hex']:cpu.mem_write(row['source'],bytes.fromhex(row['hex']))
    for rows in [channel['retained_dma_hex'],channel['physical_allocations']]:
        for row in rows:
            size=row.get('capacity',row.get('bytes'));assert size and size%4096==0
            cpu.mem_map(row['source'],size);cpu.mem_write(row['source'],bytes.fromhex(row['hex']))
    cpu.mem_write(0x2010000,bytes.fromhex(channel['stack_hex']))
    for address,key in [(0x390bc0,'raw_statistics'),(0x390bc4,'physical_statistics'),(0x390bcc,'pool_statistics')]:cpu.mem_write(address,struct.pack('<I',channel[key]))
    cpu.mem_write(GDT+8,segment(0,0x9b)+segment(0,0x93)+segment(FS,0x93));cpu.reg_write(R.UC_X86_REG_GDTR,(0,GDT,4095,0))
    for reg,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(reg,value)
    index=channel['index'];pc=0x38e4d1 if index==0 else 0x38e524;returned=pc+5
    physical=struct.unpack('<I',cpu.mem_read(0x391374,4))[0]
    assert channel['flags']==0 and not bytes(cpu.mem_read(channel['channel']+0x24,2)).strip(b'\0')
    gp=channel['returned_gp'].copy();gp[1]=channel['channel'];gp[4]=STACK
    for reg,value in zip(GP,gp):cpu.reg_write(reg,value)
    cpu.reg_write(R.UC_X86_REG_EFLAGS,channel['returned_eflags']);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
    cpu.mem_write(STACK,struct.pack('<3I',returned,physical,0x2000))
    instructions=[];stores=[];platform=[];level=0
    def word(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
    def hook(m,address,size,_):
        nonlocal level
        if address in [0x800000a0,0x800000a1]:
            sp=m.reg_read(R.UC_X86_REG_ESP);ret=word(sp)
            if address==0x800000a0:
                assert ret==0x383d8e and level==0 and m.reg_read(R.UC_X86_REG_ECX)&255==2;level=2;m.reg_write(R.UC_X86_REG_EAX,m.reg_read(R.UC_X86_REG_EAX)&0xffffff00)
            else:assert ret==0x383da3 and level==2 and m.reg_read(R.UC_X86_REG_ECX)&255==0;level=0
            m.mem_write(FS+0x24,bytes([level]));platform.append([address,ret,level]);m.reg_write(R.UC_X86_REG_ESP,sp+4);m.reg_write(R.UC_X86_REG_EIP,ret);return
        raw=bytes(m.mem_read(address,size));assert raw==image.read(address,size);assert executed.setdefault(f'{address:08x}',raw.hex())==raw.hex();instructions.append(address)
    def store(m,_,address,size,value,__):stores.append([m.reg_read(R.UC_X86_REG_EIP),address,size,value&((1<<(size*8))-1)])
    cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
    cpu.emu_start(0x38ef59,returned,count=100000);assert cpu.reg_read(R.UC_X86_REG_EIP)==returned and level==0
    table=word(channel['channel']+0x18)
    assert word(table)==physical and word(table+4)==0xc0001000 and bytes(cpu.mem_read(channel['channel']+0x24,2))==b'\1\0'
    assert any(row[0]==0x38ee67 and row[1]==0xfec00115+index*0x60 and row[3]==0 for row in stores)
    return {'index':index,'fill':channel['fill'],'channel_reference':channel,'caller_pc':pc,'caller_return':returned,'initial_gp':gp,'initial_eflags':channel['returned_eflags'],
        'buffer_physical':physical,'buffer_bytes':0x2000,'instructions':instructions,'source_stores':stores,'platform':platform,
        'returned_gp':[cpu.reg_read(reg)for reg in GP],'returned_eflags':cpu.reg_read(R.UC_X86_REG_EFLAGS),
        'stack_hex':bytes(cpu.mem_read(0x2010000,0x10000)).hex(),
        'retained_raw_hex':[dict(row,hex=bytes(cpu.mem_read(row['source'],row['bytes'])).hex())for row in channel['retained_raw_hex']],
        'retained_dma_hex':[dict(row,hex=bytes(cpu.mem_read(row['source'],row['bytes'])).hex())for row in channel['retained_dma_hex']],
        'physical_allocations':[dict(row,hex=bytes(cpu.mem_read(row['source'],row['capacity'])).hex())for row in channel['physical_allocations']],
        'source_started':False,'producer_ready':False}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--storage-reference',type=Path,required=True);p.add_argument('--channel-reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    storage=json.loads(a.storage_reference.read_text());channels=json.loads(a.channel_reference.read_text())
    assert storage['passed']and channels['passed']and storage['original_xbe_sha256']==channels['original_xbe_sha256']==NEW_PIN
    assert not storage['instruction_patches']and not channels['instruction_patches'];executed={}
    cases=[run(image,storage['cases'][0],row,executed)for row in channels['cases']if row['flags']==0]
    a.output.mkdir(parents=True);r={'schema':'H2_TU15_PCM_BUFFER_BINDING_ORACLE_V1','passed':True,'scope':__doc__,'original_xbe_sha256':NEW_PIN,
        'storage_reference_sha256':hashlib.sha256(a.storage_reference.read_bytes()).hexdigest(),'channel_reference_sha256':hashlib.sha256(a.channel_reference.read_bytes()).hexdigest(),
        'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'cases':cases,'executed_instruction_bytes':executed,'instruction_patches':[],
        'source_codec_or_start_result_supplied':False,'producer_ready':False,'initializer45_complete':False}
    (a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n');print(f'{len(cases)} genuine default PCM buffer bindings; {len(executed)} unchanged sites')
if __name__=='__main__':main()
