#!/usr/bin/env python3
"""Compare the private 1046C0 setup before its first original key comparison."""
import argparse
import copy
import json
import os
from pathlib import Path
import subprocess

import oracle_native_campaign_screen_lifecycle_tu15 as oracle
from oracle_smg_firing_1_1 import Xbe
from validate_native_flight_property_query_frame_tu15 import sha
from validate_native_object_record_ratio_public_tu15 import REGIONS, parent_rows
from validate_native_projectile_function_query_frame_tu15 import boundary_packet
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT=Path(__file__).resolve().parents[1]
ENTRY,RESUME=0x1046c0,0x104700
FRAME='src/gameplay/objects/native_tu15_object_ratio_caller_setup_frame.hpp'
DRIVER='tests/native_tu15_object_ratio_caller_setup_frame_tests.cpp'
DISPATCH_FRAME='src/gameplay/objects/native_tu15_object_ratio_caller_dispatch_frame.hpp'
DISPATCH_DRIVER='tests/native_tu15_object_ratio_caller_dispatch_frame_tests.cpp'
DISPATCH_KEYS={0x104700:0x10000573,0x104717:0x1500056e,0x104723:0x1700056c,
               0x10472f:0x1700056f,0x10473b:0x1d0007ba}
DISPATCH_CASES=((0x10000573,0x10487a),(0x11000575,0x104adf),(0x1700056c,0x104bfe),
                (0x1700056f,0x104cc5),(0x1d0007bb,0x1049d0),(0x1d0007ba,0x104747))
CONTROLS=(
    ('wrong-datum-stack','g[7]=read(0x1046ddu,g[4]+0x18u);','g[7]=read(0x1046ddu,g[4]+0x1cu);'),
    ('modeled-definition-shift','definition_index(s_.flags.logic(g[1]&0xffffu,4))',
     's_.flags.shift(s_.flags.logic(g[1]&0xffffu,4),4u,4)'),
    ('early-push-retirement','push(0x1046c0u,g[1]);','g[4]-=4u;Base::store(0x1046c0u,g[4],g[1],[]{});'),
    ('wrong-simd-seed','s_.xmm[3]=_mm_setzero_ps();','s_.xmm[3]=_mm_set1_ps(1.0f);'),
)
DISPATCH_CONTROLS=(
    ('wrong-exact-key','compare_key(0x1700056fu)','compare_key(0x1700056eu)'),
    ('unsigned-key-order','s_.flags.zf||s_.flags.sf!=s_.flags.of','s_.flags.zf||s_.flags.cf'),
    ('wrong-definition-slot','word(g[1]+g[0]+8u)','word(g[1]+g[0]+4u)'),
    ('overwrite-selector-flags','s_.xmm[2]=s_.xmm[3];','s_.xmm[2]=s_.xmm[3];s_.flags.logic(g[3],4);'),
)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--semantic-mutants',action='store_true')
    parser.add_argument('--initial-dispatch',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    frame,driver,controls=(DISPATCH_FRAME,DISPATCH_DRIVER,DISPATCH_CONTROLS) if args.initial_dispatch else (FRAME,DRIVER,CONTROLS)
    files=(frame,driver,'tests/native_tu15_object_property_frame_test_harness.hpp',
           'tools/validate_native_object_record_ratio_public_tu15.py',str(Path(__file__).relative_to(ROOT)))
    if args.initial_dispatch:files+=(FRAME,'include/halo2/native_tu15_object_ratio_caller_dispatch_kernel.hpp')
    before={name:sha(ROOT/name) for name in files}
    report=dict(passed=False,production_bound=False,completed_parent_return=False,source_before=before,
                owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[])
    try:
        if sha(args.xbe)!=PIN:
            raise ValueError('Wrong original TU1.5 image')
        image=Xbe(args.xbe);code=oracle._Code(image,'.text')
        if image.read(0x1046fd,3)!=bytes.fromhex('c1e104'):
            raise ValueError('Original 1046FD shift opcode differs')
        oracle.ENTRIES=(ENTRY,)
        metadata=oracle.metadata(image)
        cutoff=0x104747 if args.initial_dispatch else RESUME
        sites=[v for v in metadata['sites'] if ENTRY<=v['instruction']<cutoff]
        if len(sites)!=(32 if args.initial_dispatch else 19) or len(metadata['sites'])!=406:
            raise ValueError('Original setup or whole parent site count changed')
        if args.initial_dispatch:
            for pc,key in DISPATCH_KEYS.items():
                i=code.at(pc)
                if i.mnemonic!='cmp' or i.operands[-1].imm!=key:
                    raise ValueError('Authentic initial selector key changed')
        trace=[dict(entry=v['instruction'],arguments=0,gp=True,xmm=True) for v in sites
               if v['operands'] or code.at(v['instruction']).mnemonic=='push']
        rows=parent_rows(image)[:48]
        continuations={}
        for n,row in enumerate(rows):
            if args.initial_dispatch:
                key,resume=DISPATCH_CASES[n%6]
                if n%6==0 and n//6%2:key=0xffffffff
                row.update(source=key,continuation=resume,stop_before_entries=[pc for _,pc in DISPATCH_CASES])
                bank=next(b for b in row['banks'] if b['address']==0x62000000)
                data=bytearray.fromhex(bank['before']);offset=row['gp'][4]+8-0x62000000
                data[offset:offset+4]=key.to_bytes(4,'little');bank['before']=data.hex()
            else:row.update(continuation=RESUME,stop_before_entries=[RESUME])
            continuations[hex(row['continuation'])]=continuations.get(hex(row['continuation']),0)+1

        def observe(selected,folder):
            folder.mkdir()
            return physical_original(image,selected,folder,entries=(ENTRY,),regions=REGIONS,
                                     trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)

        normal,report['original_authority']=observe(rows,scratch/'normal')
        if any(not a['completed'] or a.get('stopped_at')!=row['continuation']
               for row,a in zip(rows,normal['cases'],strict=True)):
            raise ValueError('Original setup did not stop at the real continuation')
        prefixes=[];retired=[];owners=[]
        for n,(row,actual) in enumerate(zip(rows,normal['cases'],strict=True)):
            if n<16:
                for phase,field in (('before_read','reads'),('before_write','writes')):
                    for ordinal in range(len(actual[field])):
                        candidate=copy.deepcopy(row)
                        candidate.update(name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal))
                        prefixes.append(candidate)
            for ordinal,store in enumerate(actual['writes']):
                candidate=copy.deepcopy(row)
                candidate.update(name=row['name']+f'-observer-{ordinal}',fault=dict(phase='after_write',ordinal=ordinal))
                retired.append(candidate);owners.append((n,store['instruction']))
        stopped,report['prefix_authority']=observe(prefixes,scratch/'prefixes')
        observed,report['retired_authority']=observe(retired,scratch/'retired')
        stores=[[] for _ in rows]
        for row,actual,(index,pc) in zip(retired,observed['cases'],owners,strict=True):
            if actual['completed'] or actual['boundary']['instruction']!=pc:
                raise ValueError('Original setup retired store boundary changed')
            stores[index].append(boundary_packet(actual,row,pc))
            row.update(observer=True,observer_states=copy.deepcopy(stores[index]))
        for row,store in zip(rows,stores,strict=True):
            row.update(observer=True,observer_states=store)
        all_rows=rows+prefixes+retired
        cases=normal['cases']+stopped['cases']+observed['cases']
        reached={pc for actual in cases for pc in actual['executed_sites']}
        if reached!={v['instruction'] for v in sites}:
            raise ValueError('Original setup coverage incomplete')
        for row,actual in zip(all_rows,cases,strict=True):
            expected_pc=actual['boundary']['instruction'] if 'fault' in row else row['continuation']
            if actual['completed']==('fault' in row):
                raise ValueError('Original setup fault frontier changed')
            for packet in actual['calls']:
                if packet.pop('arguments')!=[]:
                    raise ValueError('Unexpected original setup call arguments')
            row.update(expected=boundary_packet(actual,row,expected_pc),reads=actual['reads'],
                       writes=actual['writes'],calls=actual['calls'])
            for bank,expected in zip(row['banks'],actual['banks'],strict=True):
                bank['after']=expected['after']
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(all_rows,separators=(',',':')))
        report.update(sites=len(reached),original_joins=len(rows),provider_prefixes=len(prefixes),
                      retired_store_prefixes=len(retired),continuations=continuations)
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name
            flags=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                   *('-I'+str(ROOT/tree) for tree in ('include','src','tests'))]
            if args.sanitizers:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']

            def build_run(target,overlays=()):
                built=subprocess.run([flags[0],*overlays,*flags[1:],ROOT/driver,'-o',target],
                                     capture_output=True,text=True,timeout=600)
                if built.returncode:
                    raise ValueError('1046C0 setup compile failed: '+built.stderr[-7000:])
                return subprocess.run([target,fixture],capture_output=True,text=True,timeout=1200)

            ran=build_run(binary)
            if ran.returncode:
                raise ValueError('1046C0 setup comparison failed: '+(ran.stdout+ran.stderr)[-8500:])
            mutants=[]
            if args.semantic_mutants:
                original=(ROOT/frame).read_text()
                for name,old,new in controls:
                    if original.count(old)!=1:
                        raise ValueError('Missing 1046C0 setup defect seam: '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);path=overlay/frame
                    path.parent.mkdir(parents=True);path.write_text(original.replace(old,new,1))
                    ran_bad=build_run(overlay/'bad',('-I'+str(overlay/'src'),))
                    diagnostic=(ran_bad.stdout+ran_bad.stderr)[-5000:]
                    if not ran_bad.returncode or not any(fragment in diagnostic for fragment in ('differ','Ray-frame bank extent')):
                        raise ValueError('1046C0 setup defect survived: '+name+' '+diagnostic)
                    mutants.append(dict(name=name,diagnostic=diagnostic))
            report['compilers'].append(dict(compiler=compiler,result=json.loads(ran.stdout),compiled_defects=mutants))
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        report['source_unchanged']=before=={name:sha(ROOT/name) for name in files}
        report['passed']=report['passed'] and report['source_unchanged']
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
