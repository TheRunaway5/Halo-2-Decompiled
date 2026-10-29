#!/usr/bin/env python3
"""Authenticate original cinematic void caller contracts without executing code.

Scan every relative32 CALL/JMP byte candidate in the owned English TU1.5 .text,
authenticate the actual instructions and retain the callers/consumer prefixes.
This is static liveness evidence, not a full rendered frame execution proof.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');section=image.section('.text');data=bytes(image.section_bytes(section))
    hits=[]
    for offset,opcode in enumerate(data[:-5]):
        if opcode not in (0xe8,0xe9):continue
        destination=(section.va+offset+5+int.from_bytes(data[offset+1:offset+5],'little',signed=True))&0xffffffff
        if destination in (0x1fcb0,0x1fce0,0x13b70):
            pc=section.va+offset;instruction=code.at(pc)
            assert instruction.mnemonic in ('call','jmp') and instruction.operands[0].imm==destination
            hits.append(dict(instruction=pc,target=destination,bytes=instruction.bytes.hex(),operation=instruction.mnemonic))
    direct=[row for row in hits if row['target']!=0x13b70]
    assert {(row['instruction'],row['target'],row['operation']) for row in direct}=={
        (0x13bd7,0x1fce0,'call'),(0x13bde,0x1fcb0,'call')}
    assert [(row['instruction'],row['target']) for row in hits if row['target']==0x13b70]==[(0x2b6d2,0x13b70)]
    def listing(begin,end):
        result=[];pc=begin
        while pc<end:
            ins=code.at(pc);assert ins and pc+ins.size<=end
            result.append(dict(instruction=pc,bytes=ins.bytes.hex(),operation=ins.mnemonic,operands=ins.op_str));pc+=ins.size
        assert pc==end
        return result
    prefixes={
        'common_caller_continuation':listing(0x13be3,0x13bf8),
        'next_owner_first_flag_definition':listing(0x22050,0x22065),
        'math_wrapper_eax_definition_before_service':listing(0x37cde8,0x37cdff),
        'device_owner_eax_definition_before_service':listing(0x14f40,0x14f60),
        'parent_continuation':listing(0x2b6d7,0x2b6fc),
        'parent_next_owner_eax_definition':listing(0x13cb0,0x13cb7)}
    expected={0x13be3:('mov','ecx, dword ptr [esi + 0xa8]'),0x13be9:('mov','edx, dword ptr [esi + 0xa4]'),
        0x13bf1:('call','0x22050'),0x22059:('sub','esp, 8'),0x2205c:('comiss','xmm2, xmm0'),
        0x37cdf6:('mov','eax, dword ptr [esp + 0xc]'),0x14f51:('mov','eax, edi'),
        0x2b6d7:('call','0x13cb0'),0x13cb0:('xor','eax, eax')}
    for pc,(mnemonic,operands) in expected.items():
        ins=code.at(pc);assert ins.mnemonic==mnemonic and ins.op_str==operands,(hex(pc),ins.op_str)
    owners={}
    for entry in (0x1fcb0,0x1fce0):
        body=_trace(code,entry,{0x1fcb0,0x1fce0});assert not body.calls and not body.tail_jumps
        ret=[code.at(pc) for pc in body.code if code.at(pc).mnemonic.startswith('ret')]
        assert len(ret)==1 and not ret[0].operands
        owners[hex(entry)]=dict(sites=len(body.code),ret=ret[0].address,stack_argument_bytes=0,
            listing=[dict(instruction=pc,bytes=code.at(pc).bytes.hex(),operation=code.at(pc).mnemonic,
                operands=code.at(pc).op_str) for pc in sorted(body.code)])
    # A raw target-word occurrence inside MOV SI,[EAX+1FC] is not a function
    # pointer. Retain the candidate and its owning instruction explicitly.
    absolute_candidates=[]
    for entry in (0x1fcb0,0x1fce0):
        for sec in image.sections:
            raw=bytes(image.section_bytes(sec));offset=0;pattern=struct.pack('<I',entry)
            while (offset:=raw.find(pattern,offset))>=0:
                absolute_candidates.append(dict(target=entry,section=sec.name,address=sec.va+offset));offset+=1
    assert absolute_candidates==[dict(target=0x1fcb0,section='.text',address=0xcdbe3)]
    ins=code.at(0xcdbe1);assert ins.mnemonic=='mov' and ins.op_str=='si, word ptr [eax + 0x1fc]'
    report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),relative_calls_and_tails=hits,
        owners=owners,consumer_prefixes=prefixes,absolute_address_candidates=absolute_candidates,
        absolute_candidate_is_displacement_overlap=dict(instruction=ins.address,bytes=ins.bytes.hex(),operands=ins.op_str),
        native_contract='void/no arguments; preserve callee-saved registers and FP; actual RET word read then SP+4; dead volatile GPR/flags/private save stack excluded',
        direction_contract='1FCB0 REP STOSD requires ordinary ABI DF-clear; reject DF-set before mutation, without lifted fallback',
        liveness=dict(ecx_edx='Overwritten at13BE3/13BE9 before any use of returned values',
            flags='Common continuation uses no flags;22050 defines flags at22059/2205C before its first conditional use',
            eax='22050 either returns without reading incomingEAX or enters37CDE8, which definesEAX at37CDF6 before any use;14F40 also definesEAX at14F51. After13B70, parent enters13CB0 which immediately zeroesEAX. No returnedEAX is consumed.',
            fp='Both leaves contain integer/store operations only; physical semantic fixture separately verifies FP state'),
        boundaries=['Static relative32/direct and literal reference inventory; no claim about obfuscated/computed addresses',
            'Static liveness proof for authenticated original consumers; composed live/rendered replay remains separately required'],
        production_original_execution=False,production_instruction_decoder=False,retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,direct_callers=direct,owners={key:value['sites'] for key,value in owners.items()})))

if __name__=='__main__':main()
