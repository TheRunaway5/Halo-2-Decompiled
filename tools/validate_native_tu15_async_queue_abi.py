#!/usr/bin/env python3
"""Unchanged-i386 complete and atomic-provider prefix proof for typed queues."""
import argparse,copy,hashlib,json,os,platform,subprocess,sys
from pathlib import Path
from native_async_queue_data_inputs import corpus
from validate_native_tu15_prt_helpers import PIN,physical_original
ROOT=Path(__file__).resolve().parents[1]
FILES=('include/halo2/native_tu15_async_queue_kernel.hpp','src/platform/native_tu15_async_queue_abi.hpp',
    'src/physics/runtime/native_tu15_raw_x87_scope.hpp','tests/native_tu15_async_queue_abi_tests.cpp',
    'tools/native_async_queue_data_inputs.py','tools/validate_native_tu15_async_queue_abi.py',
    'tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp',
    'tools/native_async_queue_data_replacements.py','tests/reference/native_tu15_async_order_reference.hpp',
    'tests/reference/native_tu15_async_queue_value_reference.hpp','include/halo2/source_async_jobs.hpp')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def expected_abi(row,original):
    return dict(gp=original['boundary_gp'],eflags=original['boundary_eflags'],xmm=original['boundary_xmm'],fp=original['after_fp'],live_x87=row['live_x87'])
def expected_calls(row,original):
    if row['entry']!=0x1232c0:return []
    return [dict(gp=c['gp'],eflags=c['eflags'],xmm=c['xmm'],fp=c['fp'],live_x87=c['live_x87'],arguments=c['arguments'])for c in original['calls']]
def compare(row,original,native):
    name=row['name'];phase=row.get('fault',{}).get('phase');stopped=bool(phase)
    if native['stopped']!=stopped or native['literal_callbacks']:raise ValueError(name+': native route/boundary differs')
    if native['abi']!=expected_abi(row,original):raise ValueError(name+': full physical ABI differs '+json.dumps(dict(native=native['abi'],original=expected_abi(row,original))))
    if native['banks']!=[dict(address=b['address'],after=b['after'])for b in original['banks']]:raise ValueError(name+': physical complete banks differ')
    stores=[[e['address'],e['before'],e['written']]for e in original['writes']]
    if native['writes']!=stores:raise ValueError(name+': physical ordered stores differ')
    reads=[[e['address'],e['bytes']]for e in original['reads']]
    if phase=='after_write':
        # Ordinary observer preimages are extra policy reads. In these two
        # owners every store is MOV/PUSH reg/immediate, not a memory RMW.
        # Remove only the immediate same-PC/extent read preceding its write.
        accesses=native['accesses'];omitted=set()
        for i,event in enumerate(accesses):
            if event[0]=='write':
                if i==0 or accesses[i-1]!=['read',event[1],event[2],event[3]]:raise ValueError('Missing explicit observer preimage')
                omitted.add(i-1)
        actual=[[e[1],e[2]]for i,e in enumerate(accesses)if e[0]=='read'and i not in omitted]
    else:actual=native['reads']
    if actual!=reads:
        first=next((i for i,(a,b)in enumerate(zip(actual,reads))if a!=b),min(len(actual),len(reads)))
        raise ValueError(name+f': physical ordered reads differ at{first} native={actual[first:first+2]} original={reads[first:first+2]}')
    if native['calls']!=expected_calls(row,original):raise ValueError(name+': genuine comparator complete entry packets differ')
    expected_pc=original['boundary']['instruction']if stopped else original['reads'][-1]['instruction']
    if native['pc']!=expected_pc:raise ValueError(name+': source cursor differs')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--leak-sanitizer',action='store_true');p.add_argument('--mutations',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686'):print('SKIP physical async ABI needs Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing original TU1.5');return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original executable')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:sha(ROOT/name)for name in FILES};report=dict(passed=False,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        from oracle_smg_firing_1_1 import Xbe
        image=Xbe(args.xbe);rows=[];recipes=[]
        if args.require_binding:
            from native_async_queue_data_replacements import replace_abi,replace_public
            for path,replace in(('src/platform/native_tu15_async_order.hpp',replace_abi),('src/platform/source_async_jobs.cpp',replace_public)):
                text=(ROOT/path).read_text()
                if replace(text)!=text:raise ValueError('Qualified async default delegation not installed '+path)
                pins[path]=sha(ROOT/path)
        for kind in('order','insert'):
            directory=scratch/kind;directory.mkdir();values,proof=corpus(kind,directory);recipes.append(proof)
            for row in values:row.update(id=len(rows),hostile_fp=15 if len(rows)%2 else 0);rows.append(row)
        report['input_recipes']=recipes
        binary=scratch/'async-queue-abi';command=[args.compiler,'-std=c++20','-O1'if args.sanitizers else'-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_async_queue_abi_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        if args.require_binding:command[1:1]=['-DHALO2_NATIVE_ASYNC_BOUND_ROUTE=1']
        subprocess.run(command,check=True);report['compile_command']=command
        env=dict(os.environ);env.pop('HALO2_REFERENCE_ASYNC_QUEUE',None)
        if args.sanitizers:env['ASAN_OPTIONS']=env.get('ASAN_OPTIONS','')+(':detect_leaks=1'if args.leak_sanitizer else':detect_leaks=0')
        guards=subprocess.run([str(binary),'--guards'],capture_output=True,text=True,env=env)
        if guards.returncode:raise ValueError('Async live admission guards failed '+guards.stderr[-2000:])
        report['admission_guards']=json.loads(guards.stdout)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        old_metadata=common.metadata
        def metadata(image):
            data=old_metadata(image);code=common._Code(image,'.text')
            for site in data['sites']:
                instruction=code.at(site['instruction'])
                if instruction.mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if instruction.mnemonic=='test':
                    for operand in site['operands']:operand.update(read=True,write=False)
            return data
        common.metadata=metadata
        regions=tuple(sorted({(b['address'],len(b['before'])//2)for r in rows for b in r['banks']}))
        def original(cases,name):
            directory=scratch/name;directory.mkdir();return physical_original(image,cases,directory,entries=(0x1ab330,0x1232c0),regions=regions,
                trace_calls=[dict(entry=0x1ab330,arguments=3,gp=True,xmm=True)],trace_extended=True,preserve_x87_prefix=True)
        def native(cases,name):
            fixture=scratch/(name+'.json');fixture.write_text(json.dumps(dict(cases=cases),separators=(',',':')))
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=env,timeout=90)
            if result.returncode:raise ValueError('Typed async probe failed '+result.stderr[-4000:])
            return json.loads(result.stdout)
        full,full_proof=original(rows,'normal');actual=native(rows,'normal-native')
        for row,cpu,value in zip(rows,full['cases'],actual,strict=True):compare(row,cpu,value)
        report['normal_cases']=len(rows);report['normal_original']=full_proof
        report['genuine_comparator_entry_packets']=sum(len(case['calls'])for row,case in zip(rows,full['cases'])if row['entry']==0x1232c0)
        prefixes=[]
        for entry in(0x1ab330,0x1232c0):
            candidates=[i for i,row in enumerate(rows)if row['entry']==entry]
            selected=sorted(candidates,key=lambda i:len(full['cases'][i]['reads']),reverse=True)[:2]
            for index in selected:
                for phase in('before_read','before_write','after_write'):
                    events=full['cases'][index]['reads'if phase=='before_read'else'writes']
                    for ordinal in range(len(events)):
                        row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefixes),name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal),hostile_fp=15);prefixes.append(row)
        partial,partial_proof=original(prefixes,'prefixes');actual=native(prefixes,'prefix-native')
        for row,cpu,value in zip(prefixes,partial['cases'],actual,strict=True):compare(row,cpu,value)
        if args.mutations:
            text=(ROOT/'include/halo2/native_tu15_async_queue_kernel.hpp').read_text();rejections=[]
            for label,old,new in(('file-order','d.incoming_file_distance<d.other_file_distance','d.incoming_file_distance>d.other_file_distance'),
                ('queue-placement','if(resumed.before){','if(!resumed.before){')):
                if text.count(old)!=1:raise ValueError('Missing complete DATA mutation witness')
                directory=scratch/('mutant-'+label);(directory/'halo2').mkdir(parents=True)
                (directory/'halo2/native_tu15_async_queue_kernel.hpp').write_text(text.replace(old,new,1));mutant=directory/'probe'
                build=list(command);build.insert(build.index('-I'+str(ROOT/'include')),'-I'+str(directory));build[-1]=str(mutant)
                subprocess.run(build,check=True);execution=subprocess.run([str(mutant),str(scratch/'normal-native.json')],capture_output=True,text=True,env=env,timeout=90)
                if execution.returncode:reason='Altered DATA policy failed on declared valid inputs'
                else:
                    values=json.loads(execution.stdout);failed=False
                    for row,cpu,value in zip(rows,full['cases'],values,strict=True):
                        try:compare(row,cpu,value)
                        except ValueError:failed=True;break
                    if not failed:raise ValueError('Meaningful DATA policy mutation escaped the original oracle '+label)
                    reason='Altered DATA policy disagreed with unchanged original CPU'
                rejections.append(dict(mutation=label,compiled=True,rejected=True,reason=reason))
            report['meaningful_data_mutation_rejections']=rejections
        report.update(passed=True,prefix_cases=len(prefixes),prefix_original=partial_proof,atomic_provider_before_read_write_and_completed_observer_prefixes=True,
            full_gp_flags_xmm_mxcsr_x87_controls_status_live80_banks_and_ordered_accesses_exact=True,
            live_x87_depths=[0,1,4,8],all_top_positions=True,hostile_memory_and_observer_fp=True,
            hostile_before_hook_fp=not args.require_binding,
            physical_original_instructions_patched=False,supplied_original_outcomes=False,production_default_binding_qualified=args.require_binding,
            actual_page_fault_atomicity_qualified=False,observer_preimage_faults_qualified=False,legacy_map_pack_qualified=False,
            leak_detection_qualified=args.sanitizers and args.leak_sanitizer)
        if any(sha(ROOT/name)!=digest for name,digest in pins.items()):raise ValueError('Typed async ABI sources changed')
    except BaseException as error:report['passed']=False;report['error']=repr(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({k:report[k]for k in('passed','normal_cases','prefix_cases')}));return 0
if __name__=='__main__':raise SystemExit(main())
