#!/usr/bin/env python3
"""Private actual 303800 predicate against unchanged physical TU1.5."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import shlex
import struct
import subprocess
import sys

from native_mopp_ray_dense_original import physical_original_dense
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from validate_native_contact_records_tu15 import inputs as record_inputs

ROOT=Path(__file__).resolve().parents[1]
ENTRY=0x303800
REGIONS=((0x1000000,4096),(0x41d000,4096),(0x62000000,8192))
KERNEL='include/halo2/native_tu15_contact_predicate_kernel.hpp'
FRAME='src/physics/runtime/native_tu15_contact_predicate_frame.hpp'
TEST='tests/native_tu15_contact_predicate_tests.cpp'
INPUTS=(KERNEL,FRAME,TEST,'tools/validate_native_contact_predicate_tu15.py',
    'src/physics/physics_live_support_tu15_generated.cpp','src/physics/native_tu15_contact_records.cpp',
    'tools/native_mopp_ray_dense_original.py','tools/validate_native_contact_records_tu15.py',
    'tools/validate_native_tu15_prt_helpers.py')


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def defects():
    def change(path,old,new):
        text=(ROOT/path).read_text()
        if text.count(old)!=1:raise ValueError('Exact predicate defect seam drift '+old)
        return path,text.replace(old,new,1)
    return {
        'stale-dot-without-real-reread':change(FRAME,'acquire_float(0x303853u,s_.gp[4]+12u);',
            'const auto stale=low_bits(s_.xmm[3]);asm volatile("flds %0"::"m"(stale):"memory","st");'),
        'reverse-separation-direction':change(KERNEL,'difference(positions.second, positions.first)',
            'difference(positions.first, positions.second)'),
        'acquire-settings-too-early':change(FRAME,
            's_.xmm[2]=vector(0x303828u,s_.gp[1]+16u);\n        at(0x30382cu); s_.gp[6]=word(s_.gp[5]+20u);',
            'at(0x30382cu); s_.gp[6]=word(s_.gp[5]+20u);\n        s_.xmm[2]=vector(0x303828u,s_.gp[1]+16u);'),
        'wrong-separation-status-mask':change(KERNEL,'view.separation_threshold(separation), 0x41u',
            'view.separation_threshold(separation), 0x05u'),
    }


def corpus(image, smoke):
    rows=[copy.deepcopy(row) for row in record_inputs(image) if row['entry']==ENTRY]
    if smoke:
        selected={'predicate_controls_0','predicate_controls_1','predicate_controls_2','predicate_controls_11',
                  'predicate_association_boundary','predicate_fault_seed','predicate_early_fault_seed'}
        rows=[row for row in rows if row['name'] in selected or row['alignment_fault']]
    rng=random.Random(ENTRY)
    for n,row in enumerate(rows):
        row['id']=n;row['gp'][4]=sp=0x62001000+(n%4)*4
        row['xmm']=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        row['eflags']=0x202|rng.choice((0,0x400,0x8d5,0x11,0x40,0x800))
        # The actual observer rewrites RET, formal1 and formal2 after banking.
        # Preserve the two remaining formal values in their actual stack slots.
        row['matrix'],row['source']=row['arguments'][:2]
        stack=bytearray(rng.randbytes(8192))
        struct.pack_into('<5I',stack,sp-0x62000000,0x63000000,*row['arguments'])
        row['banks'].append(dict(address=0x62000000,before=stack.hex()))
        if row['alignment_fault']:row['phase']='alignment_fault'
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--smoke',action='store_true')
    parser.add_argument('--prefixes',action='store_true')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--mutations',action='store_true')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:parser.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in ('x86_64','amd64'):return 77
    if not args.xbe.is_file():
        print('SKIP missing owned English TU1.5 executable')
        return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    before={path.resolve():sha(path)for tree in ('src','include','tools','tests')for path in (ROOT/tree).rglob('*')
            if path.suffix in ('.hpp','.h','.cpp','.inc','.def','.py') and path.is_file()}
    deps={ROOT/name for name in INPUTS}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[])
    try:
        if sha(args.xbe)!=NEW_PIN:raise ValueError('Original TU1.5 image drift')
        image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        for module in tuple(sys.modules.values()):
            if name:=getattr(module,'__file__',None):
                path=Path(name).resolve()
                if path in before and path.is_relative_to(ROOT/'tools'):deps.add(path)
        common.ENTRIES=(ENTRY,);original_metadata=common.metadata
        def metadata(source):
            result=original_metadata(source);code=common._Code(source,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in ('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;body=metadata(image)
        if len(body['sites'])!=60:raise ValueError('Actual original predicate owner site count drift')
        code=common._Code(image,'.text')
        io_sites=[site['instruction'] for site in body['sites'] if site['operands'] or code.at(site['instruction']).mnemonic=='push']
        rows=corpus(image,args.smoke);authorities=[]
        def measure(cases,label):
            folder=scratch/label;folder.mkdir()
            result,authority=physical_original_dense(image,cases,folder,entries=(ENTRY,),regions=REGIONS,
                trace_calls=[dict(entry=pc,gp=True,xmm=True) for pc in io_sites],trace_extended=True)
            authorities.append(authority);return result['cases']
        observed=measure(rows,'normal')
        if args.prefixes:
            pending=[]
            for name in ('predicate_fault_seed','predicate_early_fault_seed','predicate_association_boundary'):
                index=next(n for n,row in enumerate(rows)if row['name']==name)
                for phase,key in (('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal in range(len(observed[index][key])):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(pending),name=name+'-'+phase+str(ordinal),
                            phase=phase,fault=dict(phase=phase,ordinal=ordinal));pending.append(row)
            rows+=pending;observed+=measure(pending,'prefixes')
        covered=set()
        for row,result in zip(rows,observed,strict=True):
            covered.update(result['executed_sites'])
            if result['completed']!=('phase' not in row):raise ValueError('Original predicate completion differs '+row['name'])
            pc=result['boundary']['instruction']if 'phase'in row else 0x3038b3 if 0x3038b3 in result['executed_sites']else 0x3038a6
            row['expected']=dict(entry=pc,gp=result['boundary_gp'],eflags=result['boundary_eflags'],
                xmm=result['boundary_xmm'],fp=result['after_fp'],live_x87=result['boundary_live_x87'])
            row.update(reads=result['reads'],writes=result['writes'],calls=result['calls'])
            for packet in row['calls']:
                if packet.pop('arguments')!=[]:raise ValueError('Childless predicate acquired child arguments')
            for bank,actual in zip(row['banks'],result['banks'],strict=True):bank['after']=actual['after']
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')))
        mutations=defects() if args.mutations else {}
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
                '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(ROOT/TEST),'-o',str(binary),'-MMD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180)
            if built.returncode:raise ValueError('Predicate raw proof failed to compile: '+built.stderr[-7000:])
            for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(name).resolve()
                if path.is_relative_to(ROOT):deps.add(path)
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
            if result.returncode:raise ValueError('Predicate raw physical comparison failed: '+(result.stdout+result.stderr)[-9000:])
            record=dict(compiler=compiler,command=command,result=json.loads(result.stdout),compiled_defects=[])
            report['compilers'].append(record)
            for label,(path,text) in mutations.items():
                overlay=scratch/('defect-'+label)
                target=overlay/path.removeprefix('src/').removeprefix('include/')
                target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text)
                broken=scratch/(Path(compiler).name+'-'+label)
                compile_defect=[command[0],'-I'+str(overlay),*command[1:]]
                compile_defect[compile_defect.index('-o')+1]=str(broken)
                compile_defect[compile_defect.index('-MF')+1]=str(broken.with_suffix('.d'))
                compiled=subprocess.run(compile_defect,cwd=ROOT,capture_output=True,text=True,timeout=180)
                if compiled.returncode:raise ValueError('Predicate defect did not compile: '+compiled.stderr[-4000:])
                failed=subprocess.run([str(broken),str(fixture)],capture_output=True,text=True,timeout=120)
                diagnostic=failed.stdout+failed.stderr
                if failed.returncode!=1 or not any(message in diagnostic for message in (
                    'predicate full ABI differs','predicate ordered IO differs','predicate pre-IO packet','predicate stop differs')):
                    raise ValueError('Predicate defect did not produce a real semantic failure '+label+': '+diagnostic[-4000:])
                record['compiled_defects'].append(dict(name=label,sha256=sha(target),exit_code=failed.returncode,diagnostic=diagnostic[-1600:]))
        report.update(passed=True,physical_paths=len(rows),source_sites=sorted(hex(pc)for pc in covered),
            xbe_sha256=NEW_PIN,original=authorities,
            scope='Private actual 303800 typed predicate with physical aligned vector and scoped dense x87 transport. Real scratch stores and reloads; no installed or public change, arbitrary GP mutation, unsupported FP/MMX or pure-op Before claim.')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['inputs_before']={str(path.relative_to(ROOT)):before.get(path)for path in sorted(deps)}
        report['inputs_after']={str(path.relative_to(ROOT)):sha(path)for path in sorted(deps)}
        if report['inputs_after']!=report['inputs_before']:report.update(passed=False,input_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
