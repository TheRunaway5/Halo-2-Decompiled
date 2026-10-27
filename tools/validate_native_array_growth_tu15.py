#!/usr/bin/env python3
"""Offline real-i386 TU1.5 array/pool proof using genuine allocator execution.

Only the actual initialized before-input pool/global bytes are extracted from
the retained MCC multiplayer reference. Original instructions allocate/free;
no pointer outcome or source callee result is supplied. All raw banks, ELF,
observer and CMake outputs belong to the enclosing run_test.py owner.
"""
import argparse,hashlib,json,lzma,os,random,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ARCHIVE=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-original-v1457/required-failure-inputs-retirement/actual-failed-captures.tar.zst'
MEMBER='iteration-00-lower-body1-phase0/original/required-failure-inputs/'
UI,DATA,HEADER=0x4000000,0x4001000,0x4000000
STACK,SP,STOP=0x2000000,0x200f000,0x5000000
OWNERS=(0x2e3250,0x2e2e30,0x149da0,0x149e00,0x233250,0x233420)
FILES=('include/halo2/native_tu15_array_growth.hpp','src/physics/native_tu15_array_growth.cpp','include/halo2/native_tu15_array_allocator.hpp','src/physics/native_tu15_array_allocator.cpp','tests/native_tu15_array_growth_tests.cpp','tools/observe_native_array_growth_tu15.cpp')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def word(raw,p):return struct.unpack_from('<I',raw,p)[0]
def reference_inputs(image):
    manifest=json.loads(lzma.decompress(ARCHIVE.with_name('all-file-hashes.json.xz').read_bytes()))
    def read(name):
        member=MEMBER+name;raw=subprocess.check_output(['tar','--zstd','-xOf',str(ARCHIVE),member]);assert len(raw)==manifest[member]['bytes'] and hashlib.sha256(raw).hexdigest()==manifest[member]['sha256'];return raw
    capture_raw=read('capture.json');capture=json.loads(capture_raw)
    needed=(0x48c938,0x486064,0x486068,0x48606c,0x486070,0x486080,0x10076040)
    selected={r['before_file']:r for p in needed for r in capture['regions'] if r['address']<=p and p+4<=r['address']+r['size']}
    banks={r['address']:read(name) for name,r in selected.items()}
    def get(p):return next(word(raw,p-first) for first,raw in banks.items() if first<=p and p+4<=first+len(raw))
    assert get(0x48c938)==0x486080 and get(0x486064)==2 and get(0x48606c)==0x10076040 and get(0x10076040)==0x4638b8 and get(0x486080)==0x45f4cc
    for p in (0x45f4cc,0x4638b8):banks[p]=image.read(p,24)
    proof=dict(archive_sha256=sha(ARCHIVE),capture_sha256=hashlib.sha256(capture_raw).hexdigest(),members={name:manifest[MEMBER+name] for name in selected},initial_pool=0x10076040,mode=2,allocator=0x486080,allocator_vtable=0x45f4cc,pool_vtable=0x4638b8,allocation_outcomes_supplied=False)
    return banks,proof
def metadata(image,banks):
    code=_Code(image,'.text');sites=[];counts=[];listings={};gp={name:i for i,name in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
    for owner in OWNERS:
        body=complete_body(code,owner);counts.append(len(body.code));lines=[]
        for pc in sorted(body.code):
            ins=code.at(pc);lines.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n');operands=[]
            if ins.mnemonic!='lea':
                for op in ins.operands:
                    if op.type!=X86_OP_MEM:continue
                    base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index)
                    assert not op.mem.segment or ins.reg_name(op.mem.segment)=='es'
                    operands.append(dict(base=gp[base] if base else -1,index=gp[index] if index else -1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
            sites.append(dict(instruction=pc,operands=operands))
        listings[hex(owner)]=hashlib.sha256(''.join(lines).encode()).hexdigest()
    return dict(sites=sites,regions=[dict(address=p,bytes=len(raw)) for p,raw in sorted(banks.items())]),dict(body_sites=counts,listing_sha256=listings)
def cases(quick):
    rows=[]
    def add(name,entry,args,**more):rows.append(dict(name=name,entry=entry,arguments=args,receiver=0x486080 if entry in (0x149da0,0x149e00) else 0x10076040,header=HEADER,**more))
    for count in ((0,1,4) if quick else (0,1,2,3,4,7,8)):
        for stride in ((1,4,128) if quick else (1,3,4,16,128)):add(f'borrowed_{count}_{stride}',0x2e3250,[HEADER,stride],count=count,capacity=count|0x80000000)
    for count in (1,4):
        for stride in (4,128):add(f'owned_{count}_{stride}',0x2e3250,[HEADER,stride],count=count,capacity=count,prepare_bytes=count*stride)
    for size in ((1,513,1024,8192,8193) if quick else (0,1,8,9,16,17,32,33,128,256,512,513,1024,2048,4096,8192,8193)):add(f'pool_allocate_{size}',0x233250,[size,0x12])
    for size in (4,1024):add(f'router_allocate_{size}',0x149da0,[size,0x12])
    for entry in (0x149e00,0x233420):
        for size in (4,1024):add(f'release_{entry:x}_{size}',entry,[0,size,0x12],prepare_bytes=size,prepare_argument=True)
    for size in ((0,1,3,4,5,7,8,15) if quick else range(18)):
        for offset in ((1,5) if quick else (-3,0,1,3,16)):add(f'copy_{size}_{offset}',0x2e2e30,[DATA+offset,DATA,size])
    for name,p,n,occurrence,writing in (('count',HEADER+4,4,1,False),('allocation_owner',0x45f4dc,4,1,False),('data_after_allocation',HEADER,4,1,False),('capacity',HEADER+8,4,1,False),('publication',HEADER+8,4,1,True)):
        add('fault_'+name,0x2e3250,[HEADER,128],count=4,capacity=0x80000004,fault=dict(address=p,bytes=n,occurrence=occurrence,write=writing))
    def mutation(pc,p,value):return dict(instruction=pc,address=p,written=struct.pack('<I',value).hex())
    for name,changes in (('count_reread',[mutation(0x23328f,HEADER+4,1)]),('data_reread',[mutation(0x23328f,HEADER,DATA+0x800)]),('ownership_reread',[mutation(0x2e2e43,HEADER+8,0x80000004)]),('cached_capacity',[mutation(0x2e32b3,HEADER+4,99)])):
        owned=name=='ownership_reread';add('alias_'+name,0x2e3250,[HEADER,128],count=4,capacity=4 if owned else 0x80000004,mutations=changes,**({'prepare_bytes':512} if owned else {}))
    if not quick:
        for count,stride in ((0x80000000,0),(0x80000001,2),(0x40000001,4),(0xffffffff,0)):add(f'wrap_{count:x}_{stride}',0x2e3250,[HEADER,stride],count=count,capacity=count|0x80000000)
        add('capacity_bit30_owned',0x2e3250,[HEADER,128],count=4,capacity=0x40000004,prepare_bytes=512)
        for size in (8,32,128,256,512,1024,4096,8192):add(f'refill_{size}',0x233250,[size,0x12],drain_class=size)
        add('refill_new_small_page',0x233250,[512,0x12],drain_class=512,fill_small_page=True)
        add('real_pool_page_exhaustion',0x233250,[8192,0x12],drain_class=8192,exhaust_pages=True)
    return rows
def original(image,banks,all_cases,scratch):
    owned=scratch/'original';owned.mkdir();observer=owned/'observer';subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_array_growth_tu15.cpp'),'-o',str(observer)],check=True,capture_output=True)
    flat=bytearray(0x7f0000)
    for section in image.sections:
        raw=image.section_bytes(section);flat[section.va-0x10000:section.va-0x10000+len(raw)]=raw
    for p,raw in banks.items():
        if 0x10000<=p and p+len(raw)<=0x800000:flat[p-0x10000:p-0x10000+len(raw)]=raw
    (owned/'image.bin').write_bytes(flat);m,source=metadata(image,{**banks,UI:bytes(0x10000)});fixture=scratch/'fixture';fixture.mkdir();(fixture/'banks').mkdir()
    def persist(raw):
        target=fixture/'banks'/(hashlib.sha256(raw).hexdigest()+'.bin')
        if not target.exists():target.write_bytes(raw)
        return str(target.relative_to(fixture))
    for case in all_cases:
        user=bytearray(random.Random(12345).randbytes(0x10000));struct.pack_into('<3I',user,0,DATA,case.get('count',0),case.get('capacity',0))
        for offset,value in case.get('user_words',[]):struct.pack_into('<I',user,offset,value)
        (owned/'user.bin').write_bytes(user);sections=['.image 0x10000 : { *(.image) }','.user 0x4000000 : { *(.user) }'];assembly=[f'.section .image,"awx"\n.incbin "{owned/"image.bin"}"',f'.section .user,"aw"\n.incbin "{owned/"user.bin"}"']
        for i,(p,raw) in enumerate(sorted(banks.items())):
            if p<0x800000:continue
            target=owned/f'bank{i}.bin';target.write_bytes(raw);assembly.append(f'.section .bank{i},"aw"\n.incbin "{target}"');sections.append(f'.bank{i} 0x{p:x} : {{ *(.bank{i}) }}')
        stack=bytearray(0x10000);struct.pack_into('<I',stack,SP-STACK,STOP)
        for n,value in enumerate(case['arguments']):struct.pack_into('<I',stack,SP-STACK+4+n*4,value)
        (owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        index=all_cases.index(case);fp=[0x27f|((index%4)<<10),((index%8)<<11)|((index%4)<<8)|(index%32),0x1f80|((index%4)<<13)];case['before_fp']=fp
        environment=bytearray(28);struct.pack_into('<H',environment,0,fp[0]);struct.pack_into('<H',environment,4,fp[1]);struct.pack_into('<H',environment,8,0xffff);(owned/'env.bin').write_bytes(environment)
        prepare=''
        if 'drain_class'in case:
            size=case['drain_class'];pool=next(raw for p,raw in banks.items() if p<=0x10076040 and 0x10076040+0xf03cc<=p+len(raw));pool_first=next(p for p,raw in banks.items() if raw is pool);offset=0x10076040-pool_first
            index=struct.unpack_from('<b',pool,offset+0xac+size)[0] if size<=512 else word(pool,offset+0x2b0+((size-1)>>10)*4)
            iterations=word(pool,offset+0xf0388+index*4)
            if case.get('fill_small_page'):iterations+=(word(pool,offset+0x1c)-word(pool,offset+0x20))//word(pool,offset+0x68+index*4)
            if case.get('exhaust_pages'):iterations+=0x76-word(pool,offset+0xf0340)
            if iterations:prepare=f'mov ${SP-0x100},%esp\n mov ${iterations},%ebx\n drain_class:\n push $0x12\n push ${size}\n mov $0x10076040,%ecx\n call 0x233250\n dec %ebx\n jne drain_class\n'
        if 'prepare_bytes'in case:
            prepare=f'mov ${SP-0x100},%esp\n push $0x12\n push ${case["prepare_bytes"]}\n mov $0x486080,%ecx\n call 0x149da0\n'
            if case.get('prepare_argument'):prepare+=f'mov %eax,{SP+4}\n'
            else:prepare+=f'mov %eax,{HEADER}\n mov %eax,%edi\n mov ${DATA},%esi\n mov ${case.get("count",0)*case["arguments"][1]},%ecx\n rep movsb\n'
        assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 cld
 fninit
 fldenv environment
 ldmxcsr sse_control
 {prepare}
 mov ${SP},%esp
 mov ${case['receiver']},%ecx
 int3
 jmp *entry
entry: .long {case['entry']}
sse_control: .long {fp[2]}
environment: .incbin "{owned/'env.bin'}"
''');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
        active=dict(m,**{k:case[k] for k in ('fault','mutations') if k in case})
        if case.get('retained_arguments'):
            active['regions']=[*m['regions'],dict(address=SP+4,bytes=4*len(case['arguments']))]
        (owned/'metadata.json').write_text(json.dumps(active,separators=(',',':')))
        result=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'output.json'),hex(STOP)],capture_output=True,text=True);assert result.returncode==0,(case['name'],result.stdout,result.stderr)
        observed=json.loads((owned/'output.json').read_text());assert observed['mutations_applied']==len(case.get('mutations',[])) or observed['declared_fault']
        if case.get('prepare_argument'):case['arguments'][0]=observed['allocator_calls'][0]['data']
        before={b['address']:b['before'] for b in observed.pop('initial_banks')};after=observed.pop('banks');case['banks']=[dict(address=b['address'],before_file=persist(bytes.fromhex(before[b['address']])),after_file=persist(bytes.fromhex(b['after']))) for b in after];case.update(observed)
    return fixture,source
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--quick',action='store_true');args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    report=dict(passed=False,source_sha256={p:sha(ROOT/p) for p in FILES},validator_sha256=sha(__file__),scratch_cleanup_owner=str(scratch),allocator_outcomes_supplied=False,live_array_integration_proven=False,retained_large_artifacts=[])
    def save():(args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    save()
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,inputs=reference_inputs(image);all_cases=cases(args.quick);fixture_dir,source=original(image,banks,all_cases,scratch)
        fixture=fixture_dir/'original.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases),separators=(',',':')))
        executed={pc for c in all_cases for pc in c['executed_sites']};code=_Code(image,'.text')
        report.update(original_cases=len(all_cases),physical_instructions=sum(c['original_instructions'] for c in all_cases),original_ordered_reads=sum(len(c['reads']) for c in all_cases),original_ordered_stores=sum(len(c['writes']) for c in all_cases),fault_prefixes=sum(c['declared_fault'] for c in all_cases),mutations=sum(c['mutations_applied'] for c in all_cases),actual_allocator_calls=sum(len(c['allocator_calls']) for c in all_cases),fixture_sha256=sha(fixture),source_authority=source,input_provenance=inputs,all_original_instructions_unchanged=True,all_source_writes_and_retained_banks_compared=True,forward_copy_abi_direction_flag_clear=True,body_coverage={hex(owner):dict(total=len(complete_body(code,owner).code),executed=len(set(complete_body(code,owner).code)&executed)) for owner in OWNERS});save()
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        for p in FILES:
            target=project/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/p,target)
        units='src/physics/native_tu15_array_growth.cpp src/physics/native_tu15_array_allocator.cpp tests/native_tu15_array_growth_tests.cpp'
        (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(native_array_original LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(native_tu15_array_growth_tests '+units+')\ntarget_include_directories(native_tu15_array_growth_tests PRIVATE include)\ntarget_compile_features(native_tu15_array_growth_tests PRIVATE cxx_std_20)\ntarget_compile_options(native_tu15_array_growth_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(native_tu15_array_growth_tests PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME native_array_original_tu15 COMMAND native_tu15_array_growth_tests "'+str(fixture)+'")\n')
        def validate(label,compiler='g++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/label),'--target','native_tu15_array_growth_tests','--filter','^native_array_original_tu15$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];report.setdefault('commands',[]).append(command);save();return subprocess.run(command,check=False).returncode
        assert validate('native-gcc')==0,'Native array/pool differs; inspect native-gcc/tests.log'
        assert validate('native-clang','clang++')==0,'Clang native array/pool differs; inspect native-clang/tests.log'
        changes=(('capacity','src/physics/native_tu15_array_growth.cpp','initial_count+initial_count','initial_count+1u'),('copy-granularity','src/physics/native_tu15_array_growth.cpp','const auto value=s.copy_word(source);s.copy_word(destination,value);','for(unsigned i=0;i<4;++i){const auto value=s.copy_byte(source+i);s.copy_byte(destination+i,value);}'),('pool-accounting','src/physics/native_tu15_array_allocator.cpp','change(s,Counter::in_use,index,1,0x23328f);','change(s,Counter::in_use,index,0xffffffffu,0x23328f);'))
        for name,path,before,after in changes:
            target=project/path;text=target.read_text();assert text.count(before)==1;target.write_text(text.replace(before,after,1));result=validate('negative-'+name);log=(args.output/('negative-'+name)/'tests.log').read_text();assert result!=0 and any(reason in log for reason in ('array store differs','complete array/pool bank differs','actual allocator request order differs')),'Original array failed to reject '+name;target.write_text(text)
        report.update(passed=True,negative_controls_rejected=[row[0] for row in changes],remaining_explicit_owners=['default allocator149930','actual fixed/secondary virtual allocators','pool release>2000 imported CRT free'],full_heap_native_closure_claimed=False)
    except BaseException as error:report['error']=str(error);raise
    finally:save()
    print(json.dumps({k:v for k,v in report.items() if k not in ('source_sha256','command','input_provenance')}))
if __name__=='__main__':main()
