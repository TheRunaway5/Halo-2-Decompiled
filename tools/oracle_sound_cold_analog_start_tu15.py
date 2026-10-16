#!/usr/bin/env python3
"""Compare unchanged original whole cold analog start/stop against native fixtures.

Offline physical halted/reset inputs are declared explicitly. Pure action-store
preimages are unknown in the native adapter and therefore not compared. Actual
native execution owns reset, run, consumption and stop; no kernel body executes.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_sound_sdk_native_prefix_tu15 import GP,GDT,segment


def replay(image,row,entry,authenticated):
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);pages=set()
    def mapped(p,n):
        for page in range(p&~4095,(p+n+4095)&~4095,4096):
            if page not in pages:cpu.mem_map(page,4096);pages.add(page)
    mapped(0x10000,0x600000)
    for section in image.sections:
        raw=image.section_bytes(section)
        if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
    for region in row['input_regions']:
        raw=bytes.fromhex(region['hex']);mapped(region['address'],len(raw));cpu.mem_write(region['address'],raw)
    fs=row['fs_base'];mapped(GDT,4096);mapped(0x80000000,4096);mapped(0xfec00000,4096)
    cpu.mem_write(GDT+8,segment(0,0x9b)+segment(0,0x93)+segment(fs,0x93));cpu.reg_write(R.UC_X86_REG_GDTR,(0,GDT,4095,0))
    for reg,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(reg,value)
    for reg,value in zip(GP,row['input_gp']):cpu.reg_write(reg,value)
    cpu.reg_write(R.UC_X86_REG_EFLAGS,row['input_eflags']);sp=row['input_gp'][4]
    stop=struct.unpack('<I',cpu.mem_read(sp,4))[0];mapped(stop,1)
    channel=row['input_gp'][1];physical=struct.unpack('<I',cpu.mem_read(channel+0x1c,4))[0]
    cpu.mem_write(0xfec00110,struct.pack('<I',physical));cpu.mem_write(0xfec00116,b'\1\0');cpu.mem_write(0xfec0011b,b'\0' if entry==0x38f0aa else b'\x1d')
    instructions,stores,calls,inputs=[],[],[],[]
    def code(m,pc,size,_):
        if pc in [0x800000a0,0x800000a1]:
            stack=m.reg_read(R.UC_X86_REG_ESP);returned=struct.unpack('<I',m.mem_read(stack,4))[0]
            calls.append(dict(target=pc,returned=returned))
            if pc==0x800000a0:
                assert m.reg_read(R.UC_X86_REG_ECX)&255==2
                previous=bytes(m.mem_read(fs+0x24,1))[0];m.mem_write(fs+0x24,b'\2');m.reg_write(R.UC_X86_REG_EAX,(m.reg_read(R.UC_X86_REG_EAX)&0xffffff00)|previous)
            else:m.mem_write(fs+0x24,bytes([m.reg_read(R.UC_X86_REG_ECX)&255]))
            m.reg_write(R.UC_X86_REG_ESP,stack+4);m.reg_write(R.UC_X86_REG_EIP,returned);return
        assert pc in authenticated,hex(pc);assert image.read(pc,size)==bytes(m.mem_read(pc,size));instructions.append(pc)
        if pc==0x38f00e:
            assert bytes(m.mem_read(0xfec0011b,1))==b'\2'
            m.mem_write(0xfec00110,bytes(12));inputs.append(dict(before_instruction=pc,completed_native_reset_input=True))
    def write(m,kind,p,n,v,_):
        stores.append(dict(instruction=m.reg_read(R.UC_X86_REG_EIP),address=p,before=bytes(m.mem_read(p,n)).hex(),written=(v&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
    cpu.hook_add(UC_HOOK_CODE,code);cpu.hook_add(UC_HOOK_MEM_WRITE,write);cpu.emu_start(entry,stop,count=10000)
    assert cpu.reg_read(R.UC_X86_REG_EIP)==stop
    assert instructions==row['instructions'],'Whole start/stop instruction sequence differs'
    assert len(stores)==len(row['stores']),(len(stores),len(row['stores']))
    unknown=[]
    for original,native in zip(stores,row['stores']):
        expected=dict(original)
        if native['before']=='':
            assert native['instruction'] in [0x38f0f3,0x38ede5] and native['address']==0xfec0011b
            unknown.append(dict(instruction=native['instruction'],address=native['address']));expected['before']=''
        assert expected==native,{'original':original,'native':native}
    assert [cpu.reg_read(reg) for reg in GP]==row['output_gp'],'Whole start/stop GP differs'
    assert cpu.reg_read(R.UC_X86_REG_EFLAGS)&0xcc5==row['output_eflags']&0xcc5,'Whole start/stop defined flags differ'
    compared=0
    for region in row['output_regions']:
        raw=bytes.fromhex(region['hex']);assert bytes(cpu.mem_read(region['address'],len(raw)))==raw,hex(region['address']);compared+=len(raw)
    return dict(entry=entry,instructions=len(instructions),ordered_stores=len(stores),compared_bytes=compared,
                unknown_native_action_preimages=unknown,explicit_offline_reset_inputs=inputs,kernel_interface_calls=calls)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    native=json.loads(a.native.read_text());assert native['passed'];code=_Code(image,'DSOUND')
    entries=[0x38f0aa,0x38eeb5,0x38efea,0x38edf4,0x38edc5,0x38f05c,0x383d70,0x383d92]
    authenticated=set().union(*(_trace(code,e,set(entries)).code for e in entries))
    cases=[dict(fill=row['fill'],start=replay(image,row['source_start'],0x38f0aa,authenticated),stop=replay(image,row['source_stop'],0x38f05c,authenticated)) for row in native['cases'] if row['mode']=='success'];assert len(cases)==3
    report=dict(passed=True,cases=cases,original_xbe_sha256=NEW_PIN,native_report_sha256=hashlib.sha256(a.native.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),scope=__doc__,instruction_patches=[],kernel_bodies_executed=False,source_producer_or_startup45_completed=False)
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=True,cases=cases)))


if __name__=='__main__':main()
