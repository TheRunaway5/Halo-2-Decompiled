#!/usr/bin/env python3
"""Authenticate the real2660F0 writable default-point pointer and backing."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir();image=Xbe(NEW_DEFAULT);assert sha(NEW_DEFAULT)==NEW_PIN
    slot,target=0x474ce8,0x44abec
    assert image.read(slot,4)==struct.pack('<I',target)
    assert image.read(target,12)==bytes(12)
    pointer_section=next(s for s in image.sections if s.va<=slot<s.va+s.vsize)
    target_section=next(s for s in image.sections if s.va<=target<s.va+s.vsize)
    assert pointer_section.name=='.data'and pointer_section.flags==7
    assert target_section.name=='.rdata'and target_section.flags==6
    code=_Code(image,'.text');body=complete_body(code,0x2660f0);reads=[];listing=[]
    for pc in sorted(body.code):
        instruction=code.at(pc);listing.append(f'{pc:x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n')
        if any(operand.type==3 and operand.mem.disp==slot for operand in instruction.operands):
            reads.append(dict(instruction=hex(pc),bytes=instruction.bytes.hex(),operation=instruction.mnemonic,operands=instruction.op_str))
    assert len(reads)==1 and reads[0]['operation']=='mov'
    sources=['src/platform/native_tu15_world_memory.cpp','src/rendering/renderer/native_tu15_renderer_cache_pools_data.cpp','tests/native_tu15_world_memory_tests.cpp','tests/native_tu15_cold_data_tests.cpp','tests/helpers/native_campaign_perception_default_ownership.hpp','include/halo2/native_campaign_ai_perception_group_storage_tu15.hpp','src/campaign/ai/native_campaign_ai_perception_group_storage_tu15.cpp','tools/audit_native_campaign_perception_default_tu15.py']
    report=dict(passed=True,scope=__doc__,original_sha256=NEW_PIN,pointer=dict(address=hex(slot),initial_bytes=image.read(slot,4).hex(),target=hex(target),section=pointer_section.name,section_flags=pointer_section.flags,access='ReadWrite',existing_owner='Source12560 authored default render vector binding'),backing=dict(address=hex(target),initial_bytes=image.read(target,12).hex(),section=target_section.name,section_flags=target_section.flags,access='ReadOnly',existing_owner_reused=True),actual_consumer=reads,consumer_listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),source_hashes={name:sha(ROOT/name)for name in sources},behavior_changes='Correct the existing Source12560 pointer owner from ReadOnly to original ReadWrite; preserve its bits and shared readonly target. Native kernel reads the pointer lazily.',whole_world_validation_pending=True,production_original_binary_execution=False,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=True,pointer=hex(slot),target=hex(target),writable_pointer=True,readonly_zero_vector_bytes=12)))
if __name__=='__main__':main()
