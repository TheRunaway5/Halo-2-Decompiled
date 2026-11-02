#!/usr/bin/env python3
"""Captured complete modern friends caller versus exact production Rust/CXX IO.

Account, social, localization and navigation providers are controlled. Cached
opaque data, targets, detached/retained frames and native callback-boundary
state are compared. Original Live providers, original Xbox full callers and
installed-client delivery are not certified. Former C++ -INT_MIN inputs are
excluded here; the separate Rust unit control covers that safe invalid index.
"""
import argparse,copy,hashlib,itertools,json,subprocess
from pathlib import Path
ID=[255,0,128]
def base():
 return dict(friend_target=ID,activity_target=ID,account=dict(authenticated=True,available=False,tag=[128,0],message=[255,0],friends=[dict(id=ID,name='same',state='friend',presence='online',joinable=True,context=ID),dict(id='B',name='same',state='incoming',presence='offline'),dict(id=ID,name='same',state='blocked',presence='playing',joinable=False,context='other')],search=dict(id=ID,name='search',tag=ID,state='friend',presence='playing',joinable=True,context=ID)),
  social=dict(available=True,enabled=True,ready=True,message=ID,revision=0x100000001,friends=[dict(id=ID,name='same',username=[255,0],kind=0,presence=4),dict(id='B',name='same',kind=1,presence=1),dict(id=ID,name='same',kind=3,presence=2)],invitations=[dict(id=ID,name='same',status=ID),dict(id='B',name='same',status='other')]),
  frames=[dict(screen=6),dict(screen=186),dict(screen=26,context=0,entries=[dict(label='same',destination=31,value=0),dict(label='same',destination=31,value=-1)])])
def scene(screen,context,entries=None,selected=0):return [dict(screen=6),dict(screen=186),dict(screen=screen,context=context,selected=selected,entries=entries or[])]
def corpus():
 out=[]
 for screen,context,bits in itertools.product([26,31,216,0xffffffff,33,14,6],[0,43,44,46,41],range(16)):
  c=base();c.update(steam=bool(bits&1),login_client=bool(bits&2),social_client=bool(bits&4),detached=bool(bits&8));c['account']['available']=bool(bits&4);c['account']['party']=bool(bits&8);c['account']['leader']=bool(bits&2);c['social'].update(available=bool(bits&1),enabled=bool(bits&2),ready=bool(bits&4),connecting=bool(bits&8),busy=bool(bits&2));c['frames']=scene(screen,context,entries=[dict(label='existing',destination=-5,value=7)]if bits&8 else[]);c['view']=copy.deepcopy(c['frames'][-1]);
  for op in ['populate','native-populate']:out.append(copy.deepcopy(c)|dict(op=op))
 for screen,context,native,index in itertools.product([26,31,216,0xffffffff,33,14,6],[0,43,44,46,41],range(3),[0,1,2,3,4,0xffffffff]):
  c=base();c['account']['available']=native==1;c['steam']=native==2;c['frames']=scene(screen,context);out.append(c|dict(op='text',index=index))
 for native,index,missing in itertools.product(range(3),[0,2,99],['screen_count','pane_count','text_count']):
  c=base();c['account']['available']=native==1;c['steam']=native==2;c[missing]=0;out.append(c|dict(op='text',index=index))
 for screen,context,native,value,column,row in itertools.product([26,31],[0,44],range(3),[-2147483647,-1,0,1,2147483647],[0,1,2],[0,99]):
  c=base();c['account']['available']=native==1;c['steam']=native==2;c['frames']=scene(screen,context,[dict(destination=31,value=value)]);out.append(c|dict(op='column',column=column,row=row))
 for destination,value,context,bits in itertools.product([167,-5,26,31,-83,-999],[-2147483647,-1,0,1,4,6,7,8,9,2147483647],[43,46],range(8)):
  c=base();c.update(op='native-accept',reply=bool(bits&1),account_client=bool(bits&2));c['account']['busy']=bool(bits&4);c['frames']=scene(31,context,[dict(destination=destination,value=value)],99 if bits==7 else 0);out.append(c)
 for status,bits,joinable in itertools.product(range(11),range(32),[None,False,True]):
  c=base();c.update(op='native-accept',status=status,account_client=bool(bits&1),launch=bool(bits&8));c['account'].update(busy=bool(bits&2),party=bool(bits&4));c['account']['friends'][0].update(joinable=joinable,presence=['offline','online','playing','','unknown'][bits%5],context=''if bits&16 else ID);c['frames']=scene(31,43,[dict(destination=-83,value=8)]);out.append(c)
 for destination,value,context,bits in itertools.product([-5,26,31,-35,-83,-999],[-2147483647,-1,0,1,3,4,6,7,8,2147483647],[0,43,44,46],range(4)):
  c=base();c.update(op='accept',reply=bool(bits&1),social_client=bool(bits&2),login_client=False);c['account'].update(party=bool(bits&1),leader=bool(bits&2));c['frames']=scene(31,context,[dict(destination=destination,value=value)]);out.append(c)
 for revision,same,native,shape,value in itertools.product([0,1,0x100000001,0xffffffffffffffff],[False,True],range(4),range(4),[-2147483647,-1,0,1,2147483647]):
  c=base();c.update(op='update',login_client=bool(native&1),steam=bool(native&2));c['account']['available']=native==3;c['social']['revision']=revision;c['next']=copy.deepcopy(c['social']);c['next']['revision']=revision if same else(revision+1)&0xffffffffffffffff;c['next']['friends']=list(reversed(c['social']['friends']));c['next']['invitations']=list(reversed(c['social']['invitations']));c['next']['friends'].append(copy.deepcopy(c['next']['friends'][-1]));c['frames']=scene([26,216,31,33][shape],[0,43,44,41][shape],[dict(label='same',destination=31,value=value),dict(label='same',destination=-5,value=0)],0 if shape%2==0 else 1);out.append(c)
 for op,bits in itertools.product(['native','available','bind','screen'],range(64)):
  c=base();c.update(op=op,identity=bool(bits&1),login_client=bool(bits&2),steam=bool(bits&4),social_client=bool(bits&8),account_client=bool(bits&16),replacement=bool(bits&32));c['account']['available']=bool(bits&8);c['account']['authenticated']=bool(bits&2);c['social'].update(available=bool(bits&2),enabled=bool(bits&4),ready=bool(bits&16));c['next']=copy.deepcopy(c['social']);c['next']['revision']=0xffffffffffffffff;out.append(c)
 for op,callback in itertools.product(['populate','text','accept','update'],['steam','social-snapshot','profile-name','social-pump']):
  c=base();c.update(op=op,reenter_on=callback,index=2);c['next']=copy.deepcopy(c['social']);c['next']['revision']+=1;c['frames']=scene(31,43,[dict(destination=-35,value=3)]);c['account'].update(party=True,leader=True);out.append(c)
 # No captured C++ expression ever evaluates -INT_MIN in this corpus.
 for c in out:
  for f in c.get('frames',[])+c.get('frames_after',[])+[c.get('view',{})]:
   assert all(e.get('value')!=-2147483648 for e in f.get('entries',[]))
 return out
def run(native,cases):
 p=subprocess.run([str(native)],input=''.join(json.dumps(c)+'\n'for c in cases),text=True,capture_output=True)
 lines=p.stdout.splitlines()
 if lines[:1]==['Shutdown handler: initialize.']and lines[-1:]==['Shutdown handler: cleanup.']:lines=lines[1:-1]
 if p.returncode or len(lines)!=len(cases):raise RuntimeError(json.dumps(dict(exit_code=p.returncode,actual=len(lines),expected=len(cases),stderr=p.stderr[-2500:],stdout=p.stdout[:1600])))
 return [json.loads(line)for line in lines]
def batches(native,cases):
 for i in range(0,len(cases),8):
  group=cases[i:i+8];yield from zip(group,run(native,group))
def controls():
 out=[]
 for op in ['native-accept','accept','update','bind','populate','text','column']:
  c=base();c['op']=op;c['next']=copy.deepcopy(c['social']);c['next']['revision']+=1
  if op=='native-accept':c['frames']=scene(31,43,[dict(destination=-83,value=8)]);c['account_after']=copy.deepcopy(c['account']);c['account_after'].update(party=True,leader=False);c['frames_after']=scene(14,0)
  if op=='accept':c['account'].update(party=True,leader=True);c['frames']=scene(31,43,[dict(destination=-35,value=3)]);c['frames_after']=scene(31,44)
  if op=='text':c['index']=0
  if op=='column':c['column']=1
  out.append(c)
 for value in [0,7,8]:
  c=base();c.update(op='accept');c['frames']=scene(31,44,[dict(destination=-35,value=value)]);out.append(c)
 c=base();c.update(op='native-accept',reply=False);c['frames']=scene(31,43,[dict(destination=-83,value=0)]);out.append(c)
 c=base();c.update(op='native-accept',reply=False);c['frames']=scene(31,43,[dict(destination=-83,value=8)]);out.append(c)
 return out
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('native',type=Path);p.add_argument('--report',type=Path,required=True);a=p.parse_args();cases=corpus();failures=[];reads=reentries=0
 for index,(case,value)in enumerate(batches(a.native,cases)):
  reads+=len(value['before']['calls']);reentries+=value['rust']['reentries']
  if value['before']!=value['rust']and len(failures)<3:failures.append(dict(index=index,case=case,actual=value))
 selected=controls();faults=[]
 for case,value in zip(selected,run(a.native,selected)):
  for index in range(len(value['before']['calls'])):
   for nonstd in [False,True]:faults.append(case|dict(fail=index,nonstd=nonstd))
 identity=0
 for case,value in batches(a.native,faults):
  identity+=int(value['rust'].get('same_object',value['rust'].get('error')=='int'and value['rust'].get('integer')==73))
  if value['before']!=value['rust']and len(failures)<3:failures.append(dict(case=case,actual=value))
 report=dict(passed=not failures,cases=len(cases),native_io_observations=reads,rust_state_reentries=reentries,fault_controls=len(faults),preserved_native_exception_identity=identity,differences=failures,former_cpp_int_min_negations_excluded=True,native_sha256=hashlib.sha256(a.native.read_bytes()).hexdigest(),scope=__doc__)
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='differences'}))
 if failures:print(json.dumps(failures[0])[:8000])
 raise SystemExit(0 if report['passed']else 1)
if __name__=='__main__':main()
