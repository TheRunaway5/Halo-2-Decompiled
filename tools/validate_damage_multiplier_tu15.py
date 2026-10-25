#!/usr/bin/env python3
"""Audit the native scalar kernel against unchanged original D8110 execution.

Declared table inputs are component controls. Unicorn findings require the
independent original i386 CPU check before they establish FP equivalence.
"""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import random
import shutil
import struct
import subprocess
import sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def controls():
    rows=[]
    def add(name,groups,keys):
        rows.append(dict(id=len(rows),name=name,groups=[dict(name=key,modifiers=[dict(name=armor,bits=bits)for armor,bits in mods])for key,mods in groups],keys=keys,mxcsr=0x1f80,x87_control_word=0x37f))
    add('empty',[],[1,2,10,11])
    add('missing_group',[(1,[(10,0x3f000000)])],[2,3,10,11])
    add('missing_armor',[(1,[(10,0x3f000000)])],[1,2,11,12])
    add('repeated_keys',[(1,[(10,0x3f000000)])],[1,1,10,10])
    add('duplicate_groups',[(1,[(10,v)])for v in (0x3e800000,0x3f000000,0x40000000)],[1,1,10,10])
    add('duplicate_modifiers',[(1,[(10,v)for v in (0x3e800000,0x3f000000,0x40000000)])],[1,1,10,10])
    special=[0,0x80000000,0x3f800001,0xbf800000,0x7f7fffff,1,0x00800000,
        0x7f800000,0xff800000,0x7fc12345,0x7fc54321,0xffc12345,0x7f812345,0xff812345]
    for value in special:
        add('single_'+hex(value),[(1,[(10,value)])],[1,2,10,11])
    for left,right in [(0x7fc12345,0x7fc54321),(0x7fc54321,0x7fc12345),(0xffc12345,0x7fc54321),
        (0x7f812345,0x7fc54321),(0x7fc12345,0x7f812345),(0x7f7fffff,0x40000000),
        (1,0x3f000000),(0x00800000,0x3f000000),(0x80000000,0xbf800000)]:
        add('pair_'+hex(left)+'_'+hex(right),[(1,[(10,left),(11,right)])],[1,2,10,11])
    rng=random.Random(0xd8110)
    for case in range(48):
        groups=[]
        for key in sorted(rng.choices(range(8),k=rng.randrange(8))):
            mods=[(armor,rng.choice([0,0x80000000,0x3e800000,0x3f000000,0x3f800001,0x40000000,0xbf800000]))for armor in sorted(rng.choices(range(8),k=rng.randrange(6)))]
            groups.append((key,mods))
        keys=[rng.randrange(10)for _ in range(4)]
        if case%3==0 and groups:
            groups=[(rng.getrandbits(32),[(rng.getrandbits(32),value)for _,value in mods])for _,mods in groups]
            keys[0]=rng.choice(groups)[0]
        add('search_'+str(case),groups,keys)
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--hardware',action='store_true',help='Use actual original i386 results as the native scalar expectations')
    parser.add_argument('--reference',type=Path,help='Replay a retained hardware-derived scalar reference')
    parser.add_argument('--reverse-product',action='store_true',help='Reject a private swapped-operand mutation')
    parser.add_argument('--whole-owner',action='store_true',help='Also qualify both native D8110 observer APIs against fresh original i386 banks/ABI')
    parser.add_argument('--owner-reference',type=Path,help='Sealed original i386 owner banks/ABI for --whole-owner with --reference')
    parser.add_argument('--native-source',type=Path,default=ROOT,help='Native source cohort to freeze; test/oracle tools stay in this workspace')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py with oracle-capable Python')
    if args.reference and args.hardware:parser.error('Choose fresh hardware or a retained reference')
    if args.whole_owner and not(args.hardware or args.reference and args.owner_reference):
        parser.error('Whole-owner qualification requires fresh --hardware or both original references')
    if args.owner_reference and not(args.whole_owner and args.reference):parser.error('Owner reference requires --whole-owner and --reference')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    native_source=args.native_source.resolve(strict=True)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original TU1.5 changed')
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');entries={0xd8110,0xd8100,0x1408b0}
    bodies={entry:_trace(code,entry,entries)for entry in entries}
    if len(bodies[0xd8110].code)!=64 or len(bodies[0xd8100].code)!=5:raise ValueError('Original multiplier/callback changed')
    sites=set().union(*(set(body.code)for body in bodies.values()))
    rows=controls();owner_cases=[];captures=owner/'component-inputs';captures.mkdir()
    canonical_map={}
    if args.whole_owner:
        from oracle_smg_firing_1_1 import PIN
        from research_mp_sandbox_update_functions import full_tokens
        old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
        if sha(old_path)!=PIN:raise ValueError('Canonical observer authority changed')
        old=Xbe(old_path);old_code=_Code(old,'.text')
        for canonical,actual in ((0xd71c0,0xd8110),(0x124440,0xd8100),(0x13f540,0x1408b0)):
            original=_trace(old_code,canonical,{0xd71c0,0x124440,0x13f540})
            a=sorted(original.code);b=sorted(bodies[actual].code)
            if len(a)!=len(b)or full_tokens([old_code.at(p)for p in a],old)!=full_tokens([code.at(p)for p in b],image):
                raise ValueError('Whole material observer correspondence changed')
            canonical_map.update({str(p):q for p,q in zip(a,b)})
    reference=None
    if args.reference:
        reference=json.loads(args.reference.read_text());hwpath=args.reference.parent/'original-hardware.json'
        if reference['schema']!='H2_DAMAGE_MULTIPLIER_TU15_V1' or reference['original_sha256']!=NEW_PIN or sha(hwpath)!=reference['original_hardware_sha256']:
            raise ValueError('Original i386 scalar reference changed')
        hardware_rows=json.loads(hwpath.read_text())['cases']
        if len(rows)!=len(reference['cases'])or len(rows)!=len(hardware_rows):raise ValueError('Reference control count changed')
        for declared,recorded,original in zip(rows,reference['cases'],hardware_rows):
            if any(declared[key]!=recorded[key]for key in declared)or declared['id']!=original['id']or declared['name']!=original['name']:
                raise ValueError('Declared controls differ from original hardware inputs')
            if recorded['expected_bits']!=original['original_abi']['xmm'][0][0]or recorded['expected_mxcsr']!=original['original_abi']['mxcsr']:
                raise ValueError('Scalar expectations differ from independent original i386')
        rows=reference['cases']
        if args.owner_reference:
            receipt_path=args.owner_reference.with_name('original-owner-reference-retention.json')
            retention=json.loads(receipt_path.read_text())
            if sha(args.owner_reference)!=retention['sha256']:raise ValueError('Original owner reference changed')
            raw=lzma.decompress(args.owner_reference.read_bytes())
            if hashlib.sha256(raw).hexdigest()!=retention['raw_sha256']:raise ValueError('Original owner uncompressed reference changed')
            original_owner=json.loads(raw);owner_cases=original_owner['cases']
            hardware_path=args.owner_reference.parent/'original-hardware.json'
            if sha(hardware_path)!=retention['original_hardware_sha256']:raise ValueError('Original owner hardware provenance changed')
            originals=json.loads(hardware_path.read_text())['cases']
            if len(owner_cases)!=len(rows)or original_owner['canonical_observation_map']!=canonical_map:
                raise ValueError('Original owner controls/correspondence changed')
            for row,case,original in zip(rows,owner_cases,originals):
                if row['id']!=case['id']or row['name']!=case['name']or case['after']!=original['original_abi']or case['after']['xmm'][0][0]!=row['expected_bits']or case['after']['mxcsr']!=row['expected_mxcsr']:
                    raise ValueError('Original owner reference does not match independent hardware ABI')
    registers=(UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI)
    xmm_registers=(UC_X86_REG_XMM0,UC_X86_REG_XMM1,UC_X86_REG_XMM2,UC_X86_REG_XMM3,UC_X86_REG_XMM4,UC_X86_REG_XMM5,UC_X86_REG_XMM6,UC_X86_REG_XMM7)
    for row in rows:
        if reference:continue
        memory=Uc(UC_ARCH_X86,UC_MODE_32);memory.mem_map(0x10000,0x800000)
        for section in image.sections:memory.mem_write(section.va,image.section_bytes(section))
        addresses=(0x46a000,0x4ec000,*(0x10000000+n*0x1000 for n in range(7)))
        banks={address:bytearray([0x57])*0x1000 for address in addresses}
        banks[0x46a000][:]=image.read(0x46a000,0x1000)
        def word(address,value):struct.pack_into('<I',banks[address&~0xfff],address&0xfff,value&0xffffffff)
        word(0x4ecc1c,0x10000000);word(0x100000d4,0x10001000)
        word(0x10001000,len(row['groups']));word(0x10001004,0x10002000)
        for index,group in enumerate(row['groups']):
            at=0x10002000+index*12;modifiers=0x10003000+index*0x100
            word(at,group['name']);word(at+4,len(group['modifiers']));word(at+8,modifiers)
            for n,modifier in enumerate(group['modifiers']):word(modifiers+n*8,modifier['name']);word(modifiers+n*8+4,modifier['bits'])
        stop=0x70000000;sp=0x10006800;word(sp,stop);word(sp+4,row['keys'][2]);word(sp+8,row['keys'][3])
        for address,data in banks.items():
            if address>=0x10000000:memory.mem_map(address,0x1000)
            memory.mem_write(address,bytes(data))
        memory.mem_map(stop,0x1000)
        gp=[0xcafef00d,row['keys'][0],row['keys'][1],0xbadc0ffe,sp,0xd00dcafe,0xf1234567,0xe7654321]
        xmm=[[((row['id']+1)*0x1020304+n*13+k)&0xffffffff for k in range(4)]for n in range(8)]
        for register,value in zip(registers,gp):memory.reg_write(register,value)
        for register,values in zip(xmm_registers,xmm):memory.reg_write(register,sum(value<<(32*n)for n,value in enumerate(values)))
        memory.reg_write(UC_X86_REG_EFLAGS,0x202);memory.reg_write(UC_X86_REG_FPCW,row['x87_control_word']);memory.reg_write(UC_X86_REG_MXCSR,row['mxcsr'])
        visited=[];events=[];current_pc=[0]
        def observe(uc,address,size,user):
            if address==stop:return
            if address not in sites:raise ValueError('Original multiplier reached an unowned instruction '+hex(address))
            current_pc[0]=address
            visited.append(address)
        def access(uc,kind,address,size,value,user):
            events.append(dict(instruction=current_pc[0],kind='read'if kind==16 else'write',address=address,size=size))
        memory.hook_add(UC_HOOK_CODE,observe)
        if args.whole_owner:
            memory.hook_add(UC_HOOK_MEM_READ,access);memory.hook_add(UC_HOOK_MEM_WRITE,access)
        memory.emu_start(0xd8110,stop,count=20000)
        if memory.reg_read(UC_X86_REG_EIP)!=stop:raise ValueError('Original multiplier did not return')
        row.update(expected_bits=memory.reg_read(UC_X86_REG_XMM0)&0xffffffff,expected_mxcsr=memory.reg_read(UC_X86_REG_MXCSR),original_instructions=len(visited),original_multiply_count=visited.count(0xd81b1))
        # Preserve declared controls only inside the disposable owner. These
        # are not actual live capture inputs or native expected GP results.
        capture=captures/str(row['id']);capture.mkdir();regions=[]
        for n,(address,data)in enumerate(banks.items()):
            before=str(n)+'-before.bin';after=str(n)+'-after.bin';(capture/before).write_bytes(data);(capture/after).write_bytes(bytes(memory.mem_read(address,len(data))))
            regions.append(dict(address=address,size=len(data),before_file=before,after_file=after))
        initial=dict(gp=gp,xmm=xmm,eflags=0x202,mxcsr=row['mxcsr'],x87_control_word=row['x87_control_word'],x87_status=0,x87_values=[],fs_base=0)
        after=dict(gp=[memory.reg_read(register)for register in registers],xmm=[[(memory.reg_read(register)>>(32*n))&0xffffffff for n in range(4)]for register in xmm_registers],eflags=memory.reg_read(UC_X86_REG_EFLAGS),mxcsr=memory.reg_read(UC_X86_REG_MXCSR),x87_control_word=memory.reg_read(UC_X86_REG_FPCW),x87_status=0,x87_values=[])
        metadata=dict(reference='English TU1.5 only',input_kind='declared_component',after_origin='unchanged_original_unicorn',entry=0xd8110,**{'return':stop},before=initial,after=after,regions=regions,boundary='Declared component tables and caller ABI; original D8110/binary search/comparator executed unchanged; no live gameplay input or native expected result supplied')
        (capture/'capture.json').write_text(json.dumps(metadata,indent=2)+'\n')
        if args.whole_owner:
            owner_cases.append(dict(id=row['id'],name=row['name'],before=initial,instructions=visited,accesses=events,
                before_banks=[dict(address=a,hex=data.hex())for a,data in banks.items()]))
    hardware=[]
    if args.hardware:
        for row in rows:
            oracle=owner/('original-i386-'+str(row['id']));log=owner/'original-i386.log'
            command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--component-inputs',
                '--capture',str(captures/str(row['id'])),'--output',str(oracle)]
            with log.open('w')as stream:code_result=subprocess.run(command,stdout=stream,stderr=subprocess.STDOUT,timeout=90).returncode
            path=oracle/'report.json'
            if not path.exists():raise ValueError('Original i386 component failed: '+log.read_text()[-4000:])
            original=json.loads(path.read_text())
            if not original.get('original_execution_completed')or original['original_sha256']!=NEW_PIN:
                raise ValueError('Original i386 did not complete its unchanged component owner')
            actual=original['original_abi']
            hardware.append(dict(id=row['id'],name=row['name'],original_instructions=original['original_instructions'],
                compared_memory_bytes=original['compared_memory_bytes'],unicorn_full_state_matches=original['passed'],
                unicorn_differences=original['differences'],original_abi=actual,returncode=code_result,
                metadata_sha256=sha(captures/str(row['id'])/'capture.json')))
            row.update(unicorn_expected_bits=row['expected_bits'],unicorn_expected_mxcsr=row['expected_mxcsr'],
                expected_bits=actual['xmm'][0][0],expected_mxcsr=actual['mxcsr'])
            if args.whole_owner:
                case=owner_cases[row['id']];case['after']=actual
                metadata=json.loads((captures/str(row['id'])/'capture.json').read_text())
                with (oracle/'output.bin').open('rb')as raw:
                    if len(raw.read(276))!=276:raise ValueError('Truncated original i386 ABI')
                    case['after_banks']=[]
                    for bank in metadata['regions']:
                        data=raw.read(bank['size'])
                        if len(data)!=bank['size']:raise ValueError('Truncated original i386 bank')
                        case['after_banks'].append(dict(address=bank['address'],hex=data.hex()))
                    if raw.read(1):raise ValueError('Unexpected original i386 data suffix')
            shutil.rmtree(oracle)
            if row['id']%16==0:print(json.dumps(dict(hardware_cases_completed=len(hardware),cases=len(rows))),flush=True)
        log.unlink(missing_ok=True)
        (args.output/'original-hardware.json').write_text(json.dumps(dict(original_sha256=NEW_PIN,
            scope='Declared component controls executed on original i386; independent of native scalar results',cases=hardware),indent=2)+'\n')
    fixture=owner/'multiplier-cases.json';fixture.write_text(json.dumps(dict(schema='H2_DAMAGE_MULTIPLIER_TU15_V1',cases=rows),separators=(',',':'))+'\n')
    source=owner/'source';paths=['src/core/kernels.cpp','tests/damage_multiplier_tu15_tests.cpp']
    paths+=[str(path.relative_to(native_source))for path in native_source.joinpath('include').rglob('*')if path.is_file()];pins={}
    if args.whole_owner:
        paths+=['src/gameplay/units/source_damage_delivery_runtime_generated.cpp','tests/source_damage_material_native_tu15_tests.cpp']
        paths+=[str(path.relative_to(native_source))for path in native_source.joinpath('src').rglob('*.hpp')if path.is_file()]
    for name in paths:
        origin=(ROOT if name.startswith('tests/')else native_source)/name
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);before=sha(origin);shutil.copyfile(origin,target)
        if sha(target)!=before or sha(origin)!=before:raise ValueError('Native scalar source drift while freezing')
        pins[name]=before
    mutation=None
    if args.reverse_product:
        path=source/'src/core/kernels.cpp';text=path.read_text();needle='result = damage_product(modifier->multiplier, result);'
        if text.count(needle)!=1:raise ValueError('Ordered product mutation seam changed')
        before=sha(path);path.write_text(text.replace(needle,'result = damage_product(result, modifier->multiplier);'))
        pins['src/core/kernels.cpp']=sha(path);mutation=dict(before=before,after=sha(path),scope='Private operand reversal; original hardware reference unchanged')
        if args.whole_owner:
            path=source/'src/gameplay/objects/source_damage_material_native_tu15_internal.hpp';text=path.read_text()
            needle='detail::damage_product(modifier,accumulated)'
            if text.count(needle)!=1:raise ValueError('Material owner mutation seam changed')
            path.write_text(text.replace(needle,'detail::damage_product(accumulated,modifier)'))
            pins[str(path.relative_to(source))]=sha(path)
    (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(Halo2DamageMultiplier LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_executable(damage_multiplier_tu15_tests tests/damage_multiplier_tu15_tests.cpp src/core/kernels.cpp)
target_compile_features(damage_multiplier_tu15_tests PRIVATE cxx_std_20)
target_include_directories(damage_multiplier_tu15_tests PRIVATE include)
target_compile_options(damage_multiplier_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(damage_multiplier_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
'''+f'add_test(NAME native_damage_multiplier_tu15 COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- $<TARGET_FILE:damage_multiplier_tu15_tests> "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/damage-multiplier.json")\n')
    if args.whole_owner:
        owner_fixture=owner/'material-owner-cases.json'
        owner_fixture.write_text(json.dumps(dict(schema='H2_DAMAGE_MATERIAL_NATIVE_ORIGINAL_V1',canonical_observation_map=canonical_map,cases=owner_cases),separators=(',',':'))+'\n')
        if args.hardware:
            raw=owner_fixture.read_bytes();compressed=lzma.compress(raw)
            if len(compressed)>1024*1024 or lzma.decompress(compressed)!=raw:raise ValueError('Owner reference exceeds bounded retention')
            path=args.output/'original-owner-reference.json.xz';path.write_bytes(compressed)
            receipt=dict(required_reference_input=True,reason='Unchanged original i386 ABI and declared banks for D8110 native regressions',
                path=str(path),bytes=len(compressed),uncompressed_bytes=len(raw),sha256=sha(path),
                raw_sha256=hashlib.sha256(raw).hexdigest(),original_hardware_sha256=sha(args.output/'original-hardware.json'),
                cases=len(owner_cases),retained_compiled_test_binaries=False)
            (args.output/'original-owner-reference-retention.json').write_text(json.dumps(receipt,indent=2)+'\n')
        with (source/'CMakeLists.txt').open('a')as cmake:
            cmake.write('''add_executable(source_damage_material_native_tu15_tests tests/source_damage_material_native_tu15_tests.cpp src/gameplay/units/source_damage_delivery_runtime_generated.cpp)
target_compile_features(source_damage_material_native_tu15_tests PRIVATE cxx_std_20)
target_include_directories(source_damage_material_native_tu15_tests PRIVATE include src)
target_compile_options(source_damage_material_native_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(source_damage_material_native_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
add_custom_target(material_qualification DEPENDS damage_multiplier_tu15_tests source_damage_material_native_tu15_tests)
''')
            for route in ('selected','canonical'):
                cmake.write(f'add_test(NAME native_damage_material_{route} COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- $<TARGET_FILE:source_damage_material_native_tu15_tests> "{owner_fixture}" "{route}" "${{CMAKE_CURRENT_BINARY_DIR}}/damage-material-{route}.json")\n')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','damage_multiplier_tu15_tests','--filter','^native_damage_multiplier_tu15$','--jobs','2','--test-jobs','1','--output',str(args.output/'validation'),'--retain-report','damage-multiplier.json']
    if args.whole_owner:
        command[command.index('damage_multiplier_tu15_tests')]='material_qualification'
        command[command.index('^native_damage_multiplier_tu15$')]='^native_damage_(multiplier_tu15|material_)'
        for route in ('selected','canonical'):command+=['--retain-report',f'damage-material-{route}.json']
    (args.output/'source-hashes.json').write_text(json.dumps(pins,indent=2)+'\n')
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    native_path=args.output/'validation/evidence/damage-multiplier.json';native=json.loads(native_path.read_text())if native_path.exists()else{}
    worker=json.loads((args.output/'validation/report.json').read_text())
    differences=[row for row in native.get('results',[])if not row['passed']]
    def nan(bits):return bits&0x7f800000==0x7f800000 and bits&0x7fffff!=0
    rejected=bool(result!=0 and all(step['exit_code']==0 for step in worker['steps']if step['name']!='tests')and differences and
        all(row['expected_bits']!=row['actual_bits']and nan(row['expected_bits'])and nan(row['actual_bits'])and
            row['expected_mxcsr']==row['actual_mxcsr']for row in differences))
    report=dict(passed=rejected if args.reverse_product else result==0,component_oracle='Unchanged original D8110 plus original1408B0 search and D8100 callback on i386 hardware'if args.hardware or reference else'Unchanged original D8110 plus original1408B0 search and D8100 callback under offline Unicorn',original_sha256=NEW_PIN,original_native_expectations_supplied=False,owner_instruction_counts={hex(entry):len(body.code)for entry,body in bodies.items()},cases=len(rows),native_differences=native.get('differences'),native_report=str(native_path),source_unchanged=all(sha(source/name)==digest for name,digest in pins.items()),owned_scratch=str(owner),command=command,returncode=result,hardware_crosscheck_required=not(args.hardware or reference),hardware_cases=len(rows)if reference else len(hardware),hardware_reference=dict(path=str(args.reference),sha256=sha(args.reference))if reference else None,negative_control=mutation,swapped_product_rejected=rejected if args.reverse_product else None,full_gameplay_or_whole_abi_acceptance=False,retained_large_artifacts=[],tool_sha256=sha(__file__))
    if args.whole_owner:
        owner_reports=[]
        for route in ('selected','canonical'):
            path=args.output/'validation/evidence'/f'damage-material-{route}.json'
            owner_reports.append({k:v for k,v in json.loads(path.read_text()).items()if k!='results'}if path.exists()else dict(passed=False,route=route))
        report['native_owner_routes']=owner_reports
        report['whole_owner_component_ABI_and_banks_qualified']=not args.reverse_product and result==0 and all(row['passed']for row in owner_reports)
        report['scope']='Full original hardware final ABI and banks; unchanged emulator instruction/access-address-size order; intermediate FP access bytes and invalid-memory faults not qualified; no live gameplay acceptance'
        report['original_owner_reference']=dict(path=str(args.owner_reference),sha256=sha(args.owner_reference))if args.owner_reference else receipt
        if args.reverse_product:
            report['passed']=report['passed']and all(row.get('product_mutation_rejected',False)for row in owner_reports)
    report['native_source']=str(native_source)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
    return 0 if report['passed']else result or 1


if __name__=='__main__':raise SystemExit(main())
