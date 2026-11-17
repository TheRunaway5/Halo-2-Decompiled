#!/usr/bin/env python3
"""Unchanged32-site313290 arithmetic, full pre-I/O ABI and retained protocol."""
import argparse,hashlib,json,os,platform,random,re,struct,subprocess
from pathlib import Path
import validate_native_solver_hull_append_frame_tu15 as base
from validate_native_solver_hull_context_original_tu15 import DRIVER
from validate_native_tu15_prt_helpers import physical_original,PIN

def physical_inputs():
    rng=random.Random(0x313290);rows=[];bankbase=0x61000000;words=(0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000)
    for k in range(192):
        bank=bytearray(rng.randbytes(0x8000));gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=bankbase+0x1000;gp[4]=bankbase+0x7000+k%16;frame=((gp[4]-4)&~15)-0x10;pair=bankbase+0x4000 if k%8==1 else bankbase+0x2000;differences=frame if k%8==3 else bankbase+0x4000
        def put(at,value):struct.pack_into('<I',bank,at-bankbase,value&0xffffffff)
        put(gp[4],0x63000000);put(gp[4]+4,pair);put(gp[1]+0x48,bankbase+0x3000);put(gp[1]+0x50,differences);put(bankbase+0x3080,(0,1,2,3,4,0xffffffff,0x80000000,0x10000000)[k%8]);put(bankbase+0x6080,2)
        for n in range(12):put(pair+n*4,rng.getrandbits(32)if k<64 else words[(k+n*3)%len(words)])
        status=0x4100|((k%8)<<11);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);depth=(0,1,4,8)[k%4];values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');top_words=struct.unpack_from('<3I',bank,gp[4]-bankbase)
        rows.append(dict(id=k,name=f'append-{k}',entry=0x313290,continuation=False,clock_inputs=[],gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=top_words[0],matrix=top_words[1],source=top_words[2],before_fp=[cw,status,mxcsr],live_x87=[values[n%4]for n in range(depth)],xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=80,banks=[dict(address=bankbase,before=bank.hex())]))
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    if not args.xbe.is_file()or platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686'):return 77
    root=base.ROOT;scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['tools/native_solver_hull_append_replacements.py','include/halo2/native_tu15_solver_hull_append_kernel.hpp','src/physics/runtime/native_tu15_solver_hull_append_frame.hpp','tests/native_tu15_solver_hull_append_tests.cpp','tools/validate_native_solver_hull_append_frame_tu15.py','tools/validate_native_solver_hull_context_original_tu15.py',str(Path(__file__).relative_to(root))];pending=list(names);seen=set(names)
    while pending:
        current=root/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            path=next((v for v in(current.parent/header,root/'include'/header,root/'src'/header)if v.is_file()),None)
            if path is None:continue
            relative=str(path.relative_to(root))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:base.digest(root/name)for name in sorted(seen)};report=dict(passed=False,scratch_owner=str(scratch),production_bound=False,source_sha256=pins,retained_large_artifacts=[])
    try:
        if base.digest(args.xbe)!=PIN:raise ValueError('Wrong unchanged original image')
        from research_mp_sandbox_update_functions import Xbe
        from oracle_native_campaign_screen_lifecycle_tu15 import _Code
        report['reference'],body=base.reference(scratch);source=scratch/'driver.cpp';source.write_text(DRIVER.replace('native_tu15_solver_hull_context_tests.cpp','native_tu15_solver_hull_append_tests.cpp'));binary=scratch/'probe';command=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(root/'tests'),'-I'+str(scratch),str(source),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Append original fixture compile failed: '+built.stderr[-8000:])
        phased=subprocess.run([str(binary)],capture_output=True,text=True)
        if phased.returncode:raise ValueError('Append current phased source failed: '+phased.stderr[-10000:])
        report['source']=json.loads(phased.stdout.strip().splitlines()[-1]);report['semantic_mutants']=base.semantic_mutants(scratch,command);image=Xbe(args.xbe);code=_Code(image,'.text');traces=[];all_sites=set()
        for line in body.splitlines():
            found=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
            if not found:continue
            pc=int(found[1],16);all_sites.add(pc)
            if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line:traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True))
        rows=physical_inputs();bankbase=rows[0]['banks'][0]['address']
        observed,proof=physical_original(image,rows,scratch,entries=(0x313290,),regions=((bankbase,0x8000),),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True);expected=[];executed=set();packet_count=0
        for case in observed['cases']:
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original append did not return')
            row=rows[case['id']];abi=dict(entry=0x3132f2,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],fp=case['after_fp'],live_x87=row['live_x87']);stores=[]
            for event in case['writes']:
                before,raw=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                if len(raw)==16:
                    if code.at(event['instruction']).bytes[:2]!=bytes.fromhex('0f29'):raise ValueError('Append original vector opcode drift')
                    stores.extend(dict(instruction=event['instruction'],address=event['address']+n,before=before[n:n+8].hex(),written=raw[n:n+8].hex())for n in(0,8))
                else:stores.append(event)
            packets=[{key:value for key,value in trace.items()if key!='arguments'}for trace in case['calls']];expected.append(dict(abi=abi,packets=packets,clock_entries=[],child=None,stopped=False,banks=case['banks'],writes=stores));packet_count+=len(packets);executed.update(case['executed_sites'])
        inputs=scratch/'inputs.json';outputs=scratch/'outputs.json';inputs.write_text(json.dumps(rows,separators=(',',':')))
        with inputs.open('rb')as inp,outputs.open('wb')as out:run=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
        if run.returncode:raise ValueError('Append original input replay failed: '+run.stderr.decode()[-8000:])
        actual=json.loads(outputs.read_text())
        for n,(want,got)in enumerate(zip(expected,actual,strict=True)):
            got.pop('architectural_reads')
            if want!=got:
                field=next(key for key in want if want[key]!=got.get(key));report['first_difference']=dict(case=rows[n]['name'],field=field,expected=want[field],actual=got.get(field));raise ValueError('Append unchanged-i386 differs '+rows[n]['name']+' '+field)
        report.update(passed=True,hardware=dict(cases=len(rows),parent_sites=len(all_sites),executed_parent_sites=len(all_sites&executed),full_pre_io_gp_flags_xmm_fp_live80_packets=packet_count,comparison_sha256=hashlib.sha256(json.dumps(actual,sort_keys=True).encode()).hexdigest(),original_text_unmodified=True,supplied_child_outcomes=False,physical_vector_rmw_page_fault_atomicity=False,original=proof))
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(base.digest(root/name)==pin for name,pin in pins.items())and base.owner_digest()==base.PIN
    if not report['sources_unchanged']:report.update(passed=False,error='Append original probe source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:report[key]for key in('passed','error')if key in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
