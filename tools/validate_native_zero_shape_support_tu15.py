#!/usr/bin/env python3
"""Private full9-site2E8D20 zero-support DATA/raw ABI and unchanged-original proof."""
import argparse,hashlib,json,os,platform,random,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_tu15_prt_helpers import physical_original,PIN
from native_zero_shape_support_mutants import run_mutants
from native_zero_shape_support_replacements import canonical_body
ROOT=Path(__file__).resolve().parents[1]
BODY_PIN='9c6c29faa6b324f6bad5839cd47aa4bc7c15e8f7f552e4f80612b401f998ad32'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def body():
    text=canonical_body(next(value for sig,value in function_bodies((ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text())if sig.symbol=='2e8d20'))
    if hashlib.sha256(text.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete zero-support owner drift')
    return text

def reference(scratch):
    text=body();sites=[]
    for line in text.splitlines():
        pc=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
        if pc and(re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line):sites.append(int(pc[1],16))
    prefix='''static void atomic_push(WorldState&s,std::uint32_t pc,std::uint32_t value){const auto address=s.gp[4]-4u;SourcePhysicsContactRecordStore event{pc,address,{},{}};if(s.observe){event.before.resize(4);s.memory.read(address,event.before);}std::array<std::byte,4>bytes{};std::memcpy(bytes.data(),&value,4);s.memory.write(address,bytes);s.gp[4]=address;if(s.observe){event.written.assign(bytes.begin(),bytes.end());s.observe(event);}}
'''
    value=prefix+'static const std::set<std::uint32_t>io_sites{'+','.join(hex(pc)+'u'for pc in sites)+'};\n'+text.replace('run_2e8d20(','owner(',1).replace('s.push(','atomic_push(s,');(scratch/'zero_shape_support_reference.inc').write_text(value);return text,sites

def inputs():
    rng=random.Random(0x2e8d20);rows=[];base=0x61000000
    for k in range(128):
        bank=bytearray(rng.randbytes(0x6000));gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=0xdead0000+k;gp[4]=base+0x5000+k%16;output=gp[4]+8 if k%16==8 else base+0x3000+(k%32)*16
        def put(address,value):struct.pack_into('<I',bank,address-base,value&0xffffffff)
        put(gp[4],0x63000000);put(gp[4]+4,0xf1000000+k);put(gp[4]+8,output);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
        rows.append(dict(id=k,name=f'zero-support-{k}',entry=0x2e8d20,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),return_pc=0x63000000,matrix=0xf1000000+k,source=output,before_fp=[cw,0x4100|((k%8)<<11),mxcsr],live_x87=[values[n%4]for n in range((0,1,4,8)[k%4])],xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],max_steps=30,banks=[dict(address=base,before=bank.hex())]))
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--mutations',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['tools/native_zero_shape_support_replacements.py','tools/native_zero_shape_support_mutants.py','include/halo2/native_tu15_zero_shape_support_kernel.hpp','src/physics/runtime/native_tu15_zero_shape_support_frame.hpp','tests/native_tu15_zero_shape_support_tests.cpp',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        if sha(args.xbe)!=PIN:raise ValueError('Wrong original image')
        from research_mp_sandbox_update_functions import Xbe
        from oracle_native_campaign_screen_lifecycle_tu15 import _Code
        original,sites=reference(scratch);rows=inputs();image=Xbe(args.xbe);code=_Code(image,'.text');input_path=scratch/'inputs.json';input_path.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(input_path)
        command=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_zero_shape_support_tests.cpp'),'-o',str(scratch/'probe')]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Zero-support compile failed: '+built.stderr[-7000:])
        with input_path.open('rb')as inp:phased=subprocess.run([str(scratch/'probe')],stdin=inp,capture_output=True,text=True)
        if phased.returncode:raise ValueError('Zero-support phased comparison failed: '+phased.stderr[-9000:])
        report['source']=json.loads(phased.stdout.strip().splitlines()[-1]);trace=[dict(entry=pc,arguments=0,gp=True,xmm=True)for pc in sites]
        observed,proof=physical_original(image,rows,scratch,entries=(0x2e8d20,),regions=((0x61000000,0x6000),),trace_calls=trace,trace_extended=True,preserve_x87_prefix=True);expected=[];executed=set();packets=0
        for case in observed['cases']:
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Whole original zero support did not return')
            row=rows[case['id']];stores=[]
            for event in case['writes']:
                before,raw=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                if event['instruction']==0x2e8d2c:
                    if len(raw)!=16 or code.at(event['instruction']).bytes[:2]!=bytes.fromhex('0f29'):raise ValueError('Zero-support original MOVAPS projection drift')
                    stores.extend(dict(instruction=event['instruction'],address=event['address']+n,before=before[n:n+8].hex(),written=raw[n:n+8].hex())for n in(0,8))
                else:stores.append(event)
            actual_packets=[{key:value for key,value in trace.items()if key!='arguments'}for trace in case['calls']];packets+=len(actual_packets);executed.update(case['executed_sites']);abi=dict(entry=0x2e8d32,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],fp=case['after_fp'],live_x87=row['live_x87']);expected.append(dict(abi=abi,packets=actual_packets,stopped=False,banks=case['banks'],writes=stores))
        output=scratch/'outputs.json'
        with input_path.open('rb')as inp,output.open('wb')as out:run=subprocess.run([str(scratch/'probe'),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
        if run.returncode:raise ValueError('Zero-support original input replay failed: '+run.stderr.decode()[-8000:])
        actual=json.loads(output.read_text())
        for n,(want,got)in enumerate(zip(expected,actual,strict=True)):
            if want!=got:
                field=next(key for key in want if want[key]!=got.get(key));report['first_difference']=dict(case=n,field=field,expected=want[field],actual=got.get(field));raise ValueError('Zero-support unchanged-i386 differs '+str(n)+' '+field)
        if args.mutations:report['semantic_mutants']=run_mutants(ROOT,scratch,command,input_path)
        report.update(passed=True,hardware=dict(cases=len(rows),parent_sites=9,executed_parent_sites=len(set(pc for pc in executed if 0x2e8d20<=pc<=0x2e8d32)),full_pre_io_gp_flags_xmm_fp_live80_packets=packets,original_text_unmodified=True,supplied_child_outcomes=False,comparison_sha256=hashlib.sha256(json.dumps(actual,sort_keys=True).encode()).hexdigest(),physical_vector_page_fault_atomicity=False,original=proof),scope='Private dense masked/no opaqueMMX zero support; unprovided shape/direction deliberately unread, output/caller argument aliases; no production default or provider outcome.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())and hashlib.sha256(body().encode()).hexdigest()==BODY_PIN
    if not report['sources_unchanged']:report.update(passed=False,error='Zero-support source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:report[key]for key in('passed','error')if key in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
