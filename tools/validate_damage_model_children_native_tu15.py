#!/usr/bin/env python3
"""Qualify native DC540 against original instructions and declared callback ABI.

No-call controls also execute unchanged original i386 on the CPU. Controls
that call D9110 declare a clobber-only callback fixture in both implementations;
they qualify call admission/order/ABI, never D9110 gameplay results. Actual
MCC entry/return comparison remains a separate gate before runtime admission.
"""
import argparse
import hashlib
import json
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
    def add(name,variants,children,**kw):
        rows.append(dict(id=len(rows),name=name,variants=variants,children=children,key=0x10203040,**kw))
    match=(0x10203040,0xa0100005);miss=(0x10203041,0xa0100005)
    selected=(0xa0100005,1<<26);other=(0xa0100006,1<<26);off=(0xa0100005,0)
    add('null_model',[],[],null_model=True)
    add('zero_variants',[],[])
    add('negative_variants',[],[],negative_variants=True)
    add('empty_children',[[]],[])
    add('negative_child_count',[[]],[],negative_children=True)
    add('wrong_key',[[miss]],[selected])
    add('null_definition',[[(match[0],0xffffffff)]],[selected])
    add('no_live_children',[[match]],[])
    add('child_flag_off',[[match]],[off])
    add('different_child_definition',[[match]],[other])
    add('selected_child',[[match]],[selected])
    add('mixed_siblings',[[match]],[off,selected,other,selected])
    add('mixed_rows',[[miss,match,(match[0],0xffffffff)]],[selected,off])
    add('mixed_variants',[[],[miss],[match],[match,match]],[selected,other])
    rng=random.Random(0xdc540)
    for n in range(40):
        variants=[]
        for v in range(rng.randrange(5)):
            variants.append([(rng.choice([match[0],match[0]+1,0,0xffffffff]),rng.choice([match[1],match[1]+1,0xffffffff]))for _ in range(rng.randrange(4))])
        children=[(rng.choice([match[1],match[1]+1]),rng.choice([0,1<<26,0xffffffff,1<<25,1<<27]))for _ in range(rng.randrange(6))]
        add('varied_'+str(n),variants,children)
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-source',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--mutate-child-gate',action='store_true',
        help='Reject a private mutation that bypasses the original bit26 gate')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);native_source=args.native_source.resolve(strict=True)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original TU1.5 changed')
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');body=_trace(code,0xdc540,{0xdc540})
    audit=ROOT/'analysis/destruction-goal-20261022/enforcer-child-owner-original-v1.json';authority=json.loads(audit.read_text())
    if not authority['passed']or len(body.code)!=88 or hashlib.sha256(b''.join(image.read(pc,body.code[pc])for pc in sorted(body.code))).hexdigest()!=authority['reachable_bytes_sha256']:
        raise ValueError('Complete original child body changed')
    registers=(UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI)
    xmm_registers=tuple(globals()['UC_X86_REG_XMM'+str(n)]for n in range(8))
    results=[];hardware=[];raw_removed_bytes=0
    for control in controls():
        (args.output/'progress.json').write_text(json.dumps(dict(owned_scratch=str(owner),current_case=control,
            original_sha256=NEW_PIN,completed_controls=len(results),completed_no_call_CPU_controls=len(hardware),
            native_kernel_sha256=sha(ROOT/'src/gameplay/objects/source_damage_model_children_native_tu15_internal.hpp')),indent=2)+'\n')
        uc=Uc(UC_ARCH_X86,UC_MODE_32);uc.mem_map(0x10000,0x800000)
        for section in image.sections:uc.mem_write(section.va,image.section_bytes(section))
        addresses=(0x4ec000,0x4f0000,*(0x10000000+n*0x1000 for n in range(9)))
        banks={at:bytearray([0x57])*0x1000 for at in addresses}
        def word(at,value):struct.pack_into('<I',banks[at&~0xfff],at&0xfff,value&0xffffffff)
        pool,headers,tags,definition,model,variants,child_rows,bodies,stack=(0x10000000+n*0x1000 for n in range(9))
        parent=0xe0010003;parent_definition=0xa0010001;model_definition=0xa0010002
        word(0x4ecbd0,pool);word(pool+0x44,headers);word(0x4f0414,tags)
        word(headers+3*12+8,bodies);word(bodies,parent_definition)
        word(tags+16+8,definition);word(definition+0x38,0xffffffff if control.get('null_model')else model_definition)
        word(tags+32+8,model);word(model+0x50,0xffffffff if control.get('negative_variants')else len(control['variants']));word(model+0x54,variants)
        for n,rows in enumerate(control['variants']):
            at=variants+n*0x38;word(at+0x1c,0xffffffff if control.get('negative_children')else len(rows));word(at+0x20,child_rows+n*0x100)
            for m,(key,child_definition)in enumerate(rows):word(child_rows+n*0x100+m*16,key);word(child_rows+n*0x100+m*16+12,child_definition)
        handles=[0xe1230004+n for n in range(len(control['children']))]
        word(bodies+0x10,handles[0]if handles else 0xffffffff)
        for n,((child_definition,flags),handle)in enumerate(zip(control['children'],handles)):
            at=bodies+0x100+n*0x80;word(headers+(handle&65535)*12+8,at)
            word(at,child_definition);word(at+4,flags);word(at+12,handles[n+1]if n+1<len(handles)else 0xffffffff)
        stop=0x60000000;sp=stack+0x800;word(sp,stop);word(sp+4,control['key'])
        for at,data in banks.items():
            if at>=0x10000000:uc.mem_map(at,0x1000)
            uc.mem_write(at,bytes(data))
        uc.mem_map(stop,0x1000)
        gp=[parent,0x99887766,0x12345678,0xabcdef98,sp,0xfeedbeef,0x13579bdf,0x2468ace0]
        xmm=[[((control['id']+1)*0x1020304+n*19+k)&0xffffffff for k in range(4)]for n in range(8)]
        for reg,value in zip(registers,gp):uc.reg_write(reg,value)
        for reg,values in zip(xmm_registers,xmm):uc.reg_write(reg,sum(value<<(32*n)for n,value in enumerate(values)))
        initial=dict(gp=gp,xmm=xmm,eflags=0x202,mxcsr=0x1f80,x87_control_word=0x37f,x87_status=0,x87_values=[],fs_base=0)
        uc.reg_write(UC_X86_REG_EFLAGS,initial['eflags']);uc.reg_write(UC_X86_REG_FPCW,initial['x87_control_word']);uc.reg_write(UC_X86_REG_MXCSR,initial['mxcsr'])
        instructions=[];accesses=[];calls=[];current=[0]
        def inspect(machine,pc,size,user):
            if pc==0xd9110:
                state=[machine.reg_read(reg)for reg in registers];at=state[4]
                returned,child=struct.unpack('<II',bytes(machine.mem_read(at,8)))
                calls.append(dict(gp=state,eflags=machine.reg_read(UC_X86_REG_EFLAGS),**{'return':returned},child=child))
                for reg,value in zip(registers[:3],(0x10325476,0x89abcdef,0x76543210)):machine.reg_write(reg,value)
                machine.reg_write(UC_X86_REG_ESP,at+8);machine.reg_write(UC_X86_REG_EFLAGS,0x293);machine.reg_write(UC_X86_REG_EIP,returned);return
            if pc not in body.code:raise ValueError('Original child escaped complete owner '+hex(pc))
            current[0]=pc;instructions.append(pc)
        def access(machine,kind,at,size,value,user):accesses.append(dict(instruction=current[0],kind='read'if kind==16 else'write',address=at,size=size))
        uc.hook_add(UC_HOOK_CODE,inspect);uc.hook_add(UC_HOOK_MEM_READ,access);uc.hook_add(UC_HOOK_MEM_WRITE,access)
        uc.emu_start(0xdc540,stop,count=20000)
        if uc.reg_read(UC_X86_REG_EIP)!=stop:raise ValueError('Original child did not return')
        after=dict(gp=[uc.reg_read(reg)for reg in registers],xmm=[[(uc.reg_read(reg)>>(32*n))&0xffffffff for n in range(4)]for reg in xmm_registers],
            eflags=uc.reg_read(UC_X86_REG_EFLAGS),mxcsr=uc.reg_read(UC_X86_REG_MXCSR),x87_control_word=uc.reg_read(UC_X86_REG_FPCW),x87_status=0,x87_values=[])
        after_banks=[dict(address=at,hex=bytes(uc.mem_read(at,len(data))).hex())for at,data in banks.items()]
        if not calls:
            capture=owner/'capture';oracle=owner/'original';capture.mkdir();regions=[]
            for n,((at,data),final)in enumerate(zip(banks.items(),after_banks)):
                before_file=str(n)+'-before.bin';after_file=str(n)+'-after.bin'
                (capture/before_file).write_bytes(data);(capture/after_file).write_bytes(bytes.fromhex(final['hex']))
                regions.append(dict(address=at,size=len(data),before_file=before_file,after_file=after_file))
            metadata=dict(reference='English TU1.5 only',input_kind='declared_component',after_origin='unchanged_original_unicorn',
                declared_callback_invocations=0,entry=0xdc540,**{'return':stop},before=initial,after=after,regions=regions,
                boundary='Declared model/child controls; unchanged complete original DC540 with no actual D9110 call in this control')
            (capture/'capture.json').write_text(json.dumps(metadata)+'\n');log=owner/'original.log'
            command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--component-inputs','--capture',str(capture),'--output',str(oracle)]
            with log.open('w')as stream:status=subprocess.run(command,stdout=stream,stderr=subprocess.STDOUT,timeout=60).returncode
            path=oracle/'report.json';proof=json.loads(path.read_text())if path.exists()else{}
            if status or not proof.get('passed'):
                (args.output/'failure.log').write_text(log.read_text()[-8000:])
                raise ValueError('Original CPU child control failed: '+log.read_text()[-4000:])
            hardware.append(dict(id=control['id'],name=control['name'],report=proof))
            after=proof['original_abi']
            for directory in (capture,oracle):
                raw_removed_bytes+=sum(p.stat().st_size for p in directory.rglob('*')if p.is_file());shutil.rmtree(directory)
            log.unlink()
        results.append(dict(id=control['id'],name=control['name'],before=initial,after=after,instructions=instructions,accesses=accesses,calls=calls,
            before_banks=[dict(address=at,hex=data.hex())for at,data in banks.items()],after_banks=after_banks))
    source=owner/'source';pins={}
    for parent in ('include','src'):
        for p in (native_source/parent).rglob('*.hpp'):
            name=p.relative_to(native_source);target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target);pins[str(name)]=sha(p)
    for name in ('include/halo2/source_damage_model_children_tu15.hpp','src/gameplay/objects/source_damage_model_children_native_tu15_internal.hpp',
                 'src/gameplay/objects/source_damage_model_children_tu15.cpp','tests/source_damage_model_children_tu15_tests.cpp'):
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target);pins[name]=sha(ROOT/name)
    mutation=None
    if args.mutate_child_gate:
        name='src/gameplay/objects/source_damage_model_children_native_tu15_internal.hpp';target=source/name;text=target.read_text()
        needle='                                        at(0xdc612);\n                                        if(!s.flags.zf) {'
        if text.count(needle)!=1:raise ValueError('Child gate mutation seam drifted')
        before=sha(target);target.write_text(text.replace(needle,needle.replace('if(!s.flags.zf)','if(true)')))
        pins[name]=sha(target);mutation=dict(before=before,after=pins[name],scope='Private bit26 bypass; original control/reference instructions unchanged')
    fixture=owner/'controls.json';fixture.write_text(json.dumps(dict(schema='H2_DAMAGE_MODEL_CHILDREN_ORIGINAL_V1',cases=results),separators=(',',':'))+'\n')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(Halo2DamageChildren LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_executable(child_tests tests/source_damage_model_children_tu15_tests.cpp src/gameplay/objects/source_damage_model_children_tu15.cpp)
target_compile_features(child_tests PRIVATE cxx_std_20)
target_include_directories(child_tests PRIVATE include src)
target_compile_options(child_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(child_tests PRIVATE nlohmann_json::nlohmann_json)
'''
    cmake+=f'add_test(NAME native_damage_model_children COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- $<TARGET_FILE:child_tests> "{fixture}" "${{CMAKE_BINARY_DIR}}/children.json")\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','child_tests','--filter','^native_damage_model_children$',
        '--jobs','1','--test-jobs','1','--output',str(args.output/'validation'),'--retain-report','children.json']
    report=dict(passed=False,owned_scratch=str(owner),native_source=str(native_source),tool_sha256=sha(__file__),source_hashes=pins,command=command,
        original_sha256=NEW_PIN,original_body_audit_sha256=sha(audit),original_hardware_runner_sha256=sha(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),
        cases=len(results),no_call_original_i386_cases=len(hardware),callback_boundary_cases=sum(bool(r['calls'])for r in results),
        callback_boundary_invocations=sum(len(r['calls'])for r in results),raw_original_comparison_bytes_removed=raw_removed_bytes,
        scope=__doc__,negative_mutation=mutation,runtime_route_admitted=False,full_goal_complete=False,retained_large_artifacts=[])
    def save():(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    (args.output/'original-hardware.json').write_text(json.dumps(dict(original_sha256=NEW_PIN,cases=hardware),indent=2)+'\n');save()
    status=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    proof=args.output/'validation/evidence/children.json';data=json.loads(proof.read_text())if proof.exists()else{}
    passed=status==0 and data.get('passed')is True
    if args.mutate_child_gate:
        worker=json.loads((args.output/'validation/report.json').read_text())
        red=next((row for row in data.get('results',[])if row['name']=='child_flag_off'),{})
        passed=status!=0 and all(step['exit_code']==0 for step in worker['steps']if step['name']!='tests')and \
            red.get('passed')is False and 'Child callee ABI/order differs'in red.get('errors',[])
        report['private_child_gate_mutation_rejected']=passed
    report.update(returncode=status,passed=passed,native=data);save()
    print(json.dumps({k:report[k]for k in ('passed','cases','no_call_original_i386_cases','callback_boundary_cases','callback_boundary_invocations')}),flush=True)
    return 0 if report['passed']else status or 1


if __name__=='__main__':raise SystemExit(main())
