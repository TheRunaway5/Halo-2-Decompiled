#!/usr/bin/env python3
"""Compare existing frontend diagnostics with either account controller.

A shared failure is recorded as a functional failure, never as a passing menu
test. Improvements that make both implementations pass are also accepted.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--reference',type=Path,required=True)
    parser.add_argument('--candidate',type=Path,required=True)
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--mcc-root',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    assert os.environ.get('HALO2_TEST_SCRATCH')
    cases=[('matchmaking','frontend_matchmaking_http_test.py',['--mcc-root',str(args.mcc_root)],'Nonmember clan details lost the membership requirement'),
           ('gathering','frontend_matchmaking_http_test.py',['--mcc-root',str(args.mcc_root),'--gathering-ui','1'],'Matched/empty slot artwork is not an admitted source page'),
           ('clans','frontend_clans_http_test.py',[],'ONLINE bitmaps missing selected imported sampling:'),
           ('parties','frontend_parties_http_test.py',['--mcc-root',str(args.mcc_root)],'ONLINE bitmaps missing selected imported sampling:')]
    report={'passed':False,'functional_passed':False,'scope':'Modern C++/Rust account controller differential; shared menu failures remain open','cases':[]}
    try:
        for name,helper,options,marker in cases:
            pair=[]
            for owner,binary in [('cpp',args.reference),('rust',args.candidate)]:
                command=[sys.executable,'-B',str(args.source/'tests'/helper),str(binary),*options]
                run=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=180)
                text=run.stdout.decode(errors='replace')
                text=re.sub(r"(?i)(bearer\s+)[^\s'\"<>),]+",r'\1<REDACTED>',text)
                assert run.returncode!=77,f'{owner}/{name} skipped'
                if run.returncode:
                    assert marker in text,f'{owner}/{name} unexpected diagnostic: {text[-4000:]}'
                    signal=marker
                else:signal='passed'
                pair.append({'owner':owner,'exit_code':run.returncode,'signal':signal,
                             'stdout_bytes':len(run.stdout),'stdout_sha256':hashlib.sha256(run.stdout).hexdigest(),
                             'diagnostic':text[:1200] if run.returncode else ''})
            assert pair[0]['signal']==pair[1]['signal'],f'{name} changed with account controller'
            report['cases'].append({'case':name,'same_observation':True,'functional_passed':all(r['exit_code']==0 for r in pair),'observations':pair})
        report['functional_passed']=all(r['functional_passed'] for r in report['cases'])
        report['passed']=True
    finally:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS account-controller differential; actual frontend functional results:',[(r['case'],r['functional_passed']) for r in report['cases']])


if __name__=='__main__':main()
