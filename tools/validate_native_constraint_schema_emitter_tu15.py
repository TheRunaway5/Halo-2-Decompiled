#!/usr/bin/env python3
"""Physical actual32A390 schema emission, including genuine argument aliases.

The unchanged original executes its actual cdecl argument reads and stores.
The native typed owner reads callback identity at the same late point. No
callback is executed or supplied an outcome. All raw products are disposable.
"""
import argparse,copy,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_constraint_initializer_tu15 import descriptors
BASE,PACKET,SCHEMA=0x1000000,0x1000100,0x1000300
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x1000),(SP+4,16))
BACKEND='unchanged-tu15-schema14-emitter-late-argument-alias-v1'
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def cases():
    out=[]
    def add(name,callback=0x31f3a0,size=0x30,schema=SCHEMA,packet=PACKET):
        data=bytearray(random.Random(0x32a390+len(out)).randbytes(0x1000));args=[callback,size,0xf1234567,packet]
        if BASE<=packet+4<BASE+len(data):struct.pack_into('<I',data,packet+4-BASE,schema)
        argbank=bytearray(struct.pack('<4I',*args))
        if SP+4<=packet+4<SP+20:struct.pack_into('<I',argbank,packet+4-SP-4,schema)
        # The actual call's arguments are exactly these retained source cells.
        args=list(struct.unpack('<4I',argbank));out.append(dict(name=name,arguments=args,banks=[dict(address=BASE,before=data.hex()),dict(address=SP+4,before=argbank.hex())],before_fp=[0x27f,(len(out)%8)<<11,0x1f80],stop_pc=STOP))
    for callback in(0x31f3a0,0x31f410,0x31f560,0,0xf1234567):
        for size in(0,1,0x20,0x30,0xff,0x100,0x10000,0xfffffff0,0xfffffff8,0xffffffff):add(f'callback_{callback:x}_bytes_{size:x}',callback,size)
    for offset in(0,4,8,12):add(f'packet_schema_alias_{offset}',schema=PACKET+offset)
    for offset in(4,8,12):add(f'actual_argument_schema_alias_{offset}',schema=SP+offset)
    add('packet_argument_alias',packet=SP,schema=SCHEMA)
    add('read_fault_seed')
    return out
def physical(image,meta,inputs,scratch):
    owned=scratch/'physical-original';owned.mkdir(exist_ok=True);observer=owned/'observer'
    if not observer.exists():subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_constraint_step_tu15.cpp'),'-o',str(observer)],check=True,capture_output=True)
    (owned/'metadata.json').write_text(json.dumps(meta,separators=(',',':')));section=next(s for s in image.sections if s.name=='.text');(owned/'text.bin').write_bytes(image.section_bytes(section))
    for case in inputs:
        (owned/'data.bin').write_bytes(bytes.fromhex(case['banks'][0]['before']));stack=bytearray(0x10000);struct.pack_into('<5I',stack,SP-STACK,STOP,*case['arguments']);(owned/'stack.bin').write_bytes(stack);env=bytearray(28);cw,sw,mxcsr=case['before_fp'];struct.pack_into('<H',env,0,cw);struct.pack_into('<H',env,4,sw);struct.pack_into('<H',env,8,0xffff);(owned/'environment.bin').write_bytes(env)
        (owned/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{owned/'text.bin'}"
.section .data,"aw"
.incbin "{owned/'data.bin'}"
.section .stack,"aw"
.incbin "{owned/'stack.bin'}"
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
entry:.long 0x32a390
sse_control:.long {mxcsr}
environment:.incbin "{owned/'environment.bin'}"
''');(owned/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS {{\n.original 0x{section.va:x} : {{ *(.original) }}\n.data 0x{BASE:x} : {{ *(.data) }}\n.stack 0x{STACK:x} : {{ *(.stack) }}\n.bridge 0x70000000 : {{ *(.bridge) }}\n}}\n')
        subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True);r=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'observed.json'),hex(case['stop_pc'])],text=True,capture_output=True);assert r.returncode==0,(case['name'],r.stderr)
        observed=json.loads((owned/'observed.json').read_text());assert observed['after_fp']==case['before_fp']
        for bank,after in zip(case['banks'],observed.pop('banks')):assert bank['address']==after['address'];bank['after']=after['after']
        case.update(observed,physical_verified=True)
        if not any(kind in case for kind in('fault','write_fault','observer_fault')):assert case['boundary_gp'][4]==SP+4
    return inputs
def prefixes(image,complete):
    seed=next(c for c in complete if c['name']=='read_fault_seed');out=[];code=_Code(image,'.text');occurrences={}
    for read in seed['reads']:
        key=read['address'],read['bytes'];occurrences[key]=occurrences.get(key,0)+1;c=copy.deepcopy(seed);c.update(name='read_fault_'+hex(read['instruction']),stop_pc=read['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key]));out.append(c)
    for index,store in enumerate(seed['writes']):
        c=copy.deepcopy(seed);c.update(name='write_fault_'+hex(store['instruction']),stop_pc=store['instruction'],write_fault=dict(address=store['address'],bytes=len(store['written'])//2,occurrence=1));out.append(c)
        c=copy.deepcopy(seed);c.update(name='observer_after_'+hex(store['instruction']),stop_pc=store['instruction']+code.at(store['instruction']).size,observer_fault=index+1);out.append(c)
    return out
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');body=complete_body(code,0x32a390);assert len(body.code)==15;instructions=[code.at(pc)for pc in sorted(body.code)];listing=''.join(f'{i.address:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in instructions);meta=dict(sites=descriptors(instructions),regions=[dict(address=p,bytes=n)for p,n in BANKS]);complete=physical(image,meta,cases(),scratch);faults=prefixes(image,complete);all_cases=complete+physical(image,meta,faults,scratch);fixture=scratch/'emitter.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=15,cases=all_cases),separators=(',',':')))
    project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_constraint_schema_emitter.cpp';text=source.read_text();(project/source.name).write_text(text);old='word(m,observe,0x32a3a8u,schema,encoded);';assert text.count(old)==1;mutated=text.replace(old,'const auto owner=callback();'+old).replace('if(phase)phase(0x32a3aau);const auto owner=callback();','if(phase)phase(0x32a3aau);');(project/'mutated.cpp').write_text(mutated);(project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)\nassert r.returncode!=0 and ("original emitter read order differs" in r.stderr or "ordered emitter store differs" in r.stderr),(r.returncode,r.stdout,r.stderr)\nprint("Actual original emitter rejects early callback argument capture:",r.stderr.strip())\n')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_schema_emitter_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for kind in('original','negative'):
        target='native_schema_emitter_'+kind;cpp=source.name if kind=='original'else'mutated.cpp';cmake+=f'add_executable({target} "{ROOT}/tests/native_tu15_constraint_schema_emitter_tests.cpp" "{project}/{cpp}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\n';command=f'$<TARGET_FILE:{target}> "{fixture}"'if kind=='original'else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}"';cmake+=f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    for compiler in('g++','clang++'):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),'--target','native_schema_emitter_original','--target','native_schema_emitter_negative','--filter','^native_schema_emitter_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
    files=[source,ROOT/'include/halo2/native_tu15_constraint_schema_emitter.hpp',ROOT/'tests/native_tu15_constraint_schema_emitter_tests.cpp',ROOT/'tools/observe_native_constraint_step_tu15.cpp',Path(__file__)];hashes={str(p.relative_to(ROOT)):sha(project/p.name if p==source else p)for p in files};report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,required_backend=BACKEND,image_sha256=NEW_PIN,listing_sha256={'0x32a390':hashlib.sha256(listing.encode()).hexdigest()},body_sites=15,original_cases=len(all_cases),complete_original_calls=len(complete),read_fault_prefixes=sum('fault'in c for c in all_cases),rejected_store_prefixes=sum('write_fault'in c for c in all_cases),post_store_observer_rejections=sum('observer_fault'in c for c in all_cases),ordered_original_reads=sum(len(c['reads'])for c in all_cases),ordered_original_stores=sum(len(c['writes'])for c in all_cases),original_instructions=sum(c['original_instructions']for c in all_cases),source_hashes=hashes,current_sources_match_snapshot=all(sha(ROOT/p)==v for p,v in hashes.items()),fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,owned_scratch=str(scratch),retained_large_artifacts=[],negative_control='Actual callback argument read moved before header publication',excluded=['execution/qualification of arbitrary emitted callback identities','native full constraint producer bodies','generated internal-call seam and live world participation']);(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','read_fault_prefixes')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
