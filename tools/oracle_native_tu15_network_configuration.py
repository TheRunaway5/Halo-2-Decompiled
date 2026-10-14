#!/usr/bin/env python3
"""Offline differential for unchanged TU1.5 parameter normalization.

Run original3DFD6A through its complete normalization prefix, stopping before
3DFDE5's later entropy/timer/private-pool initialization. No patched instructions
or supplied subroutine results. All256 byte values are tested independently in
every field of both accepted record sizes; null/invalid sizes are also covered.
This proves the decoder, not full Xbox XNet or networking behavior.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    pins={0x3dfd6a:(123,'e9c2153d89d137c982c084ac6db2bb8071ac8552a90f32167bd937f1d7c8d30f'),
          0x3ea2d0:(76,'43eb1146d58002cab81f60411c3232c5b344e51f49007dcfccb839c4ccfe260f'),
          0x3ea320:(76,'e6281999a797cbc9e7b26900660d9667f71a5b8a82e92b14df37356a3e307988'),
          0x3ea370:(76,'412eace1dd27393df566ca2a3f1c511c4d848b63deb18f93d6c400527781fbee')}
    for p,(n,pin) in pins.items():assert hashlib.sha256(image.read(p,n)).hexdigest()==pin
    machine=Uc(UC_ARCH_X86,UC_MODE_32)
    machine.mem_map(0x10000,0x800000)
    for section in image.sections:machine.mem_write(section.va,image.section_bytes(section))
    owner,record,stack=0x50000000,0x51000000,0x52000000
    for p in (owner,record,stack):machine.mem_map(p,4096)
    rows=[]
    def run(data):
        machine.mem_write(owner,bytes(4096));machine.mem_write(record,bytes(4096));machine.mem_write(stack,bytes(4096))
        if data is not None:machine.mem_write(record+16,data)
        machine.mem_write(record,struct.pack('<I',record+16 if data is not None else 0))
        sp=stack+0xf00;machine.mem_write(sp,struct.pack('<II',0x53000000,record))
        for reg in (R.UC_X86_REG_EAX,R.UC_X86_REG_EBX,R.UC_X86_REG_EDX,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI,R.UC_X86_REG_EBP):machine.reg_write(reg,0)
        machine.reg_write(R.UC_X86_REG_ECX,owner);machine.reg_write(R.UC_X86_REG_ESP,sp);machine.reg_write(R.UC_X86_REG_EFLAGS,2)
        machine.emu_start(0x3dfd6a,0x3dfde5,count=2000)
        assert machine.reg_read(R.UC_X86_REG_EIP)==0x3dfde5
        rows.append({'input':None if data is None else data.hex(),'normalized':bytes(machine.mem_read(owner+4,76)).hex()})
    run(None)
    for size in range(256):
        if size not in (12,76):run(bytes([size]))
    for size in (12,76):
        for field in range(size):
            for value in range(256):
                data=bytearray(76);data[0]=size;data[field]=value;run(bytes(data))
    report={'schema':'H2_NATIVE_TU15_NETWORK_CONFIGURATION_ORIGINAL_V1','source_sha256':NEW_PIN,
            'instruction_patches':[],'supplied_subroutine_results':[],'cases':len(rows),'rows':rows,
            'frontier':'3DFDE5','full_xnet_startup_proven':False,'table_pins':{hex(p):pin for p,(n,pin) in pins.items()}}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,separators=(',',':'))+'\n')
    print(json.dumps({'cases':len(rows),'frontier':'3DFDE5','output':str(args.output)}))
if __name__=='__main__':main()
