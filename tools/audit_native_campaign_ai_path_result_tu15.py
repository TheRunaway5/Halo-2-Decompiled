#!/usr/bin/env python3
"""Authenticate27A460's two unexecuted guard sites for ordinary data callers."""
import argparse,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');body=complete_body(code,0x27a460)
    expected=(
        (0x27a460,'sub','esp, 0x30'),(0x27a463,'push','ebx'),(0x27a464,'push','esi'),
        (0x27a469,'push','edi'),(0x27a473,'cmp','ax, 0xffff'),
        (0x27a477,'mov','dword ptr [esp + 0x14], eax'),(0x27a47b,'je','0x27a5ee'),
        (0x27a487,'push','ebp'),(0x27a4a1,'fstp','dword ptr [esp + 0x14]'),
        (0x27a4a7,'lea','eax, [esp + 0x34]'),(0x27a4b0,'lea','eax, [esp + 0x28]'),
        (0x27a4be,'lea','ecx, [esp + 0x1c]'),(0x27a4fb,'fstp','dword ptr [esp + 0x10]'),
        (0x27a524,'pop','ebp'),(0x27a541,'mov','byte ptr [esp + 0x44], al'),
        (0x27a54b,'mov','ecx, dword ptr [esp + 0x14]'),
        (0x27a56d,'mov','word ptr [eax], dx'),(0x27a570,'mov','edx, ecx'),
        (0x27a574,'mov','cx, word ptr [eax + 2]'),(0x27a578,'cmp','cx, -1'),
        (0x27a57c,'jne','0x27a560'),(0x27a57e,'cmp','dx, -1'),
        (0x27a582,'mov','ecx, edx'),(0x27a584,'je','0x27a5c7'),
        (0x27a590,'comiss','xmm1, xmm0'),(0x27a593,'jbe','0x27a5c1'),
        (0x27a5ad,'mov','cx, word ptr [eax]'),(0x27a5b0,'cmp','cx, -1'),
        (0x27a5b4,'addss','xmm2, xmm0'),(0x27a5b8,'movaps','xmm0, xmm2'),
        (0x27a5bb,'jne','0x27a590'),(0x27a5bd,'mov','edx, edi'),
        (0x27a5bf,'jmp','0x27a5ce'),(0x27a5c1,'cmp','cx, -1'),
        (0x27a5c5,'jne','0x27a5cb'),(0x27a5c7,'mov','edx, edi'),
        (0x27a5c9,'jmp','0x27a5ce'))
    listing=[]
    for pc,operation,operands in expected:
        assert pc in body.code
        instruction=code.at(pc)
        assert (instruction.mnemonic,instruction.op_str)==(operation,operands),hex(pc)
        listing.append(dict(pc=hex(pc),bytes=instruction.bytes.hex(),operation=operation,operands=operands))
    assert code.at(0x27a5b4).eflags==0 and code.at(0x27a5b8).eflags==0
    jumps_to_guard=[hex(pc)for pc in sorted(body.code)if code.at(pc).mnemonic.startswith('j')
        and code.at(pc).operands and code.at(pc).operands[0].type==2
        and code.at(pc).operands[0].imm==0x27a5c7]
    assert jumps_to_guard==['0x27a584']
    report=dict(passed=True,original_sha256=NEW_PIN,scope='Two guard sites under the ordinary retained-record profile',
        root_sites=len(body.code),guard_sites=['0x27a5c7','0x27a5c9'],authenticated_instructions=listing,
        frame_offsets=dict(saved_found_word='entrySP-0x28',cost='entrySP-0x2C',clearance='entrySP-0x30',
            projected_points=['entrySP-0x0C..-0x04','entrySP-0x18..-0x10'],closest='entrySP-0x24..-0x1C',result_byte='entrySP+0x08'),
        invariants=[
            'The found WORD is notFFFF after27A47B. Its actual saved slot isentrySP-0x28; the later EBP push makes the cost slotentrySP-0x2C, not that saved slot.',
            'The initial reverse cursor is that saved found WORD. Every reverse backedge27A57C requires its next WORD notFFFF. DX captures the current valid WORD before the next link read. DX is therefore notFFFF at27A57E;27A584 cannot enter the guard.',
            'The scan initial CX is that DX. Every scan backedge27A5BB requires CX notFFFF. ADDSS/MOVAPS between CMP and JNE modify no EFLAGS. A terminal link instead jumps directly through27A5BD/5BF. CX is therefore notFFFF when27A5C1 is reached, and27A5C5 always skips the guard.'],
        assumptions=['Whole entry under normal call/return control, with no instruction patch or register mutation',
            'Retained data/output records do not alias the callee private frame; no external input timeline edits that private frame',
            'Callee private outputs are the authenticated disjoint three-coordinate buffers shown above'],
        limits=['This is a checked primary control-flow invariant under stated data ownership, not proof for arbitrary private-frame aliases',
            'Native implementation, search, World callers and campaign completion need their own evidence'],
        source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('Authenticated two unexecuted27A460 guard sites and their ordinary-frame invariants')


if __name__=='__main__':main()
