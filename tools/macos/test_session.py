#!/usr/bin/env python3
"""Finite command session inside tools/run_test.py for active Mac validation.

All builds/SDKs/references stay in that owner. Send JSON lines with argv arrays
or {"quit": true}; idle expiry and disconnection return control to its cleanup
supervisor. Commands/results remain in a bounded external journal.
"""
import argparse
import json
import os
from pathlib import Path
import select
import subprocess
import sys
import time


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--idle-seconds',type=int,default=1200)
    args=parser.parse_args()
    if not 30<=args.idle_seconds<=3600:parser.error('Idle timeout must be 30..3600 seconds')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output.mkdir(parents=True,exist_ok=True)
    journal={'scratch_owner':str(scratch),'pid':os.getpid(),'commands':[],'status':'active'}
    def save():
        data=json.dumps(journal,indent=2)+'\n'
        if len(data.encode())>1024*1024:raise ValueError('Session evidence budget exhausted')
        (args.output/'session.json').write_text(data)
    save();print(json.dumps({'ready':True,'scratch_owner':str(scratch),'pid':os.getpid()}),flush=True)
    pending=b''
    try:
        while True:
            while b'\n' not in pending:
                if not select.select([sys.stdin],[],[],args.idle_seconds)[0]:return
                chunk=os.read(sys.stdin.fileno(),65536)
                if not chunk:return
                pending+=chunk
                if len(pending)>1024*1024:raise ValueError('Session command budget exhausted')
            line,pending=pending.split(b'\n',1)
            request=json.loads(line)
            if request.get('quit'):break
            argv=request['argv']
            if not argv or not all(isinstance(arg,str) for arg in argv):raise ValueError('Expected a nonempty argv array')
            record={'argv':argv,'started':time.time(),'exit_code':None};journal['commands'].append(record);save()
            record['exit_code']=subprocess.run(argv,stdin=subprocess.DEVNULL).returncode
            record['finished']=time.time();save();print(json.dumps({'command_finished':len(journal['commands']),'exit_code':record['exit_code']}),flush=True)
    finally:
        journal['status']='closed';save()

if __name__=='__main__':main()
