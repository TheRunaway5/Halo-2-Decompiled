#!/usr/bin/env python3
"""Physical unchanged TU1.5 pose/read/store/RSQRT oracle, edge and fault cases.

No CPU emulator supplies math, estimates, or solver results. Metadata describes
only authenticated original memory operands to a single-step observer. The ELF
contains unchanged original.text and declared component input banks. Xbox/P3
RSQRT estimate bits remain separately unqualified from this host-original proof.
Run under tools/run_test.py; all raw inputs/ELFs/builds belong to its scratch.
"""
import argparse,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

BASE,MOTION,STEP=0x1000000,0x1000000,0x1000300
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x400),(0x41f000,0x1000),(0x46a000,0x1000))
OWNERS=(0x325170,0x323430,0x1e1010,0x37c5d0)
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def image_bytes(image,p,n):
    for s in image.sections:
        raw=image.section_bytes(s)
        if s.va<=p and p+n<=s.va+len(raw):return raw[p-s.va:p-s.va+n]
    raise AssertionError(('Missing authenticated image operands',hex(p),n))
def metadata(image):
    code=_Code(image,'.text');sizes=[];sites=[];listings={};registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
    for owner in OWNERS:
        body=complete_body(code,owner);sizes.append(len(body.code));listing=[]
        for pc in sorted(body.code):
            i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
            for op in i.operands:
                if op.type!=X86_OP_MEM:continue
                if i.mnemonic=='lea':continue # Computes an address; no data operand is read.
                access=op.access
                assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),'Missing original operand access')
                # Capstone23 reports x87 FST/FSTP memory destinations as READ.
                # Authenticate the opcode mnemonic and classify the actual
                # architectural store; never infer it from resulting bytes.
                if i.mnemonic in ('fst','fstp'):access=CS_AC_WRITE
                base=i.reg_name(op.mem.base);index=i.reg_name(op.mem.index);assert not op.mem.segment,(hex(pc),'Unexpected segment operand')
                operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
            sites.append(dict(instruction=pc,operands=operands))
        listings[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
    assert sizes==[99,58,31,69]
    return dict(sites=sites,body_sites=sizes,listing_sha256=listings,regions=[dict(address=p,bytes=n)for p,n in BANKS])
def cases(image):
    constants=[dict(address=p,before=image_bytes(image,p,n).hex())for p,n in BANKS[1:]];out=[]
    def base(seed,mode=1):
        rng=random.Random(seed);raw=bytearray(0x400)
        def vec(offset,values):struct.pack_into('<4f',raw,offset,*values)
        vec(0x20,[rng.uniform(-1,1)for _ in range(4)]);vec(0x30,[rng.uniform(-1,1)for _ in range(4)])
        vec(0x40,[rng.uniform(-4,4)for _ in range(4)]);vec(0x50,[rng.uniform(-2,2)for _ in range(4)]);vec(0x70,[rng.uniform(-10,10)for _ in range(4)])
        struct.pack_into('<f',raw,0xc,.25);struct.pack_into('<f',raw,STEP-BASE,(0,1/30,1/60,.5,-.125,1/120)[mode]);return raw
    def put(raw,offset,bits):struct.pack_into('<I',raw,offset,bits)
    def append(name,raw,cw=0x27f,mxcsr=0x1f80,sw=0,step=STEP,**kwargs):out.append(dict(name=name,motion=MOTION,step=step,receiver=MOTION,entry=0x325170,arguments=[step],banks=[dict(address=BASE,before=raw.hex()),*constants],before_fp=[cw,sw,mxcsr],**kwargs))
    for seed in range(16):
        for mode in range(6):append(f'pose_{seed}_{mode}',base(seed,mode),cw=((0x27f,0x37f,0x7f)[seed%3]&~0xc00)|((seed%4)<<10),mxcsr=(0x1f80,0x3f80,0x5f80,0x7f80,0x9fc0,0xdfc0)[mode],sw=((seed%8)<<11)|((seed%4)<<8))
    for offset in (STEP-BASE,0x30,0x3c,0x50,0x5c,0x40,0x70,0xc,0x20):
        for bits in (0,0x80000000,1,0x00800000,0x7f800000,0xff800000,0x7fc12345,0x7f812345):raw=base(11);put(raw,offset,bits);append(f'edge_{offset:x}_{bits:x}',raw)
    for n,(a,b)in enumerate(((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321))):
        for left,right in ((0x40,0x70),(STEP-BASE,0x40),(0x50,0x54),(0x50,0x30),(0x30,0x34),(0x38,0x3c)):
            raw=base(n);put(raw,left,a);put(raw,right,b);append(f'paired_nan_{n}_{left:x}_{right:x}',raw)
    for offset in (0x70,0x30,0x50,0x7c,0xc):append(f'alias_step_{offset:x}',base(offset),step=MOTION+offset)
    for n,q in enumerate(((0,0,0,0),(0,0,0,1),(1,0,0,0),(0,1,0,0),(0,0,1,0),(1e-30,1e-30,1e-30,1e-30))):
        raw=base(4);struct.pack_into('<4f',raw,0x30,*q);append(f'quaternion_control_{n}',raw)
    for name,address,pc in (('step',STEP,0x32519e),('radius',MOTION+0xc,0x3251bf),('quaternion_w',MOTION+0x3c,0x32348e),('normalization_three',0x46a0e4,0x1e101c),('normalization_half',0x46a0cc,0x1e105b),('matrix_x',MOTION+0x30,0x37c5d7),('matrix_one',0x46a0d0,0x37c63e)):
        append('fault_'+name,base(3),fault=dict(address=address,bytes=4,occurrence=1),stop_pc=pc)
    return out
def physical(image,m,all_cases,scratch):
    owned=scratch/'physical-original';owned.mkdir();observer=owned/'observer';subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_motion_pose_tu15.cpp'),'-o',str(observer)],check=True,capture_output=True)
    section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
    for c in all_cases:
        sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{text}"']
        for n,b in enumerate(c['banks']):path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
        stack=bytearray(0x10000);struct.pack_into('<2I',stack,SP-STACK,STOP,c['step']);(owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=c['before_fp'];env=bytearray(28);struct.pack_into('<H',env,0,cw);struct.pack_into('<H',env,4,sw);struct.pack_into('<H',env,8,0xffff);(owned/'environment.bin').write_bytes(env)
        assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${MOTION},%ecx
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long 0x325170
sse_control: .long {mxcsr}
environment: .incbin "{owned/'environment.bin'}"
''');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
        r=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'output.json'),hex(c.get('stop_pc',STOP))],capture_output=True,text=True)
        assert r.returncode==0,(c['name'],r.stdout,r.stderr)
        observed=json.loads((owned/'output.json').read_text());assert len(observed['banks'])==len(c['banks'])
        for b,after in zip(c['banks'],observed.pop('banks')):assert b['address']==after['address'];b['after']=after['after']
        c.update(observed,physical_verified=True)
    return all_cases
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=True)
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);all_cases=physical(image,m,cases(image),scratch)
    f=dict(image_sha256=NEW_PIN,required_backend='unchanged-tu15-physical-i386-operands-stores-rsqrt-full-fp-v1',body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],cases=all_cases);fixture=scratch/'pose-original.json';fixture.write_text(json.dumps(f,separators=(',',':')))
    project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_motion_pose.cpp';text=source.read_text();mutation='const auto estimate=_mm_rsqrt_ss(sum);';assert text.count(mutation)==1;(project/'mutated.cpp').write_text(text.replace(mutation,'const auto estimate=_mm_div_ss(broadcast(0x3f800000u),_mm_sqrt_ss(sum));'))
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and "native RSQRTSS estimate differs" in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Unchanged physical original rejected precise reciprocal-root substitution:",r.stderr.strip())
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_pose_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name,src in (('native_tu15_motion_pose_tests',source),('native_tu15_motion_pose_mutation',project/'mutated.cpp')):
        cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_motion_pose_tests.cpp" "{src}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    cmake+=f'add_test(NAME native_pose_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_motion_pose_tests> "{fixture}")\nadd_test(NAME native_pose_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_tu15_motion_pose_mutation> "{fixture}")\n';(project/'CMakeLists.txt').write_text(cmake)
    results=[]
    for compiler in ('g++','clang++'):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_tu15_motion_pose_tests','--target','native_tu15_motion_pose_mutation','--filter','^native_pose_(original_tu15|mutation_rejected)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
    files=(source,ROOT/'include/halo2/native_tu15_motion_pose.hpp',ROOT/'tests/native_tu15_motion_pose_tests.cpp',ROOT/'tools/observe_native_motion_pose_tu15.cpp',Path(__file__))
    report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(all_cases),read_fault_prefixes=sum('fault'in c for c in all_cases),paired_nan_cases=sum(c['name'].startswith('paired_nan')for c in all_cases),ordered_original_reads=sum(len(c['reads'])for c in all_cases),ordered_original_stores=sum(len(c['writes'])for c in all_cases),physical_original_instructions=sum(c['original_instructions']for c in all_cases),native_rsqrt_observations=sum(len(c['rsqrt_estimates'])for c in all_cases),image_sha256=NEW_PIN,required_backend=f['required_backend'],body_sites=f['body_sites'],listing_sha256=f['listing_sha256'],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},negative_control='Private native source copy substitutes precise1/sqrt forRSQRTSS; unchanged physical original must reject observed estimate',retained_large_artifacts=[],excluded=['Xbox/PentiumIII-specific RSQRT estimate bits','live multiplayer/full physics closure','unmasked platform traps','unbacked/device vector semantics'])
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','read_fault_prefixes','paired_nan_cases','ordered_original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
