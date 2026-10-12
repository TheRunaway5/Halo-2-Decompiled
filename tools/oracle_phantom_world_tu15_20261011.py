#!/usr/bin/env python3
"""Execute untouched TU1.5 phantom/world activation instructions with declared raw inputs.

The emulator discovers sparse memory inputs. Original host i386 execution is
the final floating-point/state authority. No contact, force, motion, activation or allocator
outcome is supplied. These are procedure contracts, not live lift scenarios.
"""
import argparse,collections,csv,itertools,json,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_vehicle_contact_state_tu15_20261011 import sha
from oracle_vehicle_motion_math_20261011 import capture,declared_json
from export_mp_physics_contact_lifecycle import segments
BASE=0x1000000;STACK=BASE+0x10000;STOP=BASE+0xf000
POOL,HEADERS,BODY,TAGS,DEF,MODEL,PHMO,PHDEF,PHSHAPES,COMPONENT_POOL,COMPONENT,ARRAY,HAVOK,MOTION,MEMBERS,ENTRIES,CLOCK,VECTOR,ISLAND=(BASE+x for x in range(0x20000,0x33000,0x1000))
WORLD,ACTIVE,INACTIVE,PENDING,BODYLIST,LISTENERS,OWNER,PROPERTIES,BINDINGS,GROUPS,SHAPEBANK=(BASE+x for x in range(0x40000,0x4b000,0x1000))
def controls():
 rows=[]
 def add(entry=0x1da750,**values):
  row=dict(entry=entry,flags=0x104000,factor=1.,members=1,identity='valid',count=1,
   motion_flag=0,island=1,kind=0,body_index=0,source_index=0,coefficients=1,point=2.,
   velocity=0.5,root_type=0,exponent=0.,angular_exponent=0.,world=0,fp_control=0x37f,mxcsr=0x1f80,depth=0,top=0,root_dynamic=0,active=0,pending=-1,pending_count=0,active_count=0,capacity=8,world_listener=0,body_listener=0,property_count=1,property_index=0,property_none=0,binding_count=0,binding_existing=0,group=-1,group_member=0,group_flag=2,body_count=1,pairs=0,selector=0)
  row.update(values);rows.append(row)
 for flags,factor in itertools.product((0,0x4000,0x100000,0x104000,0x106000),(0.,-1.,0.01,1.,float('nan'))):
  add(flags=flags,factor=factor,members=0)
 for kind,body_index,identity in itertools.product((0,1),(0,-1,2),('valid','none','stale','missing_component')):
  add(kind=kind,body_index=body_index,identity=identity,count=2)
 for coefficient,point,velocity,rounding in itertools.product(range(5),(0.,2.,-3.),(0.,0.5),range(4)):
  add(coefficients=coefficient,point=point,velocity=velocity,
   fp_control=0x37f|(rounding<<10),mxcsr=0x1f80|(rounding<<13))
 for flags,active,motionflag,root_type in itertools.product((0x104000,0x106000),(0,1),(0,1),(0,1,12)):
  add(flags=flags,island=active,motion_flag=motionflag,root_type=root_type)
 for exponent in (0.5,1.,2.):add(exponent=exponent)
 for exponent in (0.5,1.,2.):add(coefficients=4,angular_exponent=exponent)
 for world in (0,1):add(0x315cb0,world=world)
 for point in (-3.,0.,2.):add(0x24570,point=point)
 for strength,speed,limit,exponent,flag,rounding in itertools.product((-2.,0.,3.),(-1.,0.,2.),(0.,1.),(0.,1.),(0,1),range(4)):
  add(0x1da530,strength=strength,speed=speed,limit=limit,exponent=exponent,flag=flag,fp_control=0x37f|(rounding<<10),mxcsr=0x1f80|(rounding<<13))
 for entry in (0x1d9080,0x1d9100,0x1d92c0):
  for motionflag,active,value in itertools.product((0,1),(0,1),(-1.,0.,3.)):
   add(entry,motion_flag=motionflag,island=active,point=value)
 # Actual world activation and installed callbacks, using declared raw records.
 for pending in (-1,0,1,2):
  for active in (0,1):add(0x315cb0,world=1,active=active,pending=pending,pending_count=3,world_listener=1,body_listener=1)
 for pending in (-1,0,1,2):add(0x3165d0,world=1,pending=pending,pending_count=3)
 for active_count in (0,2):
  for target_index in (0,1):
   for world_listener,body_listener in itertools.product((0,1),(0,1)):
    add(0x316d40,world=1,active_count=active_count,target_index=target_index,world_listener=world_listener,body_listener=body_listener,flags=0x808)
 for property_count,property_index,property_none,flags in itertools.product((0,1,3),(0,2),(0,1),(8,0x808)):
  if property_count and property_index>=property_count:continue
  add(0x2816a0,active=1,property_count=property_count,property_index=property_index,property_none=property_none,flags=flags)
 for active,body_count in itertools.product((0,1),(0,1,2)):add(0x1d9ec0,active=active,body_count=body_count)
 for kind in range(6):
  add(0x1f3460,selector=kind)
  for group in (-1,0):
   add(0x22d460,active=1,flags=0x800,binding_count=1,selector=kind,group=group,group_member=1)
 for binding_count,binding_existing in itertools.product((0,1,3),(0,1)):
  add(0x22d460,active=1,flags=0x800,binding_count=binding_count,binding_existing=binding_existing)
 for body_count,pairs in itertools.product((0,1,2),(0,1,3)):add(0x282870,body_count=body_count,pairs=pairs)
 for entry,point,rounding in itertools.product((0x1d9080,0x1d9100,0x1d92c0),(-3.,0.,2.),range(4)):
  add(entry,world=1,active=0,point=point,flags=0x808,pending=0,pending_count=2,world_listener=1,body_listener=1,
   fp_control=0x37f|(rounding<<10),mxcsr=0x1f80|(rounding<<13))
 # The updater itself skips inactive members; retain that negative contract.
 for rounding,kind in itertools.product(range(4),range(5)):
  add(world=1,active=0,flags=0x106808,coefficients=kind,pending=0,pending_count=2,
   world_listener=1,body_listener=1,fp_control=0x37f|(rounding<<10),mxcsr=0x1f80|(rounding<<13))
 return rows
def execute(paired,rows):
 assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);code_image=_Code(image,'.text');g=Uc(UC_ARCH_X86,UC_MODE_32)
 g.mem_map(0x10000,0x800000);g.mem_map(BASE,0x120000)
 for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
 sites={q:int(p,16) for p,q in paired['instruction_map'].items()};mnemonics={p:code_image.at(q).mnemonic for q,p in sites.items()}
 selected={int(p,16):q for name in ('address_map','instruction_map','entries') for p,q in paired[name].items()}
 state={};rng=random.Random(0x1da750)
 def w(p,value):g.mem_write(p,struct.pack('<I',value&0xffffffff))
 def floats(p,*values):g.mem_write(p,struct.pack('<'+'f'*len(values),*values))
 def on_code(_g,p,_n,_u):
  if p not in sites:state['exit_entry']=p;g.emu_stop()
  else:state['instructions'].append(sites[p])
 def on_read(_g,_a,p,n,_v,_u):
  state['events'].append(dict(kind='R',instruction=sites[g.reg_read(R.UC_X86_REG_EIP)],address=p,raw=bytes(g.mem_read(p,n)).hex()))
 def on_write(_g,_a,p,n,value,_u):
  state['events'].append(dict(kind='W',instruction=sites[g.reg_read(R.UC_X86_REG_EIP)],address=p,
   before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
 g.hook_add(UC_HOOK_CODE,on_code);g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_MEM_WRITE,on_write)
 cases=[]
 for index,row in enumerate(rows):
  g.mem_write(STACK-0x2000,rng.randbytes(0x2400));g.mem_write(POOL,bytes(0x13000));g.mem_write(WORLD,bytes(0xc000))
  w(0x4ecbd0,POOL);w(POOL+0x24,12);w(POOL+0x38,2);w(POOL+0x44,HEADERS)
  w(0x4f0414,TAGS);w(TAGS+8,DEF);w(TAGS+0x18,MODEL);w(TAGS+0x28,PHMO)
  w(DEF+0x38,1);w(MODEL+0x24,2);w(PHMO+0x44,PHSHAPES);w(PHMO+0x2c,PHDEF)
  g.mem_write(PHSHAPES+8,struct.pack('<h',0));w(0x52be5c,COMPONENT_POOL);w(COMPONENT_POOL+0x44,COMPONENT)
  w(0x51df54,CLOCK);g.mem_write(CLOCK,struct.pack('<I',30<<16));floats(CLOCK+4,1./30.);w(CLOCK+8,123)
  w(0x52be68,VECTOR+0x100);floats(VECTOR+0x100,9.8);floats(VECTOR,row['point'],0.25,-0.5)
  identity=0xffffffff if row['identity']=='none' else 0x20001 if row['identity']=='stale' else 0x10001
  for n in range(2):
   body=BODY+n*0x600;component=COMPONENT+n*0xa0;array=ARRAY+n*0x100;havok=HAVOK+n*0x100;motion=MOTION+n*0x200
   g.mem_write(HEADERS+n*12,struct.pack('<HBBII',1,0,row['root_type'] if n else 1,0,body))
   w(body,0);w(body+0x14,0xffffffff);w(body+0xb4,0xffffffff if n and row['identity']=='missing_component' else n)
   g.mem_write(body+0x116,struct.pack('<hhh',0x500,0,0x580))
   floats(body+0x500,1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,row['point'] if n else 0.,0.,0.)
   w(component+8,0x10000|n);w(component+0x70,array);w(component+0x74,1);w(component+0x20,0xffffffff)
   w(array+0x40,havok);w(havok+0x3c,motion);g.mem_write(havok+0x40,bytes((row['motion_flag'],)))
   w(havok+0x44,ISLAND+n*0x100 if row['island'] else 0)
   g.mem_write(ISLAND+n*0x100+0x29,b'\x01');w(ISLAND+n*0x100+0x24,0xffffffff)
   w(motion,0x41f360);floats(motion+0x40,row['velocity'],0.,0.,0.,0.,row['velocity'],0.,0.)
   floats(motion+0x70,1.,1.,1.,0.);floats(motion+0x10,1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.)
  w(COMPONENT+4,row['flags']);w(COMPONENT+0x94,MEMBERS if row['members'] else 0)
  w(MEMBERS,ENTRIES);w(MEMBERS+4,row['count']);w(MEMBERS+8,0x80000001)
  g.mem_write(ENTRIES,struct.pack('<bbbbI',row['kind'],row['body_index'],0,row['source_index'],identity))
  g.mem_write(ENTRIES+8,struct.pack('<bbbbI',0,0,0,0,0x10000))
  w(PHDEF,0);w(PHDEF+8,0xffffffff);w(PHDEF+0xc,0x7000001)
  coefficients=((0.,0.,0.,0.,0.,0.,0.,0.),(2.,1.,0.,0.,0.,0.,0.,0.),(0.,0.,2.,1.,0.,0.,0.,0.),(0.,0.,0.,0.,2.,1.,0.,0.),(0.,0.,0.,0.,0.,0.,2.,1.))[row['coefficients']]
  floats(PHDEF+0x18,row['exponent'],0.25);floats(PHDEF+0x20,*coefficients[:6]);floats(PHDEF+0x58,*coefficients[6:]);floats(PHDEF+0x54,row['angular_exponent'])
  # World vectors and listener banks are inputs, never expected outcomes.
  w(WORLD+8,ACTIVE);w(WORLD+0xc,row['active_count']);w(WORLD+0x10,row['capacity'])
  w(WORLD+0x14,INACTIVE);w(WORLD+0x18,2);w(WORLD+0x108,PENDING);w(WORLD+0x10c,row['pending_count'])
  for n in range(4):
   w(ACTIVE+n*4,ISLAND+0x300+n*0x100);w(PENDING+n*4,HAVOK if n==row['pending'] else HAVOK+0x100)
  w(WORLD+0x78,LISTENERS);w(WORLD+0x7c,3 if row['world_listener'] else 0)
  w(LISTENERS,0);w(LISTENERS+4,LISTENERS+0x100);w(LISTENERS+8,0);w(LISTENERS+0x100,0x466cbc)
  for n in range(2):
   havok=HAVOK+n*0x100;island=ISLAND+n*0x100;props=PROPERTIES+n*0x100;owner=OWNER+n*0x100
   w(INACTIVE+n*4,island);w(havok+8,WORLD if row['world'] else 0);g.mem_write(havok+6,struct.pack('<H',8))
   w(havok+0x30,props);w(havok+0x34,row['property_count']);w(havok+0x54,owner);w(owner,0x461970)
   for k in range(3):w(props+k*8,0x2001 if k==row['property_index'] else 0x7000+k);w(props+k*8+4,0xffffffff if row['property_none'] else n)
   w(havok+0x70,LISTENERS+0x200+n*0x20);w(havok+0x74,3 if row['body_listener'] else 0)
   w(LISTENERS+0x204+n*0x20,LISTENERS+0x300);w(LISTENERS+0x300,0x466c70)
   g.mem_write(island+0x29,bytes((row['active'],)));w(island+0x24,n)
   w(island+0x20,WORLD);w(island+0x3c,BODYLIST+n*0x20);w(island+0x40,row['body_count'])
   w(island+0x68,GROUPS+0x800);w(island+0x6c,row['pairs'])
   w(BODYLIST+n*0x20,havok);w(BODYLIST+n*0x20+4,HAVOK+(1-n)*0x100)
   # The component source uses 0x60 body rows, distinct from Havok bodies.
   w(COMPONENT+n*0xa0+0x74,row['body_count']);w(ARRAY+n*0x100+0xa0,HAVOK+(1-n)*0x100)
   w(COMPONENT+n*0xa0+4,row['flags']);w(COMPONENT+n*0xa0+0x7c,BINDINGS);w(COMPONENT+n*0xa0+0x80,row['binding_count'])
  for k in range(row['pairs']):w(GROUPS+0x800+k*0x10+8,OWNER+0x800+k*0x10)
  for k in range(row['binding_count']):
   g.mem_write(BINDINGS+k*12,struct.pack('<hhI',row['selector'],0,0 if row['binding_existing'] else 0xffffffff))
  table=struct.unpack('<6I',image.read(0x1f34b4,24))
  # Recover each authored shape-block offset from its selector's ADD immediate.
  for kind,target in enumerate(table):
   offset=code_image.at(target).operands[1].imm;shape=SHAPEBANK+kind*0x200
   w(PHMO+offset,1);w(PHMO+offset+4,shape);g.mem_write(shape+0x70,struct.pack('<h',row['group']))
  w(PHMO+0x34,GROUPS);w(GROUPS+8,row['group_member']);w(GROUPS+0xc,GROUPS+0x100)
  g.mem_write(GROUPS+0x100,struct.pack('<hhI',row['selector'],0,row['group_flag']))
  w(VECTOR+0x300,row['selector'])
  w(STACK,STOP);w(STACK+4,COMPONENT);floats(STACK+8,row['factor'])
  registers={}
  if row['entry']==0x24570:registers=dict(EAX=VECTOR)
  elif row['entry']==0x315cb0:
   registers=dict(ECX=HAVOK)
  elif row['entry']==0x1da530:
   registers=dict(EBX=row['flag']);floats(STACK+4,1.);w(STACK+8,row['flag']);floats(STACK+12,2.,row['strength'],row['speed'],row['limit'],0.25,row['exponent']);w(STACK+36,row['flag']);w(STACK+40,VECTOR+0x200)
  if row['entry']==0x1d9080:registers=dict(EAX=VECTOR,ECX=0)
  elif row['entry']==0x1d9100:registers=dict(EAX=0,ECX=VECTOR)
  elif row['entry']==0x1d92c0:registers=dict(ECX=0,EDX=VECTOR,ESI=COMPONENT)
  if row['entry']==0x3165d0:registers=dict(ECX=WORLD);w(STACK+4,HAVOK)
  elif row['entry']==0x316d40:registers=dict(ECX=WORLD);w(STACK+4,ISLAND+row.get('target_index',0)*0x100)
  elif row['entry']==0x2816a0:w(STACK+4,HAVOK)
  elif row['entry']==0x22d460:w(STACK+4,0)
  elif row['entry']==0x1d9ec0:registers=dict(ESI=COMPONENT)
  elif row['entry']==0x1f3460:registers=dict(EAX=PHMO,EDX=VECTOR+0x300,ECX=VECTOR+0x308)
  elif row['entry']==0x282870:w(STACK+4,ISLAND)
  for n,name in enumerate(('EAX' ,'ECX','EDX','EBX','ESP','EBP','ESI','EDI')):g.reg_write(getattr(R,'UC_X86_REG_'+name),STACK if name=='ESP' else registers.get(name,rng.getrandbits(32)))
  for n in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{n}'),rng.getrandbits(128))
  g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_FPCW,row['fp_control']);g.reg_write(R.UC_X86_REG_MXCSR,row['mxcsr'])
  g.reg_write(R.UC_X86_REG_FPSW,row['top']<<11);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
  state.update(events=[],instructions=[],exit_entry=None);initial=capture(g)
  try:g.emu_start(selected[row['entry']],0,count=30000)
  except Exception as error:raise RuntimeError(dict(case=index,input=row,eip=hex(g.reg_read(R.UC_X86_REG_EIP)))) from error
  assert state['exit_entry'] is not None,(index,state['instructions'][-10:])
  first={};current={}
  for event in state['events']:
   p=event['address'];raw=bytes.fromhex(event['raw'] if event['kind']=='R' else event['before'])
   for n,value in enumerate(raw):
    if p+n not in current:first[p+n]=value;current[p+n]=value
    assert current[p+n]==value
   if event['kind']=='W':
    for n,value in enumerate(bytes.fromhex(event['written'])):current[p+n]=value
  mask=0x7d3f
  for site in state['instructions']:
   mnemonic=mnemonics[site]
   if mnemonic in ('fld','fistp'):mask=0x3a3f
   elif mnemonic.startswith('f') and mnemonic not in ('fnstsw','fnstcw','fldcw','fnclex'):mask=0x383f
  cases.append(dict(id=index,entry=row['entry'],entry_abi=initial,exit=capture(g),exit_entry=state['exit_entry'],return_address=STOP,
   complete=state['exit_entry']==STOP,x87_defined_status_mask=mask,initial_banks=[dict(address=p,hex=b.hex()) for p,b in segments(first)],
   final_banks=[dict(address=p,hex=b.hex()) for p,b in segments(current)],instructions=state['instructions'],
   writes=[e for e in state['events'] if e['kind']=='W'],estimates=[],timestamps=[],packets=[],declared_input=declared_json(row)))
 return cases
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--correspondence',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 paired=json.loads(a.correspondence.read_text());assert paired['extension_tool_sha256']==sha(ROOT/'tools/research_phantom_world_tu15_20261011.py')
 rows=controls();cases=execute(paired,rows);a.output.mkdir(parents=True,exist_ok=False)
 fixture=dict(schema=1,image='1.5',parity_target='English TU1.5 only',cases=cases,tool_sha256=sha(__file__),correspondence_sha256=sha(a.correspondence),
  instruction_patches=[],native_expected_data_used=False,live_physics_proof=False,raw_inputs_not_source_constructed=True)
 target=a.output/'u5.json';target.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'),allow_nan=False)+'\n')
 seen={p for c in cases for p in c['instructions']}
 report=dict(schema=1,parity_target='English TU1.5 only',tool_sha256=sha(__file__),fixture_sha256=sha(target),cases=len(cases),
  complete=sum(c['complete'] for c in cases),required_owner_prefixes=sum(not c['complete'] for c in cases),
  body_coverage={f"{r['entry']:08x}":len(set(_trace(_Code(Xbe(NEW_DEFAULT),'.text'),r['entry'],set(paired['entries'].values())).code)&seen) for r in paired['tu15_new_owners']},
  stop_entries=sorted({c['exit_entry'] for c in cases if not c['complete']}),instructions=sum(len(c['instructions']) for c in cases),
  writes=sum(len(c['writes']) for c in cases),full_state_bytes=sum(len(bytes.fromhex(b['hex'])) for c in cases for b in c['final_banks']))
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
