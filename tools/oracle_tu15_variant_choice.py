#!/usr/bin/env python3
"""Run the unchanged TU1.5 quick-option choice scan on authored time values.

Run through tools/run_test.py. This is the complete23D499 lookup, not the
external catalog/localization services used by its256C3A UI caller. The real
DVD sily records are inputs; only their containing pointer is relocated.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EDX, UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP
from oracle_tu15_team_balance import Xbe, NEW_DEFAULT, NEW_PIN, ROOT, STOP
from audit_rendering import Cache


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    cache_path=ROOT/'assets/disc/maps/mainmenu.map'
    cache_pin='1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
    assert hashlib.sha256(cache_path.read_bytes()).hexdigest()==cache_pin
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    cache=Cache(cache_path)
    parameters=[]
    for tag in cache.tags.values():
        if tag['group']!='sily':continue
        _,at=cache.tag(tag['datum'],'sily')
        if cache.u32(at)==10:parameters.append((tag,at))
    assert len(parameters)==1
    tag,at=parameters[0]
    choices=[bytes(cache.data[p:p+12]) for p in cache.block(at+28,12)]
    values=[struct.unpack_from('<i',row,4)[0] for row in choices]
    assert 8 not in values and 480 in values
    source=Xbe(NEW_DEFAULT);machine=Uc(UC_ARCH_X86,UC_MODE_32)
    machine.mem_map(0x10000,0x800000)
    for section in source.sections:machine.mem_write(section.va,source.section_bytes(section))
    owner,rows,stack=0x1100000,0x1101000,0x200ff00
    machine.mem_map(owner,0x10000);machine.mem_map(0x2000000,0x10000);machine.mem_map(STOP,0x1000)
    header=bytearray(cache.data[at:at+36]);struct.pack_into('<II',header,28,len(choices),rows)
    machine.mem_write(owner,bytes(header));machine.mem_write(rows,b''.join(choices))
    executed=set();machine.hook_add(UC_HOOK_CODE,lambda _,address,size,data:executed.add(address))
    controls=[]
    for value in dict.fromkeys([*values,8,-1,99,0x7fffffff]):
        machine.mem_write(stack,struct.pack('<Ii',STOP,value));machine.reg_write(UC_X86_REG_ESP,stack)
        machine.reg_write(UC_X86_REG_EDX,owner);machine.emu_start(0x23d499,STOP,count=1000)
        assert machine.reg_read(UC_X86_REG_EIP)==STOP
        result=machine.reg_read(UC_X86_REG_EAX)
        expected=next((i for i,v in enumerate(values) if value==v),None)
        assert result==(0 if expected is None else rows+12*expected)
        controls.append(dict(value=value,choice_index=expected,choice_string_id=None if expected is None else struct.unpack_from('<I',choices[expected],8)[0]))
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    for address in executed:
        instruction=next(decoder.disasm(source.read(address,15),address))
        assert bytes(machine.mem_read(address,instruction.size))==instruction.bytes
    report=dict(passed=True,source_xbe_sha256=NEW_PIN,source_cache_sha256=cache_pin,
        entry='0x23D499',caller='0x256C3A',whole_original_calls=len(controls),original_instructions=len(executed),
        no_instruction_patches=True,no_replaced_callees=True,parameter=10,source_parameter=tag['name'],
        choices=values,controls=controls,
        conclusion='8 seconds has no authored choice.256C3A initializes the value buffer empty and only localizes a nonnull23D499 result; the visible Round Time Limit caption therefore has an empty value for this validation-only duration.',
        limits=['The external catalog and localization services of256C3A are not emulated.'],
        scratch=str(scratch),scratch_retained=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
