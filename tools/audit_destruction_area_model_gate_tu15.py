#!/usr/bin/env python3
"""Authenticate the original per-target radial model-information gate."""
import argparse
import hashlib
import json
import os
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py or its finite session')
    image=Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest()!=NEW_PIN:raise ValueError('Original English TU1.5 changed')
    raw=image.read(0xda830,0x75)
    instructions=[dict(address=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)
        for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(raw,0xda830)]
    expected={0xda830:('sub','esp, 0x14c'),0xda870:('call','0xd80a0'),
        0xda875:('mov','ecx, dword ptr [esp + 0x160]'),0xda88b:('mov','ebp, eax'),
        0xda88d:('xor','edx, edx'),0xda88f:('cmp','ebp, edx'),0xda898:('je','0xdac59')}
    actual={row['address']:(row['mnemonic'],row['operands'])for row in instructions}
    passed=all(actual.get(address)==operation for address,operation in expected.items())
    report=dict(passed=passed,original=str(NEW_DEFAULT),original_sha256=NEW_PIN,
        original_code_start=0xda830,original_code_sha256=hashlib.sha256(raw).hexdigest(),instructions=instructions,
        scoped_fact='DA830 invokes D80A0 at DA870 and branches to DAC59 when its returned information pointer is zero',
        lookup_entry_stack_request_offset=0x164,
        stack_derivation='Caller entry ESP minus SUB14C, four saved registers and the D80A0 return word; caller request at entry ESP+4',
        whole_owner_or_runtime_qualification=False,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=passed,scope=report['scoped_fact'])),flush=True)
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
