#!/usr/bin/env python3
"""Unchanged physical TU1.5 island transition and notification qualification.

No dependency result is supplied: original default listener2E4D60 executes;
array growth and custom listeners stop at their actual source entry. Generated
fixtures, ELF, observer and compiler builds belong to run_test.py scratch.
"""
import argparse,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,WORLD,ISLAND=0x1000000,0x1000100,0x1000500
ACTIVE,INACTIVE,WL,BODIES,BODY,BL,LISTENER=BASE+0x1000,BASE+0x1100,BASE+0x1200,BASE+0x1300,BASE+0x1800,BASE+0x2000,BASE+0x2800
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x4000),(0x41d000,0x1000))
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def original_bytes(image,p,n):
    for s in image.sections:
        raw=image.section_bytes(s)
        if s.va<=p and p+n<=s.va+len(raw):return raw[p-s.va:p-s.va+n]
    raise AssertionError(('Missing original bytes',hex(p),n))
def metadata(image):
    code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
    for entry in(0x316cd0,0x322740,0x2e4d60):
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
    assert counts==[39,98,1]and original_bytes(image,0x2e4d60,3)==bytes.fromhex('c20400')
    assert struct.unpack('<2I',original_bytes(image,0x41d95c,8))==(0x2e4d60,0x2e4d60)
    assert original_bytes(image,0x3164ff,7)==bytes.fromhex('c7400858d94100')
    return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[dict(address=p,bytes=n)for p,n in BANKS])
def cases(image):
    table=dict(address=0x41d000,before=original_bytes(image,0x41d000,0x1000).hex());out=[]
    def make(name,entry=0x316cd0,active_count=3,index=0,inactive_count=1,world_pattern=(),body_count=0,body_pattern=(),capacity=8,world_count=None):
        raw=bytearray(0x4000)
        def put(p,v,n=4):raw[p-BASE:p-BASE+n]=(v&((1<<(8*n))-1)).to_bytes(n,'little')
        put(WORLD+8,ACTIVE);put(WORLD+0xc,active_count);put(WORLD+0x14,INACTIVE);put(WORLD+0x18,inactive_count);put(WORLD+0x1c,capacity)
        for n in range(active_count):put(ACTIVE+n*4,ISLAND if n==index else BASE+0x600+n*0x100)
        put(ISLAND+0x24,index);put(ISLAND+0x29,1,1);put(WORLD+0x78,WL);put(WORLD+0x7c,len(world_pattern)if world_count is None else world_count)
        put(ISLAND+0x3c,BODIES);put(ISLAND+0x40,body_count)
        for n in range(8):put(LISTENER+n*0x10,0x41d958)
        boundary=None
        for n,kind in enumerate(world_pattern):
            receiver=LISTENER+n*0x10 if kind else 0;put(WL+n*4,receiver)
            if kind==2:put(receiver,0x41d964);boundary=dict(kind='island_listener',receiver=receiver,owner=0x316790,argument=ISLAND)
        for n in range(min(body_count,3)):
            body=BODY+n*0x100;put(BODIES+n*4,body);put(body+0x70,BL+n*0x80);put(body+0x74,len(body_pattern))
            for k,kind in enumerate(body_pattern):
                receiver=LISTENER+k*0x10 if kind else 0;put(BL+n*0x80+k*4,receiver)
                if kind==2:put(receiver,0x41d964);boundary=dict(kind='body_listener',receiver=receiver,owner=0x3167a0,argument=body)
        if entry==0x316cd0 and inactive_count==(capacity&0x7fffffff):boundary=dict(kind='growth',receiver=WORLD+0x14,owner=0x2e3250,argument=4)
        n=len(out);out.append(dict(name=name,entry=entry,world=WORLD,island=ISLAND,banks=[dict(address=BASE,before=raw.hex()),table],boundary=boundary,before_fp=[(0x27f,0x37f,0x67f)[n%3],(n%8)<<11,(0x1f80,0x5f80,0x9fc0)[n%3]],stop_pc=boundary['owner']if boundary else STOP))
    patterns=((),(0,),(1,),(0,1,0,1),(1,0,1,0),(1,1,1))
    for count in(1,3,4):
        for index in range(count):
            for inactive in(0,2):
                n=len(out);make(f'transition_{count}_{index}_{inactive}',active_count=count,index=index,inactive_count=inactive,world_pattern=patterns[n%6],body_count=n%4,body_pattern=patterns[(n+2)%6],capacity=8 if n%2 else 0x80000008)
    for wi,wp in enumerate(patterns):
        for bi,bp in enumerate(patterns):
            for count in(0,1,3):make(f'notifications_{wi}_{bi}_{count}',entry=0x322740,world_pattern=wp,body_count=count,body_pattern=bp)
    for count in(0xffffffff,0x80000001):make(f'negative_world_count_{count:x}',entry=0x322740,world_count=count)
    for count in(0xffffffff,0x80000000):make(f'negative_body_count_{count:x}',entry=0x322740,body_count=count)
    for count in(0,1,3):make(f'actual_growth_prefix_{count}',inactive_count=count,capacity=count)
    make('actual_custom_island_prefix',entry=0x322740,world_pattern=(2,))
    make('actual_custom_body_prefix',entry=0x322740,body_count=1,body_pattern=(2,))
    make('read_fault_seed',active_count=3,index=1,world_pattern=(0,1,0,1),body_count=2,body_pattern=(0,1,0))
    return out
def physical(image,m,cases,scratch):
    owned=scratch/'physical-original';owned.mkdir(exist_ok=True);observer=owned/'observer'
    if not observer.exists():subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_motion_pose_tu15.cpp'),'-o',str(observer)],check=True,capture_output=True)
    section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
    for c in cases:
        sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{text}"']
        for n,b in enumerate(c['banks']):
            path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
        stack=bytearray(0x10000);args=[ISLAND]if c['entry']==0x316cd0 else[WORLD,ISLAND];struct.pack_into('<'+'I'*(1+len(args)),stack,SP-STACK,STOP,*args)
        (owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=c['before_fp'];env=bytearray(28);struct.pack_into('<H',env,0,cw);struct.pack_into('<H',env,4,sw);struct.pack_into('<H',env,8,0xffff);(owned/'environment.bin').write_bytes(env)
        assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${WORLD},%ecx
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long {c['entry']}
sse_control: .long {mxcsr}
environment: .incbin "{owned/'environment.bin'}"
''');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
        result=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'output.json'),hex(c['stop_pc'])],capture_output=True,text=True);assert result.returncode==0,(c['name'],result.stdout,result.stderr)
        observed=json.loads((owned/'output.json').read_text())
        for b,after in zip(c['banks'],observed.pop('banks')):assert b['address']==after['address'];b['after']=after['after']
        c.update(observed,physical_verified=True)
    return cases
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=True)
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);all_cases=physical(image,m,cases(image),scratch)
    seed=next(c for c in all_cases if c['name']=='read_fault_seed');seen=set();occurrences={};faults=[]
    for read in seed['reads']:
        key=(read['address'],read['bytes']);occurrences[key]=occurrences.get(key,0)+1
        if read['instruction']in seen:continue
        seen.add(read['instruction']);c=copy.deepcopy(seed);c.update(name='fault_'+hex(read['instruction']),boundary=None,stop_pc=read['instruction'],fault=dict(address=read['address'],bytes=read['bytes'],occurrence=occurrences[key]));faults.append(c)
    all_cases+=physical(image,m,faults,scratch)
    f=dict(image_sha256=NEW_PIN,required_backend='unchanged-tu15-island-transition-physical-operands-stores-full-fp-v1',transition_sites=39,notification_sites=98,default_listener_sites=1,listing_sha256=m['listing_sha256'],cases=all_cases)
    fixture=scratch/'transition.json';fixture.write_text(json.dumps(f,separators=(',',':')));project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_island_transition.cpp';text=source.read_text();mutation='const auto inactive_index=s.count(inactive)-1u;';assert text.count(mutation)==1;(project/'mutated.cpp').write_text(text.replace(mutation,'const auto inactive_index=s.count(inactive);'))
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],"--oracle-only",sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and "ordered transition store"in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Physical original rejects incorrect inactive island index:",r.stderr.strip())
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_transition_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name,src in(('native_tu15_island_transition_tests',source),('native_tu15_island_transition_mutation',project/'mutated.cpp')):
        cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_island_transition_tests.cpp" "{src}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    cmake+=f'add_test(NAME native_transition_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_island_transition_tests> "{fixture}")\nadd_test(NAME native_transition_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_tu15_island_transition_mutation> "{fixture}")\n';(project/'CMakeLists.txt').write_text(cmake)
    results=[]
    for compiler in('g++','clang++'):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_tu15_island_transition_tests','--target','native_tu15_island_transition_mutation','--filter','^native_transition_(original_tu15|mutation_rejected)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
    files=(source,ROOT/'include/halo2/native_tu15_island_transition.hpp',ROOT/'tests/native_tu15_island_transition_tests.cpp',ROOT/'tools/observe_native_motion_pose_tu15.cpp',Path(__file__))
    report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(all_cases),complete_original_calls=sum(c['boundary']is None and'fault'not in c for c in all_cases),dependency_entry_stops=sum(c['boundary']is not None for c in all_cases),read_fault_prefixes=len(faults),ordered_original_reads=sum(len(c['reads'])for c in all_cases),ordered_original_stores=sum(len(c['writes'])for c in all_cases),physical_original_instructions=sum(c['original_instructions']for c in all_cases),image_sha256=NEW_PIN,transition_sites=39,notification_sites=98,default_listener_sites=1,listing_sha256=m['listing_sha256'],required_backend=f['required_backend'],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},owned_scratch=str(scratch),negative_control='Private mutation stores count instead of count-1 as inactive island index; unchanged physical original must reject ordered store',retained_large_artifacts=[],excluded=['array growth2E3250 execution','custom listener execution','full live world composition'])
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','read_fault_prefixes','ordered_original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
