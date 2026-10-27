#!/usr/bin/env python3
"""Physical unchanged TU1.5 built-in custom-schema callback qualification.

All three genuine cdecl callbacks execute unchanged instructions. Their input
banks, alias cases, exact ordered accesses and complete FP state are compared
with proper typed native kernels. No executable patch or math outcome provider.
Run through run_test.py; all raw banks and compiler products are disposable.
"""
import argparse,copy,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,PAYLOAD,FIRST,SECOND,ROW=0x1000000,0x1000100,0x1000300,0x1000400,0x1000500
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x1000),)
OWNERS=(0x31f3a0,0x31f410,0x31f560)
BACKEND='unchanged-tu15-custom-schema-three-owners-physical-fp-v1'
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def metadata(image):
    code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
    for entry in OWNERS:
        body=complete_body(code,entry);counts.append(len(body.code));listing=[]
        for pc in sorted(body.code):
            i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
            for op in i.operands:
                if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
                access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access;assert access&(CS_AC_READ|CS_AC_WRITE)
                base,index=i.reg_name(op.mem.base),i.reg_name(op.mem.index);assert not op.mem.segment
                operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
            sites.append(dict(instruction=pc,operands=operands))
        listings[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
    assert counts==[34,26,60],counts
    return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[dict(address=p,bytes=n)for p,n in BANKS])
def cases(image):
    out=[]
    def raw(seed):
        rng=random.Random(seed);data=bytearray(0x1000)
        for p in range(0,len(data),4):struct.pack_into('<f',data,p,rng.uniform(-4,4))
        struct.pack_into('<I',data,PAYLOAD-BASE+0x28,FIRST);struct.pack_into('<I',data,PAYLOAD-BASE+0x2c,SECOND);struct.pack_into('<I',data,PAYLOAD-BASE+0x10,ROW)
        return data
    def put(data,p,v):struct.pack_into('<I',data,p-BASE,v)
    def add(name,owner,data):
        n=len(out);out.append(dict(name=f'{owner:x}_{name}',entry=owner,row=0xf1234567,payload=PAYLOAD,banks=[dict(address=BASE,before=data.hex())],before_fp=[((0x27f,0x37f,0x7f)[n%3]&~0xc00)|((n%4)<<10),((n%8)<<11)|(0x453f if n%5==0 else 0),(0x1f80,0x5fa5,0xffc0)[n%3]]))
    for owner in OWNERS:
        for seed in range(48):add(f'finite_{seed}',owner,raw(seed))
        offsets=(PAYLOAD+0x20,FIRST+0x30,FIRST+0x38,SECOND+0x3c)if owner==OWNERS[0]else(PAYLOAD,PAYLOAD+0xc,PAYLOAD+0x1c)if owner==OWNERS[1]else(PAYLOAD,ROW,ROW+0xc,ROW+0x30,ROW+0x3c)
        for p in offsets:
            for value in(0x80000000,1,0x00800000,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
                data=raw(7);put(data,p,value);add(f'edge_{p:x}_{value:x}',owner,data)
        if owner==OWNERS[0]:
            for a,b in((0x7fc12345,0xffc54321),(0x7f812345,0xff854321)):
                for p in(FIRST+0x30,FIRST+0x38):
                    data=raw(7);put(data,PAYLOAD+0x20,a);put(data,p,b);add(f'paired_nan_{p:x}_{a:x}',owner,data)
        if owner!=OWNERS[2]:
            data=raw(7);put(data,PAYLOAD+0x2c,FIRST);add('alias_bodies',owner,data)
            if owner==OWNERS[1]:
                data=raw(7);put(data,PAYLOAD+0x28,PAYLOAD-0x30);put(data,PAYLOAD+0x2c,PAYLOAD-0x20);add('alias_source_output',owner,data)
        else:
            data=raw(7);put(data,PAYLOAD+0x10,PAYLOAD);add('alias_payload_jacobian',owner,data)
            for a,b in((0x7fc12345,0xffc54321),(0x7f812345,0xff854321)):
                for p in(ROW,ROW+0xc,ROW+0x30):
                    data=raw(7);put(data,PAYLOAD,a);put(data,p,b);add(f'paired_nan_{p:x}_{a:x}',owner,data)
        add('read_fault_seed',owner,raw(7))
    return out

def physical(image, m, inputs, scratch):
    owned = scratch / 'physical-original'
    owned.mkdir(exist_ok=True)
    observer = owned / 'observer'
    if not observer.exists():
        subprocess.run(['c++', '-std=c++20', '-O2', str(ROOT / 'tools/observe_native_constraint_step_tu15.cpp'), '-o', str(observer)], check=True, capture_output=True)
    section = next(s for s in image.sections if s.name == '.text')
    text = owned / 'text.bin'
    text.write_bytes(image.section_bytes(section))
    (owned / 'metadata.json').write_text(json.dumps(m, separators=(',', ':')))
    for case in inputs:
        sections = [f'.original 0x{section.va:x} : {{ *(.original) }}']
        assembly = [f'.section .original,"ax"\n.incbin "{text}"']
        for n, bank in enumerate(case['banks']):
            path = owned / f'bank{n}.bin'
            path.write_bytes(bytes.fromhex(bank['before']))
            assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"')
            sections.append(f'.bank{n} 0x{bank["address"]:x} : {{ *(.bank{n}) }}')
        stack = bytearray(0x10000)
        struct.pack_into('<3I', stack, SP - STACK, STOP, case['row'], case['payload'])
        (owned / 'stack.bin').write_bytes(stack)
        assembly.append(f'.section .stack,"aw"\n.incbin "{owned / "stack.bin"}"')
        sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw, sw, mxcsr = case['before_fp']
        environment = bytearray(28)
        struct.pack_into('<H', environment, 0, cw)
        struct.pack_into('<H', environment, 4, sw)
        struct.pack_into('<H', environment, 8, 0xffff)
        (owned / 'environment.bin').write_bytes(environment)
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
entry: .long {case['entry']}
sse_control: .long {mxcsr}
environment: .incbin "{owned / 'environment.bin'}"
''')
        sections.append('.bridge 0x70000000 : { *(.bridge) }')
        (owned / 'reference.s').write_text('\n'.join(assembly))
        (owned / 'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n' + '\n'.join(sections) + '\n}\n')
        subprocess.run(['as', '--32', str(owned / 'reference.s'), '-o', str(owned / 'reference.o')], check=True, capture_output=True)
        subprocess.run(['ld', '-melf_i386', '-T', str(owned / 'link.ld'), str(owned / 'reference.o'), '-o', str(owned / 'reference')], check=True, capture_output=True)
        result = subprocess.run([str(observer), str(owned / 'reference'), str(owned / 'metadata.json'), str(owned / 'output.json'), hex(case.get('stop_pc', STOP))], text=True, capture_output=True)
        assert result.returncode == 0, (case['name'], result.stdout, result.stderr)
        observed = json.loads((owned / 'output.json').read_text())
        for bank, after in zip(case['banks'], observed.pop('banks')):
            assert bank['address'] == after['address']
            bank['after'] = after['after']
        case.update(observed, physical_verified=True)
        if 'fault' not in case and 'write_fault' not in case and 'observer_fault' not in case:
            assert case['boundary_gp'][4] == SP + 4, (case['name'], 'Actual cdecl RET boundary changed')
    return inputs


def fault_cases(complete):
    out=[]
    for seed in(c for c in complete if c['name'].endswith('read_fault_seed')):
        seen,occurrences=set(),{}
        for read in seed['reads']:
            key=read['address'],read['bytes'];occurrences[key]=occurrences.get(key,0)+1
            if read['instruction']in seen:continue
            seen.add(read['instruction']);case=copy.deepcopy(seed);case.update(name=seed['name']+'_fault_'+hex(read['instruction']),stop_pc=read['instruction'],fault=dict(address=read['address'],bytes=read['bytes'],occurrence=occurrences[key]));out.append(case)
    return out
def store_cases(image,complete):
    code=_Code(image,'.text');out=[]
    for seed in(c for c in complete if c['name'].endswith('read_fault_seed')):
        occurrences={}
        for index,write in enumerate(seed['writes']):
            key=write['address'],len(write['written'])//2;occurrences[key]=occurrences.get(key,0)+1
            rejected=copy.deepcopy(seed);rejected.update(name=seed['name']+'_rejected_store_'+hex(write['instruction']),stop_pc=write['instruction'],write_fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key]));out.append(rejected)
            delivered=copy.deepcopy(seed);delivered.update(name=seed['name']+'_observer_after_store_'+hex(write['instruction']),stop_pc=write['instruction']+code.at(write['instruction']).size,observer_fault=index+1);out.append(delivered)
    return out
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);complete=physical(image,m,cases(image),scratch);faults=fault_cases(complete);stores=store_cases(image,complete);all_cases=complete+physical(image,m,faults+stores,scratch);fixture=scratch/'custom-schemas.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=m['body_sites'],cases=all_cases),separators=(',',':')))
    project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_constraint_custom_schema.cpp';text=source.read_text();(project/source.name).write_text(text);assert text.count('if(n==2)')==1;(project/'mutated.cpp').write_text(text.replace('if(n==2)','if(false)'));rejection='catch(...){asm volatile("frstor %0"::"m"(prior):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}'
    assert text.count(rejection)==1;(project/'precommit.cpp').write_text(text.replace(rejection,'catch(...){throw;}'));(project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)\nassert r.returncode!=0 and (sys.argv[3] in r.stderr),(r.returncode,r.stdout,r.stderr)\nprint("Original custom schema rejects changed source operand order:",r.stderr.strip())\n')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_custom_schema_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for kind in('original','negative','precommit'):
        target='native_custom_schema_'+kind;cpp=source.name if kind=='original'else'mutated.cpp'if kind=='negative'else'precommit.cpp';cmake+=f'add_executable({target} "{ROOT}/tests/native_tu15_constraint_custom_schema_tests.cpp" "{project}/{cpp}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=f'$<TARGET_FILE:{target}> "{fixture}"'if kind=='original'else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}" "{chr(111)+"riginal custom read order differs" if kind=="negative" else "complete custom schema FP differs"}"';cmake+=f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    for compiler in('g++','clang++'):
        cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),'--target','native_custom_schema_original','--target','native_custom_schema_negative','--target','native_custom_schema_precommit','--filter','^native_custom_schema_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode,command=cmd))
    files=[source,ROOT/'include/halo2/native_tu15_constraint_custom_schema.hpp',ROOT/'tests/native_tu15_constraint_custom_schema_tests.cpp',ROOT/'tools/observe_native_constraint_step_tu15.cpp',Path(__file__)];hashes={str(p.relative_to(ROOT)):sha(project/p.name if p==source else p)for p in files};report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,required_backend=BACKEND,image_sha256=NEW_PIN,listing_sha256=m['listing_sha256'],body_sites=m['body_sites'],original_cases=len(all_cases),complete_original_calls=len(complete),read_fault_prefixes=len(faults),rejected_store_prefixes=sum('write_fault'in c for c in all_cases),post_store_observer_rejections=sum('observer_fault'in c for c in all_cases),ordered_original_reads=sum(len(c['reads'])for c in all_cases),ordered_original_stores=sum(len(c['writes'])for c in all_cases),original_instructions=sum(c['original_instructions']for c in all_cases),fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes=hashes,current_sources_match_snapshot=all(sha(ROOT/p)==v for p,v in hashes.items()),owned_scratch=str(scratch),retained_large_artifacts=[],negative_controls=['Source scale lane2 load/multiply operand order changed','Source FSTP commits pop/C1 before rejecting actual memory write'],excluded=['unknown registered callback owners','full solver/live composition','unmasked FP traps']);(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','read_fault_prefixes')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
