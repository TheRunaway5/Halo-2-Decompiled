#!/usr/bin/env python3
"""Pinned original TU1.5 closed force/torque math oracle plus negative control.

Uses real original instructions only. Physical i386 execution verifies every
final retained byte and complete FP state; Unicorn supplies ordered MOVAPS
provenance (its two64-bit write hooks are combined per original instruction).
All generated inputs/binaries/builds belong to enclosing run_test.py scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

BASE,MOTION,STEP,GRAVITY,IMPULSE=0x1000000,0x1000000,0x1000200,0x1000220,0x1000240
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x300),(0x46a000,0x1000))
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()

def inputs():
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');bodies={p:complete_body(code,p)for p in (0x3252d0,0x3250a0)}
    assert len(bodies[0x3252d0].code)==85 and len(bodies[0x3250a0].code)==53
    listings={hex(p):hashlib.sha256(''.join(f'{code.at(q).address:x} {code.at(q).bytes.hex()} {code.at(q).mnemonic} {code.at(q).op_str}\n'for q in sorted(b.code)).encode()).hexdigest()for p,b in bodies.items()}
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(BASE,0x10000);g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000)
    write=lambda p,v,n=4:g.mem_write(p,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
    scalar=lambda p,f:g.mem_write(p,struct.pack('<f',f))
    vec=lambda p,v:g.mem_write(p,struct.pack('<4f',*v))
    original_constants=bytes(g.mem_read(0x46a000,0x1000));active=False;writes=[]
    def on_write(_g,_access,p,n,value,_data):
        if not active or not(BASE<=p and p+n<=BASE+0x300):return
        site=g.reg_read(R.UC_X86_REG_EIP);i=code.at(site);assert i.mnemonic=='movaps' and n==8,(hex(site),i.mnemonic,n)
        item=dict(instruction=site,address=p,before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex())
        if writes and writes[-1]['instruction']==site and writes[-1]['address']+8==p and len(writes[-1]['written'])==16:
            writes[-1]['before']+=item['before'];writes[-1]['written']+=item['written']
        else:writes.append(item)
    g.hook_add(UC_HOOK_MEM_WRITE,on_write)
    cases=[]
    def setup(seed):
        nonlocal active
        active=False;g.mem_write(BASE,bytes(0x300));g.mem_write(STACK,bytes(0x10000));g.mem_write(0x46a000,original_constants)
        rng=random.Random(seed)
        for offset in (0x40,0x50,0xf0,0x100):vec(MOTION+offset,[rng.uniform(-4,4)for _ in range(4)])
        # Nonorthonormal matrices remain legal raw controls for this closed
        # arithmetic owner; no collision/solver/admission result is supplied.
        for offset in (0x80,0x90,0xa0):vec(MOTION+offset,[rng.uniform(-1,1)for _ in range(4)])
        vec(MOTION+0xe0,[rng.uniform(.01,2)for _ in range(4)]);vec(GRAVITY,(0,0,-4.1712594,0));vec(IMPULSE,[rng.uniform(-1,1)for _ in range(4)])
        scalar(STEP,1/30);scalar(MOTION+0x2c,.25);scalar(MOTION+0x14,.1);scalar(MOTION+0x18,.2)
    def run(name,entry,cw=0x27f,mxcsr=0x1f80,sw=0,depth=0):
        nonlocal active,writes
        for reg in (R.UC_X86_REG_EAX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI):g.reg_write(reg,0)
        g.reg_write(R.UC_X86_REG_ECX,MOTION);g.reg_write(R.UC_X86_REG_ESP,SP);g.reg_write(R.UC_X86_REG_EFLAGS,2)
        g.reg_write(R.UC_X86_REG_FPCW,cw);g.reg_write(R.UC_X86_REG_FPSW,sw);g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,mxcsr)
        write(SP,STOP);write(SP+4,STEP if entry==0x3252d0 else IMPULSE);write(SP+8,GRAVITY)
        before=[dict(address=p,before=bytes(g.mem_read(p,n)).hex())for p,n in BANKS];writes=[];active=True
        g.emu_start(entry,STOP,count=10000);active=False;assert g.reg_read(R.UC_X86_REG_EIP)==STOP
        for b,(_,n)in zip(before,BANKS):b['after']=bytes(g.mem_read(b['address'],n)).hex()
        assert all(len(w['written'])==32 for w in writes)
        cases.append(dict(name=name,entry=entry,motion=MOTION,step=STEP,gravity=GRAVITY,banks=before,writes=writes,
          before_fp=[cw,sw,mxcsr],unicorn_fp=[g.reg_read(R.UC_X86_REG_FPCW),g.reg_read(R.UC_X86_REG_FPSW),g.reg_read(R.UC_X86_REG_MXCSR)],
          impulse=list(struct.unpack('<4I',g.mem_read(IMPULSE,16))),preserved_x87_depth=depth))
    for seed in range(24):
        for mode in range(4):
            setup(seed);scalar(STEP,(0,1/30,1/60,.5)[mode]);scalar(MOTION+0x14,(0,.1,100,-1)[mode]);scalar(MOTION+0x18,(.2,100,0,-1)[mode]);scalar(MOTION+0x10c,(0,1,-1)[seed%3])
            run(f'force_{seed}_mode_{mode}',0x3252d0,cw=(0x27f,0x37f,0x107f)[seed%3],mxcsr=(0x1f80,0x3f80,0x5f80,0x9fc0)[mode],sw=(seed%8)<<11)
    for seed in range(16):setup(seed);run(f'angular_{seed}',0x3250a0,mxcsr=(0x1f80,0x3f80,0x5f80,0x9fc0)[seed%4],sw=(seed%8)<<11)
    for offset in (0,0x2c,0x14,0x18,0x10c):
        for bits in (0x80000000,0x7f800000,0xff800000,0x7fc12345,0x7f812345,1):
            setup(offset+bits);write(STEP if offset==0 else MOTION+offset,bits);run(f'exception_{offset:x}_{bits:x}',0x3252d0)
    for pattern,(a,b)in enumerate(((0x7fc12345,0x7fc54321),(0x7f812345,0x7fc54321),(0x7fc12345,0xff854321),(0xffc12345,0x7fc54321))):
        for path in range(8):
            setup(9000+pattern*8+path)
            if path==0:write(STEP,a);write(MOTION+0xf0,b)
            elif path==1:write(MOTION+0x2c,a);write(MOTION+0xf0,b)
            elif path==2:write(MOTION+0x40,a);write(MOTION+0xf0,b)
            elif path==3:write(MOTION+0x40,a);write(GRAVITY,b)
            elif path==4:write(MOTION+0x14,a);write(MOTION+0x40,b)
            elif path==5:write(MOTION+0x18,a);write(MOTION+0x50,b)
            elif path==6:write(STEP,a);write(MOTION+0x100,b);scalar(MOTION+0x10c,1)
            else:
                for lane in range(4):
                    for axis in range(3):write(MOTION+0x80+axis*16+lane*4,a+axis+lane)
                    write(IMPULSE+lane*4,b+lane);write(MOTION+0xe0+lane*4,b+0x10+lane);write(MOTION+0x50+lane*4,b+0x20+lane)
            run(f'paired_nan_{pattern}_path_{path}',0x3250a0 if path==7 else 0x3252d0)
    return dict(image_sha256=NEW_PIN,force_sites=85,angular_sites=53,listing_hashes=listings,cases=cases)

def hardware(f,scratch,banks=BANKS,physical_stores=False):
    owned=scratch/'original-force';owned.mkdir();tracer=owned/'observer'
    subprocess.run(['c++','-std=c++20','-O2',str(ROOT/('tools/observe_native_motion_forces_reference_tu15.cpp' if physical_stores else 'tools/observe_native_physics_frame_reference_tu15.cpp')),'-o',str(tracer)],check=True,capture_output=True)
    image=Xbe(NEW_DEFAULT);section=next(s for s in image.sections if s.name=='.text');path=owned/'text.bin';path.write_bytes(image.section_bytes(section))
    (owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in banks));total=0;differences=0
    for c in f['cases']:
        sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{path}"']
        for n,b in enumerate(c['banks']):
            bank=owned/f'bank{n}.bin';bank.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{bank}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
        stack=bytearray(0x10000);args=c.get('arguments',[STEP if c['entry']==0x3252d0 else IMPULSE,GRAVITY]);struct.pack_into('<'+'I'*(len(args)+1),stack,SP-STACK,STOP,*args);(owned/'stack.bin').write_bytes(stack)
        assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=c['before_fp'];environment=bytearray(28);struct.pack_into('<H',environment,0,cw);struct.pack_into('<H',environment,4,sw);struct.pack_into('<H',environment,8,0xffff);(owned/'environment.bin').write_bytes(environment)
        assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${c.get('receiver',MOTION)},%ecx
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long {c['entry']}
sse_control: .long {mxcsr}
environment: .incbin "{owned/'environment.bin'}"
''');sections.append('.bridge 0x70000000 : { *(.bridge) }')
        (owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True)
        subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
        subprocess.run([str(tracer),str(owned/'reference'),hex(c.get('boundary_entry',STOP)),hex(section.va+section.vsize),str(owned/'regions.txt'),str(owned/'output.bin')],check=True,capture_output=True)
        data=(owned/'output.bin').read_bytes();cw,sw,mxcsr,sites=struct.unpack_from('<4I',data);offset=28
        for b,(_,n)in zip(c['banks'],banks):
            observed=data[offset:offset+n].hex()
            if not physical_stores:assert observed==b['after'],(c['name'],'original hardware/Unicorn bytes differ')
            else:b['after']=observed
            offset+=n
        if physical_stores:
            count,=struct.unpack_from('<I',data,offset);offset+=4;stores=[]
            for _ in range(count):
                site,address=struct.unpack_from('<II',data,offset);offset+=8
                stores.append(dict(instruction=site,address=address,before=data[offset:offset+16].hex(),written=data[offset+16:offset+32].hex()));offset+=32
            c['physical_vector_stores_differ_from_unicorn']=stores!=c['writes'];c['writes']=stores
        assert offset==len(data);c['after_fp']=[cw,sw,mxcsr];c['physical_verified']=True;c['physical_original_instructions']=sites;total+=sites
        if c['after_fp']!=c['unicorn_fp']:differences+=1
    f['required_backend']='unchanged-tu15-physical-i386-vector-stores-banks-fp-v2' if physical_stores else 'unchanged-tu15-unicorn-vector-stores+physical-i386-banks-fp-v1';f['physical_original_instructions']=total;f['physical_fp_differences_from_unicorn']=differences
    return f

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',default='c++');a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=True)
    f=hardware(inputs(),scratch,physical_stores=True);fixture=scratch/'motion-original.json';fixture.write_text(json.dumps(f,separators=(',',':')))
    project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_motion_forces.cpp';mutation='simd(s.linear_velocity()),multiply(broadcast(gravity_step),gravity)'
    text=source.read_text();assert text.count(mutation)==1;(project/'mutated.cpp').write_text(text.replace(mutation,'simd(s.linear_velocity()),_mm_setzero_ps()'))
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and "ordered vector store differs" in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Unchanged original rejected removed gravity:",r.stderr.strip())
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_motion_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name,src in (('native_tu15_motion_forces_tests',source),('native_tu15_motion_forces_mutation',project/'mutated.cpp')):
        cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_motion_forces_tests.cpp" "{src}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    cmake+=f'add_test(NAME native_motion_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_motion_forces_tests> "{fixture}")\n'
    cmake+=f'add_test(NAME native_motion_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_tu15_motion_forces_mutation> "{fixture}")\n'
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--cmake-arg=-DCMAKE_CXX_COMPILER='+a.compiler,'--scratch-root',str(scratch/'builds'),'--output',str(a.output/'cmake'),'--target','native_tu15_motion_forces_tests','--target','native_tu15_motion_forces_mutation','--filter','^native_motion_(original_tu15|mutation_rejected)$','--jobs','2','--test-jobs','1']
    r=subprocess.run(command,check=False)
    report=dict(passed=r.returncode==0,exit_code=r.returncode,owned_scratch=str(scratch),original_cases=len(f['cases']),ordered_original_vector_stores=sum(len(c['writes'])for c in f['cases']),
      compiler=a.compiler,paired_nan_cases=sum(c['name'].startswith('paired_nan_')for c in f['cases']),physical_store_cases_differing_from_unicorn=sum(c['physical_vector_stores_differ_from_unicorn']for c in f['cases']),physical_original_instructions=f['physical_original_instructions'],physical_fp_differences_from_unicorn=f['physical_fp_differences_from_unicorn'],
      required_backend=f['required_backend'],original_image_sha256=NEW_PIN,force_sites=85,angular_sites=53,original_listing_hashes=f['listing_hashes'],
      fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,retained_large_artifacts=[],negative_control='private source mutation removes gravity contribution; unchanged original must reject ordered vector store',
      source_hashes={str(p.relative_to(ROOT)):sha(p)for p in (source,ROOT/'include/halo2/native_tu15_motion_forces.hpp',ROOT/'tests/native_tu15_motion_forces_tests.cpp',ROOT/'tools/observe_native_motion_forces_reference_tu15.cpp',Path(__file__))},command=command,
      excluded=['full world/solver closure','live multiplayer','unmasked platform faults','original x87 overflow/trap contexts'])
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('passed','original_cases','ordered_original_vector_stores')}));return r.returncode
if __name__=='__main__':raise SystemExit(main())
