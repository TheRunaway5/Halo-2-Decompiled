#!/usr/bin/env python3
"""Qualify actual Havok pool ABI using genuine initialized input and i386 CPU."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
# The physical observer uses the same offline disassembler seed as existing
# array proofs. Prefer a repository-owned seed when it is present.
for seed in sorted(ROOT.glob('.local/*/python')):
    if (seed/'capstone').is_dir():sys.path.insert(0,str(seed));break
import validate_native_array_growth_tu15 as reference
from specialize_native_operands import POOL_RELEASE_PIN,POOL_ALLOCATION_PIN,ARRAY_ALLOCATOR_ENTRIES,STACK_ARENA_ENTRIES,MAIN_HEAP_ENTRIES,MAIN_HEAP_ALLOCATION_ENTRIES,RAW_ALLOCATOR_ENTRIES,ALLOCATOR_ACTUAL_OWNER_BYTES,digest,leaf_body,specialize,CALL,CASE,MAPPING


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path);parser.add_argument('--sanitizers',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py to own hardware/compile outputs')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(dict(passed=False,scratch_directory=str(scratch),retained_large_artifacts=[]),indent=2)+'\n')
    source=root/'src/physics/runtime/physics_world_runtime_generated.cpp';canonical=source.read_text();specialized,receipt=specialize(canonical)
    assert set(receipt['native_pool_allocator_entries'])=={'233250','233420'}
    assert set(receipt['native_array_allocator_entries'])=={'149da0','149e00'}
    assert set(receipt['native_main_heap_entries'])=={'232c80','232d20'}
    assert set(receipt['native_main_heap_allocation_entries'])=={'232ad0','232d00','232d30'}
    assert digest(leaf_body(canonical,'22eb20')[0].replace('ObservedWorldState &s','STATE &s'))==POOL_RELEASE_PIN
    mapping={int(a,16):int(b,16) for a,b in CASE.findall(MAPPING.search(canonical)['cases'])}
    assert digest(leaf_body(canonical,'22e950')[0].replace('ObservedWorldState &s','STATE &s'))==POOL_ALLOCATION_PIN
    bodies=[]
    actual_entries={'22e950':0x233250,'22eb20':0x233420,'148a10':0x149da0,'148a70':0x149e00,
        '22e380':0x232c80,'22e420':0x232d20,'22e1d0':0x232ad0,'22e400':0x232d00,'22e430':0x232d30}
    for name,actual_entry in actual_entries.items():
        if name in ARRAY_ALLOCATOR_ENTRIES:assert digest(leaf_body(canonical,name)[0].replace('ObservedWorldState &s','STATE &s'))==ARRAY_ALLOCATOR_ENTRIES[name][0]
        if name in MAIN_HEAP_ENTRIES:assert digest(leaf_body(canonical,name)[0].replace('ObservedWorldState &s','STATE &s'))==MAIN_HEAP_ENTRIES[name][0]
        if name in MAIN_HEAP_ALLOCATION_ENTRIES:assert digest(leaf_body(canonical,name)[0].replace('ObservedWorldState &s','STATE &s'))==MAIN_HEAP_ALLOCATION_ENTRIES[name][0]
        body=leaf_body(specialized,name)[0];body=CALL.sub(lambda m:f'0x{mapping[int(m[1],16)]:x}u',body)
        body=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body)
        body,count=re.subn(r'(    if\(.*?return;\n)',r'\1    ++translated_fallbacks;\n',body,count=1,flags=re.S);assert count==1
        body=body.replace('{\n',f'{{\n    if(observe_entry)observe_entry(s,0x{actual_entry:x}u);\n',1)
        bodies.append(body)
    for path,name in [('src/physics/physics_live_support_tu15_generated.cpp','2e10a0'),('src/runtime/source_map_world_support_tu15_generated.cpp','2e1110')]:
        arena_canonical=(root/path).read_text();arena_specialized,arena_receipt=specialize(arena_canonical)
        assert name in arena_receipt['native_stack_arena_entries']
        original=leaf_body(arena_canonical,name);assert digest(original[0].replace(original[1]+' &s','STATE &s'))==STACK_ARENA_ENTRIES[name][0]
        arena_mapping={int(a,16):int(b,16) for a,b in CASE.findall(MAPPING.search(arena_canonical)['cases'])}
        body=leaf_body(arena_specialized,name)[0].replace(original[1]+' &s','ObservedWorldState &s')
        body=CALL.sub(lambda m:f'0x{arena_mapping[int(m[1],16)]:x}u',body)
        body,count=re.subn(r'(    if\(.*?return;\n)',r'\1    ++translated_fallbacks;\n',body,count=1,flags=re.S);assert count==1
        body=body.replace('{\n',f'{{\n    if(observe_entry)observe_entry(s,0x{name}u);\n',1)
        bodies.append(body)
    (scratch/'pool_allocator_abi_bodies.inc').write_text('namespace production {\n'+'\n'.join(bodies)+'\n}')
    assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN
    image=reference.Xbe(reference.NEW_DEFAULT);banks,provenance=reference.reference_inputs(image)
    raw_source=root/'src/physics/physics_live_support_tu15_generated.cpp';raw_canonical=raw_source.read_text()
    raw_specialized,raw_receipt=specialize(raw_canonical)
    assert set(raw_receipt['native_raw_allocator_entries'])==set(RAW_ALLOCATOR_ENTRIES)
    assert set(raw_receipt['native_allocator_actual_owner_entries'])=={f'{owner:x}' for owner in ALLOCATOR_ACTUAL_OWNER_BYTES}
    for owner,expected in ALLOCATOR_ACTUAL_OWNER_BYTES.items():assert image.read(owner,len(expected)//2).hex()==expected
    for name,(pin,_) in RAW_ALLOCATOR_ENTRIES.items():
        original=leaf_body(raw_canonical,name);assert digest(original[0].replace(original[1]+' &s','STATE &s'))==pin
        body=leaf_body(raw_specialized,name)[0].replace(original[1]+' &s','ObservedWorldState &s')
        body,count=re.subn(r'(    if\(.*?return;\n)',r'\1    ++translated_fallbacks;\n',body,count=1,flags=re.S);assert count==1
        body=body.replace('{\n',f'{{\n    if(observe_entry)observe_entry(s,0x{name}u);\n',1)
        bodies.append(body)
    (scratch/'pool_allocator_abi_bodies.inc').write_text('namespace production {\n'+'\n'.join(bodies)+'\n}')
    banks[0x46e33c]=image.read(0x46e33c,4)
    banks[0x46393c]=image.read(0x46393c,24)
    rows=[dict(c,full_abi=True) for c in reference.cases(False) if c['entry']==0x233250]
    for size in (0,1,4,8,16,32,128,256,512,513,1024,2048,4096,8192):
        rows.append(dict(name=f'actual_pool_release_abi_{size}',entry=0x233420,arguments=[0,size,0x12],
            receiver=0x10076040,header=reference.HEADER,prepare_bytes=max(size,1),prepare_argument=True,full_abi=True))
    import struct
    free_owner=struct.unpack('<I',banks[0x46e33c])[0]
    rows.append(dict(name='actual_oversize_crt_entry_boundary',entry=0x233420,arguments=[reference.DATA,8193,0x12],
        receiver=0x10076040,header=reference.HEADER,full_abi=True,required_entries=[free_owner]))
    for size in (0,1,4,16,128,512,513,1024,4096,8192):
        rows.append(dict(name=f'actual_sized_router_allocate_{size}',entry=0x149da0,arguments=[size,0x12],receiver=0x486080,header=reference.HEADER,full_abi=True))
    for size in (1,4,512,1024,8192):
        rows.append(dict(name=f'actual_sized_router_release_{size}',entry=0x149e00,arguments=[0,size,0x12],receiver=0x486080,
            header=reference.HEADER,prepare_bytes=size,prepare_argument=True,full_abi=True))
    rows.append(dict(name='actual_sized_router_release_null',entry=0x149e00,arguments=[0,4,0x12],receiver=0x486080,header=reference.HEADER,full_abi=True))
    for name,entry,call_args,required,changes in (
        ('secondary_allocation',0x149da0,[8193,0x12],0x232d00,[]),
        ('fixed_tail_allocation',0x149da0,[16,0x12],0x232d00,[(0x486064,1)]),
        ('default_release',0x149e00,[reference.DATA,16,0x12],0x149930,[])):
        rows.append(dict(name='actual_sized_router_boundary_'+name,entry=entry,arguments=call_args,receiver=0x486080,
            header=reference.HEADER,full_abi=True,required_entries=[required],input_words=changes))
    for alignment in range(1,16):
        for size in (4,1024):
            rows.append(dict(name=f'actual_sized_router_alignment_{alignment}_{size}',entry=0x149da0,arguments=[size,0x12],
                receiver=0x486080,header=reference.HEADER,full_abi=True,stack_pointer=reference.SP+alignment))
    for requested in (0,1,15,16,4095,4096,4097,8176,0x80000000,0xffffffff):
        rows.append(dict(name=f'actual_stack_arena_allocate_{requested:x}',entry=0x2e10a0,arguments=[requested],receiver=0x486080,header=reference.HEADER,full_abi=True))
        rows.append(dict(name=f'actual_stack_arena_release_{requested:x}',entry=0x2e1110,arguments=[0,requested],receiver=0x486080,
            header=reference.HEADER,full_abi=True,prepare_arena_bytes=requested))
    rows.append(dict(name='actual_stack_arena_secondary_entry',entry=0x2e10a0,arguments=[8192],receiver=0x486080,
        header=reference.HEADER,full_abi=True,required_entries=[0x232d00]))
    def get(p):return next(reference.word(raw,p-first) for first,raw in banks.items() if first<=p and p+4<=first+len(raw))
    fixed=0x10061000;secondary=0x10166410
    base,end,count=(get(fixed+off) for off in (0x18,0x1c,0x20))
    assert (base,end,count)==(0x10061030,0x15000,69) and get(secondary+0x20)==0
    selected_rows=[]
    for index in (0,1,34,68):
        record=base+end-8-index*8;data=base+get(record);size=get(record+4)
        selected_rows.append(dict(index=index,record=record,data=data,bytes=size))
        for entry,receiver,arguments in ((0x232c80,fixed,[data]),(0x232d20,fixed,[data,size,0x12]),(0x149e00,0x486080,[data,size,0x12])):
            rows.append(dict(name=f'actual_fixed_heap_release_{entry:x}_row_{index}',entry=entry,arguments=arguments,
                receiver=receiver,header=reference.HEADER,full_abi=True))
    for entry,receiver,arguments in ((0x232c80,fixed,[0xffffffff]),(0x232d20,fixed,[0xffffffff,16,0x12]),
        (0x232c80,secondary,[0x10002008]),(0x232d20,secondary,[0x10002008,16,0x12]),(0x149e00,0x486080,[0x10002008,16,0x12])):
        rows.append(dict(name=f'actual_heap_release_no_match_{entry:x}_{receiver:x}',entry=entry,arguments=arguments,
            receiver=receiver,header=reference.HEADER,full_abi=True))
    for count_input in (0,0xffffffff,0x80000000):
        rows.append(dict(name=f'actual_fixed_heap_release_signed_count_{count_input:x}',entry=0x232c80,arguments=[base],
            receiver=fixed,header=reference.HEADER,full_abi=True,input_words=[(fixed+0x20,count_input)]))
    for alignment in range(16):
        selected=selected_rows[alignment%len(selected_rows)]
        rows.append(dict(name=f'actual_fixed_heap_release_alignment_{alignment}',entry=0x149e00,
            arguments=[selected['data'],selected['bytes'],0x12],receiver=0x486080,header=reference.HEADER,
            full_abi=True,stack_pointer=reference.SP+alignment))
    import native_main_heap_allocation_abi_cases
    secondary_provenance=native_main_heap_allocation_abi_cases.extend(reference,image,banks,rows)
    import native_raw_allocator_abi_cases
    native_raw_allocator_abi_cases.extend(reference,rows,selected_rows)
    for row in rows:
        if row['entry'] in (0x232d00,0x232d20):row['raw_owner_transport']=True
        if 0x2e1030 in row.get('required_entries',[]):
            row['required_entries']=[0x1cb560 if owner==0x2e1030 else owner for owner in row['required_entries']]
            row['raw_owner_transport']=True
    reference.OWNERS=(0x149bf0,0x149c40,0x233160,0x2331a0,*ALLOCATOR_ACTUAL_OWNER_BYTES,
        0x232ad0,0x232d00,0x232d30,0x232c80,0x232d20,0x2e10a0,0x2e1110,*reference.OWNERS)
    directory,authority=reference.original(image,banks,rows,scratch)
    fixture=directory/'pool-abi.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,cases=rows),separators=(',',':')))
    binary=scratch/'pool_allocator_abi'
    command=['c++','-std=c++20','-O2','-ffp-contract=off','-fno-fast-math','-frounding-math',
        '-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(scratch),str(root/'tests/native_tu15_pool_allocator_abi_tests.cpp'),
        str(root/'src/physics/native_tu15_array_allocator.cpp'),str(root/'src/physics/native_tu15_stack_arena.cpp'),str(root/'src/physics/native_tu15_main_heap_release.cpp'),str(root/'src/physics/native_tu15_main_heap_allocate.cpp'),str(root/'src/physics/native_tu15_raw_allocator.cpp'),str(root/'src/physics/native_tu15_physical_epoch.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True);modes={}
    for mode in ('native','reference'):
        env=dict(os.environ);env.pop('HALO2_REFERENCE_POOL_ALLOCATOR',None);env.pop('HALO2_REFERENCE_MATRIX_LEAVES',None);env.pop('HALO2_REFERENCE_ARRAY_ALLOCATOR',None)
        env.pop('HALO2_REFERENCE_STACK_ARENA',None)
        env.pop('HALO2_REFERENCE_MAIN_HEAP',None)
        env.pop('HALO2_REFERENCE_RAW_ALLOCATOR',None)
        if mode=='reference':env['HALO2_REFERENCE_POOL_ALLOCATOR']='1';env['HALO2_REFERENCE_ARRAY_ALLOCATOR']='1';env['HALO2_REFERENCE_STACK_ARENA']='1';env['HALO2_REFERENCE_MAIN_HEAP']='1';env['HALO2_REFERENCE_RAW_ALLOCATOR']='1'
        result=subprocess.run([str(binary),str(fixture)],env=env,text=True,capture_output=True)
        if result.returncode:
            if args.report:args.report.write_text(json.dumps(dict(passed=False,failed_mode=mode,returncode=result.returncode,
                comparison_errors=result.stderr[-8000:],compiler_command=command,address_undefined_sanitizers=args.sanitizers,
                scratch_directory=str(scratch),retained_large_artifacts=[]),indent=2)+'\n')
            sys.stderr.write(result.stderr);result.check_returncode()
        modes[mode]=json.loads(result.stdout)
    paths=[source,root/'src/physics/runtime/native_tu15_pool_allocator_abi.hpp',root/'src/physics/runtime/native_tu15_array_allocator_abi.hpp',root/'include/halo2/native_tu15_array_allocator.hpp',
        root/'src/physics/runtime/native_tu15_stack_arena_abi.hpp',root/'include/halo2/native_tu15_stack_arena.hpp',root/'src/physics/native_tu15_stack_arena.cpp',
        root/'src/physics/runtime/native_tu15_main_heap_abi.hpp',root/'include/halo2/native_tu15_main_heap_release.hpp',root/'src/physics/native_tu15_main_heap_release.cpp',
        root/'src/physics/runtime/native_tu15_main_heap_allocate_abi.hpp',root/'include/halo2/native_tu15_main_heap_allocate.hpp',root/'src/physics/native_tu15_main_heap_allocate.cpp',root/'tools/native_main_heap_allocation_abi_cases.py',
        raw_source,root/'src/physics/runtime/native_tu15_raw_allocator_abi.hpp',root/'src/physics/runtime/native_tu15_allocator_owner_dispatch.hpp',root/'include/halo2/native_tu15_raw_allocator.hpp',root/'src/physics/native_tu15_raw_allocator.cpp',root/'tools/native_raw_allocator_abi_cases.py',root/'tests/native_tu15_allocator_actual_owner_reference.inc',
        root/'include/halo2/native_tu15_physical_epoch.hpp',root/'src/physics/native_tu15_physical_epoch.cpp',root/'src/physics/runtime/native_tu15_physical_epoch_abi.hpp',
        root/'src/physics/native_tu15_array_allocator.cpp',root/'tools/specialize_native_operands.py',root/'tools/validate_native_array_growth_tu15.py',
        root/'tools/observe_native_array_growth_tu15.cpp',root/'tests/native_tu15_pool_allocator_abi_tests.cpp',Path(__file__).resolve()]
    report=dict(passed=True,modes=modes,input_provenance=provenance,source_authority=authority,original_instructions_unchanged=True,
        allocator_outcomes_supplied=False,native_outputs_used_by_original=False,declared_test_allocators=False,
        source_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
        ordinary_pool_release_abi_qualified=True,pool_allocation_abi_qualified=True,oversize_crt_free_remains_explicit=True,
        sized_router_primary_pool_composition_qualified=True,fixed_secondary_release_composition_qualified=True,
        fixed_secondary_allocation_composition_qualified=True,lazy_default_and_alignment_owners_remain_explicit=True,genuine_fixed_ledger_rows=selected_rows,
        secondary_input_provenance=secondary_provenance,
        raw_routes_tagged_pool_and_fixed_secondary_composition_qualified=True,
        actual_owner_transport_bytes={f'{owner:x}':expected for owner,expected in ALLOCATOR_ACTUAL_OWNER_BYTES.items()},
        crt_import_1cb560_allocation_free_qualified=False,
        stack_arena_primary_pool_composition_qualified=True,
        oversized_import_slot_value=free_owner,production_oversized_import_binding_qualified=False,
        normal_return_cases=sum(not c['required_owner'] for c in rows),required_owner_entry_cases=sum(c['required_owner'] for c in rows),
        declared_required_entry_cases={hex(owner):sum(c['required_owner']and c['required_entry']==owner for c in rows)
            for owner in sorted({c['required_entry']for c in rows if c['required_owner']})},
        declared_entry_stop_protocol='Both physical observer and native/reference fixture stop before every required_entries target, including owners with metadata and implementations.',
        historical_known_owner_boundary_assertions_superseded=True,
        address_undefined_sanitizers=args.sanitizers,compiler_command=command,
        callee_entry_abi_comparisons=sum(len(c['entry_abis']) for c in rows),compared_eflags_mask=0xcd5,
        body_coverage={hex(owner):dict(total=len(body.code),executed=len(set(body.code)&{pc for c in rows for pc in c['executed_sites']}))
            for owner in (0x149da0,0x149e00,0x233250,0x233420,0x2e10a0,0x2e1110,0x232c80,0x232d20,0x232ad0,0x232d00,0x232d30,0x149bf0,0x149c40,0x233160,0x2331a0,*ALLOCATOR_ACTUAL_OWNER_BYTES)
            for body in [reference.complete_body(reference._Code(image,'.text'),owner)]},
        exceptional_partial_abi_qualification=False,live_frame_qualification=False,
        scratch_directory=str(scratch),retained_large_artifacts=[])
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
