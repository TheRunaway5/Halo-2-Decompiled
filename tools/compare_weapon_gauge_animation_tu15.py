#!/usr/bin/env python3
"""Compare actual plasma-rifle gauge banks with unchanged TU1.5 shader owners.

Use run_test.py while the named coherent validation build is still owned by
its runner. Headers and libraries are copied into this probe's scratch first.
No render callback, game-function result or original instruction is replaced.
"""
import argparse
import collections
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import struct
import subprocess
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
from original_xbox_pose_case import ROOT, sha
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from inspect_multiplayer import Cache
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, containing, full_tokens


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--validation',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    output=args.output.absolute();output.mkdir(exist_ok=False)
    report=dict(passed=False,runner_scratch=str(scratch),large_artifacts_retained=[])
    def save():(output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    save()
    validation=json.loads(args.validation.read_text())
    source=Path(validation['private_source'])
    build=Path(json.loads((args.validation.parent/'release/report.json').read_text())['build'])
    manifest=json.loads((args.validation.parent/'source-manifest.json').read_text())['source_files_sha256']
    for name in ('src/source_weapon_material_input.cpp','include/halo2/source_weapon_material_input.hpp',
                 'src/source_weapon_shader_input.cpp','src/shader_animation.cpp'):
        assert sha(source/name)==manifest[name]==sha(ROOT/name),name
    shutil.copytree(source/'include',scratch/'include')
    tokens=shlex.split((build/'CMakeFiles/source_weapon_material_input_tests.dir/link.txt').read_text())
    libraries=[];library_pins={}
    for token in tokens[tokens.index('-o')+2:]:
        assert not token.startswith('-'),token
        path=Path(token) if Path(token).is_absolute() else build/token
        library_pins[str(path)]=sha(path)
        if path.is_relative_to(build):
            copy=scratch/'libraries'/path.name;copy.parent.mkdir(exist_ok=True)
            shutil.copyfile(path,copy)
            assert sha(copy)==sha(path)==library_pins[str(path)]
            path=copy
        libraries.append(str(path))
    native_source=ROOT/'tests/source_weapon_material_input_tests.cpp'
    shutil.copyfile(native_source,scratch/'native.cpp')
    command=[tokens[0],'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off',
        '-I'+str(scratch/'include'),str(scratch/'native.cpp'),'-fuse-ld=lld',
        '-Wl,--start-group',*libraries,'-Wl,--end-group','-o',str(scratch/'native')]
    report.update(compile_command=command,library_files_sha256=library_pins,
        native_test_source_sha256=sha(native_source),snapshot_manifest_sha256=sha(args.validation.parent/'source-manifest.json'))
    report['original_gauge_fixture_sha256']=sha(ROOT/'tests/fixtures/weapon_gauge_animation_tu15.json')
    save()
    result=subprocess.run(command,capture_output=True,text=True)
    (output/'compile.log').write_text((result.stdout+result.stderr)[-65536:])
    assert result.returncode==0,result.stderr[-2000:]
    run=[str(scratch/'native'),str(ROOT/'assets/disc/maps/lockout.map'),
        str(ROOT/'tests/fixtures/weapon_material_builtins_tu15.json'),str(output/'native-acceptance.json')]
    result=subprocess.run(run,capture_output=True,text=True,timeout=60)
    (output/'native.log').write_text(result.stdout+result.stderr)
    report.update(native_command=run,native_exit_code=result.returncode,native_binary_sha256=sha(scratch/'native'))
    save();assert result.returncode==0,result.stderr
    vector=json.loads((output/'native-acceptance.json').read_text())['gauge_reference_vectors']
    assert len(vector['initial'])==len(vector['native'])==len(vector['legacy'])==188
    retail=ROOT/'assets/disc/default.xbe'
    assert sha(retail)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    assert sha(NEW_DEFAULT)==NEW_PIN
    old,new=Xbe(retail),Xbe(NEW_DEFAULT)
    old_code,new_code=_Code(old,'.text'),_Code(new,'.text')
    correspondence={};globals_map={}
    def recover(entry):
        body=_trace(old_code,entry,{entry})
        original=[old_code.at(pc) for pc in sorted(body.code)]
        pattern=[]
        for instruction in original[:12]:
            wild=set()
            for operand in instruction.operands:
                if operand.type==2 and ((instruction.mnemonic.startswith('j') or instruction.mnemonic=='call') or containing(old,operand.imm)):
                    wild.update(range(instruction.imm_offset,instruction.imm_offset+instruction.imm_size))
                if operand.type==3 and containing(old,operand.mem.disp):
                    wild.update(range(instruction.disp_offset,instruction.disp_offset+instruction.disp_size))
            pattern.extend(b'.' if i in wild else re.escape(bytes([value])) for i,value in enumerate(instruction.bytes))
        section=new.section('.text');candidates=[]
        for match in re.finditer(b''.join(pattern),new.section_bytes(section),re.S):
            target=section.va+match.start()
            try:
                other=_trace(new_code,target,{target})
                current=[new_code.at(pc) for pc in sorted(other.code)]
                if full_tokens(original,old)==full_tokens(current,new):candidates.append((target,current))
            except (AssertionError,KeyError,ValueError):continue
        assert len(candidates)==1,(hex(entry),[hex(row[0])for row in candidates])
        target,current=candidates[0]
        for before,after in zip(original,current):
            for a,b in zip(before.operands,after.operands):
                if a.type==3 and containing(old,a.mem.disp):
                    previous=globals_map.setdefault(a.mem.disp,b.mem.disp)
                    assert previous==b.mem.disp
        correspondence[f'{entry:08x}']=dict(tu15_entry=target,complete_instructions=len(original),
            normalized_complete_body_equal=True)
        return target
    entries={entry:recover(entry) for entry in (0x17550,0x176a0,0x17960,0x1b230,0xbe8b0)}
    assert 0x485a58 in globals_map and 0x4858a0 in globals_map
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    try:
        shader=cache.tags[vector['shader']]
        assert shader['name']=='objects\\weapons\\rifle\\plasma_rifle\\shaders\\plasma_rifle_gauge'
        definition=cache.block(shader['offset']+32,124)[0]
        animation_pass=cache.block(definition+44,2)[vector['template_pass']]
        animated=cache.block(definition+52,10)[(cache.h(animation_pass)&511)+vector['implementation']]
        stem=cache.tags[cache.u(definition)]
        template=cache.block(stem['offset']+88,40)[0]
        template_pass=cache.block(template+16,10)[vector['template_pass']]
        spas=cache.tags[cache.u(template_pass+4)]
        pass_definition=cache.block(spas['offset']+28,88)[0]
        implementation=cache.block(pass_definition,306)[vector['implementation']]
        weapon=next(row for row in cache.tags.values() if row['group']=='weap' and
            row['name']=='objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle')
        machine=Uc(UC_ARCH_X86,UC_MODE_32);machine.mem_map(0x10000,0x800000)
        for section in new.sections:machine.mem_write(section.va,new.section_bytes(section))
        base=cache.magic&~4095;size=(cache.magic+len(cache.data)-base+4095)&~4095
        machine.mem_map(base,size);machine.mem_write(cache.magic,bytes(cache.data))
        machine.mem_map(0x1000000,0x10000);machine.mem_map(0x2000000,0x10000)
        state=0x2100000;stack=0x2008000;stop=0x3000000
        machine.mem_map(state,0x20000);machine.mem_map(stop,4096)
        def put(address,*words):machine.mem_write(address,struct.pack('<'+'I'*len(words),*words))
        put(0x4ecbd0,0x1000000);put(0x1000044,0x1001000)
        machine.mem_write(0x1001000,struct.pack('<HBBHHI',1,0,2,0xffff,0x340,0x1002000))
        machine.mem_write(0x10020aa,b'\x02')
        put(0x1002000,int(weapon['datum'],16));put(0x1002014,0xffffffff);put(0x10020b4,0xffffffff)
        put(0x1002180,vector['heat_bits'],vector['age_bits'])
        put(0x4f0414,0x1003000)
        put(0x1003008+(int(weapon['datum'],16)&65535)*16,cache.magic+weapon['offset'])
        put(globals_map[0x485a58],0x100e000);put(0x100e000,0x10000,entries[0xbe8b0])
        put(state+12,cache.magic+definition)
        put(state+32,cache.magic+pass_definition,cache.magic+implementation)
        put(state+0x300,*vector['shader_switches'])
        put(state+0x1424,*vector['initial'][:60]);put(state+0x1530,*vector['initial'][60:])
        machine.mem_write(state+0x1520,bytes(vector['fragment_offsets']))
        machine.mem_write(globals_map[0x4858a0],struct.pack('<d',vector['seconds']))
        executed={};counts=collections.Counter()
        def audit(uc,pc,size,user):
            raw=bytes(uc.mem_read(pc,size));assert raw==new.read(pc,size),hex(pc)
            executed[pc]=raw.hex();counts[pc]+=1
        machine.hook_add(UC_HOOK_CODE,audit)
        def run(entry,args,registers):
            put(stack,stop,*args);machine.reg_write(R.UC_X86_REG_ESP,stack)
            machine.reg_write(R.UC_X86_REG_FPCW,0x27f);machine.reg_write(R.UC_X86_REG_FPSW,0)
            machine.reg_write(R.UC_X86_REG_FPTAG,0xffff);machine.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
            for register,value in registers:machine.reg_write(register,value)
            machine.emu_start(entry,stop,count=1000000)
            assert machine.reg_read(R.UC_X86_REG_EIP)==stop
        run(entries[0x17550],[cache.magic+animated],[(R.UC_X86_REG_EBX,state)])
        run(entries[0x176a0],[cache.magic+animated+6],[(R.UC_X86_REG_ESI,state)])
        run(entries[0x17960],[state,cache.magic+animated+8],[])
        actual=bytes(machine.mem_read(state+0x1424,240))+bytes(machine.mem_read(state+0x1530,512))
        original=list(struct.unpack('<188I',actual))
        native_difference=[i for i,(a,b)in enumerate(zip(original,vector['native'])) if a!=b]
        legacy_difference=[i for i,(a,b)in enumerate(zip(original,vector['legacy'])) if a!=b]
        report.update(original_xbe_sha256=NEW_PIN,source_map_sha256=sha(ROOT/'assets/disc/maps/lockout.map'),
            correspondence=correspondence,relocated_shader_context_global=globals_map[0x485a58],
            relocated_shader_clock_global=globals_map[0x4858a0],original_bank_words=original,
            compared_words=188,native_different_words=native_difference,legacy_different_words=legacy_difference,
            original_instructions_executed=sum(counts.values()),unique_original_instructions=len(executed),
            original_instruction_bytes=executed,original_adapter_calls=counts[entries[0xbe8b0]],
            original_overlay_calls=counts[entries[0x1b230]],original_instruction_patches=[],supplied_callee_results=[],
            callback_is_unchanged_original=True,declared_actual_object_and_shader_context=True,
            original_context_registration_lifecycle_verified=False,rendered_Xbox_parity_verified=False,
            native_acceptance_sha256=sha(output/'native-acceptance.json'),tool_sha256=sha(Path(__file__)))
        report['passed']=not native_difference and bool(legacy_difference)
        save()
        assert report['passed'],dict(native_difference=native_difference,legacy_difference=legacy_difference)
        print(json.dumps({k:report[k]for k in ('passed','compared_words','native_different_words','legacy_different_words','original_instructions_executed')}),flush=True)
    finally:
        cache.data.close();cache.stream.close()


if __name__=='__main__':
    main()
