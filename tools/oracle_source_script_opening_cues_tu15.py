"""Unchanged TU1.5 real-pool dispatcher for all 49 actual opening cue programs.

Declared inputs: original map syntax/script records; dormant selected threads;
external wake before tick 0; active 30 Hz clock. Literal-only non-timing void
effects are observed and return through original 211DA0. Object-list GC has
an empty explicit host boundary. No voice/object/full-scene scheduling result
or native audio-stall cause is supplied. No Xbox instruction is modified.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path.cwd()/'tools'))
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from audit_rendering import Cache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--xbe',default=str(NEW_DEFAULT))
parser.add_argument('--map',required=True)
parser.add_argument('--output-root',type=Path,required=True)
args=parser.parse_args();root=args.output_root;root.mkdir(parents=True,exist_ok=True)
x=Xbe(args.xbe);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
c=Cache(args.map);scenario=next(t for t in c.tags.values()if t['group']=='scnr')['address']-c.magic
script_offsets=c.block(scenario+0x1b8,40)
node_count=c.u32(scenario+0x238);node_offset=c.u32(scenario+0x23c)-c.magic
nodes=[struct.unpack_from('<HHHHIII',c.data,node_offset+i*20)for i in range(node_count)]
scripts=[dict(index=i,name=bytes(c.data[a:a+32]).split(b'\0')[0].decode(),type=c.u16(a+32),root=c.u32(a+36))for i,a in enumerate(script_offsets)]
def node(datum):
 n=nodes[datum&65535];assert n[0]==datum>>16;return n
def children(datum):
 n=node(datum)
 if n[3]&1:return []
 fn=node(n[6]);result=[];child=fn[4]
 while child!=0xffffffff:
  assert child not in result;result.append(child);child=node(child)[4]
 return result
def walk(datum):
 yield datum
 for child in children(datum):yield from walk(child)
selected=[s for s in scripts if any(not(node(d)[3]&1)and node(d)[1]==597 for d in walk(s['root']))]
assert len(selected)==49 and all(s['type']==1 for s in selected)
boundaries={27:'print',220:'unit_set_emotional_state',563:'cinematic_subtitle',597:'sound_impulse_start'}
for s in selected:
 for datum in walk(s['root']):
  n=node(datum)
  if n[3]&1:continue
  assert not(n[3]&2)
  if n[1]in(0,19):continue
  assert n[1]in boundaries and n[2]==4
  assert all(node(d)[3]&1 for d in children(datum))

g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
POOL,EXPRPOOL,THREADS,EXPRS,SCENARIO,SCRIPTS,TIME,GAME,BITMAP,STACK,STOP=(0x1000000,0x1001000,0x1010000,0x1080000,0x10c0000,0x10c1000,0x10c4000,0x10c5000,0x10c7000,0x11fe000,0x11ff000)
g.mem_map(0x1000000,0x200000)
def w(a,v):g.mem_write(a,struct.pack('<I',v&0xffffffff))
def u(a):return struct.unpack('<I',g.mem_read(a,4))[0]
w(0x505e5c,POOL);w(POOL+0x44,THREADS);w(POOL+0x24,0x418);w(POOL+0x48,BITMAP)
thread_indices={s['index']:i for i,s in enumerate(s for s in scripts if s['type']<=2)}
w(POOL+0x38,len(thread_indices))
w(0x505e6c,EXPRPOOL);w(EXPRPOOL+0x44,EXPRS)
g.mem_write(EXPRS,bytes(c.data[node_offset:node_offset+20*node_count]))
w(0x4ecc20,SCENARIO);w(SCENARIO+0x1bc,SCRIPTS)
g.mem_write(SCRIPTS,b''.join(bytes(c.data[a:a+40])for a in script_offsets))
w(0x51df54,TIME);g.mem_write(TIME+2,struct.pack('<h',30))
w(0x4f3218,GAME);g.mem_write(GAME+0x1120,b'\x01');g.mem_write(0x505e60,b'\x01')
entries={struct.unpack('<I',x.read(struct.unpack('<I',x.read(0x480c00+op*4,4))[0]+4,4))[0]:op for op in boundaries}
events=[];tick=0
def hook(machine,address,size,unused):
 if address==0x1e7180:
  sp=machine.reg_read(R.UC_X86_REG_ESP);machine.reg_write(R.UC_X86_REG_EIP,u(sp));machine.reg_write(R.UC_X86_REG_ESP,sp+4)
 elif address in entries:
  sp=machine.reg_read(R.UC_X86_REG_ESP);ret=u(sp);op=u(sp+4);handle=u(sp+8);assert op==entries[address]
  row=THREADS+(handle&65535)*0x418;datum=u(u(row+0x10)+4);assert node(datum)[1]==op
  if op==597:events.append(dict(tick=tick,script=u(row+4),sound=node(children(datum)[0])[6]))
  w(sp+12,ret);machine.reg_write(R.UC_X86_REG_ESP,sp+12);machine.reg_write(R.UC_X86_REG_ECX,handle);machine.reg_write(R.UC_X86_REG_EAX,0);machine.reg_write(R.UC_X86_REG_EIP,0x211da0)
g.hook_add(UC_HOOK_CODE,hook)
def call(entry,handle=None):
 w(STACK,STOP)
 if handle is not None:w(STACK+4,handle)
 g.reg_write(R.UC_X86_REG_ESP,STACK);g.reg_write(R.UC_X86_REG_FPCW,0x027f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
 g.emu_start(entry,STOP,count=1000000)
 assert g.reg_read(R.UC_X86_REG_EIP)==STOP and g.reg_read(R.UC_X86_REG_ESP)==STACK+4+(4 if handle is not None else 0)
for s in selected:
 i=thread_indices[s['index']];row=THREADS+i*0x418
 w(BITMAP+(i//32)*4,u(BITMAP+(i//32)*4)|(1<<(i%32)))
 g.mem_write(row,struct.pack('<HBBIII',1,0,0,s['index'],0xfffffffe,0));w(row+0x10,row+0x18);w(row+0x18,0);w(row+0x1c,0xffffffff)
 call(0x211f40,0x10000|i);assert u(row+8)==0
for tick in range(751):w(TIME+8,tick);call(0x2115d0)
assert len(events)==49
assert next(e['tick']for e in events if e['script']==66)==100
assert next(e['tick']for e in events if e['script']==114)==279
assert next(e['tick']for e in events if e['script']==115)==490
(root/'opening49.names').write_text(''.join(s['name']+'\n'for s in selected))
(root/'opening49.events').write_text(''.join(f"{e['tick']} {e['sound']}\n"for e in sorted(events,key=lambda e:(e['tick'],e['sound']))))
(root/'original-opening49-results.json').write_text(json.dumps(dict(scope=__doc__,xbe_sha256=NEW_PIN,map_sha256=hashlib.sha256(c.data).hexdigest(),supplied_host_boundaries=boundaries,instruction_patches=[],cues=events),indent=2)+'\n')
print('PASS unchanged TU1.5 normal dispatcher:49 actual cue programs; Pillar 100 Council 279 Truth 490')
