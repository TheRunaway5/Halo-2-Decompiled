#!/usr/bin/env python3
"""Physical original constraint326200 prefixes and actual dependency ABIs.

Unchanged original roots execute through native fixed/kinematic accumulator
setup. Unconverted allocation, dynamic setup, builder and core solver stop at
their genuine entries; their physical arguments/packets are observed rather
than calculated by a second implementation or supplied as outcomes.
"""
import argparse,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,ISLAND,STEP,SOLVER,INPUT,WORKSPACE,ALLOCATOR=0x1000000,0x1000100,0x1000200,0x1000300,0x1000400,0x1000500,0x1000600
BODY_ARRAY,CONSTRAINT_ARRAY,BODY,MOTION,ARENA,EXTRA=BASE+0x700,BASE+0x800,BASE+0x1000,BASE+0x2000,BASE+0x4000,BASE+0x8000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x10000),(0x41e000,0x2000),(0x45f000,0x1000),(0x48c000,0x1000),(0x491000,0x1000))
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def metadata(image):
    code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
    for entry in(0x326200,0x323560,0x3248d0,0x324800):
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
    assert counts==[306,25,17,3]
    assert struct.unpack('<2I',image.read(0x41f26c,8))==(0x3248d0,0x324800)
    assert struct.unpack('<I',image.read(0x41f1ec,4))[0]==0x323560
    assert struct.unpack('<I',image.read(0x41edb4,4))[0]==0x31f770
    assert image.read(0x31f728,6)==bytes.fromhex('c70690ed4100')
    return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[dict(address=p,bytes=n)for p,n in BANKS])
def cases(image):
    out=[]
    def make(name,bodies=2,kind='fixed',arena_mode='fits',results=0,constraints=0,fallback=False,changes=()):
        banks={p:bytearray(image.read(p,n)if p in(0x41e000,0x45f000)else bytes(n))for p,n in BANKS}
        def put(p,v,n=4):
            bank=next(lo for lo,size in BANKS if lo<=p and p+n<=lo+size);banks[bank][p-bank:p-bank+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
        body_bytes=bodies*0x80;jacobian_bytes=0x100;schema_bytes=0x40
        arena_bytes={'fits':0x1000,'whole':0x10,'jacobians':body_bytes+0xa0,'schemas':body_bytes+0x194}[arena_mode]
        put(0x491350,WORKSPACE);put(WORKSPACE+8,ARENA);put(WORKSPACE+0xc,arena_bytes);put(0x48c938,ALLOCATOR);put(ALLOCATOR,0x45f4cc);put(ALLOCATOR+8,EXTRA);put(ALLOCATOR+0xc,0 if fallback else 0x2000);put(ALLOCATOR+0x14,EXTRA+0x1000)
        put(ISLAND+8,0x40);put(ISLAND+0xc,jacobian_bytes);put(ISLAND+0x10,schema_bytes);put(ISLAND+0x14,body_bytes);put(ISLAND+0x18,results)
        for n in range(max(bodies,2)):
            put(BODY_ARRAY+n*4,BODY+n*0x100);put(BODY+n*0x100+0x3c,MOTION+n*0x100);put(MOTION+n*0x100,{'fixed':0x41f260,'kinematic':0x41f1e0,'dynamic':0x41f2e0}[kind]);put(MOTION+n*0x100+0x7c,0x3f800000)
            for offset in(0x40,0x50,0x70):put(MOTION+n*0x100+offset,0x3f000000+n)
        put(CONSTRAINT_ARRAY,BASE+0xa00);put(BASE+0xa00,0x41ed90);put(BASE+0xa0c,BODY);put(BASE+0xa10,BODY+0x100)
        for p,v,width in changes:put(p,v,width)
        stop=0x2e10a0 if fallback and arena_mode!='fits'else 0x325410 if kind=='dynamic'and bodies else 0x31f770 if constraints else 0x326a40
        n=len(out);out.append(dict(name=name,entry=0x326200,arguments=[STEP,SOLVER,INPUT,BASE+0x900,ISLAND,BODY_ARRAY,bodies,CONSTRAINT_ARRAY,constraints],banks=[dict(address=p,before=bytes(banks[p]).hex())for p,size in BANKS],before_fp=[(0x27f,0x37f,0x7f)[n%3],(n%8)<<11,(0x1f80,0x5f80,0x9fc0)[n%3]],stop_pc=stop))
    for bodies in(0,1,3):
        for kind in('fixed','kinematic','dynamic'):
            for mode in('fits','whole','jacobians','schemas'):
                for results in(0,1,5,0xffffffff):make(f'{kind}_{bodies}_{mode}_{results:x}',bodies,kind,mode,results)
    for mode in('fits','whole','jacobians','schemas'):
        make('actual_builder_'+mode,2,'fixed',mode,constraints=1)
        if mode!='fits':make('actual_allocator_'+mode,1,'fixed',mode,fallback=True)
    for bits in(0x80000000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
        make(f'kinematic_payload_{bits:x}',2,'kinematic',changes=((MOTION+0x40,bits,4),(MOTION+0x74,bits^0x80000000,4)))
    make('read_fault_seed_fixed',2,'fixed','whole',results=2)
    make('read_fault_seed_kinematic',2,'kinematic','jacobians',results=2)
    make('read_fault_seed_builder',2,'fixed','schemas',results=2,constraints=1)
    return out
def physical(image,m,cases,scratch):
    owned=scratch/'physical-original';owned.mkdir(exist_ok=True);observer=owned/'observer'
    if not observer.exists():subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_constraint_step_tu15.cpp'),'-o',str(observer)],check=True,capture_output=True)
    section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section))
    for c in cases:
        specific=dict(m)
        if c['stop_pc']==0x31f770 and'fault'not in c:specific['boundary_packet_stack_slot']=2
        (owned/'metadata.json').write_text(json.dumps(specific,separators=(',',':')))
        sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{text}"']
        for n,b in enumerate(c['banks']):
            path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
        stack=bytearray(0x10000);struct.pack_into('<10I',stack,SP-STACK,STOP,*c['arguments']);(owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=c['before_fp'];env=bytearray(28);struct.pack_into('<H',env,0,cw);struct.pack_into('<H',env,4,sw);struct.pack_into('<H',env,8,0xffff);(owned/'environment.bin').write_bytes(env)
        assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long 0x326200
sse_control: .long {mxcsr}
environment: .incbin "{owned/'environment.bin'}"
''');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
        r=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'output.json'),hex(c['stop_pc'])],capture_output=True,text=True);assert r.returncode==0,(c['name'],r.stdout,r.stderr)
        observed=json.loads((owned/'output.json').read_text())
        for b,after in zip(c['banks'],observed.pop('banks')):assert b['address']==after['address'];b['after']=after['after']
        c.update(observed,physical_verified=True);gp=c['boundary_gp'];stack=c['boundary_stack'];target=c['stop_pc']
        if'fault'in c:c['boundary']=None
        elif target==0x326a40:c['boundary']=dict(kind='solver',receiver=0,arguments=stack[1:5],packet=None)
        elif target==0x2e10a0:c['boundary']=dict(kind='allocate',receiver=gp[1],arguments=stack[1:2],packet=None)
        elif target==0x325410:c['boundary']=dict(kind='setup',receiver=gp[1],arguments=stack[1:4],packet=None)
        else:assert target==0x31f770;c['boundary']=dict(kind='builder',receiver=gp[1],arguments=stack[1:2],packet=list(struct.unpack('<4I',bytes.fromhex(c['boundary_packet']))))
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
    f=dict(image_sha256=NEW_PIN,required_backend='unchanged-tu15-constraint-step-physical-operands-actual-abi-v1',body_sites=[306,25,17,3],listing_sha256=m['listing_sha256'],cases=all_cases);fixture=scratch/'constraints.json';fixture.write_text(json.dumps(f,separators=(',',':')))
    project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_constraint_step.cpp';text=source.read_text();mutation='context.schemas,0x400u';assert text.count(mutation)==1;(project/'mutated.cpp').write_text(text.replace(mutation,'context.schemas,0u'))
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],"--oracle-only",sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and"ordered constraint store"in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Physical original rejects missing end-of-schema marker:",r.stderr.strip())
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_constraint_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name,src in(('native_tu15_constraint_step_tests',source),('native_tu15_constraint_step_mutation',project/'mutated.cpp')):
        cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_constraint_step_tests.cpp" "{src}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    cmake+=f'add_test(NAME native_constraint_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_constraint_step_tests> "{fixture}")\nadd_test(NAME native_constraint_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_tu15_constraint_step_mutation> "{fixture}")\n';(project/'CMakeLists.txt').write_text(cmake)
    results=[]
    for compiler in('g++','clang++'):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_tu15_constraint_step_tests','--target','native_tu15_constraint_step_mutation','--filter','^native_constraint_(original_tu15|mutation_rejected)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
    files=(source,ROOT/'include/halo2/native_tu15_constraint_step.hpp',ROOT/'tests/native_tu15_constraint_step_tests.cpp',ROOT/'tools/observe_native_constraint_step_tu15.cpp',Path(__file__))
    report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(all_cases),read_fault_prefixes=len(faults),ordered_original_reads=sum(len(c['reads'])for c in all_cases),ordered_original_stores=sum(len(c['writes'])for c in all_cases),physical_original_instructions=sum(c['original_instructions']for c in all_cases),image_sha256=NEW_PIN,body_sites=[306,25,17,3],listing_sha256=m['listing_sha256'],required_backend=f['required_backend'],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},owned_scratch=str(scratch),negative_control='Private mutation clears the actual400 end-of-schema marker; unchanged physical original rejects ordered store',retained_large_artifacts=[],excluded=['core326A40 execution','dynamic325410 setup','custom constraint builder execution','virtual allocation/release execution','post-core/profile/commit/cleanup original composition','full live world composition'])
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','read_fault_prefixes','ordered_original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
