#!/usr/bin/env python3
"""Private original3139F0 caller frame with authentic FS-bank setup."""
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
from research_mp_sandbox_update_functions import Xbe
from validate_native_solver_hull_cleanup_source_tu15 import reference,inputs,PARENT_PIN,CHILD_PIN
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
FS=0x62000000
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def mutants(scratch,command,data,expected):
    relative='physics/runtime/native_tu15_solver_hull_cleanup_frame.hpp'
    original=(ROOT/'src'/relative).read_text()
    variants=(
        ('wrong_stride32_size','push(0x313a33u,0x12u);shift<5>();','push(0x313a33u,0x12u);shift<4>();'),
        ('wide_stage80_store','store(0x313a45u,g[4]+0x14u,0u,1u);','store(0x313a45u,g[4]+0x14u,0u,4u);'),
        ('late_exception_chain_capture','load(0x313a95u,1u,g[4]+0xcu);g[7]=pop(0x313a99u);',
         'g[7]=pop(0x313a99u);load(0x313a95u,1u,g[4]+0xcu);'),
    )
    result={}
    for name,before,after in variants:
        if original.count(before)!=1:raise ValueError('Missing exact cleanup defect seam '+name)
        overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True)
        target.write_text(original.replace(before,after,1))
        trial=command.copy();trial.insert(trial.index('-I'+str(ROOT/'include')),'-I'+str(overlay));trial[-1]=str(overlay/'cleanup')
        built=subprocess.run(trial,capture_output=True,text=True,timeout=600)
        if built.returncode:raise ValueError('Cleanup defect compile failed '+name+': '+built.stderr[-5000:])
        with data.open('rb')as inp:run=subprocess.run([trial[-1]],stdin=inp,capture_output=True,text=True,timeout=600)
        if run.returncode:raise ValueError('Cleanup defect stopped before differential '+name+': '+run.stderr[-5000:])
        actual=json.loads(run.stdout)
        first=next((n for n,(a,b)in enumerate(zip(expected,actual,strict=True))if a!=b),None)
        if first is None:raise ValueError('Unchanged original cleanup did not reject '+name)
        field=next(k for k in expected[first]if expected[first][k]!=actual[first][k])
        result[name]=dict(rejected=True,case=first,first_field=field)
    return result

def physical_with_fs(image,rows,scratch,io):
    """Customize only the private bridge and operand metadata, not image text."""
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    decoder=_Code(image,'.text')
    original_run=subprocess.run
    changes={}
    def run(command,*args,**kwargs):
        if command[0]=='as':
            directory=Path(command[2]).parent
            assembly=directory/'reference.s'
            text=assembly.read_text()
            start='_start:\nfninit\nint3\njmp _start\n'
            if text.count(start)!=1:raise ValueError('Exact physical startup seam drift')
            startup='_start:\nmov $243,%eax\nmov $cleanup_tls,%ebx\nint $0x80\n'
            startup+='test %eax,%eax\njns 1f\nmov $1,%eax\nmov $126,%ebx\nint $0x80\n'
            startup+='1: mov cleanup_tls,%eax\nshl $3,%eax\nor $3,%eax\nmov %ax,%fs\nfninit\nint3\njmp _start\n'
            text=text.replace(start,startup,1)
            if text.count('.section .bridge,"ax"')!=1:raise ValueError('Physical bridge section drift')
            text=text.replace('.section .bridge,"ax"','.section .bridge,"awx"',1)
            text+='\n.balign 16\ncleanup_tls: .long -1,'+str(FS)+',0xfffff,0x51\n'
            assembly.write_text(text)
            metadata_path=directory/'metadata.json'
            metadata=json.loads(metadata_path.read_text());fs_sites=[]
            for site in metadata['sites']:
                instruction=decoder.at(site['instruction'])
                actual=[op for op in instruction.operands if op.type==3 and instruction.reg_name(op.mem.segment)=='fs']
                if not actual:continue
                if len(actual)!=1 or len(site['operands'])!=1:raise ValueError('Unexpected actual FS instruction operand shape')
                site['operands'][0]['fs_base']=FS;fs_sites.append(site['instruction'])
            if sorted(fs_sites)!=[0x3139f7,0x3139fe,0x313a9b]:raise ValueError('Actual cleanup FS site inventory drift')
            metadata_path.write_text(json.dumps(metadata))
            observer=directory/'observer.cpp';source=observer.read_text()
            needle='std::uint32_t p=o.at("displacement");'
            if source.count(needle)!=1:raise ValueError('Physical observer address seam drift')
            source=source.replace(needle,'std::uint32_t p=o.at("displacement");p+=o.value("fs_base",0u);',1)
            observer.write_text(source)
            changes.update(assembly_sha256=sha(assembly),metadata_sha256=sha(metadata_path),
                           observer_sha256=sha(observer),fs_operand_sites=[hex(p)for p in fs_sites],
                           fs_descriptor_base=hex(FS),original_image_bytes_unchanged=True)
        return original_run(command,*args,**kwargs)
    globals_=physical_original.__globals__.copy()
    globals_['subprocess']=types.SimpleNamespace(run=run)
    function=types.FunctionType(physical_original.__code__,globals_,physical_original.__name__,
                               physical_original.__defaults__,physical_original.__closure__)
    function.__kwdefaults__=physical_original.__kwdefaults__.copy()
    actual,proof=function(image,rows,scratch,entries=(0x3139f0,0x149e00),
        regions=((0x61000000,0x6000),(FS,0x1000),(0x45f000,0x1000),(0x486000,0x1000),(0x48c000,0x1000)),
        trace_calls=[dict(entry=pc,arguments=0,gp=True,xmm=True)for pc in io]+
                    [dict(entry=0x149e00,arguments=3,gp=True,xmm=True)],trace_extended=True)
    proof['owned_fs_harness']=changes
    return actual,proof

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--mutations',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    test=ROOT/'tests/native_tu15_solver_hull_cleanup_frame_tests.cpp'
    paths=[test,Path(__file__),ROOT/'tools/validate_native_solver_hull_cleanup_source_tu15.py',
           ROOT/'src/physics/runtime/native_tu15_solver_hull_cleanup_frame.hpp',
           ROOT/'include/halo2/native_tu15_solver_hull_cleanup_kernel.hpp',
           ROOT/'tools/validate_native_tu15_prt_helpers.py',
           ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp']
    pins={str(f.relative_to(ROOT)):sha(f)for f in paths}
    report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong original TU15 image')
        body=reference();(scratch/'solver_hull_cleanup_actual.inc').write_text(body)
        parent=body[body.index('static void cleanup('):]
        io,explicit,pops,sites=[],set(),set(),set()
        for line in parent.splitlines():
            pc=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
            if not pc:continue
            pc=int(pc[1],16);sites.add(pc)
            if re.search(r'\br\(s.memory',line):explicit.add(pc)
            if 's.pop()'in line:pops.add(pc)
            if pc in explicit|pops or re.search(r'\bw\(s.memory',line)or's.push('in line:io.append(pc)
        image=Xbe(a.xbe);rows=inputs(image)
        for row in rows:
            bank=bytearray.fromhex(row['banks'][0]['before'])
            offset=row['gp'][4]-0x61000000
            row.update(entry=0x3139f0,name='cleanup-'+str(row['id']),before_fp=[0x37f,0x4100,0x1f80],
                       live_x87=[],max_steps=800,return_pc=0x63000000,
                       matrix=struct.unpack_from('<I',bank,offset+4)[0],
                       source=struct.unpack_from('<I',bank,offset+8)[0])
        data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')))
        observed,report['original']=physical_with_fs(image,rows,scratch,io)
        command=[a.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(test),'-o',str(scratch/'cleanup')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        built=subprocess.run(command,capture_output=True,text=True,timeout=600)
        if built.returncode:raise ValueError('Physical cleanup frame compile failed: '+built.stderr[-6000:])
        with data.open('rb')as inp:
            replay=subprocess.run([command[-1]],stdin=inp,capture_output=True,text=True,timeout=600)
        if replay.returncode:raise ValueError('Physical cleanup native replay failed: '+replay.stderr[-6000:])
        actual=json.loads(replay.stdout);visited=set();parents=childs=read_count=write_count=0;expected=[]
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']]
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Whole original cleanup did not complete')
            packets=[];children=[];reads=[]
            original_reads=iter(e for e in case['reads']if e['instruction']in io)
            callsites=iter(pc for pc in (0x313a3a,0x313a69,0x313a92)if pc in case['executed_sites'])
            for event in case['calls']:
                pc=event['entry'];abi={k:v for k,v in event.items()if k!='arguments'}
                if pc in io:
                    packets.append(abi)
                    if pc in pops:reads.append(dict(instruction=pc,address=event['gp'][4],bytes=4))
                    if pc in explicit:
                        read=next(original_reads)
                        if read['instruction']!=pc:raise ValueError('Unclassified original cleanup read')
                        reads.append(read)
                else:
                    site=next(callsites)
                    children.append(dict(target=pc,site=site,return_pc=site+3,arguments=event['arguments'],abi=abi))
            if next(original_reads,None)is not None or next(callsites,None)is not None:raise ValueError('Incomplete original cleanup events')
            want=dict(abi=dict(entry=0x313aa5,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
                               fp=case['after_fp'],live_x87=[]),
                      banks=sorted(case['banks'],key=lambda b:b['address']),packets=packets,reads=reads,children=children,writes=case['writes'])
            if want!=got:
                field=next(k for k in want if want[k]!=got.get(k))
                old,new=want[field],got.get(field)
                if isinstance(old,list):
                    n=next((i for i,(a,b)in enumerate(zip(old,new))if a!=b),min(len(old),len(new)))
                    old,new=old[n:n+1],new[n:n+1]
                report['first_difference']=dict(case=row['name'],field=field,expected=old,actual=new)
                raise ValueError('Whole original physical cleanup differs '+row['name'])
            visited.update(case['executed_sites']);parents+=len(packets);childs+=len(children);read_count+=len(reads);write_count+=len(case['writes'])
            expected.append(want)
        if len(sites)!=56 or not sites<=visited:raise ValueError('Missing complete cleanup parent coverage')
        if a.mutations:report['compiled_defects']=mutants(scratch,command,data,expected)
        report.update(passed=True,complete_original_calls=len(rows),parent_sites=56,parent_pre_io_packets=parents,
                      genuine_149e00_entries=childs,parent_reads=read_count,ordered_stores=write_count,
                      scope='Private physical caller and authentic null-data149E00 child; no non-null release, source-default or arbitrary GP provider domains.')
    except Exception as error:report['error']=repr(error)
    try:reference();report['sources_unchanged']=all(sha(ROOT/name)==pin for name,pin in pins.items())
    except(OSError,ValueError):report['sources_unchanged']=False
    report['passed'] &=report['sources_unchanged']
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in('passed','error')if k in report}))
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

