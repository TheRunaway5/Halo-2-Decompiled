#!/usr/bin/env python3
"""Private genuine303BA0 record-copy and30BAD0 sentinel raw ABI proof."""
import argparse,copy,hashlib,json,os,platform,random,shlex,struct,subprocess
from pathlib import Path
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe
from native_mopp_ray_dense_original import physical_original_dense
from validate_native_contact_records_tu15 import inputs as record_inputs

ROOT=Path(__file__).resolve().parents[1]
ENTRIES=(0x303ba0,0x30bad0)
REGIONS=((0x1000000,4096),(0x41d000,4096),(0x62000000,8192))
INPUTS=('include/halo2/native_tu15_contact_copy_leaf_kernel.hpp','src/physics/runtime/native_tu15_contact_copy_leaf_frame.hpp',
 'tests/native_tu15_contact_copy_leaf_tests.cpp','tools/validate_native_contact_copy_leaf_tu15.py',
 'src/physics/native_tu15_contact_records.cpp','include/halo2/native_tu15_contact_records.hpp',
 'include/halo2/native_tu15_contact_agent_dispatch.hpp','tools/validate_native_contact_records_tu15.py',
 'tools/native_mopp_ray_dense_original.py','tools/validate_native_tu15_prt_helpers.py')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus(image,smoke):
    rows=[copy.deepcopy(row)for row in record_inputs(image)if row['entry']==ENTRIES[0]]
    for n in range(16):
        row=copy.deepcopy(rows[n%len(rows)]);row.update(entry=ENTRIES[1],name='invalid-sentinel-'+str(n),arguments=[],alignment_fault=False);rows.append(row)
    if smoke:rows=rows[:3]+rows[-3:]
    rng=random.Random(0x303ba0)
    for index,row in enumerate(rows):
        row['id']=index;row['gp'][4]=sp=0x62001000+(index%4)*4
        row['eflags']=0x202|rng.choice((0,0x400,0x8d5,0x11,0x40,0x800))
        row['xmm']=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        # The unchanged shared observer seeds three declared caller words
        # after installing banks. Match that actual input transport exactly;
        # copy consumes the first formal and the sentinel consumes neither.
        row['matrix']=row['arguments'][0]if row['arguments']else 0;row['source']=0
        stack=bytearray(rng.randbytes(8192));struct.pack_into('<III',stack,sp-0x62000000,0x63000000,row['matrix'],row['source'])
        row['banks'].append(dict(address=0x62000000,before=stack.hex()))
        if row.get('alignment_fault'):row['phase']='alignment_fault'
    return rows
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--smoke',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--mutations',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:sha(ROOT/name)for name in INPUTS}
    headers={path.resolve():sha(path)for tree in('include','src','tests')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc','.cpp')}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original TU15 image drift')
        image=Xbe(NEW_DEFAULT);import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=ENTRIES;original=common.metadata
        def metadata(source):
            result=original(source);code=common._Code(source,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic=='ret':site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;sites=metadata(image)['sites'];io=[site['instruction']for site in sites if site['operands']]
        rows=corpus(image,args.smoke);authorities=[]
        def measure(selected,label):
            folder=scratch/label;folder.mkdir();result,authority=physical_original_dense(image,selected,folder,entries=ENTRIES,regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True)
            authorities.append(authority);return result['cases']
        observed=measure(rows,'normal')
        if args.prefixes:
            pending=[]
            for index in(0,next(n for n,row in enumerate(rows)if row['entry']==ENTRIES[1])):
                for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal in range(len(observed[index][key])):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(pending),name=row['name']+'-'+phase+str(ordinal),phase=phase,fault=dict(phase=phase,ordinal=ordinal));pending.append(row)
            rows+=pending;observed+=measure(pending,'prefixes')
        covered=set()
        for row,result in zip(rows,observed,strict=True):
            covered.update(result['executed_sites'])
            if result['completed']!=('phase'not in row):raise ValueError('Original copy/sentinel completion differs')
            pc=result['boundary']['instruction']if'phase'in row else 0x303bfe if row['entry']==ENTRIES[0]else 0x30bad5
            row['expected']=dict(entry=pc,gp=result['boundary_gp'],eflags=result['boundary_eflags'],xmm=result['boundary_xmm'],fp=result['after_fp'],live_x87=result.get('boundary_live_x87',[]))
            row.update(reads=result['reads'],writes=result['writes'],calls=result['calls'])
            for call in row['calls']:
                if call.pop('arguments')!=[]:raise ValueError('Childless copy/sentinel acquired child arguments')
            for bank,after in zip(row['banks'],result['banks'],strict=True):bank['after']=after['after']
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[];variants={}
        if args.mutations:
            kernel_name=INPUTS[0];frame_name=INPUTS[1];kernel=(ROOT/kernel_name).read_text();frame=(ROOT/frame_name).read_text()
            def change(text,old,new):
                if text.count(old)!=1:raise ValueError('Exact private copy/sentinel defect seam drift '+old)
                return text.replace(old,new,1)
            variants['omitted-vector-field']=(kernel_name,change(kernel,'{0x30u,0x40u,0x50u,0x60u}','{0x30u,0x40u,0x50u}'))
            first='storage.word(destination,0x20,storage.word(source,0x20),Phase::copy);'
            second='storage.word(destination,0x24,storage.word(source,0x24),Phase::copy);'
            variants['reordered-scalar-fields']=(kernel_name,change(kernel,first+'\n    '+second,second+'\n    '+first))
            variants['narrow-upper-bits-lost']=(frame_name,change(frame,'s_.gp[reg]=(s_.gp[reg]&0xffffff00u)|value;','s_.gp[reg]=value;'))
            variants['wrong-sentinel-identity']=(frame_name,change(frame,'s.gp[0]=native_tu15_invalid_contact_agent().identity;','s.gp[0]=native_tu15_invalid_contact_agent().identity+1u;'))
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(ROOT/'tests/native_tu15_contact_copy_leaf_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Copy/sentinel compile failed: '+done.stderr[-7000:])
            for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(name).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
            if done.returncode:raise ValueError('Copy/sentinel physical mismatch: '+(done.stdout+done.stderr)[-9000:])
            result=dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_defects=[])
            for label,(name,text)in variants.items():
                overlay=scratch/('mutation-'+label);target=overlay/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text)
                negative=scratch/(Path(compiler).name+'-'+label);mutated=list(command)
                mutated[1:1]=['-I'+str(overlay/'include'),'-I'+str(overlay/'src')]
                mutated[mutated.index('-o')+1]=str(negative);mutated[mutated.index('-MF')+1]=str(negative.with_suffix('.d'))
                compiled=subprocess.run(mutated,capture_output=True,text=True)
                if compiled.returncode:raise ValueError('Copy/sentinel semantic defect did not compile '+compiled.stderr[-7000:])
                rejected=subprocess.run([str(negative),str(fixture)],capture_output=True,text=True,timeout=120);diagnostic=rejected.stdout+rejected.stderr
                if rejected.returncode==0 or not any(message in diagnostic for message in('contact-copy leaf full ABI differs','contact-copy leaf pre-IO packet','contact-copy leaf ordered IO differs')):
                    raise ValueError('Copy/sentinel semantic defect not meaningfully rejected '+label+': '+diagnostic[-6000:])
                result['compiled_defects'].append(dict(name=label,source=name,sha256=hashlib.sha256(text.encode()).hexdigest(),command=mutated,exit_code=rejected.returncode,diagnostic=diagnostic[-1500:]))
            results.append(result)
        report.update(passed=True,physical_paths=len(rows),source_sites=sorted(hex(pc)for pc in covered),original=authorities,compilers=results,scope='Private actual TU15 whole childless record copy and invalid-contact sentinel, with ordinary atomic vector backing and readonly GP/XMM providers/Before. Complete CPU/FP/bank/pre-I/O proof; observer preimages are separately labelled retained transport. No source/default/public integration, other image or arbitrary provider-mutation claim.')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_before']=pins;report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
