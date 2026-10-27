#!/usr/bin/env python3
"""Unchanged physical325410/2E4820/3250A0 native math qualification.

No oracle outcomes or external math are supplied. The complete original setup,
transpose and angular impulse execute on the physical host. All generated ELF,
fixtures, observer and private compiler products use run_test.py scratch.
"""
import argparse,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,MOTION,STEP,GRAVITY,OUTPUT=0x1000000,0x1000100,0x1000400,0x1000500,0x1000600
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x1000),(0x46a000,0x1000))
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def metadata(image):
    code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
    for entry in(0x325410,0x2e4820,0x3250a0):
        body=complete_body(code,entry);counts.append(len(body.code));listing=[]
        for pc in sorted(body.code):
            i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
            for op in i.operands:
                if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
                access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access;assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),'Missing architectural access')
                base=i.reg_name(op.mem.base);index=i.reg_name(op.mem.index);assert not op.mem.segment
                operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
            sites.append(dict(instruction=pc,operands=operands))
        listings[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
    assert counts==[124,24,53]
    assert struct.unpack('<I',image.read(0x41f2ec,4))[0]==struct.unpack('<I',image.read(0x41f36c,4))[0]==0x325410
    return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[dict(address=p,bytes=n)for p,n in BANKS])
def cases(image):
    out=[];constants=dict(address=0x46a000,before=image.read(0x46a000,0x1000).hex())
    def make(name,seconds=.125,linear_damping=.25,angular_damping=.5,torque=1,rotation=0,changes=(),step=STEP,gravity=GRAVITY,output=OUTPUT):
        raw=bytearray(0x1000)
        def put(p,v,n=4):raw[p-BASE:p-BASE+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
        def number(p,v):raw[p-BASE:p-BASE+4]=struct.pack('<f',v)
        def vector(p,values):
            for n,v in enumerate(values):number(p+n*4,v)
        number(MOTION+0x14,linear_damping);number(MOTION+0x18,angular_damping);number(MOTION+0x2c,.75)
        vector(MOTION+0x40,(.25,-1,2,0));vector(MOTION+0x50,(.5,1,-.75,0));vector(MOTION+0x70,(3,4,5,1));vector(MOTION+0xe0,(.5,.25,.125,2));vector(MOTION+0xf0,(1,-2,.5,0));vector(MOTION+0x100,(-.25,.125,.5,torque))
        rows=((1,0,0,0),(0,1,0,0),(0,0,1,0))if not rotation else((0,1,0,0),(-1,0,0,0),(0,0,1,0))
        for n,row in enumerate(rows):vector(MOTION+0x80+n*16,row)
        vector(gravity,(0,0,-9.8,0));number(step,seconds)
        for p,v,n in changes:put(p,v,n)
        n=len(out);out.append(dict(name=name,entry=0x325410,motion=MOTION,step=step,gravity=gravity,output=output,banks=[dict(address=BASE,before=raw.hex()),constants],before_fp=[(0x27f,0x37f,0x7f,0x67f,0xa7f)[n%5],((n%8)<<11)|((0,0x4500,0x100,0x400)[n%4]),(0x1f80,0x5f80,0x9fc0,0xffc0)[n%4]],stop_pc=STOP))
    for dt in(0,1/30,-.25,4):
        for ld in(0,.125,4):
            for ad in(0,.25,4):
                for torque in(0,1):make(f'finite_{dt}_{ld}_{ad}_{torque}',dt,ld,ad,torque,len(out)%2)
    addresses=(STEP,MOTION+0x2c,MOTION+0x14,MOTION+0x18,MOTION+0x40,MOTION+0x4c,MOTION+0x50,MOTION+0x5c,MOTION+0xf0,MOTION+0xfc,MOTION+0x100,MOTION+0x10c,MOTION+0xe0,MOTION+0x80,GRAVITY,GRAVITY+12)
    for address in addresses:
        for bits in(0x80000000,1,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'exception_{address:x}_{bits:x}',changes=((address,bits,4),),rotation=len(out)%2)
    pairs=((STEP,MOTION+0xf0),(MOTION+0x40,MOTION+0xf0),(MOTION+0x40,GRAVITY),(MOTION+0x14,MOTION+0x40),(MOTION+0x18,MOTION+0x50),(MOTION+0x100,MOTION+0x80),(MOTION+0xe0,MOTION+0x100),(MOTION+0x90,MOTION+0x50))
    for pair,(a,b)in enumerate(((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321))):
        for first,second in pairs:make(f'paired_nan_{pair}_{first:x}_{second:x}',changes=((first,a,4),(second,b,4)),rotation=pair%2)
    for output in(MOTION,MOTION+0x10,MOTION+0x30,MOTION+0x80,MOTION+0xe0):
        for torque in(0,1):make(f'alias_output_{output:x}_{torque}',torque=torque,rotation=1,output=output)
    for step in(MOTION+0x40,MOTION+0x100,MOTION+0x10c):make(f'alias_step_{step:x}',step=step,rotation=1)
    make('alias_gravity_cleared_force',gravity=MOTION+0xf0,rotation=1)
    make('read_fault_seed_torque',rotation=1)
    make('read_fault_seed_no_torque',torque=0,rotation=1)
    return out
def physical(image,m,cases,scratch):
    owned=scratch/'physical-original';owned.mkdir(exist_ok=True);observer=owned/'observer'
    if not observer.exists():subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_constraint_step_tu15.cpp'),'-o',str(observer)],check=True,capture_output=True)
    section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
    for c in cases:
        sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{text}"']
        for n,b in enumerate(c['banks']):
            path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
        stack=bytearray(0x10000);struct.pack_into('<4I',stack,SP-STACK,STOP,c['step'],c['gravity'],c['output']);(owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=c['before_fp'];env=bytearray(28);struct.pack_into('<H',env,0,cw);struct.pack_into('<H',env,4,sw);struct.pack_into('<H',env,8,0xffff);(owned/'environment.bin').write_bytes(env)
        assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${c['motion']},%ecx
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long 0x325410
sse_control: .long {mxcsr}
environment: .incbin "{owned/'environment.bin'}"
''');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
        r=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'output.json'),hex(c['stop_pc'])],capture_output=True,text=True);assert r.returncode==0,(c['name'],r.stdout,r.stderr)
        observed=json.loads((owned/'output.json').read_text())
        for b,after in zip(c['banks'],observed.pop('banks')):assert b['address']==after['address'];b['after']=after['after']
        c.update(observed,physical_verified=True)
    return cases
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=True)
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);all_cases=physical(image,m,cases(image),scratch);faults=[]
    for seed in[c for c in all_cases if c['name'].startswith('read_fault_seed')]:
        seen=set();occurrences={}
        for read in seed['reads']:
            key=(read['address'],read['bytes']);occurrences[key]=occurrences.get(key,0)+1
            if read['instruction']in seen:continue
            seen.add(read['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_fault_'+hex(read['instruction']),stop_pc=read['instruction'],fault=dict(address=read['address'],bytes=read['bytes'],occurrence=occurrences[key]));faults.append(c)
    all_cases+=physical(image,m,faults,scratch)
    f=dict(image_sha256=NEW_PIN,required_backend='unchanged-tu15-dynamic-accumulator-physical-operands-full-fp-v1',body_sites=[124,24,53],listing_sha256=m['listing_sha256'],cases=all_cases);fixture=scratch/'accumulator.json';fixture.write_text(json.dumps(f,separators=(',',':')))
    project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_dynamic_accumulator.cpp';text=source.read_text();mutation='accumulator_rotation_component(4,s.rotation_component(1),';assert text.count(mutation)==1;(project/'mutated.cpp').write_text(text.replace(mutation,'accumulator_rotation_component(5,s.rotation_component(1),'))
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],"--oracle-only",sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and"ordered accumulator store"in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Physical original rejects incorrect rotation transpose:",r.stderr.strip())
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_accumulator_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name,src in(('native_tu15_dynamic_accumulator_tests',source),('native_tu15_dynamic_accumulator_mutation',project/'mutated.cpp')):
        cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_dynamic_accumulator_tests.cpp" "{src}" "{ROOT}/src/physics/native_tu15_motion_forces.cpp")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    cmake+=f'add_test(NAME native_accumulator_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_dynamic_accumulator_tests> "{fixture}")\nadd_test(NAME native_accumulator_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_tu15_dynamic_accumulator_mutation> "{fixture}")\n';(project/'CMakeLists.txt').write_text(cmake)
    results=[]
    for compiler in('g++','clang++'):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_tu15_dynamic_accumulator_tests','--target','native_tu15_dynamic_accumulator_mutation','--filter','^native_accumulator_(original_tu15|mutation_rejected)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
    files=(source,ROOT/'include/halo2/native_tu15_dynamic_accumulator.hpp',ROOT/'tests/native_tu15_dynamic_accumulator_tests.cpp',ROOT/'tools/observe_native_constraint_step_tu15.cpp',ROOT/'src/physics/native_tu15_motion_forces.cpp',ROOT/'include/halo2/native_tu15_motion_forces.hpp',Path(__file__))
    report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(all_cases),complete_original_calls=sum('fault'not in c for c in all_cases),read_fault_prefixes=len(faults),paired_nan_cases=sum(c['name'].startswith('paired_nan')for c in all_cases),ordered_original_reads=sum(len(c['reads'])for c in all_cases),ordered_original_stores=sum(len(c['writes'])for c in all_cases),physical_original_instructions=sum(c['original_instructions']for c in all_cases),image_sha256=NEW_PIN,body_sites=[124,24,53],listing_sha256=m['listing_sha256'],required_backend=f['required_backend'],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},owned_scratch=str(scratch),negative_control='Private mutation writes wrong destination for transpose off-diagonal; unchanged physical original rejects ordered store',retained_large_artifacts=[],excluded=['unmasked platform traps','torque-enabled incoming x87 depth5 exceeds reused helper declared limit','full live constrained world composition'])
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','read_fault_prefixes','paired_nan_cases','ordered_original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
