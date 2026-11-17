#!/usr/bin/env python3
"""Qualify the private effect-reference/accessor closure against unchanged i386."""
import argparse,copy,hashlib,json,os,random,subprocess
import shlex
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from validate_native_tu15_prt_helpers import PIN
from native_bsp_admission_original import observe as physical_original

OWNERS=(0x178070,0x13a6b0,0x22b790,0x22aca0,0x22b710)
REGIONS=((0x45f000,4096),(0x486000,4096),(0x4f0000,4096),
         (0x60f81000,4096),(0x61000000,65536),(0x61080000,8192),
         (0x61100000,8192),(0x62000000,32768))
FILES=('include/halo2/native_tu15_effect_reference_element_kernel.hpp',
       'src/rendering/effects/native_tu15_effect_reference_element_frame.hpp',
       'tests/native_tu15_effect_reference_element_tests.cpp',
       'tools/validate_native_effect_reference_element_tu15.py',
       'tests/native_tu15_mopp_ray_frame_test_memory.hpp',
       'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
       'src/physics/runtime/native_tu15_host_fp_io.hpp',
       'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
       'src/physics/runtime/physics_world_runtime_internal.hpp',
       'tools/native_mopp_ray_dense_original.py',
       'tools/native_bsp_admission_original.py',
       'tools/native_geometry_visibility_original.py',
       'tools/validate_native_tu15_prt_helpers.py',
       'tools/observe_native_campaign_ai_vector_math_tu15.cpp')
FILES+=('tools/native_effect_reference_element_mutations.py',)
FILES+=('tools/native_effect_reference_element_alias_inputs.py',)
SOURCE_FILES=('tools/native_effect_reference_element_inputs.py','tools/native_effect_reference_element_replacements.py',
 'src/rendering/effects/native_tu15_effect_reference_element_admission.hpp',
 'tests/native_tu15_effect_reference_element_source_tests.cpp','tools/audit_native_effect_reference_generation_tu15.py')
SOURCE_FILES+=('tools/specialize_native_operands.py','tools/native_source_prefixes.py','tools/native_selected_operand_binding.py','tools/native_model_marker_lookup_replacements.py',
 'tools/generate_physics_live_support_tu15.py','tools/generate_combat_action_tu15.py','tools/generate_death_cycle_runtime_tu15.py','tools/generate_effect_particle_frame_tu15.py')
SOURCE_FILES+=('tools/native_effect_reference_element_source_mutations.py',)
def hashes():return {p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest()for p in FILES}
def corpus(image,smoke):
    rng=random.Random(0x178070);rows=[];table=0x61001000
    live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f',
          '01000000000000000000','00000000000000800040','00000000000000000000',
          '0000000000000080ffbf','00000000000000000080')
    for n in range(30 if smoke else 240):
        banks={base:bytearray(rng.randbytes(size))for base,size in REGIONS}
        for base in(0x45f000,0x486000):banks[base][:]=image.read(base,4096)
        def put(address,value,width=4):
            for base,bank in banks.items():
                if base<=address<=base+len(bank)-width:
                    bank[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError(f'Effect input escaped bank: {address:x}')
        def tag(index,group,definition):
            signed=index if index<32768 else index-65536
            put(table+signed*16,group);put(table+index*16+8,definition)
        sp=0x62006000+n%16;put(sp,0x63000000);put(0x4f0414,table)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
        route=n%10;entry=OWNERS[0];name=''
        if route<6:
            # Root classes include both particle types and genuinely recursive
            # parent chains. Each returned parent's +4 tag selects the next
            # real singleton/vtable method; no child outcome is supplied.
            depth=(0,0,0,1,2,3)[route]
            groups=([0x62736474],[0x65666665],[rng.choice((0,0x5052544c,0x62736475,0xffffffff))],
                    [0x70727433,0x62736474],[0x5052544d,0x70727433,0x65666665],
                    [0x70727433,0x5052544d,0x70727433,0x62736474])[route]
            handles=[];results=[]
            for j,group in enumerate(groups):
                index=(0xffff,0x8000,0x7fff,7)[(n//10)%4]if j==0 else j
                handle=(rng.getrandbits(16)<<16)|index;handles.append(handle)
                ref=0x61000100+j*0x80;definition=0x61002000+j*0x200;elements=0x61005000+j*0x400
                tag(index,group,definition);element=(n//6+j)%4;event=(0,1,0xffffffff,0x80000000)[n%4]
                put(ref+0x10,handle);put(ref+14,element,2);put(ref+0x14,event);put(ref+0x48,ref+0x80)
                if group==0x62736474:put(definition+0x18,elements)
                elif group==0x65666665:
                    events=0x61004080+j*0x100;put(definition+0x18,events)
                    put((events+(event*56&0xffffffff)+0x34)&0xffffffff,elements)
                else:put(definition+(0x7c if group==0x5052544d else 0x88),elements)
                results.append(elements+element*56)
            for j in range(1,len(groups)):put(results[j]+4,handles[j-1])
            gp[1]=0x61000100;name=f'resolve-route{route}-depth{depth}'
        elif route==6:
            entry=0x13a6b0;index=(0xffff,0x8000,0x7fff,7)[(n//10)%4]
            group=(0x5052544d,0x70727433,0xffffffff)[(n//10)%3]
            tag(index,group,0x61002000);gp[2]=(rng.getrandbits(16)<<16)|index;name='accessor'
        elif route==7:
            entry=0x22b790;index=(0xffff,0x8000,0x7fff,7)[(n//10)%4]
            tag(index,0x70727433,rng.getrandbits(32));gp[1]=0x61000200;name='bind'
            if not smoke:
                alias=(n//10)%4
                if alias==1:gp[1]=table+index*16+4;name+='-row-definition-alias'
                elif alias==2:gp[1]=sp;name+='-argument-alias'
                elif alias==3:
                    gp[1]=sp-8;tag(index,0x70727433,0x63000000);name+='-return-alias'
            put(sp+4,(rng.getrandbits(16)<<16)|index)
        else:
            entry=0x22aca0 if route==8 else 0x22b710;gp[1]=0x61000200
            put(gp[1]+8,0x61002000);put(0x61002000+(0x88 if route==8 else 0x7c),rng.getrandbits(32))
            put(sp+4,(0,1,0xffffffff,0x80000000,0x7fffffff,0x49249249)[(n//10)%6]);name='element'
        depth=(0,1,4,8)[(n//3)%4];cw=(0x7f,0x27f,0x37f)[n%3]|(((n//3)%4)<<10)
        stack=banks[0x62000000];stack_offset=sp-0x62000000
        arguments=[int.from_bytes(stack[stack_offset+i:stack_offset+i+4],'little')for i in(4,8)]
        rows.append(dict(id=n,name=f'effect_{n}_{name}',entry=entry,phase='normal',gp=gp,
            eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
            before_fp=[cw,((n%8)<<11)|(n*0x101&0x473f),0x1f80|((n%4)<<13)|(n%64)],live_x87=list(live[:depth]),
            matrix=arguments[0],source=arguments[1],max_steps=1000,stop_before_entries=[0x63000000],
            banks=[dict(address=base,before=bank.hex())for base,bank in banks.items()]))
    if not smoke:
        from native_effect_reference_element_alias_inputs import append
        rows=append(rows)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',action='append')
    p.add_argument('--sanitizers',action='store_true');p.add_argument('--smoke',action='store_true')
    p.add_argument('--prefixes',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--source-copies',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--source-mutants',action='store_true');a=p.parse_args()
    if(a.require_binding or a.source_mutants)and not a.source_copies:p.error('Source binding/defects require --source-copies')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding_required=a.require_binding,production_runtime_qualified=False,owned_scratch=str(scratch),source_before=hashes(),retained_large_artifacts=[])
    headers={p.resolve():hashlib.sha256(p.read_bytes()).hexdigest()for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.h','.hpp','.inc')}
    try:
        if a.source_copies:
            from native_effect_reference_element_inputs import prepare
            for name in SOURCE_FILES:report['source_before'][name]=hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
            report['source_ownership']=prepare(ROOT,scratch,a.require_binding)
            for name in report['source_ownership']['source_files']:report['source_before'][name]=hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
        assert hashlib.sha256(a.xbe.read_bytes()).hexdigest()==PIN;image=Xbe(a.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=OWNERS;code=common._Code(image,'.text');original=common.metadata
        def metadata(image):
            result=original(image)
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;meta=metadata(image);io=[s['instruction']for s in meta['sites']if s['operands']or code.at(s['instruction']).mnemonic in('push','pop','ret','call')]
        traces=[dict(entry=pc,gp=True,xmm=True)for pc in io];rows=corpus(image,a.smoke)
        actual,authority=physical_original(image,rows,scratch,entries=OWNERS,regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        normal_count=len(rows);prefix_proof=None
        if a.prefixes:
            prefixes=[]
            for row,observed in zip(rows[:10],actual['cases'][:10],strict=True):
                occurrences={}
                for packet in observed['calls']:
                    pc=packet['entry'];occurrence=occurrences.get(pc,0);occurrences[pc]=occurrence+1
                    if code.at(pc).mnemonic!='call':continue
                    clone=copy.deepcopy(row);clone.update(id=normal_count+len(prefixes),name=row['name']+f'_call{pc:x}_{occurrence}',phase='before_instruction',
                        native_fault_instruction=pc,native_fault_ordinal=occurrence,fault=dict(phase='before_instruction',instruction=pc,ordinal=occurrence));prefixes.append(clone)
                for ordinal,read in enumerate(observed['reads']):
                    # A CALL's target read and return-address store form one
                    # original instruction. Stop before it rather than claim
                    # a separately retired target-read boundary.
                    if code.at(read['instruction']).mnemonic=='call':continue
                    clone=copy.deepcopy(row);clone.update(id=normal_count+len(prefixes),name=row['name']+f'_read{ordinal}',phase='before_read',native_fault_ordinal=ordinal,fault=dict(phase='before_read',ordinal=ordinal));prefixes.append(clone)
                for ordinal,write in enumerate(observed['writes']):
                    if code.at(write['instruction']).mnemonic=='call':continue
                    for phase in('before_write','after_write'):
                        clone=copy.deepcopy(row);clone.update(id=normal_count+len(prefixes),name=row['name']+f'_{phase}{ordinal}',phase=phase,native_fault_ordinal=ordinal,fault=dict(phase=phase,ordinal=ordinal));prefixes.append(clone)
            folder=scratch/'prefixes';folder.mkdir()
            observed,prefix_proof=physical_original(image,prefixes,folder,entries=OWNERS,regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
            rows+=prefixes;actual['cases']+=observed['cases']
        executed=set()
        for row,observed in zip(rows,actual['cases'],strict=True):
            assert observed['completed']==(row['phase']=='normal');executed.update(observed['executed_sites'])
            row.update({k:observed[k]for k in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','reads','writes','calls')})
            row['expected_pc']=observed['executed_sites'][-1]if row['phase']=='normal'else observed['boundary']['instruction']
            for packet in row['calls']:assert packet.pop('arguments')==[]
            for bank,after in zip(row['banks'],observed['banks'],strict=True):assert bank['address']==after['address'];bank['after']=after['after']
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows),separators=(',',':')))
        outcomes=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=scratch/(Path(compiler).name+'.d')
            cmd=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_effect_reference_element_tests.cpp'),'-MMD','-MF',str(dep),'-o',str(binary)]
            if a.sanitizers:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            result=subprocess.run(cmd,capture_output=True,text=True);assert result.returncode==0,result.stderr[-7000:]
            for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(name).resolve()
                if path in headers:report['source_before'][str(path.relative_to(ROOT))]=headers[path]
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);assert result.returncode==0,(result.stdout+result.stderr)[-10000:]
            mutations=None
            if a.semantic_mutants:
                from native_effect_reference_element_mutations import qualify
                mutations=qualify(scratch,rows,io,compiler,cmd)
            source_results=[]
            if a.source_copies:
                source_binary=scratch/(Path(compiler).name+'-sources');source_dep=scratch/(Path(compiler).name+'-sources.d')
                source_cmd=list(cmd);source_cmd[source_cmd.index(str(ROOT/'tests/native_tu15_effect_reference_element_tests.cpp'))]=str(ROOT/'tests/native_tu15_effect_reference_element_source_tests.cpp')
                source_cmd[source_cmd.index('-o')+1]=str(source_binary);source_cmd[source_cmd.index('-MF')+1]=str(source_dep);source_cmd[1:1]=['-I'+str(scratch),'-I'+str(ROOT/'src/gameplay/weapons')]
                report['active_source_command']=source_cmd
                checked=subprocess.run(source_cmd,capture_output=True,text=True);assert checked.returncode==0,checked.stderr[-10000:]
                for name in shlex.split(source_dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                    path=Path(name).resolve()
                    if path in headers:report['source_before'][str(path.relative_to(ROOT))]=headers[path]
                for mode,args,env in(('native',[str(fixture)],{}),('guards',['--guards'],{}),
                    ('empty',['--empty-services',str(fixture)],{}),('reference',[str(fixture)],{'HALO2_REFERENCE_EFFECT_REFERENCE_ELEMENT':'1'})):
                    checked=subprocess.run([str(source_binary),*args],capture_output=True,text=True,env=dict(os.environ,**env))
                    assert checked.returncode==0,(mode+' '+checked.stdout+checked.stderr)[-10000:]
                    source_results.append(dict(mode=mode,result=json.loads(checked.stdout)))
                policy=[]
                for env in({}, {'HALO2_REFERENCE_EFFECT_REFERENCE_ELEMENT':'1'}):
                    checked=subprocess.run([str(source_binary),'--provider-policy-mutations',str(fixture)],capture_output=True,text=True,env=dict(os.environ,**env))
                    assert checked.returncode==0,('provider-policy '+checked.stdout+checked.stderr)[-7000:]
                    policy.append(json.loads(checked.stdout))
                if policy[0]!=policy[1]:
                    differences=[(x['route'],x['mutation'],[key for key in x if x[key]!=y[key]])for x,y in zip(policy[0]['results'],policy[1]['results'],strict=True)if x!=y]
                    raise ValueError('Owned-provider whole-entry fallback differs from positive retained source '+repr(differences[:12]))
                source_results.append(dict(mode='provider_policy_mutations',rows=policy[0]['provider_policy_rows'],
                    full_abi_banks_ordered_io_callbacks_exceptions_and_post_child_packet_parity=True,
                    prefix_descendant_legacy_flag_parity=False,
                    scope='Five actual returned policy mutations in ten externally calling contexts; pre-mutation native descendants have a separate unchanged-CPU oracle',
                    comparison_sha256=hashlib.sha256(json.dumps(policy[0],sort_keys=True,separators=(',',':')).encode()).hexdigest()))
                if a.source_mutants:
                    from native_effect_reference_element_source_mutations import qualify as qualify_source
                    source_results.append(dict(mode='compiled_source_mutations',result=qualify_source(scratch,rows,io,compiler,source_cmd)))
            outcomes.append(dict(compiler=compiler,command=cmd,result=json.loads(result.stdout),compiled_mutations=mutations,actual_source_results=source_results))
        expected=set().union(*(set(common.complete_body(code,entry).code)for entry in OWNERS))
        if not a.smoke:assert len(expected)==95 and expected<=executed,sorted(expected-executed)
        report.update(passed=True,original_paths=len(rows),normal_paths=normal_count,prefix_paths=len(rows)-normal_count,authority=authority,prefix_authority=prefix_proof,owner_sites=len(expected),executed_sites=sorted(executed),compilers=outcomes)
    except BaseException as error:report.update(error=repr(error));raise
    finally:
        report['source_after']={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
