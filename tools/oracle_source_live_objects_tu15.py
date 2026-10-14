#!/usr/bin/env python3
"""Compare native live-region/RGB readback with unchanged TU1.5 readers.

The declared storage fixture is not a gameplay/physics result. Original BC6C0
supplies its actual region count/pointers; original BCD80 supplies each current
RGB. No original instruction or callee result is patched.
"""
import argparse,json,hashlib,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    assert hashlib.sha256(a.xbe.read_bytes()).hexdigest()==NEW_PIN
    result=subprocess.run([str(a.native.resolve()),'--json'],capture_output=True,text=True,check=True)
    fixture=json.loads(result.stdout);image=Xbe(a.xbe);code=_Code(image,'.text')
    bodies={e:complete_body(code,e)for e in [0xbc6c0,0xbcd80]}
    vm=Uc(UC_ARCH_X86,UC_MODE_32);vm.mem_map(0x10000,0x800000)
    for section in image.sections:vm.mem_write(section.va,image.section_bytes(section))
    vm.mem_write(fixture['memory_address'],bytes.fromhex(fixture['memory_hex']))
    put=lambda address,value:vm.mem_write(address,struct.pack('<I',value))
    word=lambda address:struct.unpack('<I',vm.mem_read(address,4))[0]
    put(0x4ecbd0,fixture['memory_address']);stack,output,stop=0x700000,0x700100,0x710000
    executed=set();allowed=set()
    def observe(machine,pc,size,_):
        if pc==stop:machine.emu_stop();return
        assert pc in allowed,f'Unexpected original reader instruction {pc:08x}'
        assert bytes(machine.mem_read(pc,size))==image.read(pc,size),'Original reader instruction changed'
        executed.add(pc)
    vm.hook_add(UC_HOOK_CODE,observe)
    def run(entry):
        allowed.clear();allowed.update(bodies[entry].code)
        vm.reg_write(UC_X86_REG_ESP,stack);put(stack,stop);vm.reg_write(UC_X86_REG_EAX,fixture['object'])
        vm.emu_start(entry,stop,count=1000);assert vm.reg_read(UC_X86_REG_EIP)==stop
    vm.reg_write(UC_X86_REG_ESI,output);vm.reg_write(UC_X86_REG_EDI,output+4);vm.reg_write(UC_X86_REG_EBX,output+8)
    put(stack+4,output+12);run(0xbc6c0)
    count,active,cached,states=[word(output+n*4)for n in range(4)]
    assert count==fixture['region_count']
    assert list(vm.mem_read(active,count))==fixture['permutations']
    assert [list(vm.mem_read(states+i*8,3))for i in range(count)]==fixture['states']
    assert [word(states+i*8+4)for i in range(count)]==fixture['effects']
    assert cached==active+count and states==active+count*2
    for index,expected in enumerate(fixture['color_bits']):
        vm.reg_write(UC_X86_REG_EDI,index);vm.reg_write(UC_X86_REG_ESI,output+16);run(0xbcd80)
        assert vm.reg_read(UC_X86_REG_EAX)&255==1
        assert list(struct.unpack('<III',vm.mem_read(output+16,12)))==expected
    vm.reg_write(UC_X86_REG_EDI,len(fixture['color_bits']));vm.reg_write(UC_X86_REG_ESI,output+16)
    sentinel=bytes.fromhex('a5'*12);vm.mem_write(output+16,sentinel);run(0xbcd80)
    assert vm.reg_read(UC_X86_REG_EAX)&255==0 and bytes(vm.mem_read(output+16,12))==sentinel
    report=dict(reference='English retail TU1.5 only',xbe_sha256=NEW_PIN,passed=True,
        original_distinct_instructions=len(executed),regions=count,colors=len(fixture['color_bits']),
        scope='Completed field readers and native identity/storage admission, not live solver/trajectory parity')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS unchanged TU1.5 live field readers: regions={count}, colors={len(fixture["color_bits"])}')
if __name__=='__main__':main()
