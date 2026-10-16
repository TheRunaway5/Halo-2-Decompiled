#!/usr/bin/env python3
"""Replay original registered codec ISR against actual native receipt fixtures.

Only the offline oracle supplies physical status40/8, after the native report
has established its descriptor-bound Success receipt. Runtime supplies no
synthetic status and executes no original machine code/kernel body.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_sound_sdk_native_prefix_tu15 import GP, GDT, segment


def replay(image, row, authenticated):
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);pages=set()
    def mapped(p,n):
        for page in range(p&~4095,(p+n+4095)&~4095,4096):
            if page not in pages:cpu.mem_map(page,4096);pages.add(page)
    mapped(0x10000,0x600000)
    for section in image.sections:
        raw=image.section_bytes(section)
        if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
    for r in row['input_regions']:
        raw=bytes.fromhex(r['hex']);mapped(r['address'],len(raw));cpu.mem_write(r['address'],raw)
    fs=row['fs_base'];mapped(GDT,4096);mapped(0x80000000,4096);mapped(0xfec00000,4096)
    cpu.mem_write(GDT+8,segment(0,0x9b)+segment(0,0x93)+segment(fs,0x93))
    cpu.reg_write(R.UC_X86_REG_GDTR,(0,GDT,4095,0))
    for reg,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(reg,value)
    for reg,value in zip(GP,row['input_gp']):cpu.reg_write(reg,value)
    cpu.reg_write(R.UC_X86_REG_EFLAGS,row['input_eflags']);sp=row['input_gp'][4]
    stop=struct.unpack('<I',cpu.mem_read(sp,4))[0];mapped(stop,1)
    cpu.mem_write(0xfec00130,struct.pack('<I',0x40));cpu.mem_write(0xfec00116,struct.pack('<H',8))
    instructions,stores,calls=[],[],[]
    def code(m,pc,size,_):
        if pc in [0x800000a0,0x800000a1]:
            stack=m.reg_read(R.UC_X86_REG_ESP);returned=struct.unpack('<I',m.mem_read(stack,4))[0]
            calls.append(dict(target=pc,returned=returned))
            if pc==0x800000a0:
                assert m.reg_read(R.UC_X86_REG_ECX)&255==2
                previous=bytes(m.mem_read(fs+0x24,1))[0];m.mem_write(fs+0x24,b'\x02')
                m.reg_write(R.UC_X86_REG_EAX,(m.reg_read(R.UC_X86_REG_EAX)&0xffffff00)|previous)
            else:m.mem_write(fs+0x24,bytes([m.reg_read(R.UC_X86_REG_ECX)&255]))
            m.reg_write(R.UC_X86_REG_ESP,stack+4);m.reg_write(R.UC_X86_REG_EIP,returned);return
        assert pc in authenticated,(hex(pc),row['fill'])
        assert image.read(pc,size)==bytes(m.mem_read(pc,size));instructions.append(pc)
    def write(m,kind,p,n,v,_):
        stores.append(dict(instruction=m.reg_read(R.UC_X86_REG_EIP),address=p,
                           before=bytes(m.mem_read(p,n)).hex(),written=(v&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
    cpu.hook_add(UC_HOOK_CODE,code);cpu.hook_add(UC_HOOK_MEM_WRITE,write)
    cpu.emu_start(0x38f378,stop,count=10000);assert cpu.reg_read(R.UC_X86_REG_EIP)==stop
    assert instructions==row['instructions'],'Original/native instruction sequence differs'
    assert stores==row['stores'],'Original/native ordered stores/preimages differ'
    assert [cpu.reg_read(r) for r in GP]==row['output_gp'],'Original/native GP differs'
    assert cpu.reg_read(R.UC_X86_REG_EFLAGS)&0xcc5==row['output_eflags']&0xcc5,'Original/native defined flags differ'
    compared=0
    for r in row['output_regions']:
        raw=bytes.fromhex(r['hex']);assert bytes(cpu.mem_read(r['address'],len(raw)))==raw,hex(r['address']);compared+=len(raw)
    return dict(fill=row['fill'],instructions=len(instructions),ordered_stores=len(stores),compared_bytes=compared,
                original_kernel_interface_calls=calls,source_channel_words=row['source_channel_words'],native_packet_frames=row['native_packet_frames'])


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    native=json.loads(a.native.read_text());assert native['passed']
    code=_Code(image,'DSOUND');entries=[0x38f378,0x38f24b,0x38f1f4,0x38edf4,0x383d70,0x383d92]
    authenticated=set().union(*(_trace(code,e,set(entries)).code for e in entries))
    cases=[replay(image,r,authenticated) for r in native['cases'] if r['mode']=='success'];assert len(cases)==3
    a.output.parent.mkdir(parents=True,exist_ok=True);report=dict(passed=True,cases=cases,original_xbe_sha256=NEW_PIN,
        native_report_sha256=hashlib.sha256(a.native.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scope=__doc__,no_instruction_patches=True,kernel_bodies_executed=False,whole_configuration_or_startup_completed=False)
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=True,cases=cases)))


if __name__=='__main__':main()
