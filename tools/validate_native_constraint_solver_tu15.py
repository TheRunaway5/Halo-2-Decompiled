#!/usr/bin/env python3
"""Physical actual326A40 schema/iteration math qualification, never emulation.

The initial basic domain qualifies common iteration, primary2/12/13 and all
final export classes. Additional families must be added and independently
qualified before production replacement. Sources are frozen in owned scratch
before compiling while other independent row families may still be developed.
"""
import argparse,copy,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

BASE,CONFIG,SCHEMA,ROW,ACCUM,RESULTS,OUTPUT=0x1000000,0x1000100,0x1000200,0x1001000,0x1003000,0x1005000,0x1006000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((CONFIG,0x100),(SCHEMA,0x400),(ROW,0x400),(ACCUM,0x180),(RESULTS,0x100),(OUTPUT,0x300),(0x46a000,0x1000),(0x48d000,0x1000))
BACKEND='unchanged-tu15-constraint-solver-physical-operands-full-fp-v1'
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def metadata(image):
    code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
    for entry in(0x326a40,0x326620,0x326630):
        body=complete_body(code,entry);counts.append(len(body.code));listing=[]
        for pc in sorted(body.code):
            i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
            for op in i.operands:
                if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
                access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access;assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),'Missing architectural access')
                base,index=i.reg_name(op.mem.base),i.reg_name(op.mem.index);assert not op.mem.segment
                operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
            sites.append(dict(instruction=pc,operands=operands))
        listings[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
    assert counts==[2208,4,5]
    return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[dict(address=p,bytes=n)for p,n in BANKS])
def put(case,address,value,n=4):
    for bank in case['banks']:
        data=bytearray.fromhex(bank['before']);offset=address-bank['address']
        if 0<=offset and offset+n<=len(data):data[offset:offset+n]=(value&((1<<(n*8))-1)).to_bytes(n,'little');bank['before']=data.hex();return
    raise AssertionError(('Unowned solver input',hex(address),n))
def cases(image,primary_domain):
    out=[]
    def make(name,types=(),iterations=1,seed=0):
        rng=random.Random(seed);raw=bytearray(0x8000)
        def word(p,v):struct.pack_into('<I',raw,p-BASE,v&0xffffffff)
        def number(p,v):struct.pack_into('<f',raw,p-BASE,v)
        def vector(p,values):struct.pack_into('<4f',raw,p-BASE,*values)
        for p,v in((CONFIG+4,.75),(CONFIG+8,.5),(CONFIG+0xc,.75),(CONFIG+0x10,1)):number(p,v)
        word(CONFIG+0x1c,iterations)
        for body in(ACCUM,ACCUM+0x80):
            raw[body-BASE]=2
            for offset in(0x10,0x20,0x30,0x40,0x50):vector(body+offset,[rng.uniform(-.5,.5)for _ in range(4)])
            vector(body+0x30,(.5,.25,.125,.75))
        raw[ACCUM+0x100-BASE]=3
        word(SCHEMA,0x400);word(SCHEMA+4,ROW);word(SCHEMA+8,OUTPUT);word(SCHEMA+0xc,ACCUM);word(SCHEMA+0x10,ACCUM+0x80);word(SCHEMA+0x14,8)
        schema,row,result=SCHEMA+0x18,ROW,RESULTS
        for kind in types:
            length={2:4,3:8,4:16,5:16,6:8,7:20,8:24,9:8,10:16,11:12,12:4,13:4}[kind]
            secondary=4 if kind in(4,9,10,12) else 5 if kind in(3,7) else 6 if kind==8 else 3
            word(schema,kind|(length<<8)|(secondary<<16))
            size=0x20 if kind in(4,9,10,12) else 0x60 if kind in(3,7) else 0x80 if kind==8 else 0x30
            for offset in range(0,size,16):vector(row+offset,[rng.uniform(-.5,.5)for _ in range(4)])
            angular=kind in(4,9,10,12);number(row+(0x1c if angular else 0xc),rng.uniform(-1,1));number(row+(0xc if angular else 0x1c),.75)
            count=2 if kind in(3,7) else 3 if kind==8 else 1
            if kind in(4,5):number(schema+4,.25);number(schema+8,-.25)
            if kind in(6,9):number(schema+4,.25)
            if kind in(10,11):number(schema+4,-.25);number(schema+8,.25)
            if kind==10:number(schema+12,.5)
            if kind==3:number(schema+4,.125)
            if kind in(7,8):word(schema+4,OUTPUT+0x100);number(schema+8,.125);number(schema+12,.5);word(schema+16,8)
            if kind==8:number(schema+20,1)
            if count>1:
                number(row+0x1c,.5);number(row+0x4c,.5);number(row+0x2c,.5);number(row+0x5c,.5);number(row+0x3c,rng.uniform(-1,1))
            if count==3:number(row+0x6c,.5);number(row+0x7c,rng.uniform(-1,1))
            for n in range(count):number(result+n*4,rng.uniform(-.25,.25))
            schema+=length;row+=size;result+=count*4
        word(schema,0x400)
        banks=[dict(address=p,before=(bytes(raw[p-BASE:p-BASE+n])if BASE<=p<BASE+len(raw)else image.read(p,n)if p==0x46a000 else bytes(n)).hex())for p,n in BANKS]
        n=len(out);c=dict(name=name,arguments=[CONFIG,SCHEMA,ACCUM,RESULTS],banks=banks,before_fp=[((0x27f,0x37f,0x7f)[n%3]&~0xc00)|((n%4)<<10),((n%8)<<11)|((n%4)<<8),(0x1f80,0x5f80,0x9fc0,0xffc0)[n%4]],boundary=None,stop_pc=STOP,primary_types=list(types),declared_iterations=iterations)
        put(c,0x48d480,1,1);out.append(c);return c
    groups=[()]+[(kind,)for kind in primary_domain]+[(kind,kind,kind)for kind in primary_domain]+[(13,12,2)]
    if 3 in primary_domain:groups.extend(((3,3,8),(7,7,8)))
    for kinds in groups:
        for iterations in(0,1,2,3):
            for seed in range(3):make('finite_'+''.join(f'{k}_'for k in kinds)+f'{iterations}_{seed}',kinds,iterations,seed)
    for kind in primary_domain:
        for address in(CONFIG+4,CONFIG+8,ROW,ROW+0xc,ROW+0x1c,ACCUM+0x10,ACCUM+0x20,RESULTS):
            for bits in(1,0x80000000,0x7f800000,0x7fc12345,0x7f812345):c=make(f'edge_{kind}_{address:x}_{bits:x}',(kind,),2,3);put(c,address,bits)
        for index,(first,second)in enumerate(((0x7fc12345,0xffc54321),(0x7f812345,0xff854321))):
            for left,right in((CONFIG+4,ROW+0xc),(ROW,ACCUM+0x10),(ROW+0x10,ACCUM+0x20),(CONFIG+8,RESULTS)):
                c=make(f'paired_nan_{kind}_{index}_{left:x}_{right:x}',(kind,),2,7);put(c,left,first);put(c,right,second)
        fields={2:(),3:(4,),4:(4,8),5:(4,8),6:(4,),7:(8,12),8:(8,12,20),9:(4,),10:(4,8,12),11:(4,8),12:(),13:()}[kind]
        for offset in fields:
            for bits in(0,0x80000000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
                c=make(f'schema_edge_{kind}_{offset:x}_{bits:x}',(kind,),2,6);put(c,SCHEMA+0x18+offset,bits)
        make(f'read_fault_seed_{kind}',(kind,kind),2,2)
    for kind,length in((2,4),(3,8),(4,16),(7,20),(8,24),(14,8),(15,12),(16,4)):
        c=make(f'export_class_{kind}',(),0,3);put(c,SCHEMA+0x18,kind|(length<<8));put(c,SCHEMA+0x18+length,0x400)
        for n in range(12):put(c,ROW+n*4,0x3f000000+n);put(c,RESULTS+n*4,0x3f400000+n)
        if kind in(7,8):put(c,SCHEMA+0x1c,OUTPUT+0x100);put(c,SCHEMA+0x28,8)
    for count in(1,2):
        c=make(f'config_single_saved_pair_{count}',(),count,3);p=SCHEMA+0x18
        for first,second in((0x3f000000,0x3e800000),(0x3e000000,0x3f400000)):
            put(c,p,15|(12<<8)|(2<<16));put(c,p+4,first);put(c,p+8,second);p+=12
        put(c,p,16|(4<<8)|(2<<16));put(c,p+4,0x400)
    if 3 in primary_domain:
        for first_bias,second_bias in((1,-8),(-1,-1),(1,1),(-8,1),(0,0)):
            c=make(f'pair3_isolated_control_{first_bias}_{second_bias}',(3,3),1,4)
            for body in(ACCUM,ACCUM+0x80):
                for offset in(0x10,0x20):
                    for lane in range(4):put(c,body+offset+lane*4,0)
            put(c,CONFIG+4,0x3f800000)
            for row in(ROW,ROW+0x60):
                for offset in range(0,0x60,4):put(c,row+offset,0)
                for offset in(0x1c,0x2c,0x4c,0x5c):put(c,row+offset,0x3f000000)
                for offset,value in((0xc,first_bias),(0x3c,second_bias)):put(c,row+offset,struct.unpack('<I',struct.pack('<f',value))[0])
        for kind in(7,8):
            length=20 if kind==7 else 24;c=make(f'radial_header_fastpath_{kind}',(kind,),2,4);header=SCHEMA+0x18+length
            for offset,value in((0,1|(0x18<<8)|(1<<16)),(4,ROW+0x200),(8,OUTPUT+0x200),(0xc,ACCUM),(0x10,ACCUM+0x80),(0x14,8)):
                put(c,header+offset,value)
            put(c,header+0x18,13|(4<<8)|(3<<16));put(c,header+0x1c,0x400);put(c,ROW+0x21c,0x3f800000)
    if 8 in primary_domain:
        for index,(first,second)in enumerate(((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321))):
            for left,right in((ROW+0xc,ROW+0x2c),(ROW+0x3c,ROW+0x5c),(SCHEMA+0x20,ROW+0xc),(SCHEMA+0x20,ROW+0x3c)):
                c=make(f'paired_nan_8_coupling_{index}_{left:x}_{right:x}',(8,),2,4);put(c,left,first);put(c,right,second)
    c=make('actual_initializer_entry',(),1);put(c,0x48d480,0,1);c.update(stop_pc=0x3268d0,boundary='initialize')
    return out
def physical(image,m,inputs,scratch):
    owned=scratch/'physical-original';owned.mkdir(exist_ok=True);observer=owned/'observer'
    if not observer.exists():subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_constraint_solver_tu15.cpp'),'-o',str(observer)],check=True,capture_output=True)
    section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
    for c in inputs:
        sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{text}"']
        for n,b in enumerate(c['banks']):
            path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
        stack=bytearray(0x10000);struct.pack_into('<5I',stack,SP-STACK,STOP,*c['arguments']);(owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
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
entry: .long 0x326a40
sse_control: .long {mxcsr}
environment: .incbin "{owned/'environment.bin'}"
''');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
        r=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'output.json'),hex(c['stop_pc'])],capture_output=True,text=True);assert r.returncode==0,(c['name'],r.stdout,r.stderr)
        observed=json.loads((owned/'output.json').read_text())
        for b,after in zip(c['banks'],observed.pop('banks')):assert b['address']==after['address'];b['after']=after['after']
        c.update(observed,physical_verified=True)
    return inputs
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--primary-domain',choices=('basic','all'),default='all');p.add_argument('--case-prefix',action='append',default=[]);a=p.parse_args();domain=[2,12,13]if a.primary_domain=='basic'else list(range(2,14));scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=True)
    project=scratch/'project';project.mkdir();root=ROOT/'src/physics/native_tu15_constraint_solver.cpp';rows=ROOT/'src/physics/native_tu15_constraint_solver_rows.cpp';internal=ROOT/'src/physics/native_tu15_constraint_solver_internal.hpp';root_text=root.read_text();rows_text=rows.read_text();(project/'native.cpp').write_text(root_text);(project/'rows.cpp').write_text(rows_text);(project/internal.name).write_bytes(internal.read_bytes());print(json.dumps(dict(frozen_solver_source_sha256=sha(project/'native.cpp'),frozen_rows_source_sha256=sha(project/'rows.cpp'))),flush=True)
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);inputs=cases(image,domain);inputs=[c for c in inputs if not a.case_prefix or c['name'].startswith(tuple(a.case_prefix))];assert inputs,'Empty filtered original solver domain';all_cases=physical(image,m,inputs,scratch);faults=[]
    for seed in[c for c in all_cases if c['name'].startswith(('read_fault_seed','export_class','pair3_isolated_control','radial_header_fastpath'))]:
        seen=set();occurrences={}
        for read in seed['reads']:
            key=(read['address'],read['bytes']);occurrences[key]=occurrences.get(key,0)+1
            if read['instruction']in seen:continue
            seen.add(read['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_fault_'+hex(read['instruction']),stop_pc=read['instruction'],fault=dict(address=read['address'],bytes=read['bytes'],occurrence=occurrences[key]));faults.append(c)
    all_cases+=physical(image,m,faults,scratch);fixture=scratch/'solver.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],cases=all_cases),separators=(',',':')))
    mutation='iteration!=0';assert root_text.count(mutation)==1;(project/'mutated.cpp').write_text(root_text.replace(mutation,'false'))
    impulse_mutation='fsts %0';assert rows_text.count(impulse_mutation)==1;(project/'rounded-rows.cpp').write_text(rows_text.replace(impulse_mutation,r'fstps %0\n\tflds %0'))
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and"ordered solver store differs"in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Unchanged original rejects replacing later additive warm starts with copies:",r.stderr.strip())
''')
    (project/'impulse-negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and("ordered solver store differs"in r.stderr or"complete solver FP differs"in r.stderr),(r.returncode,r.stdout,r.stderr)
print("Unchanged original rejects rounding away the retained extended impulse:",r.stderr.strip())
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_solver_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name,source,row_source in(('native_solver_tests',project/'native.cpp',project/'rows.cpp'),('native_solver_mutation',project/'mutated.cpp',project/'rows.cpp'),('native_solver_impulse_mutation',project/'native.cpp',project/'rounded-rows.cpp')):cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_constraint_solver_tests.cpp" "{source}" "{row_source}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    cmake+=f'add_test(NAME native_solver_original COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_solver_tests> "{fixture}")\nadd_test(NAME native_solver_negative COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_solver_mutation> "{fixture}")\n';(project/'CMakeLists.txt').write_text(cmake)
    with(project/'CMakeLists.txt').open('a')as output:output.write(f'add_test(NAME native_solver_impulse_negative COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/impulse-negative.py" $<TARGET_FILE:native_solver_impulse_mutation> "{fixture}")\n')
    results=[]
    for compiler in('g++','clang++'):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_solver_tests','--target','native_solver_mutation','--target','native_solver_impulse_mutation','--filter','^native_solver_(original|negative|impulse_negative)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
    files=(root,rows,internal,ROOT/'include/halo2/native_tu15_constraint_solver.hpp',ROOT/'tests/native_tu15_constraint_solver_tests.cpp',ROOT/'tools/observe_native_constraint_solver_tu15.cpp',Path(__file__));hashes={str(path.relative_to(ROOT)):sha(path)for path in files};hashes[str(root.relative_to(ROOT))]=sha(project/'native.cpp');hashes[str(rows.relative_to(ROOT))]=sha(project/'rows.cpp');hashes[str(internal.relative_to(ROOT))]=sha(project/internal.name)
    original_sites=complete_body(_Code(image,'.text'),0x326a40).code;covered={pc for c in all_cases for pc in c['executed_sites']};coverage=dict(root_sites=2208,covered_root_sites=len(set(original_sites)&covered),unvisited_root_sites=[hex(pc)for pc in sorted(set(original_sites)-covered)])
    report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,tested_primary_domain=sorted({kind for c in all_cases for kind in c['primary_types']}),case_prefixes=a.case_prefix,complete_declared_case_matrix=not a.case_prefix,original_cases=len(all_cases),read_fault_prefixes=len(faults),ordered_original_stores=sum(len(c['writes'])for c in all_cases),ordered_original_reads=sum(len(c['reads'])for c in all_cases),physical_original_instructions=sum(c['original_instructions']for c in all_cases),image_sha256=NEW_PIN,required_backend=BACKEND,listing_sha256=m['listing_sha256'],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes=hashes,current_sources_match_snapshot=all(sha(ROOT/path)==value for path,value in hashes.items()),owned_scratch=str(scratch),retained_large_artifacts=[],excluded=['all controlbranch+faultdomain coverage remains bounded by declaredcases','initializerexecution/customschema14 execution','productionreplacement','unmaskedtraps'])
    report.update(original_instruction_coverage=coverage,negative_controls=['Later additive accumulator warm start replaced with copy','ActualFST retains extended impulse replaced byFSTP/FLD of roundedbinary32spill'])
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','read_fault_prefixes','ordered_original_stores')}));return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
