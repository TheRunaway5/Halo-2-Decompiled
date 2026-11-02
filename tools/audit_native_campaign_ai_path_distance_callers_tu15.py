#!/usr/bin/env python3
"""Authenticate bounded distance consumers and preserve all other callers."""
import argparse,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir();image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text')
    parents={root:complete_body(code,root)for root in(0x1b88b0,0x1bbee0)}
    expected=((0x1b8938,'call','0x200c00'),(0x1b893d,'movss','xmm1, dword ptr [0x46a11c]'),
        (0x1b8945,'comiss','xmm1, xmm0'),(0x1b8948,'jbe','0x1b895a'),
        (0x1bbf3b,'call','0x200c00'),(0x1bbf40,'movss','xmm1, dword ptr [0x456914]'),
        (0x1bbf48,'comiss','xmm1, xmm0'),(0x1bbf4b,'jbe','0x1bbf78'))
    for pc,operation,operands in expected:
        assert any(pc in body.code for body in parents.values());i=code.at(pc);assert(i.mnemonic,i.op_str)==(operation,operands)
    section=image.section('.text');raw=image.section_bytes(section);calls=[]
    for offset in range(len(raw)-4):
        if raw[offset]==0xe8 and(section.va+offset+5+int.from_bytes(raw[offset+1:offset+5],'little',signed=True))&0xffffffff==0x200c00:
            pc=section.va+offset;assert code.at(pc).mnemonic=='call';calls.append(pc)
    assert calls==[0x1b8472,0x1b8938,0x1bbf3b,0x1c1776,0x1c33e4,0x1c5715,0x1c595c]
    def listing(root):return[dict(pc=hex(pc),bytes=code.at(pc).bytes.hex(),operation=code.at(pc).mnemonic,operands=code.at(pc).op_str)for pc in sorted(parents[root].code)]
    report=dict(passed=True,original_sha256=NEW_PIN,qualified_callers=['0x1b8938','0x1bbf3b'],
        direct_call_inventory=[hex(pc)for pc in calls],unqualified_callers=[hex(pc)for pc in calls if pc not in(0x1b8938,0x1bbf3b)],
        parent_bodies={hex(root):listing(root)for root in parents},
        contract=dict(argument='Actual incoming EAX actor datum; no stack argument',
            result='Scalar XMM0, with upper96 bits zero after actor-pool lookup; exact source FP status and caller older x87 quantities',
            continuation='Both continuations define XMM1 then COMISS XMM1,XMM0, killing incoming arithmetic flags before JBE',
            stack='Real CALL creates its own return word, whole200C00 RET advances entrySP by4',
            opaque_state='Callee-saved GP and caller live extended values retained. Dead volatile GP/SIMD and private prologue state are not fabricated or claimed as outputs'),
        limits=['Only two listed numeric callers; other five direct sites stay unqualified','No full caller/action-pipeline or live World acceptance by this audit','Caller-private aliases, MMX, unmasked exceptions and full x87 stacks remain outside binding scope'],
        source_sha256={str(Path(__file__)):hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print('Authenticated two distance consumers; five other numeric callers remain open')


if __name__=='__main__':main()
