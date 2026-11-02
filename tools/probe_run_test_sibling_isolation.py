#!/usr/bin/env python3
"""Check owned watcher completion/cancellation cannot signal a sibling owner."""
import argparse,json,os,signal,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);children=scratch/'children';children.mkdir();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],unexpected_signal_origin_claimed=False)
 coordinator=None
 try:
  code='''import json,os,sys,signal
sys.path.insert(0,sys.argv[1])
from physics_qualification_budget import QualificationBudget,QualificationWatchdog
from pathlib import Path
out=Path(os.environ['HALO2_TEST_SCRATCH']);budget=QualificationBudget(total_seconds=15,cleanup_seconds=1,maximum_seconds=15);watchdog=QualificationWatchdog(budget,out).start()
try:
 print(json.dumps(dict(pid=os.getpid(),pgid=os.getpgrp(),sid=os.getsid(0),scratch=str(out))),flush=True)
 assert sys.stdin.readline()=='stop\\n'
finally:watchdog.close()
'''
  prefix=[sys.executable,str(ROOT/'tools/run_test.py'),'--scratch-root',str(children),'--']
  coordinator=subprocess.Popen([*prefix,sys.executable,'-c',code,str(ROOT/'tools')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
  state=json.loads(coordinator.stdout.readline());assert state['pid']==state['pgid']==state['sid'];assert state['pid']!=os.getpid();records=[]
  for cancel in(False,True):
   watcher_code='''import json,os,subprocess,sys,time
orphan=subprocess.Popen([sys.executable,'-c','import time;time.sleep(30)'])
print(json.dumps(dict(pid=os.getpid(),pgid=os.getpgrp(),sid=os.getsid(0),orphan=orphan.pid,scratch=os.environ['HALO2_TEST_SCRATCH'])),flush=True)
'''+('time.sleep(30)\n'if cancel else'')
   watcher=subprocess.Popen([*prefix,sys.executable,'-c',watcher_code],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
   current=json.loads(watcher.stdout.readline());assert current['pid']==current['pgid']==current['sid']and current['pgid']!=state['pgid']
   if cancel:watcher.send_signal(signal.SIGTERM)
   output,error=watcher.communicate(timeout=8);assert watcher.returncode==(143 if cancel else 0),(watcher.returncode,error)
   assert coordinator.poll()is None;os.kill(state['pid'],0);assert Path(state['scratch']).exists();assert not Path(current['scratch']).exists()
   status=Path('/proc')/str(current['orphan'])/'status';orphan_stopped=not status.exists()or'State:\tZ'in status.read_text();assert orphan_stopped
   records.append(dict(cancel=cancel,watcher=current,watcher_returncode=watcher.returncode,coordinator_alive=True,watcher_owner_removed=True,orphan_stopped=True))
  output,error=coordinator.communicate(input='stop\n',timeout=8);assert coordinator.returncode==0,(coordinator.returncode,error);assert not Path(state['scratch']).exists()
  report.update(passed=True,coordinator=state,watchers=records,coordinator_returncode=coordinator.returncode,coordinator_owner_removed=True,scope='Normal completion plus explicit outer-parent SIGTERM cancellation of independent run_test watcher trees while a sibling real QualificationWatchdog coordinator is alive. No live game or original replayer traced; no cause of historical external signal inferred.')
 finally:
  if coordinator is not None and coordinator.poll()is None:
   coordinator.send_signal(signal.SIGTERM)
   try:coordinator.communicate(timeout=8)
   except subprocess.TimeoutExpired:coordinator.kill();coordinator.communicate()
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 print(json.dumps(dict(passed=report['passed'],coordinator_returncode=report.get('coordinator_returncode'))));return 0
if __name__=='__main__':raise SystemExit(main())
