#!/usr/bin/env python3
"""Execute unchanged TU1.5 3D3D0 on authored weapon world-model chains.

Use run_test.py. The explicit ABI is one object/header/table and raw authored
weapon/HLMT prefixes. No original caller, visibility, resource, render frame,
pickup eligibility, or game lifecycle is claimed. No callees or returns supplied.
"""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from inspect_multiplayer import Cache
from oracle_smg_firing_1_1 import Xbe
from original_xbox_pose_case import BASE, sha
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    scope=json.loads((BASE/'authored-scope.json').read_text());image=Xbe(NEW_DEFAULT)
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000)
    for section in image.sections:
        raw=image.section_bytes(section)
        if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
    tags,pool,headers,body,definition,model,stack,stop=(0x1000000,0x1100000,0x1101000,
        0x1102000,0x1103000,0x1104000,0x1108000,0x1200000)
    cpu.mem_map(tags,0x100000);cpu.mem_map(pool,0x10000);cpu.mem_map(stop,0x1000)
    def put(at,value):cpu.mem_write(at,struct.pack('<I',value))
    put(0x4ecbd0,pool);put(0x4f0414,tags);put(pool+0x44,headers);put(headers+8,body)
    sites={};reads=set()
    def instruction(machine,pc,size,_):
        assert 0x3d3d0<=pc<=0x3d41f
        raw=bytes(machine.mem_read(pc,size));assert raw==image.read(pc,size)
        sites[f'{pc:08x}']=raw.hex()
    def read(_machine,_access,at,size,_value,_data):
        # The world branch has no authority to inspect FP models/counts.
        assert not (at<0x4ce3f6 and at+size>0x4ce3f4)
        assert not (at<definition+0x2b0 and at+size>definition+0x2a8)
        reads.add((at,size))
    cpu.hook_add(UC_HOOK_CODE,instruction);cpu.hook_add(UC_HOOK_MEM_READ,read)
    counts=Counter();excluded=[];maps=[];checked=skipped=0
    for index,item in enumerate(scope['maps']):
        path=Path(item['path']);assert sha(path)==item['sha256'];cache=Cache(path)
        local=Counter()
        try:
            for row in item['weapons']:
                assert row['resident'];tag=cache.tags[int(row['tag']['datum'],16)];p=tag['offset']
                raw=bytes(cache.data[p:p+0x31c]);assert raw.hex()==row['root_hex']
                world=cache.u(p+0x38)
                if world==0xffffffff:
                    skipped+=1;local['caller_model_NONE_not_executed']+=1;continue
                hlmt=cache.tags[world];assert hlmt['group']=='hlmt' and hlmt['offset'] is not None
                prefix=bytes(cache.data[hlmt['offset']:hlmt['offset']+0x18])
                render=struct.unpack_from('<I',prefix,4)[0]
                cpu.mem_write(definition,raw);cpu.mem_write(model,prefix)
                datum=int(tag['datum'],16);put(body,datum)
                put(tags+(datum&65535)*16+8,definition);put(tags+(world&65535)*16+8,model)
                put(stack,stop);cpu.reg_write(UC_X86_REG_ESP,stack)
                cpu.reg_write(UC_X86_REG_EAX,0x12340000);cpu.reg_write(UC_X86_REG_ECX,0)
                cpu.emu_start(0x3d3d0,stop,count=64)
                assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+4
                count=cpu.reg_read(UC_X86_REG_EAX);assert count==int(render!=0xffffffff)
                fp=cache.block(p+0x2a8,16)
                selected=bool(fp and cache.u(fp[0]+4)!=0xffffffff and cache.u(fp[0]+12)!=0xffffffff)
                category='world_present_native_filter_admits'if selected else 'world_present_native_filter_excludes'
                if render==0xffffffff:category='no_world_render_model'
                counts[category]+=1;local[category]+=1;checked+=1
                if render!=0xffffffff and not selected:
                    excluded.append(dict(map_index=index,weapon=tag['name'],datum=tag['datum'],
                        model=hlmt['datum'],render=f'{render:08x}',original_world_model_count=count))
            maps.append(dict(path=str(path),sha256=item['sha256'],counts=dict(local)))
        finally:
            cache.data.close();cache.stream.close()
    assert checked+skipped==1110 and len(maps)==38
    assert len(excluded)==94 and len({r['weapon']for r in excluded})==27
    report=dict(passed=True,entry='3D3D0',original_xbe_sha256=NEW_PIN,
        executed_authored_model_chains=checked,caller_model_NONE_rows_not_executed=skipped,
        counts=dict(counts),maps=maps,excluded_authored_rows_with_original_world_count_one=excluded,
        distinct_excluded_weapon_paths=27,first_person_count_and_block_reads=0,
        executed_instruction_bytes=sites,read_sites=sorted(reads),
        original_instruction_patches=[],supplied_callee_results=[],original_callees_reached=[],
        original_whole_render_admission_executed=False,native_filter_corrected=False,
        visibility_resources_framebuffer_and_owner_eligibility_verified=False,full_weapon_parity=False,
        runner_scratch=os.environ['HALO2_TEST_SCRATCH'],tool_sha256=sha(Path(__file__)),
        authored_scope_sha256=sha(BASE/'authored-scope.json'),large_artifacts_retained=[])
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,executed_authored_model_chains=checked,counts=dict(counts),
        source_world_models_excluded_by_native_filter=len(excluded))),flush=True)


if __name__=='__main__':main()
