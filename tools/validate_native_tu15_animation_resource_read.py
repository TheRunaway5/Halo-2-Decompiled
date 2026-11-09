#!/usr/bin/env python3
"""Check1260C0 CPU framing at the real animation I/O owner boundary."""
import argparse,copy,hashlib,json,os,re,subprocess
from pathlib import Path
import native_animation_resource_read_replacements as binding
import validate_native_tu15_resource_page as page
import validate_native_tu15_resource_maintenance as maintenance
from specialize_native_operands import leaf_body
from validate_native_tu15_prt_helpers import PIN,physical_original
ROOT=Path(__file__).resolve().parents[1]
FILES=('include/halo2/native_tu15_animation_resource_read_kernel.hpp','src/physics/runtime/native_tu15_animation_resource_read.hpp','tools/native_animation_resource_read_replacements.py','tools/validate_native_tu15_animation_resource_read.py','tests/native_tu15_animation_resource_read_tests.cpp','include/halo2/source_animation_cache_io_tu15.hpp','src/physics/runtime/native_tu15_halfword_bit_query.hpp','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp','tools/validate_native_tu15_resource_page.py','tools/validate_native_tu15_resource_maintenance.py','src/assets/cache.cpp')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def put(row,address,value):
    for bank in row['banks']:
        base=bank['address'];raw=bytearray.fromhex(bank['before'])
        if base<=address<=base+len(raw)-4:
            raw[address-base:address-base+4]=(value&0xffffffff).to_bytes(4,'little');bank['before']=raw.hex();return
    raise ValueError('Animation input escaped declared backing')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file():return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=args.require_binding,scratch_directory=str(scratch),retained_large_artifacts=[])
    pins={name:sha(ROOT/name)for name in FILES}
    try:
        from oracle_smg_firing_1_1 import Xbe
        image=Xbe(args.xbe);source=(ROOT/maintenance.SOURCE).read_text();candidate=binding.replace_animation_resource_read(source)
        if binding.replace_animation_resource_read(candidate)!=candidate:raise ValueError('Animation binder not idempotent')
        original=binding.canonical_body(leaf_body(source,'1260c0')[0]);native=leaf_body(candidate,'1260c0')[0]
        if args.require_binding and source!=candidate:raise ValueError('Production animation controller prefix absent')
        rejected=0
        for variant in(source,candidate):
            body=leaf_body(variant,'1260c0')[0]
            for old,new in(('0x474d70u','0x474d74u'),('0x126139u','0x126138u'),('run_1262b0(s);','run_1236a0(s);')):
                changed=variant.replace(body,body.replace(old,new,1),1)
                if changed==variant:raise ValueError('Animation body mutation witness absent')
                try:binding.replace_animation_resource_read(changed)
                except ValueError:rejected+=1
                else:raise ValueError('Animation binder accepted complete body drift')
        for old,new in((binding.PREFIX,binding.PREFIX.replace('dispatch(v,target,site,tail);','dispatch(v,target,site+1u,tail);')), (binding.ORDERED_SHIFT,binding.ORDERED_SHIFT.replace('s.reg(6,4,0)','0u'))):
            changed=candidate.replace(old,new,1)
            try:binding.replace_animation_resource_read(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Animation binder accepted prefix/primitive drift')
        dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        for old,new in(('run_1260c0(s);','run_126200(s);'),('s.required(site,target,tail);','s.required(site+1u,target,tail);')):
            changed=source.replace(dispatcher,dispatcher.replace(old,new,1),1)
            if changed==source:raise ValueError('Animation dispatcher mutation absent')
            try:binding.replace_animation_resource_read(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Animation binder accepted dispatcher drift')

        from native_animation_resource_wait_replacements import canonical_body as wait_body,PINS as WAIT_PINS
        wait=wait_body(leaf_body(source,'1262b0')[0])
        if hashlib.sha256(wait.encode()).hexdigest()!=WAIT_PINS[0]:raise ValueError('Complete original animation-wait child drift')
        start=wait.index('{')+1;wait=wait[:start]+'\n    before_owner(s,0x1262b0u);'+wait[start:]
        body='namespace retained {\nstatic void run_1236a0(WorldState&s){before_owner(s,0x1236a0u);throw std::runtime_error("Real promotion owner remains unresolved");}\nstatic void run_2dbf74(WorldState&s){before_owner(s,0x2dbf74u);throw std::runtime_error("Real yield owner remains unresolved");}\n'+wait+'\nstatic void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){if(target==0x1262b0u){run_1262b0(s);return;}before_owner(s,target);if(target==0x1a3b70u){io_owner(s);return;}throw std::runtime_error("Unknown real I/O owner");}\n'+native+'\n}'
        (scratch/'animation_resource_read_bodies.inc').write_text(body)
        binary=scratch/'animation-read';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_animation_resource_read_tests.cpp'),str(ROOT/'src/assets/cache.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise ValueError('Animation framing compile failed '+result.stderr[-6000:])
        # Starting controller inputs are the actual states/banks produced by
        # original page/datum allocation, not fabricated allocation results.
        prep=scratch/'preallocation';prep.mkdir();initial,_,_,input_proof,_=page.prepare(image,prep,False)
        initial=[row for row in initial if row['name'].endswith('-1000')and'-alias-'not in row['name']]
        sizes=(1,511,512,513,4095,4096,4097,8191,8192,8193,127,1023,1024,1025,255,256)
        for index,row in enumerate(initial):row.update(id=index);put(row,0x61006004,sizes[index])
        regions=tuple(sorted({(b['address'],len(b['before'])//2)for row in initial for b in row['banks']}))
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        raw_metadata=common.metadata
        def metadata(image):
            data=raw_metadata(image);code=common._Code(image,'.text')
            for site in data['sites']:
                if code.at(site['instruction']).mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                expected={0x16e6c5:'852c8b',0x1401a2:'851c88',0x32a672:'8500',0x32a689:'8501',0x32a696:'8501'}.get(site['instruction'])
                if expected:
                    if site['bytes']!=expected:raise ValueError('Original TEST metadata changed')
                    for operand in site['operands']:operand.update(read=True,write=False)
                if site['instruction']==0x140071:
                    if site['bytes']!='0f184c0464':raise ValueError('Original prefetch changed')
                    for operand in site['operands']:operand.update(read=False,write=False)
            return data
        common.metadata=metadata
        allocated,allocation_proof=physical_original(image,initial,prep,entries=maintenance.PAGE_ENTRIES+(0x16e8a0,),regions=regions,preserve_x87_prefix=True)
        rows=[]
        for index,(row,case)in enumerate(zip(initial,allocated['cases'],strict=True)):
            value=copy.deepcopy(row);value.update(id=index,entry=0x1260c0,name='animation-'+row['name'],gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],before_fp=case['after_fp'],stop_before_entries=[0x1a3b70],max_steps=10000)
            if value['gp'][0]==0:raise ValueError('Real preallocation failed')
            for bank,after in zip(value['banks'],case['banks'],strict=True):bank['before']=after['after']
            value['gp'][4]=0x62006000+index%16;value['matrix']=0x7000180;value['source']=index%2
            put(value,value['gp'][4],0x63000000);put(value,value['gp'][4]+4,0x7000180);put(value,value['gp'][4]+8,index%2)
            put(value,0x61006000,0x12345678);put(value,0x61006008,0x6100a000);put(value,0x4f0420,0x61008000)
            rows.append(value)
        for source_row in rows[:4]:
            value=copy.deepcopy(source_row);value.update(id=len(rows),name=source_row['name']+'-sector-saved-frame-alias')
            size=int.from_bytes(next(bytes.fromhex(bank['before'])[0x61006004-bank['address']:0x61006008-bank['address']]for bank in value['banks']if bank['address']<=0x61006004<bank['address']+len(bank['before'])//2),'little')
            put(value,0x4f0420,value['gp'][4]-((size+511)&~511));rows.append(value)
        for source_row in rows[:2]+rows[-2:]:
            value=copy.deepcopy(source_row);value.update(id=len(rows),name=source_row['name']+'-df1-existing-pages',eflags=source_row['eflags']|0x400);rows.append(value)
        directory=scratch/'framing';directory.mkdir();physical,proof=physical_original(image,rows,directory,entries=(0x1260c0,),regions=regions,preserve_x87_prefix=True)
        prefixes=[]
        for row,case in zip(rows[:4],physical['cases'][:4],strict=True):
            for ordinal,event in enumerate(case['reads']):
                value=copy.deepcopy(row);value.update(id=len(rows)+len(prefixes),name=row['name']+'-before-read-'+str(ordinal),fault=dict(phase='before_read',ordinal=ordinal));prefixes.append(value)
        directory=scratch/'prefix';directory.mkdir();prefix_physical,prefix_proof=physical_original(image,prefixes,directory,entries=(0x1260c0,),regions=regions,preserve_x87_prefix=True)
        cases=[]
        for row,case in zip(rows+prefixes,physical['cases']+prefix_physical['cases'],strict=True):
            value=copy.deepcopy(row);value.update({key:copy.deepcopy(case[key])for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes','calls')if key in case});value['calls']=[]
            value['read_instructions']=[e['instruction']for e in case['reads']];value['stop_kind']='before_read'if'fault'in row else'child';value['expected_pc']=case['boundary']['instruction']if case['boundary']else case['stopped_at']
            for bank,after in zip(value['banks'],case['banks'],strict=True):bank['after']=after['after']
            for event in value['reads']+value['writes']:event.pop('instruction')
            cases.append(value)
        def bank_bytes(banks,address,size):
            return next(bytes.fromhex(bank['before'])[address-bank['address']:address-bank['address']+size]for bank in banks if bank['address']<=address<=bank['address']+len(bank['before'])//2-size)
        service_rows=[]
        for row,case in zip(rows,physical['cases'],strict=True):
            image_bytes=bank_bytes(row['banks'],0x61006000,20);extent=int.from_bytes(image_bytes[4:8],'little');encoded=int.from_bytes(image_bytes[8:12],'little');rounded=(extent+511)&~511
            row['service']=dict(address=0x61006000,descriptor=image_bytes.hex(),bytes=extent,encoded_offset=encoded,sectors=bank_bytes(row['banks'],encoded,rounded).hex())
            value=copy.deepcopy(row);value.update(gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],before_fp=case['after_fp'])
            for bank,after in zip(value['banks'],case['banks'],strict=True):bank['before']=after['after']
            service_rows.append(value)
        service_fixture=scratch/'service.json';service_fixture.write_text(json.dumps(dict(cases=service_rows),separators=(',',':')))
        result=subprocess.run([str(binary),'--service',str(service_fixture)],capture_output=True,text=True)
        if result.returncode:raise ValueError('Actual backed animation policy failed '+result.stderr[-6000:])
        serviced=json.loads(result.stdout);post_rows=[]
        if any(value['retained_request_count']!=1 for value in serviced):raise ValueError('Backed request owner did not retain its computed real job')
        for row,value in zip(rows,serviced,strict=True):
            post=copy.deepcopy(row);post.update({key:value[key]for key in('gp','eflags','xmm','before_fp','banks')});post.update(entry=0x12613e,stop_before_entries=[])
            frame=post['gp'][4];post['return_pc']=int.from_bytes(bank_bytes(post['banks'],frame,4),'little');post['matrix']=int.from_bytes(bank_bytes(post['banks'],frame+4,4),'little');post['source']=int.from_bytes(bank_bytes(post['banks'],frame+8,4),'little')
            post_rows.append(post)
        directory=scratch/'continuation';directory.mkdir();post_physical,post_proof=physical_original(image,post_rows,directory,entries=(0x1260c0,0x1262b0),regions=regions,trace_calls=[dict(entry=0x1262b0,arguments=0,gp=True,xmm=True)],trace_extended=True,preserve_x87_prefix=True)
        post_prefix_rows=[]
        for index in list(range(4))+list(range(len(rows)-4,len(rows))):
            for ordinal,event in enumerate(post_physical['cases'][index]['reads']):
                if 0x12613e<=event['instruction']<=0x12617e:
                    value=copy.deepcopy(post_rows[index]);value.update(id=len(post_prefix_rows),name=value['name']+'-post-read-'+str(ordinal),fault=dict(phase='before_read',ordinal=ordinal),owner_index=index)
                    post_prefix_rows.append(value)
        directory=scratch/'continuation-prefix';directory.mkdir();post_prefix_physical,post_prefix_proof=physical_original(image,post_prefix_rows,directory,entries=(0x1260c0,0x1262b0),regions=regions,trace_calls=[dict(entry=0x1262b0,arguments=0,gp=True,xmm=True)],trace_extended=True,preserve_x87_prefix=True)
        comparisons=[(index,result,None)for index,result in enumerate(post_physical['cases'])]+[(row['owner_index'],result,row['fault']['ordinal'])for row,result in zip(post_prefix_rows,post_prefix_physical['cases'],strict=True)]
        for index,post,fault in comparisons:
            row,pre,service=rows[index],physical['cases'][index],serviced[index]
            value=copy.deepcopy(row);value.update(id=len(cases),name=row['name']+('-complete-backed-service'if fault is None else'-post-read-'+str(fault)),stop_before_entries=[],stop_kind='return'if fault is None else'before_read',expected_pc=0x12617e if fault is None else post['boundary']['instruction'])
            if fault is not None:value['fault']=dict(phase='before_read',ordinal=len(pre['reads'])+len(service['reads'])+fault)
            value.update({key:post[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp')})
            value['reads']=copy.deepcopy(pre['reads'])+[dict(event,instruction=0x1a3b70)for event in service['reads']]+copy.deepcopy(post['reads'])
            value['writes']=copy.deepcopy(pre['writes'])+[dict(event,instruction=0x1a3b70)for event in service['writes']]+copy.deepcopy(post['writes'])
            value['read_instructions']=[e['instruction']for e in value['reads']]
            for event in value['reads']+value['writes']:event.pop('instruction')
            packet=dict(entry=0x1a3b70,arguments=[],gp=pre['boundary_gp'],eflags=pre['boundary_eflags'],xmm=pre['boundary_xmm'],fp=pre['after_fp'],live_x87=row['live_x87'])
            value['calls']=[packet]+post['calls']
            for bank,after in zip(value['banks'],post['banks'],strict=True):bank['after']=after['after']
            cases.append(value)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases),separators=(',',':')))
        results=[]
        for forced in(False,True):
            env=dict(os.environ);env.pop('HALO2_REFERENCE_ANIMATION_RESOURCE_READ',None)
            if forced:env['HALO2_REFERENCE_ANIMATION_RESOURCE_READ']='1'
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=env)
            if result.returncode:
                report.update(completed_routes=results,failed_route='forced_reference'if forced else'native');raise ValueError('Animation framing differs '+result.stderr[-6000:])
            results.append(json.loads(result.stdout))
        sites={int(pc,16)for pc in re.findall(r's\.services\.instruction\((0x[0-9a-f]+)u\)',original)}
        executed={pc for result in physical['cases']+post_physical['cases']for pc in result['executed_sites']if pc in sites}
        if sites-executed:raise ValueError('Original animation controller sites unexecuted '+str(sorted(sites-executed)))
        drift=[name for name in FILES if pins[name]!=sha(ROOT/name)]
        if drift:raise ValueError('Animation proof sources changed '+str(drift))
        report.update(passed=True,native=results[0],forced_reference=results[1],physical_original=proof,physical_read_prefixes=prefix_proof,genuine_preallocation=allocation_proof,input_provenance=input_proof,source_sha256=pins,proof_source_drift=drift,
            all_parent_sites=len(sites),executed_parent_sites=len(executed),authentication_mutations_rejected=rejected,physical_post_read_prefixes=post_prefix_proof,canonical_source_snapshot_sha256=hashlib.sha256(source.encode()).hexdigest(),io_owner_entry_abi_qualified=True,complete_controller_returns_qualified=True,segmented_physical_cpu_continuation=post_proof,normal_backed_service_returns=len(rows),saved_frame_sector_alias_returns=sum('sector-saved-frame-alias'in row['name']for row in rows),service_scope="Exact production synchronous animation helper; declared backed sectors and retained-request owners compute bytes/receipts. Real MCC asset acceptance and Xbox async scheduling are outside this cohort.",supplied_child_outcomes=False,compiler_command=command)
        print(json.dumps(results[0]))
    except BaseException as error:report['error']=str(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
