#!/usr/bin/env python3
"""Physical original TU1.5 short/long body sleep policy qualification.

Both complete roots execute unchanged, including all native SIMD/x87 math.
No callback, policy result or numerical estimate is supplied. All ELF, fixture,
observer and compiler products belong to enclosing run_test.py scratch.
"""
import argparse,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,POLICY,BODY,MOTION,OUTPUT=0x1000000,0x1000100,0x1000300,0x1000500,0x1000900
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x1000),)
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def metadata(image):
    code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
    for entry in(0x31e5c0,0x31e690):
        body=complete_body(code,entry);counts.append(len(body.code));listing=[]
        for pc in sorted(body.code):
            i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
            for op in i.operands:
                if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
                access=op.access;assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),'Missing architectural access')
                base=i.reg_name(op.mem.base);index=i.reg_name(op.mem.index);assert not op.mem.segment
                operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
            sites.append(dict(instruction=pc,operands=operands))
        listings[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
    assert counts==[66,66]
    assert struct.unpack('<2I',image.read(0x41eb80,8))==(0x31e5c0,0x31e690)
    assert image.read(0x31e84b,6)==bytes.fromhex('c7066ceb4100')
    return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[dict(address=p,bytes=n)for p,n in BANKS])
def cases():
    out=[]
    def make(name,period,distance=.5,angle=.5,scale=1,changes=(),policy=POLICY,output=OUTPUT):
        raw=bytearray(0x1000)
        def put(p,v,n=4):raw[p-BASE:p-BASE+n]=(v&((1<<(8*n))-1)).to_bytes(n,'little')
        def number(p,v):raw[p-BASE:p-BASE+4]=struct.pack('<f',v)
        for offset in(0x10,0x30):number(policy+offset+12,1)
        for offset in(0x20,0x40):number(policy+offset+12,1)
        for offset in(0x50,0x54,0x58,0x5c):number(policy+offset,1)
        number(policy+8,scale);number(MOTION+0xb0,distance);number(MOTION+0xbc,1);number(MOTION+0x30,angle);number(MOTION+0x3c,1)
        number(MOTION+0x2b0,99);number(MOTION+0x2bc,1);put(BODY+0x3c,MOTION);put(OUTPUT,0xaabbccdd)
        for p,v,n in changes:put(p,v,n)
        n=len(out);out.append(dict(name=name,period=period,entry=0x31e5c0 if period==0 else 0x31e690,policy=policy,body=BODY,output=output,banks=[dict(address=BASE,before=raw.hex())],before_fp=[(0x27f,0x37f,0x7f,0x67f,0xa7f)[n%5],((n%8)<<11)|((0,0x4500,0x100,0x400)[n%4]),(0x1f80,0x5f80,0x9fc0,0xffc0)[n%4]],stop_pc=STOP))
    for period in(0,1):
        for distance in(0,.5,0.99999994,1,1.00000012,2):
            for angle in(0,.5,1,1.00000012):
                for scale in(0,1,4):make(f'finite_{period}_{distance}_{angle}_{scale}',period,distance,angle,scale)
        prior=0x10 if period==0 else 0x30;rotation=0x20 if period==0 else 0x40;threshold=0x50 if period==0 else 0x58
        addresses=(MOTION+0xb0,MOTION+0xbc,MOTION+0x30,MOTION+0x3c,POLICY+prior,POLICY+prior+12,POLICY+rotation,POLICY+rotation+12,POLICY+threshold,POLICY+threshold+4,POLICY+8)
        for address in addresses:
            for bits in(0x80000000,1,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'exception_{period}_{address:x}_{bits:x}',period,changes=((address,bits,4),))
        pairs=((MOTION+0xb0,POLICY+prior),(MOTION+0x30,POLICY+rotation),(MOTION+0xbc,POLICY+prior+12),(MOTION+0x3c,POLICY+rotation+12),(POLICY+threshold,POLICY+prior),(POLICY+threshold+4,POLICY+8))
        for pair,(a,b)in enumerate(((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321))):
            for first,second in pairs:make(f'paired_nan_{period}_{pair}_{first:x}_{second:x}',period,changes=((first,a,4),(second,b,4)))
        for output in(POLICY+prior,POLICY+rotation,POLICY+threshold,POLICY+8,BODY+0x3c,MOTION+0xb0):
            make(f'alias_false_output_{period}_{output:x}',period,2,.5,output=output)
            make(f'alias_true_output_{period}_{output:x}',period,0,0,output=output)
        make(f'alias_quaternion_rewrites_current_position_{period}',period,2,0,policy=MOTION+0xb0-rotation)
        make(f'alias_quaternion_changes_motion_{period}',period,2,0,policy=BODY+0x30-rotation,changes=((BODY+0x3c,MOTION,4),(MOTION+0x3c,MOTION+0x200,4)))
        make(f'fault_seed_{period}',period,2,.5)
        make(f'angular_fault_seed_{period}',period,0,.5)
    return out
def physical(image,m,cases,scratch):
    owned=scratch/'physical-original';owned.mkdir(exist_ok=True);observer=owned/'observer'
    if not observer.exists():subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_motion_pose_tu15.cpp'),'-o',str(observer)],check=True,capture_output=True)
    section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
    for c in cases:
        sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{text}"']
        for n,b in enumerate(c['banks']):
            path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
        stack=bytearray(0x10000);struct.pack_into('<3I',stack,SP-STACK,STOP,c['output'],BODY);(owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=c['before_fp'];env=bytearray(28);struct.pack_into('<H',env,0,cw);struct.pack_into('<H',env,4,sw);struct.pack_into('<H',env,8,0xffff);(owned/'environment.bin').write_bytes(env)
        assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${c['policy']},%ecx
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long {c['entry']}
sse_control: .long {mxcsr}
environment: .incbin "{owned/'environment.bin'}"
''');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
        r=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'output.json'),hex(c['stop_pc'])],capture_output=True,text=True);assert r.returncode==0,(c['name'],r.stdout,r.stderr)
        observed=json.loads((owned/'output.json').read_text())
        for b,after in zip(c['banks'],observed.pop('banks')):assert b['address']==after['address'];b['after']=after['after']
        c.update(observed,physical_verified=True)
        if'fault'not in c:
            region=next(b for b in c['banks']if b['address']<=c['output']<b['address']+len(bytes.fromhex(b['after'])))
            c['result']=bool(bytes.fromhex(region['after'])[c['output']-region['address']])
    return cases
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=True)
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);all_cases=physical(image,m,cases(),scratch);faults=[]
    for seed in[c for c in all_cases if'fault_seed'in c['name']]:
        seen=set();occurrences={}
        for read in seed['reads']:
            key=(read['address'],read['bytes']);occurrences[key]=occurrences.get(key,0)+1
            if read['instruction']in seen:continue
            seen.add(read['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_fault_'+hex(read['instruction']),stop_pc=read['instruction'],fault=dict(address=read['address'],bytes=read['bytes'],occurrence=occurrences[key]));faults.append(c)
    all_cases+=physical(image,m,faults,scratch)
    f=dict(image_sha256=NEW_PIN,required_backend='unchanged-tu15-body-policy-physical-operands-stores-full-fp-v1',body_sites=[66,66],listing_sha256=m['listing_sha256'],cases=all_cases);fixture=scratch/'policy.json';fixture.write_text(json.dumps(f,separators=(',',':')))
    project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_body_deactivation_policy.cpp';text=source.read_text();mutation='(distance_status&0x0500u)==0x0100u';assert text.count(mutation)==1;(project/'mutated.cpp').write_text(text.replace(mutation,'(distance_status&0x0500u)!=0x0400u'))
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],"--oracle-only",sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and("ordered body policy store"in r.stderr or"private output bool differs"in r.stderr),(r.returncode,r.stdout,r.stderr)
print("Unchanged physical original rejects non-strict distance admission:",r.stderr.strip())
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_body_policy_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name,src in(('native_tu15_body_deactivation_policy_tests',source),('native_tu15_body_deactivation_policy_mutation',project/'mutated.cpp')):
        cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_body_deactivation_policy_tests.cpp" "{src}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    cmake+=f'add_test(NAME native_body_policy_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_body_deactivation_policy_tests> "{fixture}")\nadd_test(NAME native_body_policy_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_tu15_body_deactivation_policy_mutation> "{fixture}")\n';(project/'CMakeLists.txt').write_text(cmake)
    results=[]
    for compiler in('g++','clang++'):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_tu15_body_deactivation_policy_tests','--target','native_tu15_body_deactivation_policy_mutation','--filter','^native_body_policy_(original_tu15|mutation_rejected)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
    files=(source,ROOT/'include/halo2/native_tu15_body_deactivation_policy.hpp',ROOT/'tests/native_tu15_body_deactivation_policy_tests.cpp',ROOT/'tools/observe_native_motion_pose_tu15.cpp',Path(__file__))
    report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(all_cases),complete_original_calls=sum('fault'not in c for c in all_cases),read_fault_prefixes=len(faults),paired_nan_cases=sum(c['name'].startswith('paired_nan')for c in all_cases),ordered_original_reads=sum(len(c['reads'])for c in all_cases),ordered_original_stores=sum(len(c['writes'])for c in all_cases),physical_original_instructions=sum(c['original_instructions']for c in all_cases),image_sha256=NEW_PIN,body_sites=[66,66],listing_sha256=m['listing_sha256'],required_backend=f['required_backend'],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},owned_scratch=str(scratch),negative_control='Private mutation admits equal/greater distance instead of strict less; unchanged physical original rejects result/stores',retained_large_artifacts=[],excluded=['unmasked platform traps','full live world composition'])
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','read_fault_prefixes','paired_nan_cases','ordered_original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
