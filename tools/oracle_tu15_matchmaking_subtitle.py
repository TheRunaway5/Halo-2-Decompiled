#!/usr/bin/env python3
"""Bounded unchanged TU1.5 subtitle selector and derived-screen vtable audit.

Only the selector block after its external globals lookup and before its
external UNIC lookup executes. The exact pinned DVD key bank is inspected
separately. This does not claim to execute localization or the full UI graph.
Run through tools/run_test.py; retain only the small JSON report.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP
from oracle_tu15_team_balance import Xbe, NEW_DEFAULT, NEW_PIN, ROOT
from audit_rendering import Cache
from export_matchmaking_ui_tu15 import Definition, XBOX_PIN


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    original=ROOT/'assets/disc/maps/mainmenu.map'
    assert hashlib.sha256(original.read_bytes()).hexdigest()==XBOX_PIN
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    cache=Cache(original);definition=Definition(cache)
    screen=next(t for t in cache.tags.values() if t['group']=='wgit' and
        definition.u16(cache.tag(t['datum'],'wgit')[1]+4)==210)
    screen_data=definition.screen(screen)
    wigl=next(t for t in cache.tags.values() if t['group']=='wigl' and t['name']==r'ui\ui_shared_globals')
    _,globals_offset=cache.tag(wigl['datum'],'wigl')
    bank=definition.unicode_strings(cache.u32(globals_offset+0x144))
    assert screen_data['button_key']==0 and '0' not in bank

    source=Xbe(NEW_DEFAULT);machine=Uc(UC_ARCH_X86,UC_MODE_32)
    machine.mem_map(0x10000,0x800000)
    for section in source.sections:machine.mem_write(section.va,source.section_bytes(section))
    owner,stack=0x1100000,0x200f000
    machine.mem_map(owner,0x1000);machine.mem_map(0x2000000,0x10000)
    machine.mem_write(owner+0x144,struct.pack('<I',1))
    executed=set();machine.hook_add(UC_HOOK_CODE,lambda _,a,z,u:executed.add(a))
    controls=[]
    for value in range(23):
        #14C233's unchanged prologue has already allocated its local table.
        #EAX is the explicit result of the excluded globals lookup.
        machine.reg_write(UC_X86_REG_EBP,stack)
        machine.reg_write(UC_X86_REG_ESP,stack-0x100)
        machine.reg_write(UC_X86_REG_EAX,owner)
        machine.reg_write(UC_X86_REG_ESI,value)
        machine.emu_start(0x14c243,0x14c39f,count=1000)
        assert machine.reg_read(UC_X86_REG_EIP)==0x14c39f
        sid=machine.reg_read(UC_X86_REG_EDI)
        controls.append(dict(type=value,string_id=f'{sid:08x}',text=bank.get(str(sid))))
    assert controls[0]['string_id']=='00000000' and controls[0]['text'] is None
    assert controls[13]['string_id']=='080006da' and controls[13]['text'] is not None
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    for at in executed:
        instruction=next(decoder.disasm(source.read(at,15),at))
        assert bytes(machine.mem_read(at,instruction.size))==instruction.bytes

    derived=struct.unpack('<28I',source.read(0x466648,112))
    base=struct.unpack('<28I',source.read(0x464b28,112))
    changes={i:v for i,v in enumerate(derived) if v!=base[i]}
    assert changes=={0:0x257d93,3:0x257e59,10:0x257e1a,18:0x257dc6,26:0x25768c}
    report=dict(passed=True,scope=__doc__,source_xbe_sha256=NEW_PIN,source_cache_sha256=XBOX_PIN,
        selector_range=['0x14C243','0x14C39F'],controls=controls,unchanged_selector_instructions=len(executed),
        no_instruction_patches=True,no_replaced_callees=True,derived_vtable='0x466648',base_vtable='0x464B28',
        vtable=[dict(slot=i,derived=f'{v:08x}',base=f'{base[i]:08x}',overridden=v!=base[i]) for i,v in enumerate(derived)],
        original_screen_button_key=screen_data['button_key'],
        derived_override_roles={'0':'destructor','3':'update','10':'input event','18':'layout build','26':'screen loader'},
        source_graph={'common_subtitle':'2369F4 reads definition+6,259EC6 initializes an empty buffer,14C233 selects its SID',
            'inherited_update':'234DAE advances animation then dispatches child updates; it has no subtitle getter',
            'inherited_bitmap_load':'236119 iterates declared bitmap resources',
            'input':'257E1A retains original B/Back handler2584A9'},
        conclusion='The authenticated definition selects SID0, absent from its actual key bank. The derived vtable has no extra caption getter or render override. No visible PC Cancel caption is supplied by this pinned definition/selector path.',
        limits=['Localization and the complete window/widget graph are not executed by this bounded control.',
            'The supplied video screenshots visibly show B CANCEL but have no identified update version; that visual discrepancy remains explicit.'],
        scratch=str(scratch),scratch_retained=False)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,controls=len(controls),instructions=len(executed),report=str(args.report))))


if __name__=='__main__':main()
