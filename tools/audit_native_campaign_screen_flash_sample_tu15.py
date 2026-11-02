#!/usr/bin/env python3
"""Recover exact156E40 caller, typed dependencies and persistent operands."""
import argparse
import hashlib
import json
import os
from pathlib import Path
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text')
    entries={0x156e40,0x17f8a0,0x37dc0a,0x37d46c,0x331ecb,0x37dc14,0x331f68,0x37d536}
    bodies={entry:_trace(code,entry,entries) for entry in entries}
    uses=[]
    for entry,body in bodies.items():
        for pc in sorted(body.code):
            instruction=code.at(pc)
            for operand in instruction.operands:
                if operand.type==X86_OP_MEM and operand.mem.disp>=0x400000:
                    uses.append(dict(owner=hex(entry),pc=hex(pc),address=hex(operand.mem.disp),
                        bytes=operand.size,indexed=bool(operand.mem.base or operand.mem.index),
                        operation=instruction.mnemonic,operands=instruction.op_str))
    # Actual XLAT index is masked to15; descriptor has16 class-pair methods.
    # Authored curve enum1..7 owns seven complete1024-byte rows; type0 bypasses.
    spans=[(0x46ecb4,16,'actual class lookup masked index0..15'),
           (0x48c8cc,80,'actual fmod descriptor and16 selected method identities'),
           (0x47524c,14,'seven authored camera-effect request type words')]
    spans.extend((0x4782a8+kind*1024,1024,'authored transition channel'+str(kind))
                 for kind in range(1,8))
    operands=[dict(address=hex(at),bytes=width,indexed=False,role=role) for at,width,role in spans]
    operands.extend(row for row in uses if not row['indexed'])
    caller=[]
    pc=0x2b8b9
    while pc<=0x2b958:
        instruction=code.at(pc)
        caller.append(dict(pc=hex(pc),bytes=instruction.bytes.hex(),
            operation=instruction.mnemonic,operands=instruction.op_str))
        pc+=instruction.size
    call=code.at(0x2b8ec)
    assert call.mnemonic=='call' and call.operands[0].type==X86_OP_IMM and call.operands[0].imm==0x156e40
    returns=[dict(pc=hex(pc),cleanup=code.at(pc).op_str)
             for pc in bodies[0x156e40].code if code.at(pc).mnemonic=='ret']
    assert returns and all(not row['cleanup'] for row in returns)
    data=[]
    for address,width,role in spans:
        section=next(s for s in image.sections if s.va<=address< s.va+s.vsize)
        value=image.read(address,width)
        data.append(dict(address=hex(address),bytes=width,role=role,
                         section=section.name,flags=section.flags,sha256=hashlib.sha256(value).hexdigest(),
                         value_hex=value.hex() if width<=80 else None))
    files=['include/halo2/player_screen_flash.hpp','src/gameplay/units/player_screen_flash.cpp',
           'include/halo2/native_campaign_screen_flash_sample_tu15.hpp',
           'src/campaign/native_campaign_screen_flash_sample_tu15.cpp',
           'src/campaign/native_campaign_screen_flash_sample_tu15_binding.cpp']
    report=dict(passed=True,original_sha256=NEW_PIN,
        root_entry='0x156e40',root_sites=len(bodies[0x156e40].code),
        original_owners={hex(entry):dict(sites=len(body.code),
            branches=[dict(pc=hex(pc),target=hex(target),kind=kind) for pc,target,kind in body.sites])
            for entry,body in bodies.items()},
        absolute_data_operands=operands,primary_persistent_operands=uses,authored_data=data,
        caller=dict(owner='0x2b770',site='0x2b8ec',real_return='0x2b8f1',
            input_user='EAX from camera[+8]; call skipped forNONE',
            output='ESI=pre-CALL SP+0x40; callee entrySP+0x44,24-byte packet',
            return_cleanup=returns,continuation=caller,
            liveness='Nonvolatiles EBX/EBP/ESI/EDI and x87 live values survive. Continuation replacesXMM0/flags immediately; EAX/ECX/EDX replaced before use. The following packet constructor receives the completed24-byte output pointer; no flash result register is consumed.'),
        source_sha256={file:hashlib.sha256(Path(file).read_bytes()).hexdigest() for file in files if Path(file).is_file()},
        existing_native_core=dict(path='src/gameplay/units/player_screen_flash.cpp',
            scope='PlayerScreenFlashPlayer::evaluate models local admitted request state only, not global fade, live source-bank aliases/read ordering or CRT numerical policy. It is useful semantic precedent, not full156E40 source-memory closure.'),
        remaining=['Computed CRT handlers other than37DC14/331F68 stop at actual331F55 required-owner boundary.',
            'True CRT math-error callback3334F0 remains unclosed; no supplied success.',
            'This source/caller audit does not qualify new core; independent original comparison is separately required.'],
        retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,sites={hex(entry):len(body.code)for entry,body in bodies.items()},
                         caller='0x2b8ec',returns=returns)))


if __name__=='__main__':
    main()
