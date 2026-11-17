#!/usr/bin/env python3
"""Compare private inverse quaternion/scale phases with unchanged TU1.5 120740."""
import argparse
import copy
import hashlib
import json
import os
import platform
import random
import re
import shlex
import subprocess
import sys
from pathlib import Path

from native_mopp_ray_dense_original import physical_original_dense
from native_sphere_frustum_mmx_original import physical_original_mmx
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ROOT = Path(__file__).resolve().parents[1]
ENTRY, RET, STOP = 0x120740, 0x1208EC, 0x63000000
REGIONS = ((0x46a000,4096),(0x61000000,8192),(0x62000000,4096))
TEST = 'tests/native_tu15_codec_inverse_transform_tests.cpp'
FRAME = 'src/physics/runtime/native_tu15_codec_inverse_transform_frame.hpp'
KERNEL = 'include/halo2/native_tu15_codec_inverse_transform_kernel.hpp'
SOURCE_PINS = {
    'src/physics/physics_live_support_tu15_generated.cpp': 'ba7e11aee6856dc25355763b2c05de90b988a63567aa6cddb1a059f57e26cba8',
    'src/gameplay/units/source_combat_action_tu15_generated.cpp': 'd58dc1c6083f4887e8187d4a90ab78d9386b5b34bd5e14ce15d8c5fbd806600b',
}
CONSTANTS = {0x46a11c: '00000040',0x46a0d0: '0000803f',0x46a0dc: '000080bf',0x46a348: '00401c46'}
DEFECTS = (
    ('missing-conjugation', KERNEL, 'w=view.multiply(w,view.read(Read::conjugate_sign));', 'w=view.add(w,view.read(Read::conjugate_sign));'),
    ('swapped-dot-nan-operands', KERNEL, 'dot=view.multiply(dot,view.read(Read::dot_x_quaternion));', 'dot=view.multiply(view.read(Read::dot_x_quaternion),dot);'),
    ('reissued-scale-test-read', KERNEL, 'view.divide(reciprocal,view.read(Read::scale_divisor))', 'view.divide(reciprocal,view.read(Read::scale_one_test))'),
    ('wrong-scale-test-flags', FRAME, 'bool scale_test_passed()const{return s_.flags.pf;}', 'bool scale_test_passed()const{return !s_.flags.zf;}'),
    ('early-final-z-store', KERNEL, '    view.store(Store::reciprocal_scale,reciprocal);\n    view.store(Store::signed_z,signed_value);',
     '    view.store(Store::signed_z,signed_value);\n    view.store(Store::reciprocal_scale,reciprocal);'),
)


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def owner_bodies():
    result={}
    for name,pin in SOURCE_PINS.items():
        text=(ROOT/name).read_text()
        values=re.findall(r'static void run_120740\(WorldState &s\) \{.*?\n\}',text,re.S)
        if len(values)!=1 or hashlib.sha256(values[0].encode()).hexdigest()!=pin:
            raise ValueError('Complete inverse transform source owner changed: '+name)
        result[name]=[int(pc,16) for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',values[0])]
    return result


def cases(image,smoke):
    rng=random.Random(0x120740)
    scalars=(0x00000000,0x80000000,0x3f800000,0xbf800000,0x40000000,0x3e000000,
             0x00000001,0x80000001,0x7f800000,0xff800000,0x7fc12345,0xff812345)
    words=(0x3f800000,0xbf800000,0x3e800000,0x40000000,0x80000000,0x80000001,
           0x00000000,0x00800000,0x7f7fffff,0x7fc22222,0xffc33333,0x7f844444)
    live=tuple((0x8000000000000000|rng.getrandbits(60)).to_bytes(8,'little').hex()+'ff3f' for _ in range(8))
    constants=image.read(0x46a000,4096)
    rows=[]
    for mode in range(2 if smoke else 16):
        for scale in scalars:
            index=len(rows)
            bank={base:bytearray(constants if base==0x46a000 else rng.randbytes(size)) for base,size in REGIONS}
            sp=0x62000100+index%16
            source=0x61000080+4*(mode%4)
            destination=(0x61000400,source,source+4,source-4,sp-0x20,0x46a0c8,sp-0x30,0x46a110)[mode%8]

            def put(address,value):
                for base,data in bank.items():
                    if base<=address<=base+len(data)-4:
                        data[address-base:address-base+4]=value.to_bytes(4,'little');return
                raise ValueError('Inverse transform declared input escaped backing')

            row=[words[(index+lane*3)%len(words)]for lane in range(7)]
            # Independent noncanonical NaNs in the first point/quaternion
            # product expose operand order rather than merely NaN-ness.
            if index%len(scalars)==0:row[0],row[4]=0xffc22345,0x7fc54321
            for lane,value in enumerate((*row,scale)):put(source+lane*4,value)
            for lane,value in enumerate((STOP,0,0)):put(sp+lane*4,value)
            gp=[rng.getrandbits(32)for _ in range(8)]
            gp[1],gp[2],gp[4]=destination,source,sp
            depth=(0,1,4,8)[mode%4]
            cw=(0x7f,0x27f,0x37f)[mode%3]|((mode%4)<<10)
            sw=((index%8)<<11)|((index<<7)&0x4700)|(index&0x3f)
            mxcsr=0x1f80|((mode%4)<<13)|(index&0x3f)|(0,0x40,0x8000,0x8040)[mode//4]
            rows.append(dict(id=index,name=f'codec-inverse-transform-{mode}-{index}',entry=ENTRY,matrix=0,source=0,
                gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                before_fp=[cw,sw,mxcsr],live_x87=live[:depth],max_steps=200,
                banks=[dict(address=base,before=data.hex())for base,data in bank.items()]))
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT)
    for option in ('smoke','prefixes','mmx','sanitizers','semantic-mutants'):
        parser.add_argument('--'+option,action='store_true')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):
        return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True,exist_ok=False)
    before={path.resolve():sha(path) for folder in ('tools','include','src','tests') for path in (ROOT/folder).rglob('*')
            if path.is_file() and path.suffix in ('.py','.hpp','.h','.cpp','.inc')}
    used={ROOT/TEST,ROOT/FRAME,ROOT/KERNEL,Path(__file__).resolve(),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'}
    report=dict(passed=False,private_unbound=True,readonly_gp_xmm_before_provider_domain=True,source_body_pins=SOURCE_PINS,
                owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[])
    try:
        sources=owner_bodies()
        if sha(args.xbe)!=NEW_PIN:
            raise ValueError('Expected unchanged English TU1.5 XBE')
        image=Xbe(args.xbe);code=_Code(image,'.text');graph=_trace(code,ENTRY,{ENTRY})
        sites=sorted(graph.code)
        if len(sites)!=104 or any(values!=sites for values in sources.values()) or any(code.at(pc).mnemonic=='call' for pc in sites):
            raise ValueError('Inverse transform complete original/source site set changed')
        if any(image.read(address,4).hex()!=value for address,value in CONSTANTS.items()):
            raise ValueError('Inverse transform original numerical constants changed')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        base=common.metadata;common.ENTRIES=(ENTRY,)

        def metadata(ref):
            result=base(ref)
            for site in result['sites']:
                if site['instruction'] in (0x12086f,0x120870,RET):
                    if site['operands'] or code.at(site['instruction']).mnemonic not in ('pop','ret'):
                        raise ValueError('Inverse transform implicit stack-read operand changed')
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result

        common.metadata=metadata
        io=[site['instruction'] for site in metadata(image)['sites'] if site['operands'] or code.at(site['instruction']).mnemonic=='push']
        trace=[dict(entry=pc,gp=True,xmm=True) for pc in io]
        rows=cases(image,args.smoke);normal=len(rows);proofs=[]

        def measure(values,label):
            folder=scratch/label;folder.mkdir();opaque='active_mmx' in values[0]
            runner=physical_original_mmx if opaque else physical_original_dense
            result,proof=runner(image,values,folder,entries=(ENTRY,),regions=REGIONS+(((0x64000000,4096),)if opaque else()),
                                trace_calls=trace,trace_extended=True)
            proofs.append(proof)
            return result['cases']

        observed=measure(rows,'normal')
        if args.mmx:
            rng=random.Random(0x120740a11);opaque=[]
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal]);row.update(id=len(rows)+len(opaque),name=row['name']+f'-mmx-{mask}',live_x87=[])
                row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[
                    (rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
                opaque.append(row)
            rows+=opaque;observed+=measure(opaque,'opaque')
        if args.prefixes:
            selected=[(rows[n],observed[n]) for n in (0,2,4,13) if n<normal]
            if args.mmx:selected.extend((rows[n],observed[n]) for n in (normal,normal+2,normal+4))
            seen=set();pending=[]
            for row,result in selected:
                opaque='active_mmx' in row
                for phase,key in (('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(result[key]):
                        signature=(opaque,phase,event['instruction'])
                        if signature in seen:continue
                        seen.add(signature);prefix=copy.deepcopy(row)
                        prefix.update(id=0,name=row['name']+f'-{phase}-{event["instruction"]:x}',phase=phase,fault=dict(phase=phase,ordinal=ordinal))
                        pending.append(prefix)
            for opaque in (False,True):
                batch=[row for row in pending if ('active_mmx' in row)==opaque]
                if not batch:continue
                for index,row in enumerate(batch,len(rows)):row['id']=index
                rows+=batch;observed+=measure(batch,'prefix-'+str(opaque))
        coverage=set()
        for row,result in zip(rows,observed,strict=True):
            coverage.update(result['executed_sites'])
            if result['completed']!=('phase' not in row):
                raise ValueError('Inverse transform original completion differs')
            pc=result['boundary']['instruction'] if 'phase'in row else RET
            row['expected']=dict(entry=pc,gp=result['boundary_gp'],eflags=result['boundary_eflags'],xmm=result['boundary_xmm'],
                                 fp=result['after_fp'],live_x87=result.get('boundary_live_x87',[]))
            if 'active_mmx'in result:row['expected']['active_mmx']=result['active_mmx']
            row.update(reads=result['reads'],writes=result['writes'],calls=result['calls'])
            for call in row['calls']:
                if call.pop('arguments'):raise ValueError('Unexpected inverse transform trace argument')
            for bank,after in zip(row['banks'],result['banks'],strict=True):
                if bank['address']!=after['address']:raise ValueError('Inverse transform original bank identity changed')
                bank['after']=after['after']
        if coverage!=set(sites):raise ValueError('Inverse transform original branch coverage incomplete')
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')))
        report.update(physical_paths=len(rows),normal_paths=normal,original_sites=len(coverage),original=proofs,
                      prefix_paths=sum('phase'in row for row in rows))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                     '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            done=subprocess.run(command,capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Inverse transform harness compile failed: '+done.stderr[-12000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:used.add(path)
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Inverse transform comparison failed: '+(done.stdout+done.stderr)[-12000:])
            result=dict(compiler=compiler,result=json.loads(done.stdout),compiled_defects=[])
            report['compilers'].append(result)
            if args.semantic_mutants:
                for name,relative,old,new in DEFECTS:
                    text=(ROOT/relative).read_text()
                    if text.count(old)!=1:raise ValueError('Inverse transform defect seam changed '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);path=overlay/relative;path.parent.mkdir(parents=True)
                    path.write_text(text.replace(old,new,1));mutant=overlay/'probe'
                    command_mutant=[command[0],'-I'+str(overlay/'include'),'-I'+str(overlay/'src'),*command[1:]]
                    command_mutant[command_mutant.index('-o')+1]=str(mutant)
                    command_mutant[command_mutant.index('-MF')+1]=str(overlay/'probe.d')
                    done=subprocess.run(command_mutant,capture_output=True,text=True,timeout=480)
                    if done.returncode:raise ValueError('Inverse transform mutant failed to build '+name+' '+done.stderr[-9000:])
                    done=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=480)
                    diagnostic=done.stdout+done.stderr
                    if not done.returncode or 'codec inverse transform' not in diagnostic:
                        raise ValueError('Inverse transform defect survived '+name+' '+diagnostic[-7000:])
                    result['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-7000:]))
        if owner_bodies()!=sources:raise ValueError('Exact inverse transform source bodies changed during proof')
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        used.update(Path(module.__file__).resolve() for module in tuple(sys.modules.values())
                    if getattr(module,'__file__',None) and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        report['source_before']={str(path.relative_to(ROOT)):before[path] for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name) for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
