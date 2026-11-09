#!/usr/bin/env python3
"""Unchanged-original complete physical epoch CPU-family ABI proof."""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace

ROOT=Path(__file__).resolve().parents[1]
for seed in sorted(ROOT.glob('.local/*/python')):
    if (seed/'capstone').is_dir():sys.path.insert(0,str(seed));break
import validate_native_array_growth_tu15 as reference
from validate_native_physical_allocation_abi_tu15 import physical_inputs
import generate_physics_live_support_tu15 as generator
from specialize_native_operands import leaf_body

IMAGES={0x12f040:(122,'ab1d322d207234f005c2eb22e9f29da5aa35e7d0990dc92e0eb03da160d3889c'),
    0x12f0c0:(73,'a60b8f3171bddd5813033c2adbd04502bae86a9b4bb2556372a35162918fd8fb'),
    0x13fd10:(120,'f6eee0fd7c7a53a33a058ab7903d7ef6b713e896f7d570f5f7e5a93012bbea29')}


def recover(image,scratch):
    code=reference._Code(image,'.text');bodies={entry:reference.complete_body(code,entry)for entry in IMAGES};addresses={};sites={}
    for entry,(length,pin)in IMAGES.items():assert hashlib.sha256(image.read(entry,length)).hexdigest()==pin
    for body in bodies.values():
        assert not body.tables
        for pc in body.code:
            sites[f'{pc:08x}']=pc;ins=code.at(pc);addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
            for operand in ins.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=reference.NEW_PIN,entries={f'{entry:08x}':entry for entry in IMAGES},
        body_sites={f'{entry:08x}':sorted(body.code)for entry,body in bodies.items()},instruction_map=sites,
        address_map={f'{p:08x}':q for p,q in addresses.items()},routines=[dict(entry=entry,tables=[],byte_tables=[])for entry in IMAGES])
    pairing=scratch/'epoch-pairing.json';pairing.write_text(json.dumps(paired));args=SimpleNamespace(output=scratch/'epoch-reference.cpp',manifest=scratch/'epoch-reference.json',correspondence=pairing,additional_entries=pairing)
    generator.generate(args,paired,complete_listings=True);source=args.output.read_text()
    declarations=''.join(f'static void run_{entry:x}(WorldState &s);\n'for entry in IMAGES);functions=declarations;pure=declarations
    for entry in IMAGES:
        body=leaf_body(source,f'{entry:x}')[0];pure+=body+'\n';start=body.index('{\n')+2
        functions+=body[:start]+(f'    if(child_entry_observer)child_entry_observer(s,0x{entry:x}u);\n'
            f'    if(run_epoch(s,0x{entry:x}u))return;\n'
            f'    ++translated_fallbacks[0x{entry:x}u];\n')+body[start:]+'\n'
    (scratch/'physical_epoch_body.inc').write_text(functions)
    (scratch/'physical_epoch_reference.inc').write_text(pure)
    (scratch/'physical_epoch_dispatch.inc').write_text('switch(target){'+''.join(f'case 0x{entry:x}u:production::run_{entry:x}(s);return;'for entry in IMAGES)+'default:required_entry=target;throw RequiredOwner{};}')
    return bodies


def cases():
    rows=[]
    def add(name,entry,alignment,cohort,words=(),pool=0x1017d000):
        rows.append(dict(name=name,entry=entry,arguments=[],receiver=0x486080,header=reference.HEADER,full_abi=True,
            stack_pointer=reference.SP+alignment,direction_flag=bool(alignment&1),abi_entries=list(IMAGES),cohort=cohort,input_words=list(words),
            input_registers=dict(eax=pool,edx=0xc3c3c3c3,ebx=0xa5a5a5a5,ebp=0x11335577,esi=0x22446688,edi=0x99aabbcc)))
    for alignment in range(16):
        for entry in IMAGES:add(f'genuine_{entry:x}_{alignment}',entry,alignment,'genuine-empty-arrays')
        for counter,epoch in ((0,0),(0xffffffff,0),(0,0x7fffffff),(0xffffffff,0x7fffffff)):
            add(f'declared_rollovers_{counter:x}_{epoch:x}_{alignment}',0x12f0c0,alignment,'declared-rollovers-real-empty-arrays',[(0x4f2d58,counter),(0x1017d038,epoch)])
    profiles=((0,0,0,False,False),(1,0,0,False,True),(1,1,0,False,True),(3,5,0,False,True),
        (35,0x80000002,4,False,True),(64,0x80000001,0x80000001,False,True),(0x80000000,0,0,False,True),
        (1,1,0,True,True),(3,5,0,True,True))
    for profile,(count,bits0,bits1,zero_stride,enabled)in enumerate(profiles):
        words=[(0x4f2d24,reference.UI+0x100),(0x4f2d34,reference.UI+0x200),(0x4f2d58,0xffffffff),
            (reference.UI+0x238,0x7fffffff),(reference.UI+0x264,reference.UI+0x300)]
        for array,records,bitmap,stride in ((reference.UI+0x100,reference.UI+0x1000,reference.UI+0x800,32),
            (reference.UI+0x300,reference.UI+0x2000,reference.UI+0x900,24)):
            words.extend(((array+0x24,0 if zero_stride else stride),(array+0x29,int(enabled)),(array+0x38,count),
                (array+0x44,0 if zero_stride else records),(array+0x48,bitmap),(bitmap,bits0),(bitmap+4,bits1)))
        stamps=(0,0x7ffffefe,0x7ffffeff,0x7fffff00,0x7ffffffe,0x7fffffff,0xffffffff)
        for index,slot in enumerate((0,1,2,31,32,34,63)):
            obj=reference.UI+0x4000+slot*0x100;words.extend(((reference.UI+0x1000+slot*32+16,0 if slot==1 else obj),
                (reference.UI+0x2000+slot*24+20,stamps[index])))
        for alignment in (0,1,7,15):
            for entry in IMAGES:add(f'declared_records_{profile}_{entry:x}_{alignment}',entry,alignment,'declared-owned-record-inputs',words,reference.UI+0x200)
        if profile==2:
            for alignment in (0,1,7,15):
                sp=reference.SP+alignment
                for offset in (4,8):add(f'saved_reset_{offset}_{alignment}',0x12f040,alignment,'declared-saved-frame-alias',
                    [*words,(reference.UI+0x1010,sp-offset-0x54)],reference.UI+0x200)
                for offset in (4,8,12):add(f'saved_rebase_{offset}_{alignment}',0x13fd10,alignment,'declared-saved-frame-alias',
                    [*words,(reference.UI+0x344,sp-offset-20)],reference.UI+0x200)
                add(f'saved_child_policy_{alignment}',0x12f0c0,alignment,'declared-native-child-saved-policy-alias',
                    [*words,(0x4f2d58,0),(reference.UI+0x344,sp-24-20)],reference.UI+0x200)
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,scratch_directory=str(scratch),production_bound=False,retained_large_artifacts=[])
    try:
        assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);bodies=recover(image,scratch);banks,provenance=physical_inputs(image)
        binding=None
        if args.require_binding:
            import re
            from specialize_native_operands import specialize,PHYSICAL_EPOCH_ACTUAL_OWNER_IMAGES
            assert PHYSICAL_EPOCH_ACTUAL_OWNER_IMAGES==IMAGES
            source=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';text=source.read_text();specialized,receipt=specialize(text)
            assert set(receipt.get('native_physical_epoch_actual_owner_entries',[]))=={f'{entry:x}'for entry in IMAGES}
            context=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',text)[0]
            for changed in (context.replace('switch(target)','switch(target+0u)',1),context.replace('s.required(site,target,tail)','s.required(site,target^1u,tail)',1)):
                assert changed!=context
                try:specialize(text.replace(context,changed))
                except ValueError as error:assert str(error)=='Complete13FE50 dispatcher drift'
                else:raise AssertionError('Changed complete dispatcher escaped bound page-owner authentication')
            binding=dict(canonical_sha256=reference.sha(source),specialized_sha256=hashlib.sha256(specialized.encode()).hexdigest(),
                actual_entries=receipt['native_physical_epoch_actual_owner_entries'],complete_dispatch_drift_rejections=2)
        manifest=json.loads(lzma.decompress(reference.ARCHIVE.with_name('all-file-hashes.json.xz').read_bytes()))
        capture=json.loads(subprocess.check_output(['tar','--zstd','-xOf',str(reference.ARCHIVE),reference.MEMBER+'capture.json']))
        region=next(r for r in capture['regions']if r['address']<=0x4f2d58 and 0x4f2d5c<=r['address']+r['size']);member=reference.MEMBER+region['before_file']
        raw=subprocess.check_output(['tar','--zstd','-xOf',str(reference.ARCHIVE),member]);assert len(raw)==manifest[member]['bytes']and hashlib.sha256(raw).hexdigest()==manifest[member]['sha256'];banks[region['address']]=raw
        provenance['epoch_counter_member']={region['before_file']:manifest[member]}
        rows=cases();reference.OWNERS=tuple(IMAGES);directory,authority=reference.original(image,banks,rows,scratch)
        fixture=directory/'epoch-abi.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,cases=rows),separators=(',',':')))
        binary=scratch/'physical-epoch';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_physical_epoch_abi_tests.cpp'),str(ROOT/'src/physics/native_tu15_physical_epoch.cpp'),'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        if args.require_binding:command+=['-DHALO2_EPOCH_WITH_OWNER_DISPATCH',*[str(ROOT/'src/physics'/name)for name in (
            'native_tu15_array_allocator.cpp','native_tu15_stack_arena.cpp','native_tu15_main_heap_release.cpp','native_tu15_main_heap_allocate.cpp','native_tu15_raw_allocator.cpp')]]
        subprocess.run(command,check=True);modes={}
        for mode in (('native','reference','raw-reference-isolation','tick-reference-isolation')if args.require_binding else('native','reference')):
            env=dict(os.environ)
            for key in ('HALO2_REFERENCE_PHYSICAL_EPOCHS','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_PARTITION_TEMPORARIES'):env.pop(key,None)
            env.pop('HALO2_REFERENCE_RAW_ALLOCATOR',None);env.pop('HALO2_REFERENCE_TICK_CONVERSION',None)
            if mode=='reference':env['HALO2_REFERENCE_PHYSICAL_EPOCHS']='1'
            if mode=='raw-reference-isolation':env['HALO2_REFERENCE_RAW_ALLOCATOR']='1'
            if mode=='tick-reference-isolation':env['HALO2_REFERENCE_TICK_CONVERSION']='1'
            result=subprocess.run([str(binary),str(fixture)],env=env,text=True,capture_output=True)
            if result.returncode:report['comparison_errors']=result.stderr[-4000:];sys.stderr.write(result.stderr);result.check_returncode()
            modes[mode]=json.loads(result.stdout)
        executed={pc for row in rows for pc in row['executed_sites']};files=[ROOT/'include/halo2/native_tu15_physical_epoch.hpp',ROOT/'src/physics/native_tu15_physical_epoch.cpp',
            ROOT/'src/physics/runtime/native_tu15_physical_epoch_abi.hpp',ROOT/'tests/native_tu15_physical_epoch_abi_tests.cpp',Path(__file__),Path(reference.__file__),
            ROOT/'tools/observe_native_array_growth_tu15.cpp',Path(generator.__file__),ROOT/'tools/validate_native_physical_allocation_abi_tu15.py']
        if args.require_binding:files.extend((ROOT/'src/physics/runtime/native_tu15_allocator_owner_dispatch.hpp',ROOT/'tools/specialize_native_operands.py'))
        report.update(passed=True,modes=modes,input_provenance=provenance,source_authority=authority,original_instructions_unchanged=True,
            production_bound=args.require_binding,actual_owner_binding=binding,
            algorithm_outcomes_supplied=False,external_platform_services=False,
            retained_memory_exception_protocol_qualified=all(mode['retained_faults']['prefixes']>0 for mode in modes.values()),
            exception_qualification_scope='Pinned canonical SourcePhysicsContactMemory read/write faults, with and without store preimage observers; full partial ABI/access/store/banks/live80-bit/cursor. Not physical instruction or page-fault atomicity.',
            physical_instruction_page_fault_atomicity_qualified=False,
            body_coverage={hex(entry):dict(total=len(body.code),executed=len(executed&set(body.code)))for entry,body in bodies.items()},
            address_undefined_sanitizers=args.sanitizers,source_sha256={str(p.relative_to(ROOT)):reference.sha(p)for p in files},
            recovered_reference_sha256=reference.sha(scratch/'physical_epoch_body.inc'))
        print(json.dumps(report,indent=2));return 0
    except BaseException as error:report['error']=repr(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())
