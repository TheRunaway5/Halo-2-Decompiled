#!/usr/bin/env python3
"""Retain only original binder words consumed by the material regressions.

Run inside tools/run_test.py. Full original banks/listings stay in the owned
scratch; compact reference inputs and complete-report hashes are retained.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]

def save(path,value):
    data=(json.dumps(value,separators=(',',':'))+'\n').encode()
    if len(data)>1024*1024:raise ValueError('Compact original fixture exceeds report budget')
    path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    return {'path':str(path.relative_to(ROOT)),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}

def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    receipts=[]
    prior_dir=scratch/'pass-contract'
    subprocess.run([sys.executable,str(ROOT/'tools/inspect_camouflage_pass_contract.py'),'--output',str(prior_dir)],check=True)
    prior_bytes=(prior_dir/'report.json').read_bytes();prior_digest=hashlib.sha256(prior_bytes).hexdigest()
    prior=json.loads(prior_bytes);unique=[];indices=[];lookup={}
    for row in prior['cases']:
        compact={key:row[key] for key in ['layer','initial_vertex_hex','vertex_hex','initial_pixel_hex','pixel_hex']}
        compact['mappings']=row['mappings'];compact['scalar_bits']={'37':row['scalar_bits']['37']}
        key=json.dumps(compact,sort_keys=True)
        if key not in lookup:lookup[key]=len(unique);unique.append(compact)
        indices.append(lookup[key])
    receipts.append(save(ROOT/'tests/fixtures/camouflage_state_inputs_original.json',{
        'original_report_sha256':prior_digest,'cases':unique,'case_indices':indices}))
    for tool,name in [('oracle_camouflage_pass_services.py','report.json'),
                      ('oracle_camouflage_binder_boundaries.py','binder-report.json')]:
        output=scratch/tool.removesuffix('.py')
        command=[sys.executable,str(ROOT/'tools'/tool),'--output',str(output)]
        if name=='binder-report.json':command+=['--prior',str(prior_dir/'report.json'),'--prior-sha256',prior_digest]
        subprocess.run(command,check=True)
        data=(output/name).read_bytes();report=json.loads(data)
        provenance={'original_report_sha256':hashlib.sha256(data).hexdigest(),
                    'original_tool':tool,'original_tool_sha256':hashlib.sha256((ROOT/'tools'/tool).read_bytes()).hexdigest()}
        if name=='report.json':
            result={**provenance,'inputs':{'object':report['inputs']['object']},'cases':[]}
            for row in report['cases']:
                keys=['complete_original_return']
                if row['complete_original_return']:
                    keys+=['layer','implementation','primary_bits','state75_at_binder_entry','state75','fog_context_before']
                compact={key:row[key] for key in keys}
                if row['complete_original_return']:
                    for prefix in ['initial','final']:
                        raw=bytes.fromhex(row[prefix+'_bank_hex'])
                        compact[prefix+'_pixel_hex']=raw[0x1424:0x1514].hex()
                        compact[prefix+'_vertex_hex']=raw[0x1530:0x1730].hex()
                result['cases'].append(compact)
            target=ROOT/'tests/fixtures/camouflage_pass_banks_original.json'
        else:
            keys=['prior_case','layer','initial_state75','state75','requested_fpcw','mappings']
            provenance={'original_cases_sha256':hashlib.sha256(json.dumps(report['state_cases'],sort_keys=True,separators=(',',':')).encode()).hexdigest(),
                        'original_tool':tool,'original_tool_sha256':provenance['original_tool_sha256'],
                        'prior_report_sha256':prior_digest}
            result={**provenance,'state_cases':[{key:row[key] for key in keys} for row in report['state_cases']]}
            target=ROOT/'tests/fixtures/camouflage_state_banks_original.json'
        receipts.append(save(target,result))
    save(ROOT/'analysis/equipment-20261022/camouflage-fixtures.json',{'fixtures':receipts,'raw_outputs':'Owned scratch only; removed by run_test.py'})

if __name__=='__main__':main()
