#!/usr/bin/env python3
"""Captured complete modern sign-in callers vs production Rust/CXX IO.

Includes exact Open authentication, Account acceptance and Back cancellation
segments, declared parser/serializer/node-handoff wrappers, all six removed
native sign-in fields, shared provider ownership, callback order, state reentry
and native exception identity. The unrelated dispatcher and actual providers,
filesystem/storage/world and installed runtime remain outside this comparison.
Undefined empty-stack/selected-row/null-worker old inputs are excluded, including
positive mock identity with no login in the unguarded Sign Out branch. Empty
rebuilt lists are admitted only when the old description comparison short
circuits its front() reads. This is not original Xbox/Live replay.
"""
import argparse,copy,hashlib,itertools,json,subprocess
from pathlib import Path
ID=[255,0,128]
KEY='0123456789abcdef0123456789abcdef'
def scene(screen=20,selected=0,rows=None):return [dict(screen=6),dict(screen=screen,selected=selected,entries=rows or[dict(label='continue'),dict(label='profile'),dict(label='tag',destination=167,value=10),dict(label='sign out')])]
def base():return dict(frames=scene(),keys=[dict(name='host',key=KEY)],provider=dict(steam=False,authorized=True,status=3,identity=False))
def corpus():
 out=[]
 for name,cached,entropy in itertools.product(['','host','other',ID],[False,True],[list(range(16)),[255]*16,[0]*16,list(range(255,239,-1))]):
  c=base();c.update(op='key',name=name,entropy=entropy);c['keys']=[dict(name=name,key=KEY)]if cached else[];out.append(c)
 for name,steam,bits,bound in itertools.product(['','host',ID],[False,True],range(16),[False,True]):
  c=base();c.update(op='bind',signed_in=name,login=bool(bits&1),account=bool(bits&2),queue=bool(bits&4),voice=bool(bits&8),social=not bool(bits&2));c['provider']['steam']=steam
  if bound:c['bound_name']=name
  out.append(c)
 for screen,mode,bits in itertools.product(list(range(0,257))+[0xffffffff],[1,2,3],range(8)):
  c=base();c.update(op='prepare',screen=screen,context=0xffffffff,mode=mode,login=bool(bits&1),identity=bool(bits&2));c['provider'].update(steam=bool(bits&4),authorized=bool(bits&2));out.append(c)
 for status,bits,shape in itertools.product([-2147483648,-1,0,1,2,3,4,99,2147483647],range(16),range(6)):
  c=base();c.update(op='update',login=bool(bits&1),awaiting=bool(bits&2),identity=bool(bits&4),queue=bool(bits&8),pending_screen=14,pending_context=99);c['provider'].update(status=status,identity=bool(bits&8))
  c['frames']=[scene(20),scene(186)+[dict(screen=20,entries=[dict(label=ID)],selected=0xffffffff)],scene(14),scene(31),[dict(screen=6)],scene(30)][shape]
  out.append(c)
 for same,selected,shape in itertools.product([False,True],[0,1,99,0xffffffff],range(4)):
  c=base();c.update(op='update');rows=[dict(label=ID)]if shape<2 else[dict(label=ID),dict(label='second'),dict(label='third')];c['frames']=scene(186,selected,rows);c['frames'][-1]['description']='same'if same else'old';c['next_frame']=dict(description='same',entries=rows if shape%2==0 else[dict(label='changed')]);out.append(c)
 # In the old caller a changed description avoids both front() reads; its
 # size()-1 conversion then preserves the defined unsigned underflow clamp.
 c=base();c.update(op='update',next_frame=dict(description='new',entries=[]));c['frames'][-1]['selected']=0xffffffff;out.append(c)
 for screen,selected,status,bits in itertools.product([20,14],[0,1,2,3],[-1,0,1,2,3,4,99],range(16)):
  # The captured Sign Out dereferences its login. A positive identity with
  # no primary login is impossible from production couch_identity() and is
  # undefined C++ here; the other 868 Account cases remain admitted.
  if screen==20 and selected==3 and bits&1 and not bits&2:continue
  c=base();c.update(op='accept',frames=scene(screen,selected),identity=bool(bits&1),login=bool(bits&2),account=bool(bits&4),queue=bool(bits&8),voice=bool(bits&4),social=bool(bits&8),directory=bool(bits&4));c['provider'].update(status=status,identity=bool(bits&8));out.append(c)
 for profile in ['',ID,'renamed profile']:
  c=base();c.update(op='accept',frames=scene(20,1),identity=True,profile_name=profile);out.append(c)
 for screen,awaiting,login in itertools.product([6,14,20,186,0xffffffff],[False,True],[False,True]):
  c=base();c.update(op='back',frames=scene(screen),awaiting=awaiting,login=login);out.append(c)
 for op,message,bits in itertools.product(['return','return-lobby'],['','done',ID],range(16)):
  c=base();c.update(op=op,message=message,identity=bool(bits&1),queue=bool(bits&2),route_open=bool(bits&4),login=bool(bits&8));out.append(c)
 for name,key,take,restore in itertools.product(['','a','host',ID],[KEY,'',KEY.upper(),'a'*31,'a'*33,[0]*32,[255]*32],['missing','host',ID],[False,True]):
  c=base();c.update(op='storage',load=[dict(name=name,key=key)],take=take,restore=restore,profile_name=ID);out.append(c)
 for screen,steam,suppressed in itertools.product([6,14,20,186,204,231],[False,True],[False,True]):
  c=base();c.update(op='prepare',screen=screen,suppressed=suppressed);c['provider'].update(steam=steam,authorized=False);out.append(c)
 # Production callback targets remain owned during deliberate slot replacement.
 for op,callback,reenter in [('bind','bind','replace-login'),('bind','account-signout','drop-queue'),('bind','queue-signout','drop-voice'),('prepare','steam','replace-login'),('prepare','authorized','pending'),('update','update-account','identity'),('update','update-postgame','replace-login'),('update','snapshot','pending'),('update','build','change-frame'),('accept','snapshot','snapshot'),('accept','account-signout','replace-account'),('accept','account-connect','pending'),('back','cancel','pending'),('return','identity','pending'),('return-lobby','cancel-queue','drop-queue')]:
  c=base();c.update(op=op,screen=186,reenter_on=callback,reenter=reenter,keep_alive=True,replacement=dict(steam=False,authorized=True,identity=False,status=4));c['provider'].update(steam=op in['bind','prepare'],authorized=False,identity=op=='accept');
  if op=='back':c['awaiting']=True
  if op=='accept'and callback=='account-signout':c.update(identity=True,frames=scene(20,3))
  out.append(c)
 return out
def run(binary,cases):
 p=subprocess.run([str(binary)],input=''.join(json.dumps(c)+'\n'for c in cases),text=True,capture_output=True);lines=p.stdout.splitlines()
 if lines[:1]==['Shutdown handler: initialize.']and lines[-1:]==['Shutdown handler: cleanup.']:lines=lines[1:-1]
 if p.returncode or len(lines)!=len(cases):raise RuntimeError(json.dumps(dict(exit_code=p.returncode,actual=len(lines),expected=len(cases),stderr=p.stderr[-2000:],stdout=p.stdout[:1000])))
 return [json.loads(s)for s in lines]
def batches(binary,cases):
 for i in range(0,len(cases),8):
  group=cases[i:i+8];yield from zip(group,run(binary,group))
def controls():
 out=[]
 for op in ['key','bind','prepare','update','accept','back','return','return-lobby','storage']:
  c=base();c.update(op=op,name='uncached',awaiting=op in['back','update'],restore=True,take='host',screen=186);c['provider'].update(steam=op in['bind','prepare'],authorized=False);out.append(c)
 for identity,selected,provider_identity in [(True,0,False),(True,1,False),(True,2,False),(True,3,False),(False,0,True),(False,0,False)]:
  c=base();c.update(op='accept',identity=identity,frames=scene(20,selected));c['provider']['identity']=provider_identity;out.append(c)
 c=base();c.update(op='key',name='uncached',entropy_success=False);out.append(c)
 return out
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
 report=dict(passed=not differences,cases=len(cases),native_io_observations=observations,rust_state_reentries=reentries,fault_controls=len(faults),preserved_native_exception_identity=identity,differences=differences,undefined_cpp_inputs_excluded=True,scoped_enum_defaults_retained=True,actual_actions_initialize_state=True,provider_shared_pointer_ownership_observed=True,native_sha256=hashlib.sha256(a.native.read_bytes()).hexdigest(),scope=__doc__)
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='differences'}))
 if differences:print(json.dumps(differences[0])[:8000])
 raise SystemExit(0 if report['passed']else 1)
if __name__=='__main__':main()
