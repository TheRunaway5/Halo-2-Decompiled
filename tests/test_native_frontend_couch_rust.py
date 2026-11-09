#!/usr/bin/env python3
"""Authenticated complete modern couch/controller/identity callers vs production Rust/CXX IO.

Actual selection/update actions initialize Rust state. Controlled providers,
profile metadata, frames and rosters compare callback order, shared-pointer
ownership, failure prefixes and reentry. Only the two declared Back branches
and profile-list predicates are wrapped; the rest of the dispatcher, original
Xbox/Live providers and installed/native-runtime delivery stay unqualified.
Out-of-range selected entries/slots, null factories/workers and null options
inside the old dereference are excluded from C++ execution; Rust error controls
are separate. Scoped-enum int defaults remain in the valid comparison domain.
"""
import argparse,copy,hashlib,itertools,json,subprocess
from pathlib import Path
ID=[255,0,128]
def base():
 return dict(options=1,profiles=['','one','two','three'],login1=dict(identity=ID,authorized=True,status=3),login2=dict(identity='two',authorized=True,status=3),accounts=6,primary=dict(identity='primary',steam=True),tags=[dict(name='one',value=ID)],frames=scene())
def scene(screen=31,context=0x434f5543,entries=None,selected=0):return [dict(screen=6,context=0),dict(screen=screen,context=context,selected=selected,entries=entries or[dict(label=ID,destination=-80,value=1)])]
def corpus():
 out=[]
 for screen,context,options,bits in itertools.product([31,10,186,14,30,0xffffffff],[0,0x434f5543],[None,1,2,3],range(8)):
  c=base();c.update(op='populate',options=options,primary_login=bool(bits&1),detached=bool(bits&4));c['login1'].update(status=bits,authorized=bool(bits&2),steam=bool(bits&4));c['frames']=scene(screen,context);c['view']=copy.deepcopy(c['frames'][-1]);out.append(c)
 for index,status,bits in itertools.product([0,1,2,0xffffffff],[-1,0,1,2,3,4,99],range(8)):
  c=base();c.update(op='text',index=index);c['login1'].update(status=status,authorized=bool(bits&1),steam=bool(bits&2));
  if bits&4:c['login1']=None
  out.append(c)
 for op,status,bits in itertools.product(['mutable','identity'],[-2147483648,-1,0,1,2,3,4,5,6,7,8,9,99,2147483647],range(8)):
  c=base();c.update(op=op,status=status,primary_login=bool(bits&1),account_client=bool(bits&2),authenticated=bool(bits&4));out.append(c)
 for controller,status,bits in itertools.product([0,1,2,3,4,0xffffffff],[0,3,4,8,9],range(4)):
  c=base();c.update(op='open',controller=controller,status=status,account_client=bool(bits&1),authenticated=bool(bits&2));c['profiles'][2]='';out.append(c)
 for screen,context,options,action,bits in itertools.product([31,217,10],[0,0x434f5543],[None,1,2],[0,1,2,-1,99],range(8)):
  c=base();c.update(op='accept',options=options,factory=bool(bits&1),primary_login=bool(bits&2),status=5 if bits&4 else 3);c['login1'].update(authorized=bool(bits&4),status=1 if bits&2 else 3);c['frames']=scene(screen,context,[dict(destination=-80,value=action)]);out.append(c)
 for status,slot in itertools.product([0,1,2,3,4,99],range(4)):
  c=base();c.update(op='accept');c['login1']['authorized']=False;c['login'+str(slot)]=dict(identity='other',status=status,authorized=False);out.append(c)
 for joining,online,destination in itertools.product([1,2,3],[False,True],[-80,-5,31]):
  c=base();c.update(op='accept',options=None,joining=joining,joining_online=online,host_profile=7);c['frames']=scene(10,0,[dict(destination=destination,value=0)]);out.append(c)
 for identities,bits,shape in itertools.product([['A','B','C'],['A','A','B'],['primary','B','C'],['',None,''],[ID,ID,'B']],[0,1,2,3,4,5,6,7],range(4)):
  c=base();c.update(op='update',authenticated=bool(bits&1),account_client=bool(bits&2),populate_built=bool(bits&4),subjects=[None,'old','old','old']);
  for i,value in enumerate(identities,1):c['login'+str(i)]=dict(identity=value,authorized=bool(bits&4),status=3)
  c['frames']=[scene(),scene(186,0)+[dict(screen=186,context=0,description='new description')],scene(14,0),scene(31,0x434f5543,selected=99)][shape];out.append(c)
 for same,selected,count in itertools.product([False,True],[0,1,99,0xffffffff],[0,1,3]):
  c=base();c.update(op='update');rows=[dict(label='same',destination=-80,value=i)for i in range(count)];c['next_frame']=dict(entries=rows);c['frames']=scene(31,0x434f5543,rows if same and rows else[dict(label='old',value=1)],selected);out.append(c)
 for input_id,screen,mode,bits in itertools.product([-2147483648,-1,0,5,7,9,2147483647],[9,14,26,30,186],[1,2,3],range(4)):
  c=base();c.update(op='controller',input=input_id,mode=mode,options=None,profiles=['','occupied','',''],join=bool(bits&1),system_link_host=bool(bits&2),inputs=[5,7,-1,-1]);c['frames']=scene(screen,2 if screen==26 else 0);out.append(c)
 for op,joining,bits in itertools.product(['finish','leave','back','clear'],[None,1,2,3],range(8)):
  c=base();c.update(op=op,joining=joining,joining_online=bool(bits&1),status=5 if bits&2 else 3,workshop=bool(bits&4),controller=bits%5,subjects=[None,ID,'two','three']);c['frames']=scene(10 if joining is not None and bits&1 else 31,0 if joining is not None and bits&1 else 0x434f5543);out.append(c)
 for screen,bits in itertools.product([10,31,14,0xffffffff],range(8)):
  c=base();c.update(op='back',joining=2 if bits&1 else None,overlay=bool(bits&2),resume=bool(bits&4),options=1 if bits&2 else None);c['frames']=[dict(screen=screen,context=0)]if bits&2 else scene(screen);out.append(c)
 for op,joining,options,screen in itertools.product(['screen','guest-entry','profile-entry','selected-controller'],[None,1,3],[None,1,2],[10,193,31,0xffffffff]):
  c=base();c.update(op=op,joining=joining,options=options,screen=screen,name='one',joining_online=True);out.append(c)
 for rows in [[],[dict(controller=0)],[dict(controller=1),dict(controller=0),dict(controller=2),dict(controller=1)],[dict(controller=3,name=ID)]]:
  c=base();c.update(op='players',players=rows);out.append(c)
 # Keep a common external provider owner during intentional slot replacement:
 # the captured C++ call target must remain alive until its method returns.
 for op,callback,reenter,slot in [('populate','snapshot','replace-local',1),('populate','steam','replace-local',1),('text','snapshot','replace-local',1),('back','authorized','open-options',2),('back','cancel','open-options',2),('accept','profile-key','replace-local',1),('accept','signout','replace-account',1),('clear','cancel','replace-local',2),('update','authorized','replace-local',1),('update','snapshot','change-profile',1),('controller','snapshot','open-options',1)]:
  c=base();c.update(op=op,index=1,reenter_on=callback,reenter=reenter,reenter_slot=slot,keep_alive=True,replacement_login=dict(generation=71,identity='replacement',authorized=False,status=4));c['login1']['authorized']=False
  if op=='controller':c.update(options=None,profiles=['','one','','']);c['frames']=scene(186,0)
  if op=='back':c['login1']['authorized']=False
  out.append(c)
 # Defined short-circuit: no frame is read before the primary input rebind.
 c=base();c.update(op='controller',options=None,authenticated=False,frames=[],inputs=[-1,-1,-1,-1],input=9,join=True);out.append(c)
 return out
def run(native,cases):
 p=subprocess.run([str(native)],input=''.join(json.dumps(c)+'\n'for c in cases),text=True,capture_output=True);lines=p.stdout.splitlines()
 if lines[:1]==['Shutdown handler: initialize.']and lines[-1:]==['Shutdown handler: cleanup.']:lines=lines[1:-1]
 if p.returncode or len(lines)!=len(cases):raise RuntimeError(json.dumps(dict(exit_code=p.returncode,actual=len(lines),expected=len(cases),stderr=p.stderr[-2000:],stdout=p.stdout[:1000])))
 return [json.loads(line)for line in lines]
def batches(native,cases):
 for i in range(0,len(cases),8):
  group=cases[i:i+8];yield from zip(group,run(native,group))
def controls():
 out=[]
 for op in ['populate','text','accept','update','clear','open','controller','finish','leave','back','identity','players']:
  c=base();c.update(op=op,index=1,controller=1,profiles=['','one','',''],players=[dict(controller=1)]);c['login1']['authorized']=False
  if op=='controller':c['options']=None;c['frames']=scene(186,0)
  if op=='finish':c.update(joining=2,joining_online=True)
  out.append(c)
 for value in [0,2]:
  c=base();c.update(op='accept');c['frames']=scene(entries=[dict(value=value)]);out.append(c)
 c=base();c.update(op='accept',options=None,joining=2,joining_online=True);c['frames']=scene(10,0,[dict(destination=-80)]);out.append(c)
 return out
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('native',type=Path);p.add_argument('--report',type=Path,required=True);a=p.parse_args();cases=corpus();differences=[];reads=reentries=0
 for index,(case,value)in enumerate(batches(a.native,cases)):
  reads+=len(value['before']['calls']);reentries+=value['rust']['reentries']
  if value['before']!=value['rust']and len(differences)<3:differences.append(dict(index=index,case=case,actual=value))
 selected=controls();faults=[]
 for case,value in zip(selected,run(a.native,selected)):
  for index in range(len(value['before']['calls'])):
   for nonstd in [False,True]:faults.append(case|dict(fail=index,nonstd=nonstd))
 identity=0
 for case,value in batches(a.native,faults):
  identity+=int(value['rust'].get('same_object',value['rust'].get('error')=='int'and value['rust'].get('integer')==73))
  if value['before']!=value['rust']and len(differences)<3:differences.append(dict(case=case,actual=value))
 report=dict(passed=not differences,cases=len(cases),native_io_observations=reads,rust_state_reentries=reentries,fault_controls=len(faults),preserved_native_exception_identity=identity,differences=differences,undefined_cpp_inputs_excluded=True,scoped_enum_defaults_retained=True,actual_actions_initialize_state=True,provider_shared_pointer_ownership_observed=True,native_sha256=hashlib.sha256(a.native.read_bytes()).hexdigest(),scope=__doc__)
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='differences'}))
 if differences:print(json.dumps(differences[0])[:8000])
 raise SystemExit(0 if report['passed']else 1)
if __name__=='__main__':main()
