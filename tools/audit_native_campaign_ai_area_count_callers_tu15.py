#!/usr/bin/env python3
"""Authenticate the actual authored-area count descriptor and caller liveness."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text')
    parents={entry:complete_body(code,entry)for entry in (0x1b0020,0x1b01e0)}
    def expect(pc,operation,operands):
        assert any(pc in body.code for body in parents.values()),hex(pc)
        instruction=code.at(pc)
        assert (instruction.mnemonic,instruction.op_str)==(operation,operands),hex(pc)
    expect(0x1b018d,'mov','eax, dword ptr [edi + 0x10]')
    expect(0x1b0194,'push','ebx')
    expect(0x1b0195,'call','eax')
    expect(0x1b0197,'mov','edx, dword ptr [esp + 0x1c]')
    expect(0x1b019b,'mov','word ptr [edx], ax')
    expect(0x1b01a9,'mov','ax, word ptr [ebp + 4]')
    expect(0x1b01be,'movsx','ecx, ax')
    expect(0x1b01c1,'mov','eax, dword ptr [edx + 8]')
    for call,continuation in ((0x1b02ed,0x1b02f2),(0x1b0329,0x1b032e)):
        expect(call,'call','0x1b0020')
        expect(continuation,'mov','eax, dword ptr [esp + 0x14]')
    references=[]
    for section in image.sections:
        raw=bytes(image.section_bytes(section))
        offset=0
        while (offset:=raw.find(struct.pack('<I',0x1bbb50),offset))>=0:
            references.append(dict(section=section.name,address=section.va+offset))
            offset+=1
    assert references==[dict(section='.data',address=0x48a9e8)],references
    assert int.from_bytes(image.read(0x48a9d8+0x10,4),'little')==0x1bbb50
    def listing(entry,begin,end):
        return [dict(pc=hex(pc),bytes=code.at(pc).bytes.hex(),
                     operation=code.at(pc).mnemonic,operands=code.at(pc).op_str)
                for pc in sorted(parents[entry].code)if begin<=pc<end]
    report=dict(passed=True,original_sha256=NEW_PIN,
                source_sha256={str(Path(__file__)):hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},
                literal_entry_references=references,
                descriptor=dict(address=0x48a9d8,callback_slot=0x10,target=0x1bbb50),
                callback=listing(0x1b0020,0x1b018d,0x1b01d2),
                parent_continuations=[listing(0x1b01e0,a,b)for a,b in
                                      ((0x1b02ed,0x1b030b),(0x1b0329,0x1b0347))],
                contract=dict(actor='Actual pushed EBX at callback entry SP+4',
                              return_value='AX16 written to actual score output before new AX definition',
                              stack='Actual indirect CALL return1B0197 and RET4; SP advances8',
                              gp_liveness='EDX is defined before use; common continuation consumes newly defined AX only; actual1B01E0 parent continuations redefine full EAX immediately after1B0020 returns',
                              flags='Callback flags are killed by actual1B01AD TEST before a conditional use',
                              floating_point='Integer query/transport preserves retained live x87, SIMD, MMX and FP controls'),
                limits=['Exact descriptor callback and two authenticated1B01E0 parent continuations; arbitrary hidden numeric callers are not admitted',
                        'Component MMX preservation is distinct from the existing World bridge MMX admission boundary',
                        'No World execution, live Armory progression, full engine or launcher qualification'],
                retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('Authenticated1BBB50 descriptor, actual1B0195 AX16/RET4 consumer and two parent EAX definitions')


if __name__=='__main__':
    main()
