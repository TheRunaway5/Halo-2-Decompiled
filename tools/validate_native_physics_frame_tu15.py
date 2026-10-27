#!/usr/bin/env python3
"""Compare proper native frame semantics with unchanged English TU1.5 code.

Declared prefix/tail fixtures are component evidence, not multiplayer or full
native closure. Original execution stops before unowned dependency entries;
no instruction, callee, admission, destructor, or solver result is replaced.
All fixture banks/build products live in the enclosing run_test.py owner.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import struct
import shutil

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

WORLD=0x1000000
MANAGER,WORLD_ISLANDS,ISLAND,BODIES,BODY,PROPERTIES,POOL,COMPONENTS,QUEUE,CLOCK=(WORLD+n for n in (0x100,0x200,0x300,0x400,0x500,0x600,0x700,0x800,0x900,0xa00))
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=[(0x46a000,0x1000),(0x46e000,0x1000),(0x48b000,0x1000),(0x51d000,0x1000),
       (0x52b000,0x1000),(0x52d000,0x1000),(0x556000,0x1000),(WORLD,0x4000),(0x80061000,0x1000)]

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()

def fixture(destination):
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    roots=(0x1cc8d0,0x2834b0,0x282010,0x1cd5c0,0x315c90,0x283450,0x2f30f0,0x32b3d6,0x32b26c,0x32b2fe)
    listings=[]
    for root in roots:
        body=complete_body(code,root)
        raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in (code.at(p) for p in sorted(body.code)))
        listings.append(dict(entry=root,instruction_sites=len(body.code),listing_sha256=hashlib.sha256(raw.encode()).hexdigest()))
    assert listings[0]['instruction_sites']==137
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(WORLD,0x10000);g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000);g.mem_map(0x80061000,0x1000)
    original=[(p,bytes(g.mem_read(p,n)))for p,n in BANKS]
    write=lambda p,n,size=4:g.mem_write(p,(n&((1<<(8*size))-1)).to_bytes(size,'little'))
    read=lambda p,size=4:int.from_bytes(g.mem_read(p,size),'little')
    active=False;writes=[];reads=[];boundary=None;targets={};executed=set()
    def on_read(_g,_access,p,n,_value,_data):
        if active and any(lo<=p and p+n<=lo+size for lo,size in BANKS):
            reads.append(dict(instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,size=n,stores_before=len(writes)))
    def on_write(_g,_access,p,n,value,_data):
        if active and any(lo<=p and p+n<=lo+size for lo,size in BANKS):
            writes.append(dict(instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
    def on_code(_g,p,_size,_data):
        nonlocal boundary
        if not active:return
        executed.add(p)
        if p in targets:
            owner=targets[p];boundary=dict(owner=owner,identity=g.reg_read(R.UC_X86_REG_ECX) if owner=='world' else g.reg_read(R.UC_X86_REG_EAX))
            if owner=='world':
                step=read(g.reg_read(R.UC_X86_REG_ESP)+4);boundary['step']=[read(step),read(step+4)]
            g.emu_stop()
    g.hook_add(UC_HOOK_MEM_WRITE,on_write);g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_CODE,on_code)
    def setup():
        nonlocal active
        active=False
        for p,raw in original:g.mem_write(p,raw)
        g.mem_write(STACK,bytes(0x10000));g.mem_write(WORLD,bytes(0x4000));g.mem_write(0x80061000,bytes(0x1000))
        for p,n in [(0x52be48,WORLD),(0x52d880,MANAGER),(0x51df54,CLOCK),(CLOCK+4,0x3d088889),
                    (0x46a0d0,0x3f800000),(MANAGER+0x14,0xffffffff),(MANAGER+8,QUEUE),
                    (0x52be5c,POOL),(POOL+0x44,COMPONENTS),(WORLD+8,WORLD_ISLANDS),
                    (WORLD_ISLANDS,ISLAND),(ISLAND+0x3c,BODIES),(BODIES,BODY),
                    (BODY+0x30,PROPERTIES),(BODY+0x34,1),(PROPERTIES,0x2001),(PROPERTIES+4,0x12340001),
                    (COMPONENTS+0xa0+8,0x12345678),(0x48b85c,0)]:write(p,n)
        write(0x48b873,1,1);write(0x556588,0,1);write(0x5565ac,0);write(0x52d888,0,2)
        for r in (R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI):g.reg_write(r,0)
    cases=[]
    def run(name,mode='root',cw=0x27f,mxcsr=0x1f80):
        nonlocal active,writes,reads,boundary,targets
        g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_FPCW,cw);g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,mxcsr)
        write(SP,STOP)
        #1CC9CC is the source tail after the two saved GPRs and0x24 locals.
        g.reg_write(R.UC_X86_REG_ESP,SP-(0x2c if mode=='tail' else 0))
        before=[dict(address=p,before=bytes(g.mem_read(p,n)).hex())for p,n in BANKS]
        writes=[];reads=[];boundary=None;targets={0x31b1a0:'world',0xbda50:'publication'};active=True
        entry=0x1cc9cc if mode=='tail' else 0x1cc8d0
        g.emu_start(entry,STOP,count=200000);active=False
        assert boundary or g.reg_read(R.UC_X86_REG_EIP)==STOP,(name,hex(g.reg_read(R.UC_X86_REG_EIP)))
        for b,(_,n)in zip(before,BANKS):b['after']=bytes(g.mem_read(b['address'],n)).hex()
        cases.append(dict(name=name,mode=mode,banks=before,writes=writes,reads=reads,boundary=boundary,
          guarded_command_drain=mode=='root' and read(0x52be48)!=0,
          before_fp=[cw,0,mxcsr],after_fp=[g.reg_read(R.UC_X86_REG_FPCW),g.reg_read(R.UC_X86_REG_FPSW),g.reg_read(R.UC_X86_REG_MXCSR)]))
    setup();write(0x52be48,0);run('null_world')
    values=(0x3d088889,0x3c888889,0x3f000000,0,0x80000000,0xbf800000,1,0x00800000,0x7f800000,0x7fc12345)
    for n,dt in enumerate(values):
        for mx in (0x1f80,0x3f80,0x5fc0,0xdf80):
            setup();write(CLOCK+4,dt);run(f'prefix_dt_{n}_mx_{mx:x}',cw=(0x27f,0xf7f,0x137f)[n%3],mxcsr=mx)
    for active_count in (0,1,3):
        for deferred in (0,1,3):
            for depth in (0,0xffff):
                setup();write(WORLD+0xc,active_count);write(0x52d888,depth,2)
                for n in range(active_count):
                    addr=ISLAND+n*0x1000;write(WORLD_ISLANDS+n*4,addr);write(addr+0x40,0 if n==1 else 1);write(addr+0x3c,BODIES)
                write(MANAGER+0xc,deferred)
                for n in range(deferred):
                    addr=BODY+n*0x100;write(QUEUE+n*4,addr);write(addr+6,2 if n%2==0 else 0,2)
                run(f'tail_active_{active_count}_deferred_{deferred}_depth_{depth}',mode='tail')
    # All-original protected reference boundaries and wrap semantics. Every
    # queue body is declared input; no destruction success value is supplied.
    for enabled,size,offset in ((0,0,0),(1,0x80,0),(1,0x80,0x80)):
        setup();write(MANAGER+0xc,1);addr=0x80061000+offset;write(QUEUE,addr);write(addr+6,1,2)
        write(0x556588,enabled,1);write(0x5565ac,size);run(f'protected_{enabled}_{size}_{offset}',mode='tail')
    for count,key,publish in ((1,0x2001,1),(0,0,1),(2,0x1000,1)):
        setup();write(WORLD+0xc,1);write(ISLAND+0x40,1);write(BODY+0x34,count);write(PROPERTIES,key)
        if count==2:write(PROPERTIES+8,0x2001);write(PROPERTIES+12,0x43210001)
        write(COMPONENTS+(0 if count==0 else 0xa0)+4,0x20)
        run(f'publication_count_{count}',mode='tail')
    result=dict(image='English TU1.5',image_sha256=NEW_PIN,root_instruction_sites=137,listings=listings,cases=cases,
      oracle='unchanged original instructions in Unicorn; no callback result replacement',
      exclusions=['full residual composition','multiplayer','native-only closure','unmasked platform faults'],executed_sites=len(executed))
    destination.write_text(json.dumps(result,separators=(',',':')))
    return result

def hardware(f,scratch):
    """Re-execute the same unchanged source prefix/tail on the physical CPU.

    Unicorn supplies ordered persistent-write provenance; physical i386 source
    execution independently verifies every final bank, semantic boundary, and
    complete FP environment. Its precision flags/reserved bits are authoritative.
    """
    owned=scratch/'original-hardware';owned.mkdir();tracer=owned/'observer'
    subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_physics_frame_reference_tu15.cpp'),'-o',str(tracer)],check=True,capture_output=True)
    image=Xbe(NEW_DEFAULT);section=next(s for s in image.sections if s.name=='.text');text=owned/'source-text.bin';text.write_bytes(image.section_bytes(section))
    (owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in BANKS))
    total=0;fp_differences=0
    for c in f['cases']:
        sections=[f'.original 0x{section.va:x} : {{ *(.original) }}']
        assembly=[f'.section .original,"ax"\n.incbin "{text}"']
        for n,b in enumerate(c['banks']):
            path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']))
            assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
        stack=bytearray(0x10000);struct.pack_into('<I',stack,SP-STACK,STOP);(owned/'stack.bin').write_bytes(stack)
        assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,_,mxcsr=c['before_fp'];entry=0x1cc9cc if c['mode']=='tail' else 0x1cc8d0;esp=SP-(0x2c if c['mode']=='tail' else 0)
        assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldcw control
 ldmxcsr sse_control
 xor %eax,%eax
 xor %ecx,%ecx
 xor %edx,%edx
 xor %ebx,%ebx
 xor %ebp,%ebp
 xor %esi,%esi
 xor %edi,%edi
 push $2
 popfl
 mov ${esp},%esp
 int3
 jmp *entry
.balign 4
entry: .long {entry}
control: .long {cw}
sse_control: .long {mxcsr}
''')
        sections.append('.bridge 0x70000000 : { *(.bridge) }')
        (owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True)
        subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
        target=STOP if c['boundary']is None else (0x31b1a0 if c['boundary']['owner']=='world' else 0xbda50)
        subprocess.run([str(tracer),str(owned/'reference'),hex(target),hex(section.va+section.vsize),str(owned/'regions.txt'),str(owned/'output.bin')],check=True,capture_output=True)
        data=(owned/'output.bin').read_bytes();cw,status,mxcsr,sites,identity,seconds,inverse=struct.unpack_from('<7I',data);offset=28
        for b,(_,n)in zip(c['banks'],BANKS):
            assert data[offset:offset+n].hex()==b['after'],(c['name'],'Unicorn and hardware retained bytes differ',hex(b['address']))
            offset+=n
        assert len(data)==offset
        if c['boundary']is not None:
            assert identity==c['boundary']['identity'],(c['name'],'semantic dependency identity differs')
            if c['boundary']['owner']=='world':assert[seconds,inverse]==c['boundary']['step'],(c['name'],'semantic step words differ')
        if[cw,status,mxcsr]!=c['after_fp']:fp_differences+=1
        c['unicorn_fp']=c['after_fp'];c['after_fp']=[cw,status,mxcsr];c['physical_original_instructions']=sites;c['physical_verified']=True;total+=sites
    f['oracle']='unchanged original TU1.5 Unicorn ordered stores plus physical i386 original banks/boundary/full FP verification; no dependency results supplied'
    f['physical_original_instructions']=total;f['physical_fp_differences_from_unicorn']=fp_differences
    f['required_backend']='unchanged-tu15-unicorn-writes+physical-i386-banks-boundary-fp-v1'
    return f

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);a=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=True)
    project=scratch/'physics-frame-project';project.mkdir()
    frozen=project/'source';shutil.copytree(ROOT/'include',frozen/'include')
    sources=('src/physics/native_tu15_physics_frame.cpp','src/physics/native_tu15_physics_frame_residuals.cpp',
             'tests/native_tu15_physics_frame_tests.cpp')
    for name in sources:
        target=frozen/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target)
    tested=('include/halo2/native_tu15_physics_frame.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp',*sources)
    source_hashes={name:sha(frozen/name)for name in tested}
    header_hashes={str(p.relative_to(frozen)):sha(p)for p in (frozen/'include').rglob('*')if p.is_file()}
    (a.output/'frozen-header-sha256.json').write_text(json.dumps(header_hashes,indent=2,sort_keys=True)+'\n')
    fixture_path=scratch/'original-frame.json';f=hardware(fixture(fixture_path),scratch)
    fixture_path.write_text(json.dumps(f,separators=(',',':')))
    original_kernel=(frozen/'src/physics/native_tu15_physics_frame.cpp').read_text()
    mutation='s.clear_allocation_failure();s.stepping(s.manager(),true);'
    assert original_kernel.count(mutation)==1
    (project/'mutated_frame.cpp').write_text(original_kernel.replace(mutation,'s.clear_allocation_failure();s.stepping(s.manager(),false);'))
    read_mutation='return read(p.identity+n*8u);'
    assert original_kernel.count(read_mutation)==1
    (project/'mutated_reads.cpp').write_text(original_kernel.replace(read_mutation,'(void)read(p.identity+n*8u+4u);return read(p.identity+n*8u);'))
    (project/'assert_negative.py').write_text('''import json,subprocess,sys
r=subprocess.run([sys.argv[1],"--oracle-only",sys.argv[2]],capture_output=True,text=True)
assert r.returncode!=0 and sys.argv[3] in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Unchanged original rejected private native mutation:",r.stderr.strip())
''')
    # A small owned project avoids rebuilding unrelated game/runtime modules.
    # Compilation and CTest still use the repository's disposable CMake owner.
    (project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(native_physics_frame_validation LANGUAGES CXX)
enable_testing()
add_executable(native_tu15_physics_frame_tests "{frozen}/tests/native_tu15_physics_frame_tests.cpp" "{frozen}/src/physics/native_tu15_physics_frame.cpp" "{frozen}/src/physics/native_tu15_physics_frame_residuals.cpp")
target_include_directories(native_tu15_physics_frame_tests PRIVATE "{frozen}/include")
target_compile_features(native_tu15_physics_frame_tests PRIVATE cxx_std_20)
target_compile_options(native_tu15_physics_frame_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME native_physics_frame_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_physics_frame_tests> --oracle-fixture "{fixture_path}")
add_executable(native_tu15_physics_frame_mutation "{frozen}/tests/native_tu15_physics_frame_tests.cpp" "{project}/mutated_frame.cpp" "{frozen}/src/physics/native_tu15_physics_frame_residuals.cpp")
target_include_directories(native_tu15_physics_frame_mutation PRIVATE "{frozen}/include")
target_compile_features(native_tu15_physics_frame_mutation PRIVATE cxx_std_20)
target_compile_options(native_tu15_physics_frame_mutation PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME native_physics_frame_original_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/assert_negative.py" $<TARGET_FILE:native_tu15_physics_frame_mutation> "{fixture_path}" "persistent store differs at 1")
add_executable(native_tu15_physics_frame_read_mutation "{frozen}/tests/native_tu15_physics_frame_tests.cpp" "{project}/mutated_reads.cpp" "{frozen}/src/physics/native_tu15_physics_frame_residuals.cpp")
target_include_directories(native_tu15_physics_frame_read_mutation PRIVATE "{frozen}/include")
target_compile_features(native_tu15_physics_frame_read_mutation PRIVATE cxx_std_20)
target_compile_options(native_tu15_physics_frame_read_mutation PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME native_physics_frame_original_read_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/assert_negative.py" $<TARGET_FILE:native_tu15_physics_frame_read_mutation> "{fixture_path}" "source read count differs")
''')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/'cmake'),
      '--target','native_tu15_physics_frame_tests','--target','native_tu15_physics_frame_mutation','--target','native_tu15_physics_frame_read_mutation',
      '--filter','^native_physics_frame_original_(tu15|mutation_rejected|read_mutation_rejected)$','--jobs','2','--test-jobs','1']
    result=subprocess.run(command,check=False)
    report=dict(passed=result.returncode==0,exit_code=result.returncode,original_cases=len(f['cases']),original_root_sites=137,
      original_instruction_sites_executed=f['executed_sites'],ordered_original_persistent_stores=sum(len(c['writes'])for c in f['cases']),
      original_image_sha256=NEW_PIN,original_owners=f['listings'],comparison_scope=f['oracle'],excluded=f['exclusions'],
      physical_original_instructions=f['physical_original_instructions'],physical_fp_differences_from_unicorn=f['physical_fp_differences_from_unicorn'],
      required_backend=f['required_backend'],negative_control='Private source copy changes native manager stepping=true to false; unchanged original must reject persistent store1',
      read_negative_control='Private source copy fetches property values before testing keys; unchanged original must reject its data access sequence',
      source_hashes=source_hashes,root_sources_still_match=all(sha(ROOT/name)==pin for name,pin in source_hashes.items()),
      tool_hashes={str(p.relative_to(ROOT)):sha(p)for p in (ROOT/'tools/observe_native_physics_frame_reference_tu15.cpp',Path(__file__))},
      ordered_original_data_reads=sum(len(c['reads'])for c in f['cases']),reads_checked_relative_to_stores=True,
      fixture_sha256=sha(fixture_path),fixture_bytes=fixture_path.stat().st_size,retained_large_artifacts=[],command=command)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('passed','original_cases','ordered_original_persistent_stores','retained_large_artifacts')}))
    return result.returncode
if __name__=='__main__':raise SystemExit(main())
