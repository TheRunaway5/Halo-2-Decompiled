#!/usr/bin/env python3
"""Compare12FE00 native caller transport with unchanged original execution."""
import argparse
import hashlib
import json
import os
import lzma
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace

ROOT=Path(__file__).resolve().parents[1]
for seed in sorted(ROOT.glob('.local/*/python')):
    if (seed/'capstone').is_dir():sys.path.insert(0,str(seed));break
import validate_native_array_growth_tu15 as reference
import native_lazy_heap_abi_inputs as inputs
import generate_physics_live_support_tu15 as generator
from native_weapon_effect_datum_bridge_tu15 import remove_native_effect_datum
from specialize_native_operands import leaf_body
from native_page_allocate_replacements import canonical_body as page_allocate_body
from native_datum_specific_replacements import canonical_body as specific_body,BODY_PINS as SPECIFIC_PINS
from native_datum_initialize_replacements import canonical_body as initialize_body,BODY_PINS as INITIALIZE_PINS

LISTING_PIN='12f8baf71f48d314efe83dcffe7c5ad5eae4302853d7c3c2cff6a6273d428fdd'


CHILDREN=(0x13fe50,0x32a660,0x13fe00,0x140310,0x16e6a0,0x16e8a0,0x16e770,0x16e860,0x2db4ca)


def recovered_body(image,scratch):
    code=reference._Code(image,'.text');body=reference.complete_body(code,0x12fe00)
    listing=''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(body.code))
    assert hashlib.sha256(listing.encode()).hexdigest()==LISTING_PIN
    sites={f'{pc:08x}':pc for pc in sorted(body.code)};addresses={}
    for pc in sorted(body.code):
        ins=code.at(pc);addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
        for operand in ins.operands:
            if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
            if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    entries=(0x12fe00,*CHILDREN);bodies={entry:reference.complete_body(code,entry)for entry in entries}
    for entry,child in bodies.items():
        assert not child.tables
        for pc in sorted(child.code):
            sites[f'{pc:08x}']=pc;ins=code.at(pc);addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
            for operand in ins.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=reference.NEW_PIN,entries={f'{entry:08x}':entry for entry in entries},
        body_sites={f'{entry:08x}':sorted(child.code)for entry,child in bodies.items()},instruction_map=sites,
        address_map={f'{p:08x}':q for p,q in addresses.items()},routines=[dict(entry=entry,tables=[],byte_tables=[])for entry in entries])
    pairing=scratch/'physical-pairing.json';pairing.write_text(json.dumps(paired))
    args=SimpleNamespace(output=scratch/'physical-reference.cpp',manifest=scratch/'physical-reference.json',
        correspondence=pairing,additional_entries=pairing)
    generator.generate(args,paired,complete_listings=True)
    reference_source=remove_native_effect_datum(args.output.read_text())
    recovered=leaf_body(reference_source,'12fe00')[0]
    start=recovered.index('{\n')+2
    recovered=recovered[:start]+('    if(native_physical_allocation_detail::run(s,[](auto &v,std::uint32_t owner,std::uint32_t site,bool tail){dispatch(v,owner,site,tail);}))return;\n'
        '    ++translated_fallbacks;\n')+recovered[start:]
    (scratch/'physical_allocation_body.inc').write_text(recovered+'\n')
    children=''.join(f'static void run_{entry:x}(WorldState &s);\n'for entry in CHILDREN)
    for entry in CHILDREN:
        child=leaf_body(reference_source,f'{entry:x}')[0]
        if entry==0x13fe50:
            child=page_allocate_body(child)
            assert hashlib.sha256(child.encode()).hexdigest()=='ce273066496b0ee1694f6ead6056091fa8c5bc1090bc56767f40c52552b57963'
        if entry in(0x16e770,0x16e860):
            # These remain genuine translated continuations in this caller
            # fixture; their separate qualified production owners are not
            # inherited merely because the common generator now binds them.
            child=(specific_body if entry==0x16e770 else initialize_body)(child)
            assert hashlib.sha256(child.encode()).hexdigest()in(SPECIFIC_PINS if entry==0x16e770 else INITIALIZE_PINS)
        start=child.index('{\n')+2
        child=child[:start]+f'    if(child_entry_observer)child_entry_observer(s,0x{entry:x}u);\n    if(stop_owners.contains(0x{entry:x}u)){{required_entry=0x{entry:x}u;throw RequiredOwner{{}};}}\n    ++reference_children[0x{entry:x}u];\n'+child[start:]
        children+=child+'\n'
    (scratch/'physical_allocation_children.inc').write_text(children)
    (scratch/'physical_allocation_dispatch.inc').write_text('switch(target){'+''.join(f'case 0x{entry:x}u:production::run_{entry:x}(s);return;'for entry in CHILDREN)+'default:required_entry=target;throw RequiredOwner{};}')
    return body


def physical_inputs(image):
    banks,provenance=inputs.load(reference,image)
    manifest=json.loads(lzma.decompress(reference.ARCHIVE.with_name('all-file-hashes.json.xz').read_bytes()))
    def read(name):
        raw=subprocess.check_output(['tar','--zstd','-xOf',str(reference.ARCHIVE),reference.MEMBER+name]);proof=manifest[reference.MEMBER+name]
        assert len(raw)==proof['bytes']and hashlib.sha256(raw).hexdigest()==proof['sha256'];return raw
    capture=json.loads(read('capture.json'));members={}
    def load(address):
        region=next(r for r in capture['regions']if r['address']<=address and address+4<=r['address']+r['size'])
        name=region['before_file'];raw=read(name);banks[region['address']]=raw;members[name]=manifest[reference.MEMBER+name]
        return reference.word(raw,address-region['address'])
    for address in (0x4f2d24,0x4f2d2c,0x4f2d30):
        value=load(address)
        if address==0x4f2d24:load(value)
    provenance['physical_members']=members
    provenance['genuine_physical_base']=next(reference.word(raw,0x4f2d30-first)for first,raw in banks.items()if first<=0x4f2d30 and 0x4f2d34<=first+len(raw))
    provenance['declared_backing_cases']='Only separately labelled cases replace physical base with the test-owned caller bank; algorithm computes addresses and outcomes.'
    return banks,provenance


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.report.parent.mkdir(parents=True,exist_ok=True)
    report=dict(passed=False,scratch_directory=str(scratch),retained_large_artifacts=[],production_bound=False)
    try:
        assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT)
        body=recovered_body(image,scratch);banks,provenance=physical_inputs(image)
        rows=[]
        for alignment in range(16):
            rows.append(dict(name=f'genuine_page_owner_entry_{alignment}',cohort='genuine-page-owner-entry',
                entry=0x12fe00,arguments=[(0,1,4096,0x800000,0xffffdfe0,0xffffffff)[alignment%6],0x12345678,0x87654321,0xa5a5a5a5],
                receiver=0x486080,header=reference.HEADER,full_abi=True,stack_pointer=reference.SP+alignment,
                direction_flag=bool(alignment&1),required_entries=[0x13fe50],
                input_registers=dict(eax=0x87654321,edx=0xc3c3c3c3,ebx=0xa5a5a5a5,ebp=0x11335577,esi=0x22446688,edi=0x99aabbcc)))
        for alignment in range(16):
            common=dict(entry=0x12fe00,receiver=0x486080,header=reference.HEADER,full_abi=True,
                stack_pointer=reference.SP+alignment,direction_flag=False)
            rows.append(dict(common,name=f'genuine_datum_owner_entry_{alignment}',cohort='genuine-page-algorithm',
                arguments=[4096,1,2,3],required_entries=[0x16e770]))
            rows.append(dict(common,name=f'declared_backing_protect_entry_{alignment}',cohort='declared-backing-real-page-datum',
                arguments=[4096,0x12345678,0x87654321,0xa5a5a5a5],input_words=[(0x4f2d30,reference.UI)],required_entries=[0x2db4ca]))
            rows.append(dict(common,name=f'declared_backing_zero_{alignment}',cohort='declared-backing-zero-protection',
                arguments=[0,4,5,6],input_words=[(0x4f2d30,reference.UI)]))
            rows.append(dict(common,name=f'declared_backing_zero_link_{alignment}',cohort='declared-backing-existing-list-zero-protection',
                arguments=[0,4,5,6],input_words=[(0x4f2d30,reference.UI),(0x4f2d2c,reference.UI+0x800)]))
            rows.append(dict(common,name=f'declared_page_refusal_{alignment}',cohort='declared-capacity-real-page-refusal',
                arguments=[4096,4,5,6],input_words=[(0x1017d048,0)]))
            if alignment&1:
                rows.append(dict(common,name=f'genuine_backward_page_fault_prefix_{alignment}',cohort='genuine-backward-page-fault-prefix',
                    arguments=[4096,1,2,3],direction_flag=True,stop_before_sites=[0x140277]))
        for row in rows:row['abi_entries']=[0x13fe50,0x16e770,0x2db4ca]
        reference.OWNERS=(0x12fe00,*CHILDREN);directory,authority=reference.original(image,banks,rows,scratch)
        fixture=directory/'physical-abi.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,cases=rows),separators=(',',':')))
        binary=scratch/'physical-allocation-abi';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_physical_allocation_abi_tests.cpp'),
            str(ROOT/'src/physics/native_tu15_physical_allocation.cpp'),'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        subprocess.run(command,check=True);modes={}
        for mode in ('native','reference'):
            env=dict(os.environ)
            for key in ('HALO2_REFERENCE_PHYSICAL_ALLOCATION','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_PARTITION_TEMPORARIES'):env.pop(key,None)
            if mode=='reference':env['HALO2_REFERENCE_PHYSICAL_ALLOCATION']='1'
            result=subprocess.run([str(binary),str(fixture)],env=env,text=True,capture_output=True)
            if result.returncode:report['comparison_errors']=result.stderr[-4000:];sys.stderr.write(result.stderr);result.check_returncode()
            modes[mode]=json.loads(result.stdout)
        files=[ROOT/'include/halo2/native_tu15_physical_allocation.hpp',ROOT/'src/physics/native_tu15_physical_allocation.cpp',
            ROOT/'src/physics/runtime/native_tu15_physical_allocation_abi.hpp',ROOT/'tests/native_tu15_physical_allocation_abi_tests.cpp',Path(__file__),
            Path(reference.__file__),ROOT/'tools/observe_native_array_growth_tu15.cpp',Path(generator.__file__),
            ROOT/'tools/native_weapon_effect_datum_bridge_tu15.py',ROOT/'tools/native_page_allocate_replacements.py',ROOT/'tools/native_datum_specific_replacements.py',ROOT/'tools/native_datum_initialize_replacements.py']
        files += [ROOT/name for name in ('src/physics/runtime/native_tu15_page_window_abi.hpp','src/physics/runtime/native_tu15_page_retire_abi.hpp',
            'src/physics/runtime/native_tu15_datum_delete_abi.hpp','include/halo2/native_tu15_page_window_kernel.hpp',
            'include/halo2/native_tu15_page_retire_kernel.hpp','include/halo2/native_tu15_datum_delete_kernel.hpp',
            'src/physics/runtime/native_tu15_stack_probe_abi.hpp','include/halo2/native_tu15_stack_probe_kernel.hpp')]
        executed={pc for row in rows for pc in row['executed_sites']}
        report.update(passed=True,modes=modes,input_provenance=provenance,source_authority=authority,
            original_instructions_unchanged=True,allocator_outcomes_supplied=False,native_outputs_used_by_original=False,
            body_coverage=dict(total=len(body.code),executed=len(executed&set(body.code)),unexecuted=sorted(set(body.code)-executed)),
            normal_return_qualified=False,child_owner_closure_qualified=False,exceptional_partial_abi_qualification=False,
            parent_normal_return_qualified_for_declared_inputs=True,normal_return_cases=sum(not row['required_owner']for row in rows),
            native_algorithm_scope='12FE00 caller only; authenticated canonical child continuations remain separately counted.',
            cohorts={name:sum(row['cohort']==name for row in rows)for name in sorted({row['cohort']for row in rows})},
            backward_page_clear_fault='Original DF-set datum initialization overwrites its metadata; stop before real140277 fault. This is an actual-child prefix, not a normal return.',
            address_undefined_sanitizers=args.sanitizers,source_sha256={str(p.relative_to(ROOT)):reference.sha(p)for p in files})
        report['recovered_reference_sha256']={name:reference.sha(scratch/name)for name in ('physical_allocation_body.inc','physical_allocation_children.inc')}
        print(json.dumps(report,indent=2));return 0
    except BaseException as error:report['error']=repr(error);raise
    finally:args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())
