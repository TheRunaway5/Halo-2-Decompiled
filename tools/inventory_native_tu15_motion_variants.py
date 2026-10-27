#!/usr/bin/env python3
"""Authenticate motion virtual step owners from the pinned TU1.5 image.

Run through tools/run_test.py. Only small reports and original listings are
retained; decoded source/image data stays in the offline process.
"""
import argparse
import bisect
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from functions import _Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM,X86_OP_IMM
TABLES=(0x41f1e0,0x41f260,0x41f2e0,0x41f360)

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();assert os.environ['HALO2_TEST_SCRATCH'];args.output.mkdir(parents=True,exist_ok=False)
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');registry=ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv';starts={int(line.split('\t')[0],16) for line in registry.read_text().splitlines()[1:]};ordered=sorted(starts)
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True;decoder.skipdata=True;section=image.section('.text');published={p:[] for p in TABLES};constructors={}
    for instruction in decoder.disasm(image.section_bytes(section),section.va):
        if instruction.mnemonic!='mov' or len(instruction.operands)!=2:continue
        destination,value=instruction.operands
        if destination.type!=X86_OP_MEM or destination.size!=4 or value.type!=X86_OP_IMM or value.imm not in published:continue
        found=None
        for owner in reversed(ordered[max(0,bisect.bisect_right(ordered,instruction.address)-8):bisect.bisect_right(ordered,instruction.address)]):
            trace=_trace(code,owner,starts)
            if instruction.address in trace.code:found=(owner,trace);break
        assert found,(hex(instruction.address),hex(value.imm));owner,trace=found;constructors[owner]=trace
        published[value.imm].append(dict(instruction=instruction.address,owner=owner,destination=instruction.op_str,original_bytes=instruction.bytes.hex()))
    assert all(published.values()),published
    bodies={0x323730,0x8fdb0,0x2e4d60,0x323430,0x1e1010,0x37c5d0,0x2e5810}
    records=[]
    for table in TABLES:
        raw=image.read(table,12);destructor,pose,force=struct.unpack('<3I',raw)
        assert all(code.inside(v) for v in (destructor,pose,force));records.append(dict(table=table,prefix_hex=raw.hex(),prefix_sha256=hashlib.sha256(raw).hexdigest(),pose=pose,force=force,published_by=published[table]));bodies.update((pose,force))
    listings=[]
    for entry in sorted(bodies|constructors.keys()):
        trace=constructors.get(entry) or _trace(code,entry,starts)
        path=args.output/f'{entry:08x}.asm.tsv';path.write_text('\n'.join(f'{p:08x}\t{code.at(p).bytes.hex()}\t{code.at(p).mnemonic}\t{code.at(p).op_str}' for p in sorted(trace.code))+'\n')
        direct=[];indirect=[]
        for pc in sorted(trace.code):
            instruction=code.at(pc)
            if instruction.mnemonic!='call':continue
            if instruction.operands[0].type==X86_OP_IMM:direct.append(dict(instruction=pc,target=instruction.operands[0].imm))
            else:indirect.append(dict(instruction=pc,operand=instruction.op_str))
        listings.append(dict(entry=entry,instructions=len(trace.code),listing_file=path.name,listing_sha256=sha(path),instruction_sha256=hashlib.sha256(b''.join(code.at(p).bytes for p in sorted(trace.code))).hexdigest(),direct_calls=direct,indirect_calls=indirect,switch_tables=len(trace.tables)))
    report=dict(parity_target='English TU1.5 only',original_sha256=NEW_PIN,validator_sha256=sha(__file__),function_registry_sha256=sha(registry),tables=records,bodies=listings,caller_dispatch=dict(entry=0x320b70,pose_vtable_offset=4,force_vtable_offset=8),existing_native_owners=[0x325170,0x3252d0],new_owner_chain=[0x323730,0x323430,0x1e1010,0x37c5d0,0x2e5810],new_return_only_owners=[0x8fdb0,0x2e4d60],live_kind_usage_proven=False,all_motion_vtables_exhaustively_proven=False,production_original_execution=False,retained_large_artifacts=[])
    (args.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
