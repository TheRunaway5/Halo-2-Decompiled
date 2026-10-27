#!/usr/bin/env python3
"""Physical original TU1.5 island-deactivation/controller qualification.

Unchanged original code supplies retained read/store order and full FP state.
Actual default policy3233C0 executes to completion; custom31E5C0/31E690 stop
before execution. No external callback result is replayed or manufactured.
All generated data/ELFs/builds belong to enclosing run_test.py scratch.
"""
import argparse,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,ISLAND,STEP,WORLD,OUTPUT=0x1000000,0x1000100,0x1000200,0x1000300,0x1000900
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x1000),(0x41e000,0x2000))
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def original_bytes(image,p,n):
    for s in image.sections:
        raw=image.section_bytes(s)
        if s.va<=p and p+n<=s.va+len(raw):return raw[p-s.va:p-s.va+n]
    raise AssertionError(('Missing original bytes',hex(p),n))
def metadata(image):
    code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
    for entry in (0x320c80,0x3233c0):
        body=complete_body(code,entry);counts.append(len(body.code));listing=[]
        for pc in sorted(body.code):
            i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
            for op in i.operands:
                if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
                access=CS_AC_WRITE if i.mnemonic in ('fst','fstp')else op.access
                assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),'Missing architectural operand access')
                base=i.reg_name(op.mem.base);index=i.reg_name(op.mem.index);assert not op.mem.segment
                operands.append(dict(base=registers[base]if base else -1,index=registers[index]if index else -1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
            sites.append(dict(instruction=pc,operands=operands))
        listings[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
    assert counts==[119,3]
    assert struct.unpack('<2I',original_bytes(image,0x41eb80,8))==(0x31e5c0,0x31e690)
    assert struct.unpack('<2I',original_bytes(image,0x41f1c4,8))==(0x3233c0,0x3233c0)
    return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[dict(address=p,bytes=n)for p,n in BANKS])
def cases(image):
    table=dict(address=0x41e000,before=original_bytes(image,0x41e000,0x2000).hex());out=[]
    def make(name,short,long,seconds,count,checks,cw=0x27f,sw=0,mxcsr=0x1f80,policy=None,alias=None,output=OUTPUT,changes=(),fault=None,stop=None):
        raw=bytearray(0x1000);put=lambda p,v,n=4:raw.__setitem__(slice(p-BASE,p-BASE+n),(v&((1<<(n*8))-1)).to_bytes(n,'little'));number=lambda p,v:raw.__setitem__(slice(p-BASE,p-BASE+4),struct.pack('<f',v))
        put(ISLAND+0x20,WORLD);number(ISLAND+0x34,short);number(ISLAND+0x38,long);put(ISLAND+0x2c,checks,1);number(WORLD+0x30,1);number(WORLD+0x34,2);put(ISLAND+0x40,count);put(ISLAND+0x3c,BASE+0x500)
        for n in range(3):put(BASE+0x500+n*4,BASE+0x600+n*0x80);put(BASE+0x600+n*0x80+0x54,BASE+0x780 if policy else 0)
        if policy:put(BASE+0x780,0x41f1b0 if policy=='default'else 0x41eb6c);put(BASE+0x786,1,2)
        step=STEP if alias is None else alias;number(step,seconds);put(OUTPUT,0xaabbccdd)
        for address,bits,width in changes:put(address,bits,width)
        boundary=None
        if policy in ('short','long'):
            owner=0x31e5c0 if policy=='short'else 0x31e690;boundary=dict(policy=BASE+0x780,owner=owner,body=BASE+0x600,period=0 if policy=='short'else 1)
        c=dict(name=name,entry=0x320c80,receiver=ISLAND,arguments=[output,step],island=ISLAND,step=step,output=output,banks=[dict(address=BASE,before=raw.hex()),table],boundary=boundary,before_fp=[cw,sw,mxcsr],default_policy=policy=='default')
        if fault:c['fault']=fault;c['stop_pc']=stop;c['boundary']=None
        else:c['stop_pc']=boundary['owner']if boundary else STOP
        out.append(c)
    for phase,(short,long,seconds)in enumerate(((0,0,0),(0,0,.5),(.5,0,.5),(.5,0,.50000006),(0,2,.125),(1,2,.125),(2,4,-.5),(1e-38,1e-38,1e-38))):
        for count in (0,1,3,0xffffffff):
            for checks in (0,4,5,0x7f,0x80,0xff):
                n=len(out);make(f'phase_{phase}_count_{count}_checks_{checks}',short,long,seconds,count,checks,cw=(0x27f,0x37f,0x107f,0x67f)[n%4],sw=(n%8)<<11,mxcsr=(0x1f80,0x5f80,0x9fc0)[n%3])
    for period,(short,long)in (('short',(1.5,0)),('long',(0,2.5))):
        for checks in (0,5,0x80,0xff):make(f'authentic_policy_prefix_{period}_{checks}',short,long,.125,2,checks,policy=period)
        for count in (1,3):
            for checks in (0,5,0x80):make(f'default_policy_complete_{period}_{count}_{checks}',short,long,.125,count,checks,policy='default')
    for alias in (ISLAND+0x34,ISLAND+0x38,WORLD+0x30):
        for seconds in (0,.5,1.25):make(f'alias_step_{alias:x}_{seconds}',1.5,2.5,seconds,0,5,alias=alias)
    for output in (ISLAND+0x2c,ISLAND+0x34,ISLAND+0x38,ISLAND+0x40):make(f'alias_output_{output:x}',1.5,2.5,.125,0,5,output=output)
    for address in (STEP,ISLAND+0x34,ISLAND+0x38,WORLD+0x30,WORLD+0x34):
        for bits in (0x80000000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'exception_{address:x}_{bits:x}',1.5,2.5,.125,0,5,changes=((address,bits,4),))
    for n,(a,b)in enumerate(((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321))):
        for first,second in ((STEP,ISLAND+0x34),(ISLAND+0x38,STEP),(ISLAND+0x34,WORLD+0x30),(ISLAND+0x38,WORLD+0x34)):
            make(f'paired_nan_{n}_{first:x}_{second:x}',1.5,2.5,.125,0,5,changes=((first,a,4),(second,b,4)))
    faults=(('seconds',STEP,4,1,0x320c84),('short_elapsed',ISLAND+0x34,4,1,0x320c8a),('world',ISLAND+0x20,4,1,0x320c8d),('long_elapsed',ISLAND+0x38,4,1,0x320c94),('seconds_reload',STEP,4,2,0x320c97),('short_elapsed_reload',ISLAND+0x34,4,2,0x320ca1),('short_interval',WORLD+0x30,4,1,0x320ca7),('long_interval',WORLD+0x34,4,1,0x320cbb),('short_wrap_load',ISLAND+0x34,4,3,0x320ccf),('cached_count',ISLAND+0x40,4,1,0x320cd2),('short_wrap_interval',WORLD+0x30,4,2,0x320cd5),('bodies',ISLAND+0x3c,4,1,0x320ce3),('policy',BASE+0x654,4,1,0x320cec),('policy_reread',BASE+0x654,4,2,0x320cf5),('policy_vtable',BASE+0x780,4,1,0x320cf8),('policy_owner',0x41eb80,4,1,0x320d00))
    for name,address,width,occurrence,pc in faults:make('fault_'+name,1.5,0,.125,2,5,policy='short',fault=dict(address=address,bytes=width,occurrence=occurrence),stop=pc)
    make('fault_short_counter',1.5,0,.125,0,5,fault=dict(address=ISLAND+0x2c,bytes=1,occurrence=1),stop=0x320d16)
    make('fault_long_wrap_count',0,2.5,.125,0,5,fault=dict(address=ISLAND+0x40,bytes=4,occurrence=1),stop=0x320d48)
    make('fault_long_wrap_interval',0,2.5,.125,0,5,fault=dict(address=WORLD+0x34,bytes=4,occurrence=2),stop=0x320d4b)
    return out
def physical(image,m,cases,scratch):
    owned=scratch/'physical-original';owned.mkdir();observer=owned/'observer';subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_motion_pose_tu15.cpp'),'-o',str(observer)],check=True,capture_output=True)
    section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
    for c in cases:
        sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{text}"']
        for n,b in enumerate(c['banks']):path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
        stack=bytearray(0x10000);struct.pack_into('<3I',stack,SP-STACK,STOP,c['output'],c['step']);(owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=c['before_fp'];env=bytearray(28);struct.pack_into('<H',env,0,cw);struct.pack_into('<H',env,4,sw);struct.pack_into('<H',env,8,0xffff);(owned/'environment.bin').write_bytes(env)
        assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${ISLAND},%ecx
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long 0x320c80
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
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);all_cases=physical(image,m,cases(image),scratch)
    f=dict(image_sha256=NEW_PIN,required_backend='unchanged-tu15-deactivation-physical-operands-stores-full-fp-v2',deactivation_sites=119,default_policy_sites=3,listing_sha256=m['listing_sha256'],cases=all_cases);fixture=scratch/'deactivation.json';fixture.write_text(json.dumps(f,separators=(',',':')))
    project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_island_deactivation.cpp';text=source.read_text();mutation='std::bit_cast<std::int8_t>(checks)<5';assert text.count(mutation)==1;(project/'mutated.cpp').write_text(text.replace(mutation,'checks<5'))
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],"--oracle-only",sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and "ordered deactivation store" in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Physical original rejects unsigned short-check counter:",r.stderr.strip())
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_deactivation_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name,src in (('native_tu15_island_deactivation_tests',source),('native_tu15_island_deactivation_mutation',project/'mutated.cpp')):cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_island_deactivation_tests.cpp" "{src}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    cmake+=f'add_test(NAME native_deactivation_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_island_deactivation_tests> "{fixture}")\nadd_test(NAME native_deactivation_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_tu15_island_deactivation_mutation> "{fixture}")\n';(project/'CMakeLists.txt').write_text(cmake)
    results=[]
    for compiler in ('g++','clang++'):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_tu15_island_deactivation_tests','--target','native_tu15_island_deactivation_mutation','--filter','^native_deactivation_(original_tu15|mutation_rejected)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
    files=(source,ROOT/'include/halo2/native_tu15_island_deactivation.hpp',ROOT/'tests/native_tu15_island_deactivation_tests.cpp',ROOT/'tools/observe_native_motion_pose_tu15.cpp',Path(__file__))
    report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(all_cases),complete_original_calls=sum(c['boundary']is None and'fault'not in c for c in all_cases),authentic_policy_stops=sum(c['boundary']is not None for c in all_cases),default_policy_complete_calls=sum(c['default_policy']for c in all_cases),read_fault_prefixes=sum('fault'in c for c in all_cases),paired_nan_cases=sum(c['name'].startswith('paired_nan')for c in all_cases),ordered_original_reads=sum(len(c['reads'])for c in all_cases),ordered_original_stores=sum(len(c['writes'])for c in all_cases),physical_original_instructions=sum(c['original_instructions']for c in all_cases),image_sha256=NEW_PIN,deactivation_sites=119,default_policy_sites=3,listing_sha256=f['listing_sha256'],required_backend=f['required_backend'],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},owned_scratch=str(scratch),negative_control='Private source changes signed8counter test to unsigned; physical original must reject ordered store',retained_large_artifacts=[],excluded=['custom31E5C0/31E690 policy execution','live full world composition','unmasked platform traps'])
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','read_fault_prefixes','paired_nan_cases','ordered_original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
