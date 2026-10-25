#!/usr/bin/env python3
"""Finite command session inside tools/run_test.py for active Mac validation.

All builds/SDKs/references stay in that owner. Send JSON objects {"argv": [...]}
or {"quit": true}; idle expiry and disconnection return control to its cleanup
supervisor. Commands/results remain in a bounded external journal.
"""
import argparse
import hashlib
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
    journal={'scratch_owner':str(scratch),'pid':os.getpid(),'commands':[],
             'rejected_requests':0,'rejections':[],'status':'active'}
    def save():
        data=json.dumps(journal,indent=2)+'\n'
        if len(data.encode())>1024*1024:raise ValueError('Session evidence budget exhausted')
        temporary=args.output/'.session.json.tmp'
        temporary.write_text(data)
        temporary.replace(args.output/'session.json')
    def reject(line,reason):
        journal['rejected_requests']+=1
        rejection={'sha256':hashlib.sha256(line).hexdigest(),'bytes':len(line),
                   'reason':str(reason)[:256],'time':time.time()}
        # Keep the session usable after bad requests without an unbounded
        # external diagnostic journal. The total remains exact.
        if len(journal['rejections'])<64:journal['rejections'].append(rejection)
        save();print(json.dumps({'request_rejected':rejection}),flush=True)
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
            try:
                request=json.loads(line)
                if not isinstance(request,dict):raise ValueError('Expected a JSON object')
                if set(request)=={'quit'} and request['quit'] is True:break
                if set(request)!={'argv'}:raise ValueError('Expected only argv, or exactly {"quit": true}')
                argv=request['argv']
                if (not isinstance(argv,list) or not argv or
                    not all(isinstance(arg,str) and '\x00' not in arg for arg in argv) or not argv[0]):
                    raise ValueError('Expected a nonempty argv array of NUL-free strings and a nonempty executable')
            except (ValueError,TypeError) as error:
                reject(line,error);continue
            record={'argv':argv,'started':time.time(),'exit_code':None};journal['commands'].append(record);save()
            try:record['exit_code']=subprocess.run(argv,stdin=subprocess.DEVNULL).returncode
            except OSError as error:
                record['exit_code']=127;record['launch_error']=str(error)[:512]
            record['finished']=time.time();save();print(json.dumps({'command_finished':len(journal['commands']),'exit_code':record['exit_code']}),flush=True)
    finally:
        journal['status']='closed';save()

if __name__=='__main__':main()
