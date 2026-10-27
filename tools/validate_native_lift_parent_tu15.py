#!/usr/bin/env python3
"""Compare native scalar seams inside untouched TU1.5 parent1DA750.

Original instructions execute only in the disposable offline i386 oracle.
The focused native project borrows authenticated, read-only dependency archives
from an active owned main build. Its new source/build/fixture stay in this
run_test.py owner's scratch; no frozen/main source or archive is changed.
"""
import argparse
import copy
import itertools
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from export_mp_physics_contact_lifecycle import segments
from validate_native_lift_power_tu15 import OWNERS,FS,THREAD,TLS_TABLE,TLS_SLOT,THREAD_DATA,ERRNO,GDT,SOURCES,sha
import oracle_phantom_force_tu15_20261011 as parent
import oracle_vehicle_motion_math_hardware_20261011 as physical

CALLS=(0x1dabbe,0x1daf99,0x1db2d0,0x1db4db)
SEMANTIC={0x1da530,0x1da54e,0x1da559,0x1da56e,0x1da5ae,0x1da5e1,0x1da643,0x1da6ea,0x32e9b7,0x32e9c3}
CORRESPONDENCE=ROOT/'analysis/multiplayer/sandbox-source/phantom-force-tu15-20261011/research-verified/report.json'
ARCHIVES=('environment_state','constraint_cleanup','contact_state','motion_math','motion','update','runtime')

def controls(quick):
    sample=next(row for row in parent.controls() if row['entry']==0x1da750 and row['coefficients']==1 and row['members']==1)
    rows=[]
    for coefficient,point,velocity,exponent,rounding,precision in itertools.product(range(1,5),(2.,-3.),(0.,0.5),(0.,0.5,3.),range(4),(0,2,3)):
        if quick and (point!=2. or velocity!=0.5 or rounding!=0):continue
        if quick and precision!=3:continue
        row=dict(sample,coefficients=coefficient,point=point,velocity=velocity,exponent=exponent,angular_exponent=exponent,fp_control=0x7f|(precision<<8)|(rounding<<10),mxcsr=0x1f80|(rounding<<13),top=len(rows)%8)
        rows.append(row)
    return rows

def original(rows):
    paired=json.loads(CORRESPONDENCE.read_text());image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    starts={int(line.split('\t')[0],16) for line in (ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').read_text().splitlines()[1:]}
    known=set(paired['instruction_map'].values());leaf_sites=set()
    for owner in OWNERS:
        trace=_trace(code,owner,starts);assert not trace.tables
        for pc in trace.code:
            if pc not in known:paired['instruction_map'][f'{pc:08x}']=pc;known.add(pc)
            leaf_sites.add(pc)
        if owner not in paired['entries'].values():paired['entries'][f'{owner:08x}']=owner
    records=((FS+0x24,0,1),(FS+0x28,THREAD,4),(THREAD+0x28,1,4),(0x48c920,1,4),(FS+4,TLS_TABLE,4),(TLS_TABLE+4,TLS_SLOT,4),(TLS_SLOT+0xc,THREAD_DATA,4),(0x573840,THREAD_DATA,4),(ERRNO,0x12345678,4))
    capture=parent.capture;captures=0
    def initialized(g):
        nonlocal captures
        if captures%2==0:
            index=captures//2
            def segment(base,access):return struct.pack('<II',0xffff|((base&0xffff)<<16),((base>>16)&255)|(access<<8)|0xcf0000|(base&0xff000000))
            g.mem_write(GDT+8,segment(0,0x9b)+segment(0,0x93)+segment(FS,0x93));g.reg_write(R.UC_X86_REG_GDTR,(0,GDT,0xfff,0))
            for reg,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):g.reg_write(reg,value)
            for p,value,size in records:g.mem_write(p,value.to_bytes(size,'little'))
            g.reg_write(R.UC_X86_REG_FPSW,(rows[index]['top']<<11)|((index&7)<<8)|((index&8)<<11)|(index*7%64))
        captures+=1;return capture(g)
    parent.capture=initialized
    try:cases=parent.execute(paired,rows)
    finally:parent.capture=capture
    assert captures==len(rows)*2
    cases=[c for c in cases if c['complete'] and any(w['instruction'] in CALLS for w in c['writes'])]
    assert cases,'No original parent reaches a scalar seam'
    for case in cases:
        data={b['address']+i:v for b in case['initial_banks'] for i,v in enumerate(bytes.fromhex(b['hex']))}
        # These are declared by the raw parent input constructor even when
        # its unmodified branch does not read gravity. They let the private
        # discarded-return control reach a real numeric/retained mismatch.
        declared=(*records,(0x52be68,parent.VECTOR+0x100,4),(parent.VECTOR+0x100,struct.unpack('<I',struct.pack('<f',9.8))[0],4))
        for p,value,size in declared:
            for i,v in enumerate(value.to_bytes(size,'little')):
                if p+i in data:assert data[p+i]==v
                else:data[p+i]=v
        case['initial_banks']=[dict(address=p,hex=b.hex()) for p,b in segments(data)]
    fixture=dict(image='1.5',cases=cases,original_sha256=NEW_PIN,native_expected_data_used=False)
    return fixture,paired,leaf_sites

def hardware(fixture,paired,directory):
    bridge=physical.ASM
    physical.ASM=bridge.replace('setup:\n',f'''setup:
 mov $123,%eax
 mov $1,%ebx
 mov $fs_descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 js failure
 mov $7,%eax
 mov %ax,%fs
''')+f'\n.align 4\nfs_descriptor: .long 0,{FS},0xfffff,0x51\n'
    try:return physical.execute(fixture,paired,NEW_DEFAULT,directory)
    finally:physical.ASM=bridge

def qualify(fixture,paired,leaf_sites,oracle):
    code=_Code(Xbe(NEW_DEFAULT),'.text');image=Xbe(NEW_DEFAULT);actual_to_canonical={v:int(k,16) for k,v in paired['instruction_map'].items()}
    leaf={actual_to_canonical[p] for p in leaf_sites};physical_cases={c['id']:c for c in oracle['cases']};supplements=[]
    for case in fixture['cases']:
        actual=physical_cases[case['id']]
        for key in ('exit','final_banks','instructions','writes'):case[key]=actual[key]
        case.update(stack_begin=parent.STACK-0x2000,stack_end=parent.STACK+0x400,scalar_calls=[0]*4)
        first={b['address']+i:v for b in case['initial_banks'] for i,v in enumerate(bytes.fromhex(b['hex']))}
        final={b['address']+i:v for b in case['final_banks'] for i,v in enumerate(bytes.fromhex(b['hex']))}
        # Authenticated absolute readonly operands on the actual physical path.
        # Unicorn only discovers footprint; its branch/transcendental result
        # cannot authorize a default read or define the expected arithmetic.
        for canonical in case['instructions']:
            instruction=code.at(paired['instruction_map'][f'{canonical:08x}'])
            if instruction.mnemonic=='lea':continue
            for operand in instruction.operands:
                if operand.type!=3 or operand.mem.base or operand.mem.index or operand.mem.segment or not (operand.access&1):continue
                p=operand.mem.disp;size=operand.size
                if not (0x400000<=p and p+size<=0x480000) or all(p+i in first for i in range(size)):continue
                raw=image.read(p,size)
                for i,v in enumerate(raw):
                    if p+i in first:assert first[p+i]==v
                    else:first[p+i]=final[p+i]=v
                supplements.append(dict(case=case['id'],instruction=canonical,address=p,hex=raw.hex()))
        active=None;private_final=set();semantic=[];private_before_masks=[]
        for event in case['writes']:
            pc=event['instruction'];p=event['address'];raw=bytes.fromhex(event['written'])
            if pc in CALLS:
                active=p;case['scalar_calls'][CALLS.index(pc)]+=1
            elif pc not in leaf:active=None
            private=active is not None and pc in leaf and p<active-4
            if private:assert parent.STACK-0x2000<=p and p+len(raw)<=active-4
            before_mask=sum(1<<i for i in range(len(raw)) if p+i in private_final)
            for i in range(len(raw)):
                if private:private_final.add(p+i)
                else:private_final.discard(p+i)
            if not private:
                assert active is None or pc in SEMANTIC or pc in CALLS,(hex(pc),hex(p),hex(active))
                semantic.append({k:event[k] for k in ('instruction','address','before','written')})
                private_before_masks.append(before_mask)
        case['semantic_writes']=semantic
        case['private_before_byte_masks']=private_before_masks
        case['original_private_leaf_final_bytes']=[dict(address=p,bytes=len(b)) for p,b in segments({p:0 for p in private_final})]
        case['initial_banks']=[dict(address=p,hex=b.hex()) for p,b in segments(first)]
        case['final_banks']=[dict(address=p,hex=b.hex()) for p,b in segments(final)]
    fixture['oracle_backend']='original_i386_hardware_single_step'
    return supplements

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--frozen-source',type=Path,required=True);parser.add_argument('--frozen-build',type=Path,required=True);parser.add_argument('--quick',action='store_true');parser.add_argument('--negative-control',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();args.output=args.output.resolve();args.frozen_source=args.frozen_source.resolve();args.frozen_build=args.frozen_build.resolve();args.output.mkdir(parents=True,exist_ok=False)
    paths=(*SOURCES,'src/physics/physics_phantom_force_tu15_generated.cpp','tests/native_lift_scalar_parent_tu15_tests.cpp')
    report=dict(passed=False,source_sha256={p:sha(ROOT/p) for p in paths},validator_sha256=sha(__file__),original_sha256=NEW_PIN,retained_large_artifacts=[],scratch_cleanup_owner=str(scratch),live_multiplayer_proven=False,pentium_iii_transcendental_approximation_bits_proven=False)
    def save():(args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    save()
    try:
        fixture,paired,leaf_sites=original(controls(args.quick));oracle=hardware(fixture,paired,scratch/'hardware');supplements=qualify(fixture,paired,leaf_sites,oracle)
        target=scratch/'original-parent.json';target.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(original_parent_calls=len(fixture['cases']),scalar_calls=[sum(c['scalar_calls'][i] for c in fixture['cases']) for i in range(4)],physical_instructions=oracle['single_stepped_original_instructions'],cpu_model=oracle['cpu_model'],oracle_sha256=sha(target),hardware_tool_sha256=oracle['tool_sha256'],trace_observer_sha256=oracle['trace_observer_sha256'],paired_instruction_bytes_changed=oracle['paired_instruction_bytes_changed'],physically_observed_readonly_supplements=supplements,
            semantic_stores=sum(len(c['semantic_writes']) for c in fixture['cases']),original_private_helper_final_bytes=sum(r['bytes'] for c in fixture['cases'] for r in c['original_private_leaf_final_bytes']),original_private_leaf_last_writer_before_bytes=sum(mask.bit_count() for c in fixture['cases'] for mask in c['private_before_byte_masks']),
            retained_bytes_compared=sum(len(bytes.fromhex(b['hex'])) for c in fixture['cases'] for b in c['final_banks'])-sum(r['bytes'] for c in fixture['cases'] for r in c['original_private_leaf_final_bytes']),complete_caller_fp_status_control_and_defined_flags_checked=True,caller_ratio_errno_and_retained_fields_checked=True,all_semantic_write_addresses_and_values_checked=True,private_numerical_helper_machine_postimage_claimed=False,volatile_return_gp_xmm_postimage_claimed=False)
        assert all(report['scalar_calls']),report['scalar_calls'];save()
        source=scratch/'source';shutil.copytree(args.frozen_source/'include',source/'include');(source/'src').mkdir()
        frozen_headers={}
        for path in (args.frozen_source/'src').glob('*.hpp'):
            shutil.copyfile(path,source/'src'/path.name);frozen_headers[str(path)]=sha(path)
        for path in paths:
            out=source/path;out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/path,out)
        force=source/'src/physics/physics_phantom_force_tu15_generated.cpp';force.write_text(force.read_text().replace('"physics/vehicles/physics_vehicle_motion_math_internal.hpp"','"physics_vehicle_motion_math_internal.hpp"'))
        archives=[args.frozen_build/f'libhalo2_physics_vehicle_{name}.a' for name in ARCHIVES];archive_hashes={str(p):sha(p) for p in archives};report.update(readonly_frozen_dependency_archives=archive_hashes,readonly_frozen_internal_headers=frozen_headers,focused_force_include_relocation_sha256=sha(force));save()
        units=['src/physics/physics_phantom_force_tu15_generated.cpp','src/physics/source_lift_scalar.cpp','src/physics/source_lift_power_tu15.cpp','src/physics/source_power_special_tu15.cpp','src/physics/source_math_error_tu15.cpp','src/physics/native_lift_scalar_consumer_tu15.cpp','tests/native_lift_scalar_parent_tu15_tests.cpp']
        (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(native_lift_parent_tu15 LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json REQUIRED)
add_executable(native_lift_parent_tu15_tests '''+' '.join(units)+''')
target_compile_features(native_lift_parent_tu15_tests PRIVATE cxx_std_20)
target_include_directories(native_lift_parent_tu15_tests PRIVATE include src)
target_compile_options(native_lift_parent_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(native_lift_parent_tu15_tests PRIVATE nlohmann_json::nlohmann_json "-Wl,--start-group" '''+' '.join('"'+str(p)+'"' for p in archives)+''' "-Wl,--end-group")
add_test(NAME native_lift_parent_tu15 COMMAND native_lift_parent_tu15_tests "'''+str(target)+'''")
''')
        def validate(label):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--output',str(args.output/label),'--scratch-root',str(scratch/'builds'),'--target','native_lift_parent_tu15_tests','--filter','^native_lift_parent_tu15$','--jobs','2','--test-jobs','1'];report.setdefault('commands',[]).append(command);save();return subprocess.run(command,check=False).returncode
        result=validate('native')
        assert all(sha(Path(p))==v for p,v in archive_hashes.items()),'A frozen dependency archive changed during read-only test'
        assert result==0,'Composed parent differs; inspect native/tests.log'
        if args.negative_control:
            text=force.read_text();old='s.xmm[0]=_mm_set_ss(result.result.velocity_delta);';assert text.count(old)==1
            force.write_text(text.replace(old,'s.xmm[0]=_mm_set_ss(0.f);',1))
            rejected=validate('negative-scalar-return');log=(args.output/'negative-scalar-return/tests.log').read_text()
            assert rejected!=0 and any(reason in log for reason in ('semantic store','caller retained bank','caller defined flags','complete caller FP')),'Original parent failed to reject the discarded native scalar result'
            report.update(negative_control_rejected=True,negative_control='private scalar return delta replaced with zero')
        report['passed']=True
    except BaseException as error:report['error']=str(error);raise
    finally:save()
    print(json.dumps(report,sort_keys=True))

if __name__=='__main__':main()
