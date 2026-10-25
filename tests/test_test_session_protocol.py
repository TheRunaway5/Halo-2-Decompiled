#!/usr/bin/env python3
"""Direct session-protocol/liveness regression; run through tools/run_test.py."""
import argparse,json,os,subprocess,sys,tempfile,time
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

def need(value,message):
    if not value:raise RuntimeError(message)
def stop(process):
    if process.poll() is None:process.terminate()
    try:process.wait(timeout=3)
    except subprocess.TimeoutExpired:process.kill();process.wait()
def wait_for(predicate,seconds=5):
    deadline=time.monotonic()+seconds
    while time.monotonic()<deadline:
        if predicate():return
        time.sleep(.02)
    raise RuntimeError('Timed out waiting for owned session control')
def write_command(path):
    return [sys.executable,'-c','from pathlib import Path; Path('+repr(str(path))+').write_text("complete")']

def direct(tool,scratch):
    folder=scratch/'direct';folder.mkdir();first=folder/'first';last=folder/'last'
    environment=dict(os.environ,HALO2_TEST_SCRATCH=str(folder),PYTHONDONTWRITEBYTECODE='1')
    malformed=[[],None,True,'array-is-not-an-object',{}, {'argv':'not-an-array'}, {'argv':[]},
        {'argv':[17]}, {'argv':['']}, {'argv':['program','\x00']}, {'quit':'yes'}, {'quit':True,'argv':['ignored']}]
    lines=[json.dumps(value).encode()+b'\n' for value in malformed]
    lines.append(b'{invalid-json}\n')
    lines.extend((json.dumps({'argv':write_command(first)}).encode()+b'\n',
        json.dumps({'argv':[str(folder/'missing-executable')]}).encode()+b'\n',
        json.dumps({'argv':write_command(last)}).encode()+b'\n',b'{"quit":true}\n'))
    process=subprocess.Popen([sys.executable,'-B',str(tool),'--output',str(folder/'session'),'--idle-seconds','30'],
        env=environment,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    try:stdout,stderr=process.communicate(b''.join(lines),timeout=15)
    finally:stop(process)
    need(process.returncode==0,'Malformed request killed session; exit='+str(process.returncode)+' stderr='+stderr.decode(errors='replace')[-2000:])
    journal=json.loads((folder/'session/session.json').read_text())
    need(first.read_text()==last.read_text()=='complete','Valid commands after malformed requests did not execute')
    need(journal['status']=='closed' and len(journal['commands'])==3,'Session did not process exactly three valid command requests')
    need([row['exit_code'] for row in journal['commands']]==[0,127,0],'Missing executable was not isolated as a failed command')
    need(journal['rejected_requests']==len(lines)-4 and len(journal['rejections'])==len(lines)-4,'Malformed request accounting differs')
    need(all('sha256' in item and 'reason' in item for item in journal['rejections']),'Bounded rejection diagnostics missing')
    return dict(rejected=journal['rejected_requests'],successful_commands=2,failed_command_continued=True)

def submission(tool,helper,scratch):
    folder=scratch/'submission';folder.mkdir();session=folder/'session';session.mkdir()
    pipe=session/'commands.pipe';os.mkfifo(pipe);descriptor=os.open(pipe,os.O_RDWR|os.O_NONBLOCK)
    environment=dict(os.environ,HALO2_TEST_SCRATCH=str(folder),PYTHONDONTWRITEBYTECODE='1')
    log=(folder/'session.log').open('wb')
    process=subprocess.Popen([sys.executable,'-B',str(tool),'--output',str(session),'--idle-seconds','30'],
        env=environment,stdin=descriptor,stdout=log,stderr=subprocess.STDOUT)
    checks=0
    def invoke(directory,command,expected):
        result=subprocess.run([sys.executable,'-B',str(helper),'--session',str(directory),'--',*command],
            env=environment,capture_output=True,text=True,timeout=5)
        need((result.returncode==0)==expected,'Submission helper result differs: '+result.stderr[-1000:])
        return result
    try:
        wait_for(lambda:(session/'session.json').exists())
        marker=folder/'typed-success';invoke(session,write_command(marker),True);wait_for(marker.exists);checks+=1
        # The actual failure shape is rejected while this same owned session
        # remains live; a subsequent typed command must still be admitted.
        os.write(descriptor,b'[]\n');second=folder/'after-bad-shape'
        invoke(session,write_command(second),True);wait_for(second.exists);checks+=1
        wrong=folder/'regular';wrong.mkdir();(wrong/'commands.pipe').write_bytes(b'unchanged')
        (wrong/'session.json').write_text((session/'session.json').read_text())
        invoke(wrong,write_command(folder/'must-not-run'),False)
        need((wrong/'commands.pipe').read_bytes()==b'unchanged','Non-FIFO output was mutated');checks+=1
        stale=folder/'stale';stale.mkdir();os.mkfifo(stale/'commands.pipe')
        journal=json.loads((session/'session.json').read_text());journal['pid']=99999999
        (stale/'session.json').write_text(json.dumps(journal));invoke(stale,write_command(folder/'must-not-run'),False);checks+=1
        invoke(session,[],False);checks+=1
        invoke(session,[sys.executable,'x'*8192],False);checks+=1
        os.write(descriptor,b'{"quit":true}\n');process.wait(timeout=5)
        invoke(session,write_command(folder/'must-not-run'),False);checks+=1
        need(not (folder/'must-not-run').exists(),'Rejected submission executed')
        need(json.loads((session/'session.json').read_text())['rejected_requests']==1,'Live malformed request was not retained')
    finally:
        stop(process);os.close(descriptor);log.close()
    return dict(controls=checks,actual_session_recovered=True)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session-tool',type=Path,default=ROOT/'tools/macos/test_session.py')
    parser.add_argument('--submit-helper',type=Path)
    parser.add_argument('--report-output',type=Path)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix='session-protocol-',dir=scratch) as directory:
        work=Path(directory);result=dict(passed=False,gpu_work=False,scratch_owner=str(scratch),session_protocol=direct(args.session_tool,work))
        if args.submit_helper:result['submission_helper']=submission(args.session_tool,args.submit_helper,work)
        result['passed']=True
    need(not work.exists(),'Owned protocol test context remains')
    result['test_context_removed']=True
    if args.report_output:args.report_output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(result,sort_keys=True),flush=True)
if __name__=='__main__':main()
