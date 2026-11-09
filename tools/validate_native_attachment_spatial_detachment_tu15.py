#!/usr/bin/env python3
"""Compare C5250/1D30F0 and real16E8A0 retirement with unchanged TU1.5.

Run through tools/run_test.py with the existing .tools/python interpreter.
Declared before-state pools are DATA; original instructions compute every
deletion, link and flag result. No source instruction or callee result changes.
"""
import argparse,copy,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ROOT=Path(__file__).resolve().parents[1];U=base.UI
ROOTOBJ=U+0x400;INNER=U+0x600;OUTER=U+0x700;LIGHTPOOL=U+0x800;OBJECTPOOL=U+0x900
HEADERS=U+0x1000;LIGHTS=U+0x2000;OUTROWS=U+0x4000;INROWS=U+0x5000
INBITS=U+0x6000;OUTBITS=U+0x6100;HEADS=U+0x6200;HEAD=U+0x6300;OTHERHEAD=U+0x6304;OBJECT=U+0x7000
KEY=0x12340002;OUTDATUM=0x23450002;OBJECTDATUM=0x45670002


def save(path,value):path.write_text(json.dumps(value,indent=2)+'\n')


def state(flags=10,outer_stride=12,inner_stride=24,debug=0,length=3,kind='middle',slot=-1):
    words=[]
    def put(p,v):words.append((p-U,v&0xffffffff))
    for first,extent in((LIGHTS,0x1000),(OUTROWS,0x400),(INROWS,0x400),(HEADERS,0x100)):
        for p in range(first,first+extent,4):put(p,0)
    for root in(ROOTOBJ,):
        put(root,HEADS);put(root+4,INNER);put(root+8,OUTER)
    for pool,rows,stride,bits,live in((OUTER,OUTROWS,outer_stride,OUTBITS,(1,2,4)),(INNER,INROWS,inner_stride,INBITS,(1,2,3,4,7))):
        for offset,value in((0x20,8),(0x24,stride),(0x28,debug<<16),(0x34,8),(0x38,5 if pool==OUTER else 8),(0x3c,len(live)),(0x44,rows),(0x48,bits)):
            put(pool+offset,value)
        put(bits,sum(1<<i for i in live))
        for n in live:put(rows+n*stride,0x2345 if pool==OUTER else 0x3456)
    put(LIGHTPOOL+0x44,LIGHTS);put(LIGHTS+2*0x110,(flags<<16)|0x1234)
    chain=(2,4,1)[:length]
    for n,s in zip((2,4,1),(0,1,slot)):
        put(OUTROWS+n*outer_stride+4,s&0xffff)
        pos=chain.index(n)if n in chain else-1
        put(OUTROWS+n*outer_stride+8,(0x23450000|chain[pos+1])if pos>=0 and pos+1<len(chain)else 0xffffffff)
    for n,key,next_index in((1,KEY^1,2),(2,KEY,3),(3,KEY,-1),(4,KEY,-1),(7,KEY,-1)):
        put(INROWS+n*inner_stride+4,key)
        put(INROWS+n*inner_stride+8,0xffffffff if next_index<0 else 0x34560000|next_index)
    put(HEADS,0x34560001 if kind=='middle'else 0x34560002 if kind=='first'else 0xffffffff if kind=='empty'else 0x34560001)
    if kind=='missing':put(INROWS+2*inner_stride+4,KEY^2);put(INROWS+3*inner_stride+4,KEY^3)
    put(HEADS+4,0x34560004);put(HEADS+slot*4,0x34560007)
    first=(0x23450000|chain[0])if chain else 0xffffffff
    put(HEAD,first);put(OTHERHEAD,0xdeadbeef);put(LIGHTS+2*0x110+0x10,first)
    put(OBJECTPOOL+0x44,HEADERS);put(HEADERS+2*12,0x4567);put(HEADERS+2*12+8,OBJECT)
    put(OBJECT+4,0x10);put(OBJECT+0xc,0xffffffff);put(OBJECT+0x10,0xffffffff);put(OBJECT+0x14,0xffffffff)
    put(OBJECT+0x11c,(0x500<<16)|8);put(OBJECT+0x500,0);put(OBJECT+0x504,KEY)
    return words


def cases():
    result=[]
    def add(name,entry,**options):
        args=[KEY,HEAD]if entry==0x1d30f0 else[OBJECTDATUM,1,0,0,0]if entry==0xc0f60 else[]
        result.append(dict(name=name,entry=entry,arguments=args,receiver=0,incoming_eax=KEY,
            incoming_edi=ROOTOBJ,input_registers={'eax':KEY,'edi':ROOTOBJ},user_words=state(**options)))
    for flags in(0,1,2,8,10,0x82,0xff):
        for length in(0,1,3):
            for debug in(0,8):add(f'light_{flags}_{length}_{debug}',0xc5250,flags=flags,length=length,debug=debug)
    for length in(0,1,3):
        for kind in('first','middle','missing','empty'):
            for outer_stride,inner_stride in((12,24),(16,20),(13,21)):
                for debug in(0,8):add(f'chain_{length}_{kind}_{outer_stride}_{inner_stride}_{debug}',0x1d30f0,length=length,kind=kind,outer_stride=outer_stride,inner_stride=inner_stride,debug=debug)
    for flags in(0,2,10):
        for debug in(0,8):add(f'object_tree_{flags}_{debug}',0xc0f60,flags=flags,debug=debug)
    for target in(OTHERHEAD,HEAD):
        add('head_parameter_reread_'+str(target),0x1d30f0)
        result[-1].update(retained_arguments=True,mutations=[dict(instruction=0x16e8f8,address=base.SP+8,written=struct.pack('<I',target).hex())])
    for occurrence in(1,2):
        add('head_parameter_read_fault_'+str(occurrence),0x1d30f0)
        result[-1].update(retained_arguments=True,fault=dict(address=base.SP+8,bytes=4,occurrence=occurrence,write=False))
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    names=['src/physics/native_tu15_object_detachment.cpp','include/halo2/native_tu15_object_detachment.hpp',
        'include/halo2/native_tu15_object_deletion_spatial_binding.hpp','include/halo2/native_tu15_object_deletion_detaching_binding.hpp',
        'include/halo2/native_tu15_datum_delete_kernel.hpp','src/physics/native_tu15_object_deletion.cpp',
        'src/physics/native_tu15_activation_services.cpp','tools/validate_native_array_growth_tu15.py',
        'tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_attachment_spatial_detachment_tests.cpp',str(Path(__file__).relative_to(ROOT))]
    pins={n:base.sha(ROOT/n)for n in names};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        assert base.sha(NEW_DEFAULT)==NEW_PIN
        image=Xbe(NEW_DEFAULT);base.OWNERS=(0xc5250,0x1d30f0,0x16e8a0,0xc0f60,0xbbd10)
        # 1D311D reuses the dead key-argument stack word as private next-link
        # scratch. Only the live late head-pointer DATA formal is observed.
        # Original instructions and their private frame remain unchanged.
        original_source=inspect.getsource(base.original)
        old="dict(address=call_sp+4,bytes=4*len(case['arguments']))"
        assert original_source.count(old)==1
        original_source=original_source.replace(old,'dict(address=call_sp+8,bytes=4)')
        environment=dict(base.__dict__);exec(original_source,environment);original=environment['original']
        banks={0x4ecbd0:struct.pack('<I',OBJECTPOOL),0x4ecbdc:struct.pack('<I',LIGHTPOOL),0x4ecbe0:struct.pack('<III',HEADS,INNER,OUTER)}
        (scratch/'returns').mkdir();rows=cases();fixture,authority=original(image,banks,rows,scratch/'returns')
        code=_Code(image,'.text');prefix=[];atomic=[]
        for seed in(row for row in rows if row['name']in('light_10_3_8','chain_3_middle_12_24_8','chain_3_middle_13_21_8')):
            for category in('reads','writes'):
                seen=set();occurrences={}
                for event in seed[category]:
                    address,width=event['address'],event['bytes']if category=='reads'else len(event['written'])//2
                    key=address,width;occurrences[key]=occurrences.get(key,0)+1
                    if event['instruction']in seen:continue
                    instruction=code.at(event['instruction'])
                    if category=='writes'and any(op.type==X86_OP_MEM and op.access&CS_AC_READ and op.access&CS_AC_WRITE for op in instruction.operands):
                        atomic.append(event['instruction']);continue
                    seen.add(event['instruction']);row=copy.deepcopy(seed)
                    row.update(name=seed['name']+'_fault_'+category+'_'+hex(event['instruction']),fault=dict(address=address,bytes=width,occurrence=occurrences[key],write=category=='writes'))
                    prefix.append(row)
        if prefix:
            (scratch/'prefix').mkdir()
            extra,proof=original(image,banks,prefix,scratch/'prefix')
            for row in prefix:
                for bank in row['banks']:
                    for key in('before_file','after_file'):
                        bank[key]=str((extra/bank[key]).resolve())
            rows+=prefix
        save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        source=(ROOT/'src/physics/native_tu15_object_detachment.cpp').read_text();(project/'native.cpp').write_text(source)
        shutil.copyfile(ROOT/'tests/native_tu15_attachment_spatial_detachment_tests.cpp',project/'driver.cpp')
        shutil.copyfile(ROOT/'src/physics/native_tu15_object_deletion.cpp',project/'deletion.cpp')
        shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'activation.cpp')
        variants={
            'wrong_guard':('if(!(flags&2u))','if(!(flags&8u))'),
            'wrong_stride':('(datum&0xffffu)*0x110u','(datum&0xffffu)*0x100u'),
            'wrong_next':('datum=next;','datum=word(m,row+8u);'),
            'wrong_reread':('if(nonempty)output=parameters.head_pointer();','if(false)output=parameters.head_pointer();')}
        for name,(needle,replacement)in variants.items():assert source.count(needle)==1;(project/(name+'.cpp')).write_text(source.replace(needle,replacement))
        (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode==1 and "differs" in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint(r.stderr[-2000:])\n')
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(nativeAttachmentDetach LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
        for name in('native',*variants):
            cmake+=f'add_executable(detach_{name} $<TARGET_OBJECTS:driver> {name}.cpp deletion.cpp activation.cpp)\ntarget_include_directories(detach_{name} PRIVATE include)\ntarget_compile_features(detach_{name} PRIVATE cxx_std_20)\n'
            negative=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" '
            cmake+=f'add_test(NAME detach_{name} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:detach_{name}> "{fixture}/original.json")\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),'--jobs','2','--test-jobs','1','--filter','^detach_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
            for name in('native',*variants):command+=['--target','detach_'+name]
            completed=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            (args.output/(compiler+'.log')).write_text(completed.stdout[-60000:]);results.append(dict(compiler=compiler,exit_code=completed.returncode,command=command))
        diagnostics={}
        for result in results:
            worker=json.loads((args.output/result['compiler']/'report.json').read_text())
            retained=next((item['retained_path']for item in worker.get('retained_small_reports',[])if item.get('path')=='Testing/Temporary/LastTest.log'and item.get('status')=='retained'),None)
            diagnostics[result['compiler']]=[line for line in Path(retained).read_text().splitlines()if'differs'in line]if retained else[]
        report.update(passed=all(r['exit_code']==0 for r in results)and all(len(lines)==4 for lines in diagnostics.values())and all(base.sha(ROOT/n)==pin for n,pin in pins.items()),compiler_results=results,
            original_backend='Unchanged original x86 instructions executed by a native Linux i386 ELF under PTRACE_SINGLESTEP; no Unicorn/interpreter backend.',
            original_sha256=NEW_PIN,source_authority=authority,cases=len(rows),fault_prefixes=sum(r.get('declared_fault',False)for r in rows),negative_controls=list(variants),negative_diagnostics=diagnostics,
            ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),
            atomic_rmw_write_faults_not_claimed=sorted(set(atomic)),instruction_patches=[],supplied_child_results=False,
            formal_observation='Only the live late head-pointer formal is DATA; dead key-slot/private stack writes are not typed-kernel outputs.',
            formal_lifetime_witness={'key_last_use':'1D30FA reads [entrySP+4] into EBP, which remains the cached key.',
                'private_reuse':'1D311D stores the cached next outer link to [entrySP+4] after the key is dead there.',
                'live_head_pointer':'1D30F0 reads [entrySP+8];1D3170 re-reads it after nonempty chains;1D3176 stores through the selected pointer.',
                'controls':'Two genuine head-formal read-fault prefixes and completed16E8F8 store mutations preserve/check both late-pointer outcomes.'},
            body_coverage={hex(owner):dict(total=len(base.complete_body(code,owner).code),executed=len(set(base.complete_body(code,owner).code)&{site for row in rows for site in row['executed_sites']}))for owner in base.OWNERS},
            scope='Whole C5250 and1D30F0 with genuine16E8A0 frees and C0F60 caller closure, ordered DATA reads/stores, complete declared banks/FP, valid DF0 and lazy head-pointer re-read.')
    except BaseException as error:report['error']=str(error)or repr(error);raise
    finally:save(args.output/'report.json',report)
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
