#!/usr/bin/env python3
"""Private whole actual2E47B0/2E4690 geometry DATA and raw ABI proof."""
import argparse,copy,hashlib,json,os,platform,random,re,shlex,struct,subprocess
from pathlib import Path
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe
from native_mopp_ray_dense_original import physical_original_dense
from validate_native_geometry_tu15 import inputs as geometry_inputs

ROOT=Path(__file__).resolve().parents[1]
ENTRIES=(0x2e47b0,0x2e4690)
REGIONS=((0x1000000,4096),(0x62000000,8192))
INPUTS=('include/halo2/native_tu15_geometry_leaf_kernel.hpp','src/physics/runtime/native_tu15_geometry_leaf_frame.hpp',
    'tests/native_tu15_geometry_leaf_tests.cpp','tools/validate_native_geometry_leaf_tu15.py',
    'src/physics/native_tu15_geometry.cpp','include/halo2/native_tu15_geometry.hpp',
    'tools/validate_native_geometry_tu15.py','tools/native_mopp_ray_dense_original.py','tools/validate_native_tu15_prt_helpers.py')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus(smoke):
    rows=[];rng=random.Random(0x2e47b0)
    for original in geometry_inputs():
        if original['entry']not in ENTRIES:continue
        if smoke and sum(row['entry']==original['entry']for row in rows)>=4:continue
        row=copy.deepcopy(original);index=len(rows);row['id']=index
        row['gp'][4]=sp=0x62001000+(index%4)*4
        row['eflags']=0x202|rng.choice((0,0x400,0x8d5,0x11,0x40,0x800))
        row['xmm']=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        stack=bytearray(rng.randbytes(8192));struct.pack_into('<III',stack,sp-0x62000000,0x63000000,row['matrix'],row['source'])
        row['banks'].append(dict(address=0x62000000,before=stack.hex()))
        if row.get('alignment_fault'):row['phase']='alignment_fault'
        rows.append(row)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--smoke',action='store_true');p.add_argument('--prefixes',action='store_true');p.add_argument('--sanitizers',action='store_true');p.add_argument('--mutations',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:sha(ROOT/name)for name in INPUTS}
    headers={path.resolve():sha(path)for tree in('include','src','tests')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc','.cpp')}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original TU15 image differs')
        image=Xbe(NEW_DEFAULT);import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=ENTRIES;original=common.metadata
        def metadata(source):
            result=original(source);code=common._Code(source,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;sites=metadata(image)['sites'];code=common._Code(image,'.text')
        # The shared runner adds PUSH's implicit write after this metadata
        # stage. Include its actual entry packet in the physical trace too.
        io=[site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic=='push']
        rows=corpus(a.smoke);proofs=[]
        def measure(selected,label):
            owned=scratch/label;owned.mkdir()
            result,authority=physical_original_dense(image,selected,owned,entries=ENTRIES,regions=REGIONS,
                trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True)
            proofs.append(authority);return result['cases']
        actual=measure(rows,'normal')
        if a.prefixes:
            pending=[]
            for index in(0,next(n for n,row in enumerate(rows)if row['entry']==ENTRIES[1])):
                for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal in range(len(actual[index][key])):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(pending),name=row['name']+'-'+phase+str(ordinal),phase=phase,fault=dict(phase=phase,ordinal=ordinal));pending.append(row)
            rows+=pending;actual+=measure(pending,'prefixes')
        coverage=set()
        for row,observed in zip(rows,actual,strict=True):
            coverage.update(observed['executed_sites'])
            if observed['completed']!=('phase'not in row):raise ValueError('Original geometry completion differs')
            last=0x2e481c if row['entry']==ENTRIES[0]else 0x2e46db
            pc=observed['boundary']['instruction']if'phase'in row else last
            row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',[]))
            row.update(reads=observed['reads'],writes=observed['writes'],calls=observed['calls'])
            for call in row['calls']:
                if call.pop('arguments')!=[]:raise ValueError('Childless geometry acquired child arguments')
            for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        variants={}
        if a.mutations:
            kernel_name='include/halo2/native_tu15_geometry_leaf_kernel.hpp';frame_name='src/physics/runtime/native_tu15_geometry_leaf_frame.hpp'
            kernel=(ROOT/kernel_name).read_text();frame=(ROOT/frame_name).read_text()
            def changed(text,old,new):
                if text.count(old)!=1:raise ValueError('Exact private geometry mutation anchor differs '+old)
                return text.replace(old,new,1)
            variants['wrong-transposed-axis']=(kernel_name,changed(kernel,'d.z_axis=_mm_shuffle_ps(d.xy_high,d.z_high,0x88);','d.z_axis=_mm_shuffle_ps(d.xy_high,d.z_high,0xdd);'))
            reassociated=changed(kernel,'d.translated_z=add(d.z_term,d.translation);','d.translated_z=d.z_term;')
            reassociated=changed(reassociated,'d.result=add(add(d.x_term,d.y_term),d.translated_z);','d.result=add(add(add(d.x_term,d.y_term),d.translated_z),d.translation);')
            variants['reassociated-translation']=(kernel_name,reassociated)
            variants['missing-intermediate-publication']=(frame_name,changed(frame,'void z_term_ready(const Position&d){s_.xmm[2]=_mm_shuffle_ps(d.input,d.input,0xaa);s_.xmm[3]=d.z_term;}','void z_term_ready(const Position&){ }'))
            variants['wrong-source-formal']=(frame_name,changed(frame,'s_.gp[0]=word(s_.gp[5]+12u);','s_.gp[0]=word(s_.gp[5]+8u);'))
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(ROOT/'tests/native_tu15_geometry_leaf_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Geometry leaf compile failed: '+done.stderr[-7000:])
            for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(name).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
            if done.returncode:raise ValueError('Geometry leaf mismatch: '+(done.stdout+done.stderr)[-9000:])
            result=dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_defects=[])
            for label,(name,text)in variants.items():
                overlay=scratch/('mutation-'+label);target=overlay/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text)
                negative=scratch/(Path(compiler).name+'-'+label)
                mutated=list(command);mutated[mutated.index('-o')+1]=str(negative);mutated[mutated.index('-MF')+1]=str(negative.with_suffix('.d'))
                mutated[1:1]=['-I'+str(overlay/'include'),'-I'+str(overlay/'src')]
                compiled=subprocess.run(mutated,capture_output=True,text=True)
                if compiled.returncode:raise ValueError('Geometry semantic defect did not compile: '+compiled.stderr[-7000:])
                rejected=subprocess.run([str(negative),str(fixture)],capture_output=True,text=True,timeout=120)
                diagnostic=(rejected.stdout+rejected.stderr)
                if rejected.returncode==0 or not any(message in diagnostic for message in('geometry leaf full ABI differs','geometry leaf pre-IO packet','geometry leaf ordered IO differs')):
                    raise ValueError('Geometry semantic defect was not meaningfully rejected '+label+': '+diagnostic[-6000:])
                result['compiled_defects'].append(dict(name=label,source=name,sha256=hashlib.sha256(text.encode()).hexdigest(),command=mutated,exit_code=rejected.returncode,diagnostic=diagnostic[-1500:]))
            results.append(result)
        report.update(passed=True,physical_paths=len(rows),source_sites=sorted(hex(pc)for pc in coverage),original=proofs,compilers=results,
            scope='Private actual TU15 whole childless geometry leaves with declared ordinary atomic vector backing, semantic policy shared from the existing public algorithms, complete GP/flags/XMM/live80 and pre-I/O transport. The default four-lane vector provider, Before-only non-I/O stops, mutable GP/XMM providers, source-store 8+8 behavior, other images, production default and public facade extraction remain separate.')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
