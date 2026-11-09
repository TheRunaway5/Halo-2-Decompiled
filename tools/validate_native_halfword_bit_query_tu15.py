#!/usr/bin/env python3
"""Qualify the native FA5D0 bitfield query against unchanged i386 instructions."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import sys
from generated_routine_ownership import function_bodies
from native_halfword_bit_query_replacements import canonical_body,replace_halfword_bit_query,PREFIX,PIN as BODY_PIN,ORIGINAL_SHIFT,ORDERED_SHIFT
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
for seed in sorted(ROOT.glob('.local/*/python')):
    if(seed/'capstone').is_dir():sys.path.insert(0,str(seed));break
SOURCE='src/physics/physics_live_support_tu15_generated.cpp'
REGIONS=((0x61000000,1024),(0x62000000,1024))
SITES=(0xfa5d0,0xfa5d5,0xfa5d7,0xfa5db,0xfa5de,0xfa5e0,0xfa5e3)
IO=(0xfa5d7,0xfa5db,0xfa5e3)


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def corpus():
    rng=random.Random(0xfa5d0);rows=[]
    for index in (*range(256),0x80000000,0x7fffffff,0xffffffff,0x12345600,0x12345610,0x12345620):
      for word in (0,0xffff,0x8001):
        ordinal=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        stack=0x62000200+ordinal%16;pointer=0x61000020+ordinal%16
        if ordinal%13==0:pointer=stack
        elif ordinal%17==0:pointer=stack+4
        def put(at,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=at and at+width<=base+size:
                    bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Bit-query fixture escaped bank')
        put(pointer,word,2);put(stack,0x63000000);put(stack+4,pointer);put(stack+8,0x12345678)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=index;gp[4]=stack
        live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f',
                     '00000000000000800040','00000000000000c0ff3f','00000000000000000000','0000000000000080ffbf')
        depth=(0,1,4,8)[ordinal%4];live=list(live_values[:depth]);flags=0x202|(rng.getrandbits(12)&0xcd5)
        cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10);status=((ordinal%8)<<11)|(ordinal&0x473f)
        mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
        rows.append(dict(id=ordinal,name=f'bit_{index:x}_word_{word:x}',entry=0xfa5d0,gp=gp,eflags=flags,
            matrix=pointer,source=0x12345678,max_steps=16,xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
            before_fp=[cw,status,mxcsr],live_x87=live,banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    # Every architectural read is stopped independently; count-zero and
    # count-nonzero shifts retain different pre-read flag ownership.
    for seed in rows[:96:3]:
        for ordinal in range(3):
            row=copy.deepcopy(seed);row.update(id=len(rows),name=seed['name']+f'_before_read_{ordinal}',fault=dict(phase='before_read',ordinal=ordinal));rows.append(row)
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--compiler',default='c++');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file():return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=args.require_binding,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        source=(ROOT/SOURCE).read_text();body=next(b for m,b in function_bodies(source)if m[1]=='fa5d0');original=canonical_body(body)
        raw=source.replace(body,original,1);native=replace_halfword_bit_query(raw)
        if args.require_binding and native!=source:raise ValueError('Production FA5D0 prefix missing')
        if replace_halfword_bit_query(native)!=native:raise ValueError('FA5D0 binding is not idempotent')
        rejected=0
        for victim in (raw,native):
            current=next(b for m,b in function_bodies(victim)if m[1]=='fa5d0')
            for before,after in (('s.gp[1]),2)','s.gp[1]),4)'),('s.reg(1,1,0)','s.reg(1,2,0)'),('0xfa5e3u','0xfa5e4u')):
                changed=current.replace(before,after,1)
                if changed==current:raise ValueError('Missing bit-query drift witness')
                try:replace_halfword_bit_query(victim.replace(current,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Changed whole bit-query body was admitted')
        native_body=next(b for m,b in function_bodies(native)if m[1]=='fa5d0')
        retained=original.replace(ORIGINAL_SHIFT,ORDERED_SHIFT)
        from specialize_native_operands import specialize
        specialized,receipt=specialize(native);again,_=specialize(specialized)
        if again!=specialized or receipt['native_halfword_bit_query_entries']!=['fa5d0']:raise ValueError('FA5D0 specialization changed routing/idempotence')
        specialized_body=next(b for m,b in function_bodies(specialized)if m[1]=='fa5d0')
        tracked=lambda body:body.replace(PREFIX,PREFIX+'\n++fallback_calls;',1)
        (scratch/'halfword_bit_query_bodies.inc').write_text('namespace primary {\n'+tracked(native_body)+'\n}\nnamespace specialized {\n'+tracked(specialized_body)+'\n}\nnamespace reference {\n'+retained+'\n}\n')
        binary=scratch/'halfword-bit-query';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_halfword_bit_query_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        compiled=subprocess.run(command,capture_output=True,text=True,timeout=120)
        if compiled.returncode:raise ValueError('Bit-query compilation failed: '+compiled.stderr[:5000])
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        old_metadata=common.metadata
        def metadata(image):
            result=old_metadata(image)
            for site in result['sites']:
                if site['instruction']==0xfa5e3:site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata
        from oracle_smg_firing_1_1 import Xbe
        image=Xbe(args.xbe)
        if hashlib.sha256(image.read(0xfa5d0,22)).hexdigest()!='5abfe12c262852ef3accf49511290db90c5c30ead56d3e145d88fe030272b395':raise ValueError('Whole FA5D0 original image changed')
        rows=corpus();physical,proof=physical_original(image,rows,scratch,entries=(0xfa5d0,),regions=REGIONS,
            trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in IO],trace_extended=True,preserve_x87_prefix=True)
        executed=set();packets=0
        for row,case in zip(rows,physical['cases'],strict=True):
            executed.update(case['executed_sites']);row.update({key:case[key]for key in ('completed','boundary_gp','boundary_eflags','boundary_xmm','after_fp','calls','reads')})
            row['expected_pc']=0xfa5e3 if case['completed']else case['boundary']['instruction']
            if not case['live_x87_unchanged']:raise ValueError('Physical bit-query changed caller FP')
            for event in row['calls']:event.pop('arguments');packets+=1
            after={bank['address']:bank['after']for bank in case['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'halfword-bit-query.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')))
        modes={};environment=dict(os.environ);environment.pop('HALO2_REFERENCE_HALFWORD_BIT_QUERY',None)
        for name,enabled in (('native',False),('reference',True)):
            env=dict(environment)
            if enabled:env['HALO2_REFERENCE_HALFWORD_BIT_QUERY']='1'
            run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=env,timeout=90)
            if run.returncode:raise ValueError(name+' bit-query comparison failed: '+run.stderr[:10000])
            modes[name]=json.loads(run.stdout)
        if executed!=set(SITES):raise ValueError('Incomplete original bit-query site coverage')
        paths=(SOURCE,'include/halo2/native_tu15_halfword_bit_query_kernel.hpp','src/physics/runtime/native_tu15_halfword_bit_query.hpp',
               'tools/native_halfword_bit_query_replacements.py','tests/native_tu15_halfword_bit_query_tests.cpp','tools/validate_native_halfword_bit_query_tu15.py')
        report.update(passed=True,physical_original=proof,physical_cases=len(rows),normal_cases=sum(row['completed']for row in rows),specialization_receipt=receipt,
            retained_read_faults=sum(not row['completed']for row in rows),all_original_sites=sorted(executed),
            complete_physical_pre_io_packets=packets,modes=modes,authentication_rejections=rejected,compiler_command=command,
            canonical_body_sha256=BODY_PIN,sources={name:sha(ROOT/name)for name in paths},whole_runtime_qualified=False)
    except Exception as error:report['error']=str(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],physical_cases=len(rows),modes=modes)));return 0


if __name__=='__main__':raise SystemExit(main())
