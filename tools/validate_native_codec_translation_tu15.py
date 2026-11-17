#!/usr/bin/env python3
"""Private full2961F0 translation row against unchanged TU1.5 hardware."""
import argparse,copy,hashlib,json,os,random,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe
from native_mopp_ray_dense_original import physical_original_dense
from native_sphere_frustum_mmx_original import physical_original_mmx
from native_packed_rotation_source_inputs import UNITS,authenticate,function,digest
import native_codec_translation_replacements as binding
REGIONS=((0x510000,8192),(0x61000000,8192),(0x62000000,8192))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus(smoke):
    from validate_native_packed_rotation_tu15 import corpus as packed
    rows=packed(False);rng=random.Random(0x2961f0);result=[]
    for index in range(16 if smoke else 192):
        row=copy.deepcopy(rows[index]);row.update(entry=0x2961f0,name=f'codec-translation-{index}')
        banks={bank['address']:bytearray.fromhex(bank['before'])for bank in row['banks']};factory=0x61001000;source=0x61000100+index%4;sp=row['gp'][4]
        destination=(source,source+4,source-4,sp-12,0x510f98,factory+12,0x61000500)[index%7]
        frame_index=(0,1,0x7fffffff,0xffffffff)[index%4]
        def put(address,value):
            for base,bank in banks.items():
                if base<=address<=base+len(bank)-4:bank[address-base:address-base+4]=(value&0xffffffff).to_bytes(4,'little');return
            raise ValueError('Codec translation input escaped backing')
        put(0x510f98,frame_index);put(0x510f60,factory);put(factory+12,source-factory-frame_index*12);put(0x510fa0,destination-16);put(sp,0x63000000);put(sp+4,0);put(sp+8,0)
        for n in range(3):put(source+n*4,rng.getrandbits(32))
        for bank in row['banks']:bank['before']=banks[bank['address']].hex()
        result.append(row)
    return result
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--smoke',action='store_true');p.add_argument('--prefixes',action='store_true');p.add_argument('--mmx',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--xbe',type=Path,default=NEW_DEFAULT);a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    import platform
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    files=('include/halo2/native_tu15_codec_translation_kernel.hpp','src/physics/runtime/native_tu15_codec_translation_frame.hpp','tests/native_tu15_codec_translation_tests.cpp','tools/validate_native_codec_translation_tu15.py','tools/native_sphere_frustum_mmx_original.py','tools/native_mopp_ray_dense_original.py','tools/validate_native_tu15_prt_helpers.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
    pins={name:sha(ROOT/name)for name in files};headers={p.resolve():sha(p)for tree in('src','include','tests')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc','.cpp')};report=dict(passed=False,private_unbound=not a.require_binding,require_binding=a.require_binding,readonly_gp_provider_domain=True,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=NEW_PIN:raise ValueError('Original TU15 code image differs')
        owners=[]
        for name in UNITS:
            source=(ROOT/name).read_text();index,parts=authenticate(source);binding.authenticate(source);
            if a.require_binding and binding.installed_entries(source)!=['2961f0']:raise ValueError('Codec default source missing')
            body=binding.canonical_body(function(source,'run_2961f0'));owners.append(dict(source=name,body_sha256=digest(body),resolver_sha256=digest(parts['resolver']),dispatcher_sha256=digest(parts['dispatcher']),state_sha256=digest(parts['state'])))
        image=Xbe(a.xbe);import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=(0x2961f0,);original=common.metadata
        def metadata(source):
            result=original(source);code=common._Code(source,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic=='ret':site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;sites=metadata(image)['sites'];io=[site['instruction']for site in sites if site['operands']];rows=corpus(a.smoke);normal_count=len(rows)
        if a.mmx:
            rng=random.Random(0x2961f0a11)
            for mask in range(16 if a.smoke else 256):
                row=copy.deepcopy(rows[mask%normal_count]);row.update(id=len(rows),name=f'codec-translation-opaque-{mask}',live_x87=[]);row['before_fp'][1]&=~0x3800;row['active_mmx']=dict(known_mask=mask,physical=[(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);rows.append(row)
        proofs=[]
        def measure(values,label):
            path=scratch/label;path.mkdir();opaque='active_mmx'in values[0];runner=physical_original_mmx if opaque else physical_original_dense
            result,proof=runner(image,values,path,entries=(0x2961f0,),regions=REGIONS+(((0x64000000,4096),)if opaque else()),trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True);proofs.append(proof);return result['cases']
        actual=measure(rows[:normal_count],'normal')
        if len(rows)>normal_count:actual+=measure(rows[normal_count:],'opaque')
        if a.prefixes:
            pending=[]
            for index in list(range(min(8,normal_count)))+([normal_count+n for n in(0,1,7,15)if normal_count+n<len(rows)]if a.mmx else[]):
                for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal in range(len(actual[index][key])):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(pending),name=row['name']+f'-{phase}{ordinal}',phase=phase,fault=dict(phase=phase,ordinal=ordinal));pending.append(row)
            for opaque in(False,True):
                selected=[row for row in pending if('active_mmx'in row)==opaque]
                if selected:rows+=selected;actual+=measure(selected,'prefix-'+str(opaque))
        coverage=set()
        for row,observed in zip(rows,actual,strict=True):
            if observed['completed']!=('phase'not in row):raise ValueError('Original codec translation completion differs')
            coverage.update(observed['executed_sites']);pc=observed['boundary']['instruction']if'phase'in row else 0x29621e
            row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',[]))
            if'active_mmx'in observed:row['expected']['active_mmx']=observed['active_mmx']
            row.update(reads=observed['reads'],writes=observed['writes'],calls=observed['calls'])
            for call in row['calls']:
                if call.pop('arguments')!=[]:raise ValueError('Childless codec acquired call arguments')
            for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d');command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_codec_translation_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Codec translation compile failed '+done.stderr[-7000:])
            for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(name).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
            if done.returncode:raise ValueError('Codec translation differs '+(done.stdout+done.stderr)[-8000:])
            mutations=[]
            if a.semantic_mutants:
                frame='src/physics/runtime/native_tu15_codec_translation_frame.hpp';kernel='include/halo2/native_tu15_codec_translation_kernel.hpp'
                definitions=(
                    ('wrong-row-stride',frame,(('g[1]=g[2]+g[1]*4u;','g[1]=g[2]+g[1]*8u;'),)),
                    ('wrong-third-result-register',frame,(('s_.gp[component_==2u?1u:0u]=value;','s_.gp[0]=value;'),)),
                    ('missing-output-add-flags',frame,(('s_.gp[2]=s_.flags.plus(s_.gp[2],16u,4);','s_.gp[2]+=16u;'),)),
                    ('backwards-component-stream',kernel,(('native_tu15_copy_words(view,row,false);','native_tu15_copy_words(view,row,true);'),)),
                    ('premature-output-add',frame,(('return {g[1],g[2]+16u,3u,0u};','g[2]=s_.flags.plus(g[2],16u,4);return {g[1],g[2],3u,0u};'),('if(!component_)s_.gp[2]=s_.flags.plus(s_.gp[2],16u,4);',''))))
                for name,relative,replacements in definitions:
                    source=(ROOT/relative).read_text()
                    for old,new in replacements:
                        if source.count(old)!=1:raise ValueError('Missing codec translation mutation seam '+name)
                        source=source.replace(old,new,1)
                    overlay=scratch/(Path(compiler).name+'-'+name);target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(source);mutant=overlay/'test';mutated=list(command);mutated[1:1]=['-I'+str(overlay/'src'),'-I'+str(overlay/'include')];mutated[mutated.index('-o')+1]=str(mutant)
                    checked=subprocess.run(mutated,capture_output=True,text=True)
                    if checked.returncode:raise ValueError('Codec translation mutant compile failed '+checked.stderr[-5000:])
                    checked=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True);diagnostic=(checked.stdout+checked.stderr)[-4000:]
                    if not checked.returncode or 'codec translation'not in diagnostic:raise ValueError('Codec translation mutant survived/unrecognized '+name+': '+diagnostic)
                    mutations.append(dict(name=name,rejected=True,diagnostic=diagnostic,exit_code=checked.returncode))
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_mutations=mutations))
        report.update(passed=True,compilers=results,physical_paths=len(rows),source_sites=sorted(hex(pc)for pc in coverage),source_owners=owners,original=proofs)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
