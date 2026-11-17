#!/usr/bin/env python3
"""Qualify real 314710→313BB0 raw composition with observed external clocks."""
import argparse
import hashlib
import json
import os
import platform
import re
import struct
import subprocess
import types
from pathlib import Path

from generated_routine_ownership import function_bodies
from validate_native_solver_hull_owner_frame_tu15 import (
    BASE, BODY_PIN, CHILDREN, DEPTH, DRIVER, ENTRY, LAST, PIN, REGIONS, STOP, TABLE,
    Xbe, actual_body, inputs as owner_inputs, sha,
)
from validate_native_solver_hull_search_setup_frame_tu15 import FS, FS_SITES
from validate_native_tu15_prt_helpers import physical_original

ROOT=Path(__file__).resolve().parents[1]
OUTER,OUTER_LAST=0x314710,0x3147cd
OUTER_PIN='0d8b7578de1f94bc8e3d8b9c905307c6c5cda881de39844aad279565952e08f2'
CLOCKS={0x31472e,0x3147b1}
CLOCK_AFTER={0x314730,0x3147b3}
ALL_CHILDREN={**CHILDREN,ENTRY:1}
READONLY_HULL_REDUCTION=False
READONLY_SIMPLEX_REDUCTION=False


def outer_body():
    source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
    return next(body for sig,body in function_bodies(source)if sig.symbol=='314710')


def parent(pc):
    return ENTRY<=pc<=LAST or OUTER<=pc<=OUTER_LAST


def inputs(image,smoke):
    rows=owner_inputs(image,smoke)
    for row in rows:
        n=row['id'];sp=row['gp'][4];transform=row['gp'][1]
        main=next(bank for bank in row['banks']if bank['address']==BASE)
        raw=bytearray.fromhex(main['before']);first,second,config,_,differences=struct.unpack_from('<5I',raw,transform+0x40-BASE)
        out=struct.unpack_from('<I',raw,sp+4-BASE)[0]
        struct.pack_into('<7I',raw,sp-BASE,STOP,first,second,transform,config,differences,out)
        main['before']=raw.hex()
        page=next(bank for bank in row['banks']if bank['address']==0x48c000);value=bytearray.fromhex(page['before'])
        cursor=BASE+0x5700;limit=cursor+(0,12,24)[n%3]
        struct.pack_into('<2I',value,0x974,cursor,limit);page['before']=value.hex()
        row.update(name='context-'+row['name'],entry=OUTER,matrix=first,source=second,expected_clocks=(0,1,2)[n%3])
    return rows


def prepare_driver(scratch):
    text=DRIVER.read_text()
    def replace(old,new):
        nonlocal text
        if text.count(old)!=1:raise ValueError('Owned context proof overlay lost its exact source anchor')
        text=text.replace(old,new,1)
    replace('#include "physics/runtime/native_tu15_solver_hull_owner_frame.hpp"',
            '#include "physics/runtime/native_tu15_solver_hull_owner_frame.hpp"\n'
            '#include "physics/runtime/native_tu15_solver_hull_context_frame.hpp"')
    replace('return pc>=0x313bb0u&&pc<=0x3146f4u;',
            'return (pc>=0x313bb0u&&pc<=0x3146f4u)||(pc>=0x314710u&&pc<=0x3147cdu);')
    replace('    SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};', '''    SourcePhysicsWorldRuntimeServices services;unsigned callbacks{},clock_index{};
    const auto clocks=row.at("observed_clocks").get<std::vector<std::uint64_t>>();
    services.timestamp_counter=[&]{check(clock_index<clocks.size(),"Context requested an unauthenticated clock");return clocks[clock_index++];};''')
    replace('    native_solver_hull_owner_detail::run_private_owner(state,call,before);', '''    const auto actual_child=[&](WorldState& next,std::uint32_t target,std::uint32_t site){
        check(target==0x313bb0u&&site==0x314796u,"Wrong actual context solver child");
        enter(next,target,site,1u);native_solver_hull_owner_detail::run_private_owner(next,call,before);
    };
    native_solver_hull_context_detail::run_private_owner(state,actual_child,before);
    check(clock_index==clocks.size(),"Context skipped an actual observed clock");''')
    path=scratch/'composed_context.cpp';path.write_text(text)
    return path


def observe_original(image,rows,scratch,trace):
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    decoder=_Code(image,'.text');run=subprocess.run
    def launch(command,*args,**kwargs):
        if command[0]=='as':
            directory=Path(command[2]).parent;assembly=directory/'reference.s';text=assembly.read_text()
            opening='_start:\nfninit\nint3\njmp _start\n'
            if text.count(opening)!=1 or text.count('.section .bridge,"ax"')!=1:raise ValueError('Private composed FS bridge drift')
            entry=('_start:\nmov $243,%eax\nmov $context_tls,%ebx\nint $0x80\n'
                   'test %eax,%eax\njns 1f\nmov $1,%eax\nmov $126,%ebx\nint $0x80\n'
                   '1: mov context_tls,%eax\nshl $3,%eax\nor $3,%eax\nmov %ax,%fs\nfninit\nint3\njmp _start\n')
            text=text.replace(opening,entry,1).replace('.section .bridge,"ax"','.section .bridge,"awx"',1)
            assembly.write_text(text+'\n.balign 16\ncontext_tls: .long -1,'+str(FS)+',0xfffff,0x51\n')
            metadata=directory/'metadata.json';auth=json.loads(metadata.read_text());found=[]
            for site in auth['sites']:
                instruction=decoder.at(site['instruction'])
                operands=[op for op in instruction.operands if op.type==3 and instruction.reg_name(op.mem.segment)=='fs']
                if not operands:continue
                if len(operands)!=1 or len(site['operands'])!=1:raise ValueError('Unexpected actual composed FS operand')
                site['operands'][0]['fs_base']=FS;found.append(site['instruction'])
            if set(found)!=set(FS_SITES)|{0x3139f7,0x3139fe,0x313a9b}:raise ValueError('Original composed FS site inventory drift')
            metadata.write_text(json.dumps(auth))
            observer=directory/'observer.cpp';source=observer.read_text();needle='std::uint32_t p=o.at("displacement");'
            if source.count(needle)!=1:raise ValueError('Actual composed FS consumer drift')
            source=source.replace(needle,needle+'p+=o.value("fs_base",0u);',1)
            if READONLY_HULL_REDUCTION or READONLY_SIMPLEX_REDUCTION:
                anchor='auto retained=[&](std::uint32_t p,unsigned n){'
                if source.count(anchor)!=1:raise ValueError('Actual composed readonly table observer drift')
                readonly=''
                if READONLY_HULL_REDUCTION:
                    readonly+='if((n==1u&&p>=0x30e908u&&p<0x30e921u)||(n==4u&&p>=0x30e8dcu&&p<0x30e908u))return true;'
                if READONLY_SIMPLEX_REDUCTION:
                    readonly+='if((n==1u&&p>=0x30f70cu&&p<0x30f725u)||(n==4u&&p>=0x30f6e0u&&p<0x30f70cu))return true;'
                source=source.replace(anchor,anchor+readonly,1)
            observer.write_text(source)
        return run(command,*args,**kwargs)
    context=physical_original.__globals__.copy();context['subprocess']=types.SimpleNamespace(run=launch)
    original=types.FunctionType(physical_original.__code__,context,physical_original.__name__,
                                physical_original.__defaults__,physical_original.__closure__)
    original.__kwdefaults__=physical_original.__kwdefaults__.copy()
    return original(image,rows,scratch,entries=(OUTER,)+tuple(ALL_CHILDREN),regions=REGIONS,
                    trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--smoke',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    paths={DRIVER,Path(__file__),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
           ROOT/'src/physics/runtime/native_tu15_solver_hull_context_frame.hpp'};pending=list(paths)
    while pending:
        path=pending.pop();text=path.read_text();names=re.findall(r'^#include "([^"\n]+)"',text,re.M)
        if path.suffix=='.py':names+=['tools/'+m+'.py'for m in re.findall(r'^(?:from|import) (\w+)',text,re.M)]
        for name in names:
            found=next((v for v in(path.parent/name,ROOT/name,ROOT/'include'/name,ROOT/'src'/name,ROOT/'tests'/name)if v.is_file()),None)
            if found is not None and found not in paths:paths.add(found);pending.append(found)
    pins={str(path.relative_to(ROOT)):sha(path)for path in sorted(paths)}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong actual TU1.5 original image')
        child,outer=actual_body(),outer_body()
        if (hashlib.sha256(child.encode()).hexdigest(),hashlib.sha256(outer.encode()).hexdigest())!=(BODY_PIN,OUTER_PIN):
            raise ValueError('Complete actual composed source owners drift')
        image=Xbe(a.xbe);rows=inputs(image,a.smoke)
        if image.read(0x313bf5,7).hex()!='ff2485f8463100' or any(image.read(pc,2)!=b'\x0f\x31'for pc in CLOCKS):
            raise ValueError('Actual composed switch or RDTSC opcode drift')
        sites=set();io=set(CLOCKS);read_sites=set();stack=set();vectors=set()
        for line in (child+outer).splitlines():
            found=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
            if not found:continue
            pc=int(found[1],16);sites.add(pc)
            if re.search(r'\b(?:r|v|w|vw)\(s.memory|s.push\(|s.pop\(',line):io.add(pc)
            if re.search(r'\b(?:r|v)\(s.memory',line):read_sites.add(pc)
            if 's.pop()'in line:stack.add(pc)
            if 'vw(s.memory'in line:vectors.add(pc)
        if ENTRY not in io:raise ValueError('Actual child entry packet inventory drift')
        trace=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=DEPTH.get(pc,0))for pc in sorted(io)]
        trace.extend(dict(entry=pc,arguments=n,gp=True,xmm=True)for pc,n in ALL_CHILDREN.items())
        observed,report['original']=observe_original(image,rows,scratch,trace)
        for row,case in zip(rows,observed['cases'],strict=True):
            clocks=[(e['gp'][2]<<32)|e['gp'][0]for e in case['calls']if e['entry']in CLOCK_AFTER]
            if len(clocks)!=row['expected_clocks']:raise ValueError('Actual context clock schedule differs')
            row['observed_clocks']=clocks
        data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(data)
        driver=prepare_driver(scratch);report['owned_driver_overlay_sha256']=sha(driver)
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(driver),'-o',str(scratch/'hull_context')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private hull composed context compile failed: '+built.stderr[-10000:])
        out=scratch/'native.json'
        with data.open('rb') as inp,out.open('wb') as output:
            done=subprocess.run([command[-1]],stdin=inp,stdout=output,stderr=subprocess.PIPE,timeout=120)
        if done.returncode:raise ValueError('Native hull context stopped: '+done.stderr.decode()[-6000:])
        actual=json.loads(out.read_text());seen=set();counts={name:0 for name in('packets','children','reads','writes')};return_sites={}
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']];stop=case.get('stopped_at')
            if stop!=STOP or not case['live_x87_unchanged']:raise ValueError('Original composed hull did not return')
            calls=[];stores=[];packets=[];children=[];reads=[]
            original_reads=iter(v for v in case['reads']if parent(v['instruction']))
            for event in case['writes']:
                pc=event['instruction'];opcode=image.read(pc,1)[0]
                if opcode==0xe8 or pc in(0x3133ab,0x313425):
                    returned=struct.unpack('<I',bytes.fromhex(event['written']))[0];width=3 if pc in(0x3133ab,0x313425)else 5
                    if returned!=pc+width:raise ValueError('Actual composed child return drift')
                    calls.append(returned)
                if not parent(pc):continue
                if pc not in vectors:stores.append(event);continue
                if len(event['before'])!=32 or len(event['written'])!=32:raise ValueError('Actual composed MOVAPS width drift')
                stores.extend(dict(event,address=event['address']+n,before=event['before'][n*2:n*2+16],written=event['written'][n*2:n*2+16])for n in(0,8))
            for event in case['calls']:
                pc=event['entry'];arity=len(event['arguments'])
                if pc in ALL_CHILDREN and not(pc==ENTRY and arity==0):
                    children.append(dict(event,return_word=calls[len(children)]));continue
                if pc in stack:reads.append(dict(instruction=pc,address=event['gp'][4],bytes=4))
                elif pc==0x313bf5:
                    address=TABLE+event['gp'][0]*4
                    if address>=TABLE+20:raise ValueError('Original table index escaped actual table')
                    reads.append(dict(instruction=pc,address=address,bytes=4))
                elif pc in read_sites:
                    read=next(original_reads)
                    if read['instruction']!=pc:raise ValueError(f'Actual composed read differs at {pc:x}')
                    reads.append(read)
                packets.append({k:v for k,v in event.items()if k!='arguments'})
            if next(original_reads,None)is not None or len(children)!=len(calls):raise ValueError('Unclassified actual composed transport')
            want=dict(abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
                 fp=case['after_fp'],live_x87=row['live_x87']),packets=packets,children=children,
                 banks=sorted(case['banks'],key=lambda bank:bank['address']),reads=reads,writes=stores)
            for field,expected in want.items():
                if expected==got[field]:continue
                received=got[field];where=None
                if isinstance(expected,list):
                    where=next((n for n,(x,y) in enumerate(zip(expected,received))if x!=y),min(len(expected),len(received)))
                    expected,received=expected[where:where+1],received[where:where+1]
                if field=='banks' and expected and received:
                    x,y=expected[0]['after'],received[0]['after'];at=next(n for n,(a,b)in enumerate(zip(x,y))if a!=b)//2
                    expected,received=dict(address=expected[0]['address']+at,bits=x[at*2:at*2+16]),dict(bits=y[at*2:at*2+16])
                report['first_difference']=dict(case=row['name'],field=field,ordinal=where,expected=expected,actual=received)
                raise ValueError('Actual composed hull differs: '+row['name']+' '+field)
            for key in counts:counts[key]+=len(want[key])
            executed=set(case['executed_sites']);seen.update(executed)
            for pc in (0x314350,0x314475,0x314491,0x3146f4):
                if pc in executed:return_sites[hex(pc)]=return_sites.get(hex(pc),0)+1
        outer_sites={pc for pc in sites if pc>=OUTER};child_sites=sites-outer_sites
        if outer_sites-seen:raise ValueError('Incomplete actual outer context site coverage: '+repr(sorted(outer_sites-seen)))
        report.update(passed=True,complete_original_calls=len(rows),outer_sites=len(outer_sites),executed_outer_sites=len(outer_sites&seen),
                      solver_sites=len(child_sites),executed_solver_sites=len(child_sites&seen),counts=counts,
                      actual_clocks=sum(len(row['observed_clocks'])for row in rows),solver_return_sites=return_sites,
                      original_text_unmodified=True,supplied_child_outcomes=False,
                      scope='Private actual context plus complete backed inline solver, with each genuinely observed RDTSC value as external native input. No retained source, allocator return, broader support owner or default claim.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private composed hull source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','executed_outer_sites','executed_solver_sites')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
