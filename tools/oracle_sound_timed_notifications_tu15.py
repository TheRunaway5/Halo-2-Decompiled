#!/usr/bin/env python3
"""Compare unchanged TU1.5 timed CPU bodies with actual native clock/timer inputs.

The native artifact records real clock reads, timerfd outcomes and real queued
callback/cancellation receipts. Only those native platform inputs are replayed
offline; production never receives replayed clocks, timer outcomes or callbacks.
The bounded initial source bytes include genuine earlier constructors and the
declared inactive voice admission. This is not complete389378 configuration.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

def run(image, operation, executed):
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set()
    def map_bytes(address, data):
        for page in range(address&~4095,(address+len(data)+4095)&~4095,4096):
            if page not in mapped:cpu.mem_map(page,4096);mapped.add(page)
        cpu.mem_write(address,data)
    for section in image.sections:
        data=image.section_bytes(section)
        if section.va+len(data)<=0x610000:map_bytes(section.va,data)
    for row in operation['before']:
        data=bytes.fromhex(row['hex']);address=row['address']
        if row['readonly'] and address<0x610000:
            assert data==image.read(address,len(data)),f'Unpinned immutable input{address:08X}'
        map_bytes(address,data)
    map_bytes(0x80000000,bytes(4096));map_bytes(0x2100000,bytes(4096));map_bytes(0x2200000,bytes(4096))
    fs=operation['fs']
    def segment(base,access):return struct.pack('<Q',(base&65535)<<16|(base&0xff0000)<<16|(base&0xff000000)<<32|65535|0xf<<48|access<<40|0xc<<52)
    cpu.mem_write(0x2200008,segment(0,0x9b)+segment(0,0x93)+segment(fs,0x93));cpu.reg_write(R.UC_X86_REG_GDTR,(0,0x2200000,4095,0))
    for reg,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(reg,value)
    regs=[getattr(R,'UC_X86_REG_'+n) for n in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']]
    for reg,value in zip(regs,operation['input_gp']):cpu.reg_write(reg,value)
    cpu.reg_write(R.UC_X86_REG_EFLAGS,operation['input_flags']);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
    instructions=[];stores=[];platform=iter(operation['platform']);level=0;events=[]
    def word(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
    def hook(m,pc,n,_):
        nonlocal level
        if pc in [0x80000080,0x80000095,0x80000061,0x80000089,0x800000a0,0x800000a1]:
            sp=m.reg_read(R.UC_X86_REG_ESP);ret=word(sp);eax=m.reg_read(R.UC_X86_REG_EAX);pop=4
            if pc in [0x800000a0,0x800000a1]:
                if pc==0x800000a0:
                    assert ret==0x383d8e and level==0 and m.reg_read(R.UC_X86_REG_ECX)&255==2;level=2;m.reg_write(R.UC_X86_REG_EAX,eax&0xffffff00)
                else:assert ret==0x383da3 and level==2 and m.reg_read(R.UC_X86_REG_ECX)&255==0;level=0
                m.mem_write(fs+0x24,bytes([level]))
            else:
                row=next(platform);assert row['target']==pc and row['instruction']+6==ret
                if pc==0x80000080:
                    assert not row['has_boolean_result'];m.mem_write(word(sp+4),struct.pack('<Q',row['system_time']));pop=8
                else:
                    assert row['has_boolean_result'];m.reg_write(R.UC_X86_REG_EAX,(eax&0xffffff00)|int(row['boolean_result']))
                    pop=20 if pc==0x80000095 else 8
                    if pc==0x80000095:
                        assert struct.unpack('<Q',m.mem_read(sp+8,8))[0]==row['system_time']
                events.append(row)
            m.reg_write(R.UC_X86_REG_ESP,sp+pop);m.reg_write(R.UC_X86_REG_EIP,ret);return
        data=bytes(m.mem_read(pc,n));assert data==image.read(pc,n),f'Changed original instruction{pc:08X}'
        assert executed.setdefault(f'{pc:08x}',data.hex())==data.hex();instructions.append(pc)
    def write(m,access,address,n,value,_):stores.append([m.reg_read(R.UC_X86_REG_EIP),address,n,value&((1<<(n*8))-1)])
    cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,write)
    cpu.emu_start(operation['entry'],0x2100000,count=100000)
    assert cpu.reg_read(R.UC_X86_REG_EIP)==0x2100000 and level==0
    assert next(platform,None) is None
    assert instructions==operation['instructions'],'Unchanged original instruction path differs'
    assert stores==operation['stores'],'Unchanged original ordered stores differ'
    gp=[cpu.reg_read(r)&0xffffffff for r in regs];flags=cpu.reg_read(R.UC_X86_REG_EFLAGS)
    assert gp==operation['gp'],'Unchanged original returned GP differs'
    assert flags&0xcc5==operation['flags']&0xcc5,'Unchanged original defined flags differ'
    for row in operation['after']:
        assert bytes(cpu.mem_read(row['address'],len(row['hex'])//2)).hex()==row['hex'],f'Original complete source extent differs{row["address"]:08X}'
    return dict(entry=operation['entry'],instructions=len(instructions),stores=len(stores),platform_inputs=events,passed=True)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--native-report',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    native=json.loads(args.native_report.read_text());assert native['passed'];executed={};cases=[]
    for row in native['cases']:
        assert row['same_event_port_timer'] and row['actual_expirations']==2 and row['completed']==1
        cases.append(dict(fill=row['fill'],operations=[run(image,operation,executed) for operation in row['operations']]))
    args.output.mkdir(parents=True,exist_ok=True)
    report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,native_report_sha256=hashlib.sha256(args.native_report.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=cases,executed_instruction_bytes=executed,instruction_patches=[],production_replayed_platform_results=[],whole_configuration_or_gameplay_verified=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'{len(cases)} actual-clock/timer native cases match whole original timed bodies ({len(executed)} unchanged sites)')
if __name__=='__main__':main()
