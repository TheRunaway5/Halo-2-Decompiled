#!/usr/bin/env python3
"""Captured modern postgame callers, leave action and prior/navigation hooks.

Uses real native report/result objects and captured account helper definitions
which call their existing Rust leaves. Compares callback ordering, actual shared
owner identities/lifetimes, first-report selection, replacement during select,
leave acceptance, refusal, failure prefixes and reentry. Native rendering and the PCR
factory remain outside the differential; their source is authenticated unchanged
after the two declared substitutions. Not original Xbox/Live service replay.
Undefined empty-stack dereferences and null report dereferences in the captured
branches are excluded; separate Rust owner-error controls cover those boundaries.
"""
import argparse,copy,hashlib,itertools,json,subprocess
from pathlib import Path
REVIEW=0x10000|185
def scene(screen=14,report=None):return [dict(screen=6),dict(screen=screen,report=report)]
def base():return dict(frames=scene(),prior=dict(id=10,count=2,selected=1),view=dict(report=dict(id=50,count=3,review=dict(id=51,count=3))))
def leave_cases():
 reports=(None,dict(online=False,leave=True),dict(online=True,leave=False),dict(online=True,leave=True))
 for first,second,third,party,account,accepted in itertools.product(
         reports,reports,reports,(False,True),(False,True),(False,True)):
  frames=[dict(screen=screen,report=report) for screen,report in zip((6,REVIEW,7),(first,second,third))]
  yield base()|dict(op='leave',frames=frames,party=party,account=account,leave_accepted=accepted,revision=0xffffffff)
 online=dict(id=20,leave=True)
 for callback,reenter in [('party','replace-frame'),('party','clear-stack'),('leave_session','clear-stack'),
                         ('refresh_account','replace-frame'),('return_online','clear-stack')]:
  yield base()|dict(op='leave',frames=scene(REVIEW,online),reenter_on=callback,reenter=reenter,index=1)
 yield base()|dict(op='leave',frames=scene(REVIEW,online),system_link=True,leave_accepted=False)
 yield base()|dict(op='leave',frames=scene(REVIEW,online),system_link=True,party=False,account=False)
def corpus():
 out=[]
 for op,count,bits in itertools.product(['host','update'],[0,1,2,3,4,8,16,32],range(16)):
  c=base();c.update(op=op,authenticated=bool(bits&1),party=bool(bits&2),leader=bool(bits&4),members=[dict(self=i==0,leader=i==1,postgame=(i==0 if bits&8 else None))for i in range(count)]);c['frames']=scene(REVIEW,dict(id=20,online=True,**{'continue':True}));out.append(c)
 for own,leader,extra in itertools.product([None,False,True],[None,False,True],range(5)):
  c=base();rows=[dict(self=True,postgame=own),dict(leader=True,postgame=leader)]
  rows += [[],[dict(self=True,postgame=False)],[dict(leader=True,postgame=True)],[dict(self=True,leader=True,postgame=False)],[dict(self=True,postgame=True),dict(leader=True,postgame=False)]][extra]
  c.update(op='update',members=rows,frames=scene(REVIEW,dict(id=20)));out.append(c)
 for first,second,third in itertools.product([None,dict(online=False),dict(online=True,**{'continue':False}),dict(online=True,**{'continue':True})],repeat=3):
  c=base();c.update(op='update',frames=[dict(screen=s,report=None if r is None else r|dict(id=i+20))for i,(s,r)in enumerate(zip([6,REVIEW,14],[first,second,third]))]);out.append(c)
 c=base();c.update(op='update',frames=[]);out.append(c)
 for prior,results,count,online in itertools.product([False,True],[False,True],[0,1,16,17,0xffffffff],[False,True]):
  c=base();c.update(op='prior',prior=dict(id=10,results=results,count=count,online=online)if prior else None);out.append(c)
 for mode,screen,count,bits in itertools.product([-2147483648,-1,0,1,2,3,4,2147483647],[6,14,REVIEW,0xffffffff],[0,1,16],range(16)):
  c=base();c.update(op='review',mode=mode,frames=scene(screen),prior=dict(id=10,count=count,online=bool(bits&1),results=bool(bits&2),select=bool(bits&4),selected=0xffffffff)if bits&8 else None);out.append(c)
 for length,screen,report,online in itertools.product([1,2,3],[6,14,REVIEW,0xffffffff],[False,True],[False,True]):
  c=base();c.update(op='close',frames=[dict(screen=6)]*(length-1)+[dict(screen=screen,report=dict(id=20,online=online)if report else None)],revision=0xffffffff);out.append(c)
 for op,id_value,online,review in itertools.product(['capture','navigation'],[0,16,185,REVIEW,0xffffffff],[False,True],[None,dict(id=51,count=2)]):
  c=base();c.update(op=op,id=id_value,view=dict(report=dict(id=50,online=online,review=review)));out.append(c)
 for op,id_value in [('capture',185),('capture',0),('navigation',16),('navigation',0)]:
  c=base();c.update(op=op,id=id_value,view=dict(report=None));out.append(c)
 for op,callback,reenter in [('update','continue','clear-stack'),('update','continue','replace-frame'),('update','continue','replace-prior'),('review','select','replace-prior'),('review','select','reset-prior'),('review','select','capture'),('review','open','replace-prior')]:
  c=base();c.update(op=op,reenter_on=callback,reenter=reenter,keep_alive=True,replacement=dict(id=71,count=3,selected=2));
  if op=='update':c['frames']=scene(REVIEW,dict(id=20))
  out.append(c)
 return out+list(leave_cases())
def run(binary,cases):
 p=subprocess.run([str(binary)],input=''.join(json.dumps(c)+'\n'for c in cases),text=True,capture_output=True);lines=p.stdout.splitlines()
 if lines[:1]==['Shutdown handler: initialize.']and lines[-1:]==['Shutdown handler: cleanup.']:lines=lines[1:-1]
 if p.returncode or len(lines)!=len(cases):raise RuntimeError(json.dumps(dict(exit_code=p.returncode,actual=len(lines),expected=len(cases),stderr=p.stderr[-2000:],stdout=p.stdout[:1000])))
 return [json.loads(s)for s in lines]
def batches(binary,cases):
 for i in range(0,len(cases),8):
  group=cases[i:i+8];yield from zip(group,run(binary,group))
def controls():
 a=base();a.update(op='update',frames=scene(REVIEW,dict(id=20)))
 b=base();b.update(op='review')
 c=base();c.update(op='review',prior=dict(id=10,count=2,select=False))
 d=base();d.update(op='review',reenter_on='select',reenter='replace-prior',keep_alive=True,replacement=dict(id=71,count=3))
 leave=base()|dict(op='leave',frames=scene(REVIEW,dict(id=20,leave=True)))
 return [a,b,c,d,leave,leave|dict(leave_accepted=False),leave|dict(system_link=True)]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('native',type=Path);p.add_argument('--report',type=Path,required=True);a=p.parse_args();cases=corpus();differences=[];observations=reentries=0
 for i,(case,result)in enumerate(batches(a.native,cases)):
  observations+=len(result['before']['calls']);reentries+=result['rust']['reentries']
  if result['before']!=result['rust']and len(differences)<3:differences.append(dict(index=i,case=case,actual=result))
 selected=controls();faults=[]
 for case,value in zip(selected,run(a.native,selected)):
  for index in range(len(value['before']['calls'])):
   for nonstd in [False,True]:faults.append(case|dict(fail=index,nonstd=nonstd))
 identity=0
 for case,value in batches(a.native,faults):
  identity+=int(value['rust'].get('same_object',value['rust'].get('error')=='int'and value['rust'].get('integer')==73))
  if value['before']!=value['rust']and len(differences)<3:differences.append(dict(case=case,actual=value))
 report=dict(passed=not differences,cases=len(cases),native_io_observations=observations,rust_state_reentries=reentries,fault_controls=len(faults),preserved_native_exception_identity=identity,differences=differences,undefined_cpp_inputs_excluded=True,actual_native_report_objects=True,native_report_identity_and_lifetime_observed=True,first_online_report_preserved=True,fresh_prior_after_selection_preserved=True,native_sha256=hashlib.sha256(a.native.read_bytes()).hexdigest(),scope=__doc__)
 report['leave_cases']=sum(case['op']=='leave' for case in cases)
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='differences'}))
 if differences:print(json.dumps(differences[0])[:8000])
 raise SystemExit(0 if report['passed']else 1)
if __name__=='__main__':main()
