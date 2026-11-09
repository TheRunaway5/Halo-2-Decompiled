#!/usr/bin/env python3
"""Qualify native126060 using genuine page/span and specific-datum outcomes."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import native_resource_page_replacements as binding
import validate_native_tu15_resource_maintenance as maintenance
from specialize_native_operands import leaf_body
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
FILES=('include/halo2/native_tu15_resource_page_kernel.hpp','src/physics/runtime/native_tu15_resource_page.hpp','tools/native_resource_page_replacements.py','tools/validate_native_tu15_resource_page.py','tests/native_tu15_resource_page_tests.cpp')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def prepare(image,scratch,required):
    source=(ROOT/maintenance.SOURCE).read_text();candidate=binding.replace_resource_page(source)
    if required and source!=candidate:raise ValueError('Production126060 binding absent')
    if binding.replace_resource_page(candidate)!=candidate:raise ValueError('Page binding is not idempotent')
    rejected=0
    for variant in(source,candidate):
        body=leaf_body(variant,'126060')[0]
        for old,new in(('0x474d78u','0x474d7cu'),('run_16e770(s);','run_13fe50(s);')):
            changed=variant.replace(body,body.replace(old,new,1),1)
            if changed==variant:raise ValueError('Missing page body mutation witness')
            try:binding.replace_resource_page(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Page binder accepted complete-body drift')
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
    for old,new in(('run_126060(s);','run_1260c0(s);'),('s.required(site,target,tail);','s.required(site+1u,target,tail);')):
        changed=source.replace(dispatcher,dispatcher.replace(old,new,1),1)
        if changed==source:raise ValueError('Missing page dispatcher mutation witness')
        try:binding.replace_resource_page(changed)
        except ValueError:rejected+=1
        else:raise ValueError('Page binder accepted complete dispatch drift')
    changed=candidate.replace(binding.PREFIX,binding.PREFIX.replace('dispatch(v,target,site,tail);','dispatch(v,target,site+1u,tail);'),1)
    try:binding.replace_resource_page(changed)
    except ValueError:rejected+=1
    else:raise ValueError('Page binder accepted altered native prefix')
    pins,provenance,_=maintenance.sources(scratch,False,image,True)
    rendered=(scratch/'resource_maintenance_bodies.inc').read_text()
    rendered=rendered.replace('\n    before_owner(s,0x126060u);','',1)
    old=leaf_body(rendered,'126060')[0];original=binding.canonical_body(old)
    if hashlib.sha256(original.encode()).hexdigest()!=binding.PIN:raise ValueError('Complete private page reference changed')
    start=original.index('{')+1;native=original[:start]+binding.PREFIX+original[start:];rendered=rendered.replace(old,native,1)
    for entry in('13fe50','16e770'):
        body=leaf_body(rendered,entry)[0];start=body.index('{')+1
        rendered=rendered.replace(body,body[:start]+f'\n    before_owner(s,0x{entry}u);'+body[start:],1)
    (scratch/'resource_page_bodies.inc').write_text(rendered)
    base=maintenance.corpus();rows,input_proof=maintenance.page_inputs(image,base)
    rows=[row for row in rows if '-genuine-page-'in row['name']]
    for n,row in enumerate(rows):
        row.update(id=n,entry=0x126060,name='page-'+row['name'],stop_before_entries=[])
        row['gp'][0]=1 if 'mode9-'in row['name']else 2;row['gp'][7]=0x61006000
    rows.extend(alias_cases(rows))
    return rows,{**pins,**{name:sha(ROOT/name)for name in FILES}},provenance,input_proof,rejected


def alias_cases(rows):
    """Use real owners with descriptor/output and parent saved-word overlap."""
    added=[]
    for source in rows:
        if not source['name'].endswith('-1000'):continue
        for label,resource in (('saved-si',source['gp'][4]-20),('saved-bx',source['gp'][4]-16),('allocated-row',0x61004000),('next-row',0x6100400c)):
            row=copy.deepcopy(source);row.update(id=len(rows)+len(added),name=source['name']+'-alias-'+label)
            row['gp'][7]=resource
            for bank in row['banks']:
                base=bank['address'];raw=bytearray.fromhex(bank['before'])
                if base<=resource+4<=base+len(raw)-4:
                    raw[resource+4-base:resource+8-base]=(4096).to_bytes(4,'little');bank['before']=raw.hex();break
            else:raise ValueError('Resource alias escaped declared backing')
            # The size is captured before real page-owner calls use their
            # private frame; the late page-handle store may replace a parent
            # saved register, or overlap its genuine allocated record.
            added.append(row)
    return added


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file():return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=args.require_binding,scratch_directory=str(scratch),retained_large_artifacts=[])
    try:
        from oracle_smg_firing_1_1 import Xbe
        image=Xbe(args.xbe);rows,pins,page_proof,input_proof,rejected=prepare(image,scratch,args.require_binding)
        binary=scratch/'resource-page';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_resource_page_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise ValueError('Resource page compile failed '+result.stderr[-5000:])
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        old_metadata=common.metadata
        def metadata(image):
            value=old_metadata(image);code=common._Code(image,'.text')
            for site in value['sites']:
                if code.at(site['instruction']).mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                expected={0x16e6c5:'852c8b',0x1401a2:'851c88',0x32a672:'8500',0x32a689:'8501',0x32a696:'8501'}.get(site['instruction'])
                if expected:
                    if site['bytes']!=expected or len(site['operands'])!=1:raise ValueError('Original TEST metadata changed')
                    site['operands'][0].update(read=True,write=False)
                if site['instruction']==0x140071:
                    if site['bytes']!='0f184c0464':raise ValueError('Original advisory prefetch changed')
                    for operand in site['operands']:operand.update(read=False,write=False)
            return value
        common.metadata=metadata;entries=maintenance.PAGE_ENTRIES+(0x16e8a0,)
        regions=tuple(sorted({(bank['address'],len(bank['before'])//2)for row in rows for bank in row['banks']}))
        trace=[dict(entry=target,arguments=0,gp=True,xmm=True)for target in(0x13fe50,0x16e770)]
        physical,proof=physical_original(image,rows,scratch,entries=entries,regions=regions,trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)
        prefixes=[]
        witnesses=list(range(4))+list(range(len(rows)-4,len(rows)))
        for index in witnesses:
            row,case=rows[index],physical['cases'][index]
            for ordinal,event in enumerate(case['reads']):
                if 0x126060<=event['instruction']<=0x1260b5:
                    value=copy.deepcopy(row);value.update(id=len(rows)+len(prefixes),name=row['name']+'-before-read-'+str(ordinal),fault=dict(phase='before_read',ordinal=ordinal));prefixes.append(value)
        directory=scratch/'prefix';directory.mkdir();pref,prefix_proof=physical_original(image,prefixes,directory,entries=entries,regions=regions,trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)
        cases=[]
        for row,case in zip(rows+prefixes,physical['cases']+pref['cases'],strict=True):
            value=copy.deepcopy(row);value.update({key:case[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes','calls')})
            value['read_instructions']=[event['instruction']for event in case['reads']];value['stop_kind']='before_read'if'fault'in row else'return'
            value['expected_pc']=case['boundary']['instruction']if'fault'in row else next(pc for pc in(0x1260b0,0x1260b5)if pc in case['executed_sites'])
            if not case['live_x87_unchanged']:raise ValueError('Original page controller changed live x87')
            for bank,after in zip(value['banks'],case['banks'],strict=True):bank['after']=after['after']
            for event in value['reads']+value['writes']:event.pop('instruction')
            cases.append(value)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases),separators=(',',':')));results=[]
        for forced in(False,True):
            env=dict(os.environ);env.pop('HALO2_REFERENCE_RESOURCE_PAGE',None)
            if forced:env['HALO2_REFERENCE_RESOURCE_PAGE']='1'
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=env)
            if result.returncode:raise ValueError('Resource page comparison failed '+result.stderr[-7000:])
            results.append(json.loads(result.stdout))
        original=binding.canonical_body(leaf_body((ROOT/maintenance.SOURCE).read_text(),'126060')[0]);sites={int(pc,16)for pc in re.findall(r's\.services\.instruction\((0x[0-9a-f]+)u\)',original)}
        executed={pc for row in physical['cases']for pc in row['executed_sites']if pc in sites}
        if sites-executed:raise ValueError('Page controller original sites remain '+str(sorted(sites-executed)))
        drift=[name for name in FILES if sha(ROOT/name)!=pins[name]]
        if drift:raise ValueError('Page proof source changed during run '+str(drift))
        report.update(passed=True,native=results[0],forced_reference=results[1],physical_original=proof,physical_read_prefixes=prefix_proof,source_sha256=pins,proof_source_drift=drift,
            normal_full_returns=len(rows),alias_full_returns=sum('-alias-'in row['name']for row in rows),all_parent_sites=len(sites),executed_parent_sites=len(executed),authentication_mutations_rejected=rejected,page_reference=page_proof,page_input_provenance=input_proof,
            supplied_child_outcomes=False,child_native_dependency_closure_qualified=False,physical_page_faults_qualified=False,compiler_command=command)
        print(json.dumps(results[0]))
    except BaseException as error:report['error']=str(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
