#!/usr/bin/env python3
"""Qualify the complete private 1201E0 product against unchanged TU1.5."""
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

ROOT=Path(__file__).resolve().parents[1]
ENTRY,RET,STOP=0x1201E0,0x120309,0x63000000
REGIONS=((0x61000000,8192),(0x62000000,4096))
KERNEL='include/halo2/native_tu15_codec_quaternion_product_kernel.hpp'
FRAME='src/physics/runtime/native_tu15_codec_quaternion_product_frame.hpp'
TEST='tests/native_tu15_codec_quaternion_product_tests.cpp'
OWNERS={
    'src/physics/physics_live_support_tu15_generated.cpp':'21e368b3ba9b3a890a9f328e7462b871fa7f73f3700ba440827065815fafb88e',
    'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp':'0ccee3871480d826fb78b1cdfad386eba96a7c82721d57d167f0b0cdb94da51e',
}
DEFECTS=(
    ('wrong-left-stage',KERNEL,'if(view.begin())view.stage_left();','if(view.begin())view.stage_right();'),
    ('early-staged-z',FRAME,'        publish(store_pcs[3],g[4]+16u,g[Register]);\n        publish(store_pcs[2],g[4]+12u,g[6]);',
     '        publish(store_pcs[2],g[4]+12u,g[6]);\n        publish(store_pcs[3],g[4]+16u,g[Register]);'),
    ('wrong-cross-sign',KERNEL,'Term{right_y,left_z,true}','Term{right_y,left_z,false}'),
    ('swapped-nan-product',KERNEL,'result=view.multiply(result,view.read(component,1u,row[0].second));',
     'result=view.multiply(view.read(component,1u,row[0].second),result);'),
    ('wrong-stack-release',FRAME,'s_.flags.plus(s_.gp[4],16u,4u)','s_.flags.plus(s_.gp[4],12u,4u)'),
)


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def original_owners():
    scopes={}
    for name,pin in OWNERS.items():
        text=(ROOT/name).read_text()
        bodies=re.findall(r'static void run_1201e0\(WorldState &s\) \{.*?\n\}',text,re.S)
        if len(bodies)!=1 or hashlib.sha256(bodies[0].encode()).hexdigest()!=pin:
            raise ValueError('Complete quaternion-product source owner changed: '+name)
        scopes[name]=[int(pc,16)for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',bodies[0])]
    return scopes


def cases(smoke):
    rng=random.Random(ENTRY);rows=[]
    values=(0,0x80000000,0x00000001,0x80000001,0x00800000,0x3f800000,0xbf800000,0x3e800000,
            0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3eaaaaab)
    live=tuple((0x8000000000000000|rng.getrandbits(60)).to_bytes(8,'little').hex()+'ff3f'for _ in range(8))
    for mode in range(8 if smoke else 12):
        for profile in range(4 if smoke else 16):
            n=len(rows);sp=0x62000100+n%16
            left,right=0x61000100,0x61000200
            if mode==3:right=left
            if mode==8:left=sp-16
            if mode==9:right=sp-20
            if mode==10:left,right=sp-20,sp-16
            out=(0x61000300,left,right,left,left+4,right-4,sp-20,sp-16,left,right,sp-20,0x61000310)[mode]
            data={base:bytearray(rng.randbytes(size))for base,size in REGIONS}

            def put(address,word):
                for base,bank in data.items():
                    if base<=address<=base+len(bank)-4:bank[address-base:address-base+4]=word.to_bytes(4,'little');return
                raise ValueError('Quaternion product fixture escaped declared backing')

            a=[values[(profile+lane*3+mode)%len(values)]for lane in range(4)]
            b=[values[(profile+lane*7+mode+2)%len(values)]for lane in range(4)]
            if profile==0:a[0],b[3]=0xffc22345,0x7fc54321
            for lane in range(4):put(left+lane*4,a[lane]);put(right+lane*4,b[lane]);
            for lane,word in enumerate((STOP,0,0)):put(sp+lane*4,word)
            gp=[rng.getrandbits(32)for _ in range(8)];gp[0],gp[1],gp[2],gp[4]=right,left,out,sp
            cw=(0x7f,0x27f,0x37f)[mode%3]|((mode%4)<<10)
            sw=((n%8)<<11)|((n<<7)&0x4700)|(n&0x3f)
            mx=0x1f80|((mode%4)<<13)|(n&0x3f)|(0,0x40,0x8000,0x8040)[mode%4]
            rows.append(dict(id=n,name=f'quaternion-product-{mode}-{profile}',entry=ENTRY,gp=gp,matrix=0,source=0,
                eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                before_fp=[cw,sw,mx],live_x87=live[:(0,1,4,8)[mode%4]],max_steps=100,
                banks=[dict(address=base,before=bank.hex())for base,bank in data.items()]))
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append');parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT)
    for option in ('smoke','mmx','prefixes','sanitizers','semantic-mutants'):parser.add_argument('--'+option,action='store_true')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH'not in os.environ:parser.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    before={path.resolve():sha(path)for tree in ('tools','src','include','tests')for path in (ROOT/tree).rglob('*')
            if path.is_file()and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used={ROOT/KERNEL,ROOT/FRAME,ROOT/TEST,Path(__file__).resolve(),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'}
    report=dict(passed=False,private_unbound=True,readonly_gp_xmm_before_provider_domain=True,owned_scratch=str(scratch),
                retained_large_artifacts=[],source_owner_pins=OWNERS,compilers=[])
    try:
        scopes=original_owners()
        if sha(args.xbe)!=NEW_PIN:raise ValueError('Original English TU1.5 XBE changed')
        image=Xbe(args.xbe);code=_Code(image,'.text');sites=sorted(_trace(code,ENTRY,{ENTRY}).code)
        if len(sites)!=75 or any(scopes[name]!=sites for name in OWNERS)or any(code.at(pc).mnemonic=='call'for pc in sites):
            raise ValueError('Complete original quaternion product instructions changed')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        old=common.metadata;common.ENTRIES=(ENTRY,)

        def metadata(ref):
            result=old(ref)
            for site in result['sites']:
                if site['instruction']in(0x120305,RET):
                    if site['operands']:raise ValueError('Implicit quaternion product stack read changed')
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result

        common.metadata=metadata
        io=[site['instruction']for site in metadata(image)['sites']if site['operands']or code.at(site['instruction']).mnemonic=='push']
        trace=[dict(entry=pc,gp=True,xmm=True)for pc in io]
        rows=cases(args.smoke);normal=len(rows);proofs=[]

        def measure(values,label):
            folder=scratch/label;folder.mkdir();opaque='active_mmx'in values[0]
            runner=physical_original_mmx if opaque else physical_original_dense
            result,proof=runner(image,values,folder,entries=(ENTRY,),regions=REGIONS+(((0x64000000,4096),)if opaque else()),
                                trace_calls=trace,trace_extended=True)
            proofs.append(proof);return result['cases']

        originals=measure(rows,'normal')
        if args.mmx:
            rng=random.Random(0x1201e0a11);opaque=[]
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal]);row.update(id=len(rows)+len(opaque),name=row['name']+f'-opaque-{mask}',live_x87=[])
                row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[
                    (rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
                opaque.append(row)
            rows+=opaque;originals+=measure(opaque,'opaque')
        if args.prefixes:
            seen=set();pending=[]
            for row,original in zip(rows,originals,strict=True):
                opaque='active_mmx'in row
                for phase,key in (('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(original[key]):
                        signature=(opaque,phase,event['instruction'])
                        if signature in seen:continue
                        seen.add(signature);prefix=copy.deepcopy(row)
                        prefix.update(id=0,name=row['name']+f'-{phase}-{event["instruction"]:x}',phase=phase,fault=dict(phase=phase,ordinal=ordinal))
                        pending.append(prefix)
            for opaque in(False,True):
                subset=[row for row in pending if ('active_mmx'in row)==opaque]
                if not subset:continue
                for index,row in enumerate(subset,len(rows)):row['id']=index
                rows+=subset;originals+=measure(subset,'prefix-'+str(opaque))
        covered=set()
        for row,original in zip(rows,originals,strict=True):
            covered.update(original['executed_sites'])
            if original['completed']!=('phase'not in row):raise ValueError('Original quaternion-product completion differs')
            pc=original['boundary']['instruction']if 'phase'in row else RET
            row['expected']=dict(entry=pc,gp=original['boundary_gp'],eflags=original['boundary_eflags'],xmm=original['boundary_xmm'],
                                 fp=original['after_fp'],live_x87=original.get('boundary_live_x87',[]))
            if 'active_mmx'in original:row['expected']['active_mmx']=original['active_mmx']
            row.update(reads=original['reads'],writes=original['writes'],calls=original['calls'])
            for packet in row['calls']:
                if packet.pop('arguments'):raise ValueError('Unexpected quaternion product trace arguments')
            for bank,after in zip(row['banks'],original['banks'],strict=True):
                if bank['address']!=after['address']:raise ValueError('Original quaternion product bank address changed')
                bank['after']=after['after']
        if covered!=set(sites):raise ValueError('Original quaternion product branch coverage incomplete')
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')))
        report.update(physical_paths=len(rows),normal_paths=normal,original_sites=len(covered),original=proofs,
                      fault_paths=sum('phase'in row for row in rows))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                     '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            done=subprocess.run(command,capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Quaternion-product compile failed: '+done.stderr[-10000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:used.add(path)
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Original quaternion product comparison failed: '+(done.stdout+done.stderr)[-11000:])
            result=dict(compiler=compiler,result=json.loads(done.stdout),compiled_defects=[]);report['compilers'].append(result)
            if args.semantic_mutants:
                for name,relative,old,new in DEFECTS:
                    text=(ROOT/relative).read_text()
                    if text.count(old)!=1:raise ValueError('Quaternion product defect seam changed '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);header=overlay/relative;header.parent.mkdir(parents=True)
                    header.write_text(text.replace(old,new,1));mutant=overlay/'probe'
                    changed=[command[0],'-I'+str(overlay/'include'),'-I'+str(overlay/'src'),*command[1:]]
                    changed[changed.index('-o')+1]=str(mutant);changed[changed.index('-MF')+1]=str(overlay/'probe.d')
                    done=subprocess.run(changed,capture_output=True,text=True,timeout=480)
                    if done.returncode:raise ValueError('Quaternion product mutant compile failed '+name+' '+done.stderr[-8000:])
                    done=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=480);diagnostic=done.stdout+done.stderr
                    if not done.returncode or 'quaternion product'not in diagnostic:raise ValueError('Quaternion product defect survived '+name+' '+diagnostic[-7000:])
                    result['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-7000:]))
        if original_owners()!=scopes:raise ValueError('Exact quaternion product source scope drift')
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        used.update(Path(module.__file__).resolve()for module in tuple(sys.modules.values())
                    if getattr(module,'__file__',None)and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        report['source_before']={str(path.relative_to(ROOT)):before[path]for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name)for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
