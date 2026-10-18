"""Unchanged TU1.5 HS runner/wake/sleep, all 49 actual opening cue threads.

Declared inputs: authored scenario scripts/nodes, dormant thread banks and an
active 30 Hz game clock. Non-timing void builtins with literal-only arguments
are observed at their HS evaluator boundary and return void through original
211DA0. They do not provide a voice, object, admission or render result. This
measures wake-relative cue request ticks, not full scene or device scheduling.
"""
import sys, struct, hashlib, json, argparse
from pathlib import Path
sys.path.insert(0,str(Path.cwd()/'tools'))
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from audit_rendering import Cache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--map',type=Path,default=Path('assets/disc/maps/00a_introduction.map'))
parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT)
args=parser.parse_args()
root=args.output.resolve();root.mkdir(parents=True,exist_ok=False)
c=Cache(args.map)
scenario=next(t for t in c.tags.values() if t['group']=='scnr')['address']-c.magic
scripts=c.block(scenario+0x1b8,40)
node_count=c.u32(scenario+0x238)
node_offset=c.u32(scenario+0x23c)-c.magic
strings=c.u32(scenario+0x1b4)-c.magic
def node(datum):
 index=datum&65535
 assert index<node_count
 salt,opcode,kind,flags,next_id,text,value=struct.unpack_from('<4H3I',c.data,node_offset+20*index)
 assert salt==datum>>16
 return dict(datum=f'{datum:08x}',opcode=opcode,type=kind,flags=flags,next=next_id,string_offset=text,value=value)
def tree(datum):
 n=node(datum)
 if n['flags']&1:
  n['bits']=f"{n['value']:08x}"
 else:
  function=node(n['value']);n['call']=c.string(strings+function['string_offset'],4096)
  n['script_call']=bool(n['flags']&2);n['arguments']=[];child=function['next'];seen=set()
  while child!=0xffffffff:
   assert child not in seen;seen.add(child)
   n['arguments'].append(tree(child));child=node(child)['next']
 return n
with Path(args.map).open('rb') as stream:
 map_pin=hashlib.file_digest(stream,'sha256').hexdigest()
authored=dict(map_sha256=map_pin,scripts=[])
for i,address in enumerate(scripts):
 kind,return_type,datum=struct.unpack_from('<HHI',c.data,address+32)
 # Only dormant cue threads are part of this bounded clock comparison.
 if kind<=2:
  authored['scripts'].append(dict(index=i,name=c.string(address,32),type=kind,return_type=return_type,root=tree(datum)))
 else:authored['scripts'].append(dict(index=i,type=kind,root={}))
assert authored['map_sha256']=='5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28'
def walk(n):
 yield n
 for a in n.get('arguments',[]):yield from walk(a)
selected=[s for s in authored['scripts'] if any(n.get('call')=='sound_impulse_start'for n in walk(s['root']))]
assert len(selected)==49 and all(s['type']==1 for s in selected)
boundary={}
nodes={}
for script in selected:
 for n in walk(script['root']):
  nodes[int(n['datum'],16)]=n
  if 'call' in n and n['call'] not in ('begin','sleep'):
   assert not n['script_call'] and n['type']==4
   assert all('bits'in a and a['flags']&1 for a in n['arguments'])
   boundary[n['opcode']]=n['call']
x=Xbe(args.xbe);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN

g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
POOL,EXPRPOOL,THREADS,EXPRS,SCENARIO,SCRIPTS,TIME,GAME,STACK,STOP=(
 0x1000000,0x1001000,0x1010000,0x1080000,0x10c0000,0x10c1000,
 0x10c4000,0x10c5000,0x11fe000,0x11ff000)
g.mem_map(0x1000000,0x200000)
def w(a,v):g.mem_write(a,struct.pack('<I',v&0xffffffff))
def u(a):return struct.unpack('<I',g.mem_read(a,4))[0]
w(0x505e5c,POOL);w(POOL+0x44,THREADS)
w(0x505e6c,EXPRPOOL);w(EXPRPOOL+0x44,EXPRS)
g.mem_write(EXPRS,bytes(c.data[node_offset:node_offset+20*node_count]))
w(0x4ecc20,SCENARIO);w(SCENARIO+0x1bc,SCRIPTS)
g.mem_write(SCRIPTS,b''.join(bytes(c.data[a:a+40])for a in scripts))
w(0x51df54,TIME);g.mem_write(TIME+2,struct.pack('<h',30))
w(0x4f3218,GAME);g.mem_write(GAME+0x1120,b'\x01')
g.mem_write(0x505e60,b'\x01')
thread_indices={s['index']:i for i,s in enumerate(s for s in authored['scripts']if s['type']<=2)}
entries={struct.unpack('<I',x.read(struct.unpack('<I',x.read(0x480c00+op*4,4))[0]+4,4))[0]:op for op in boundary}
events=[];tick=0
def hook(machine,address,size,unused):
 if address not in entries:return
 sp=machine.reg_read(R.UC_X86_REG_ESP);ret=u(sp);op=u(sp+4);handle=u(sp+8)
 assert op==entries[address]
 thread=THREADS+(handle&0xffff)*0x418;frame=u(thread+0x10)
 n=nodes[u(frame+4)]
 assert n['opcode']==op and n['type']==4
 events.append(dict(tick=tick,script=u(thread+4),opcode=op,name=n['call'],arguments=[a['bits']for a in n['arguments']]))
 # Original return routine completes this void evaluator at its real frame.
 # No Xbox instruction is changed; this is an explicit host-effect boundary.
 w(sp+12,ret);machine.reg_write(R.UC_X86_REG_ESP,sp+12)
 machine.reg_write(R.UC_X86_REG_ECX,handle);machine.reg_write(R.UC_X86_REG_EAX,0)
 machine.reg_write(R.UC_X86_REG_EIP,0x211da0)
g.hook_add(UC_HOOK_CODE,hook)
def call(entry,handle,stack_arg=False):
 w(STACK,STOP)
 if stack_arg:w(STACK+4,handle)
 g.reg_write(R.UC_X86_REG_ESP,STACK);g.reg_write(R.UC_X86_REG_EAX,handle)
 g.reg_write(R.UC_X86_REG_FPCW,0x027f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
 g.emu_start(entry,STOP,count=200000)
 assert g.reg_read(R.UC_X86_REG_EIP)==STOP
 assert g.reg_read(R.UC_X86_REG_ESP)==STACK+4+(4 if stack_arg else 0)
for script in selected:
 index=thread_indices[script['index']];thread=THREADS+index*0x418
 g.mem_write(thread,struct.pack('<HBBIII',1,0,0,script['index'],0xfffffffe,0))
 w(thread+0x10,thread+0x18);w(thread+0x18,0);w(thread+0x1c,0xffffffff)
 call(0x211f40,0x10000|index,True)
 assert u(thread+8)==0
for tick in range(751):
 w(TIME+8,tick)
 for script in selected:call(0x211840,0x10000|thread_indices[script['index']])
cues=[e for e in events if e['name']=='sound_impulse_start']
assert len(cues)==49
by_script={s['index']:s for s in selected}
for e in cues:
 e['script_name']=by_script[e['script']]['name']
assert next(e['tick'] for e in cues if e['script']==66)==100
assert next(e['tick'] for e in cues if e['script']==114)==279
assert next(e['tick'] for e in cues if e['script']==115)==490
out=dict(scope=__doc__,xbe_sha256=NEW_PIN,map_sha256=authored['map_sha256'],
 unmodified_entries=['00211f40','00211840','00211b10','00210aa0','00211cb0','00211da0'],
 instruction_patches=[],supplied_host_boundaries=boundary,cues=sorted(cues,key=lambda e:e['script']),
 original_voice_or_readiness_supplied=False,full_scene_scheduling_verified=False)
(root/'report.json').write_text(json.dumps(out,indent=2)+'\n')
(root/'fixtures.tsv').write_text(''.join(f"{e['script']} {e['tick']} {e['arguments'][0]}\n"for e in out['cues']))
print('PASS unchanged TU1.5 wake/sleep/runner: all 49 actual cue threads; Pillar=100 Council=279 Truth=490, relative Prophet gap=211 ticks')
