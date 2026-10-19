#!/usr/bin/env python3
"""Fingerprint unchanged TU1.5 20700 ->20E30 retirement of declared CPU rings.

The fixture retains input formulas and complete output hashes, not executable
bytes or raw memory banks. No native output supplies original input/expectation.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

BASE=0x52dfd8
SIZE=0x40c0
STACK=0x1000000
STOP=0x1010000

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True)
    a=p.parse_args()
    assert not a.fixture.exists() and not a.report.exists(),'Preserve prior evidence'
    x=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for section in x.sections:u.mem_write(section.va,x.section_bytes(section))
    original=bytes(u.mem_read(0x10000,0x800000))
    u.mem_map(STACK,0x8000);u.mem_map(STOP,4096)
    sites=set();stores=[];instructions=0
    def instruction(cpu,pc,size,_):
        nonlocal instructions
        assert bytes(cpu.mem_read(pc,size))==x.read(pc,size),'Original instruction changed'
        sites.add(pc);instructions+=1
    def write(cpu,access,address,size,value,_):
        assert (BASE<=address and address+size<=BASE+SIZE or STACK<=address and address+size<=STACK+0x8000), \
            'Original retirement wrote outside its ring or caller stack'
        if BASE<=address<BASE+SIZE:stores.append((address,size))
    u.hook_add(UC_HOOK_CODE,instruction);u.hook_add(UC_HOOK_MEM_WRITE,write)
    rows=[]
    for count in (0,1,2,31,32,33,510,511):
        for seed in (7,91):
            pool=bytearray((n*17+seed)&255 for n in range(SIZE))
            pool[0]=0 # Declared disabled-admission branch of complete original20700.
            struct.pack_into('<I',pool,0x531fbc-BASE,count)
            u.mem_write(0x10000,original);u.mem_write(BASE,bytes(pool))
            u.mem_write(STACK,b'\xa5'*0x8000);u.mem_write(STACK+0x4000,struct.pack('<I',STOP))
            for reg in (R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,
                        R.UC_X86_REG_EBX,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI):
                u.reg_write(reg,0)
            u.reg_write(R.UC_X86_REG_ESP,STACK+0x4000);u.reg_write(R.UC_X86_REG_EFLAGS,0x202)
            stores.clear();instructions=0
            u.emu_start(0x20700,STOP,count=100000)
            assert u.reg_read(R.UC_X86_REG_EIP)==STOP
            result=bytes(u.mem_read(BASE,SIZE))
            rows.append({'count':count,'seed':seed,'initial_sha256':hashlib.sha256(pool).hexdigest(),
                'retired_sha256':hashlib.sha256(result).hexdigest(),'original_instructions':instructions,
                'original_ring_stores':len(stores)})
    fixture={'schema':1,'xbe_sha256':NEW_PIN,'original_entry':0x20700,'retire_entry':0x20e30,
        'base':BASE,'size':SIZE,'input_formula':'byte[n]=(17*n+seed)&255; byte[0]=0; LE32[count at531FBC]',
        'cases':rows}
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(fixture,indent=2)+'\n')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps({'cases':len(rows),
        'unchanged_original_instruction_sites':sorted(sites),'fixture_sha256':hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
        'fixture_bytes':a.fixture.stat().st_size,'raw_banks_retained':False,
        'boundary':'Declared CPU rings and integer retirement only; no GPU/world-migration or visual parity claim'},indent=2)+'\n')
    print(f'PASS {len(rows)} unchanged TU1.5 retirement cases; no raw banks retained')

if __name__=='__main__':main()
