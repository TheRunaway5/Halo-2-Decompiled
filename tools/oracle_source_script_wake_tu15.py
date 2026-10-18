"""Unchanged TU1.5 normal HS dispatch with declared synthetic script/thread banks.

The actual 2115D0 dispatcher, 16EA60 pool traversal, 211B10/211CB0/211DA0
continuations, 210AA0 sleep, 210F00 wake, 212130 lookup, and 211F40 wake
helper execute without instruction patches. Literal-only print is an explicit
non-timing host boundary, returning through original 211DA0. The dispatcher
tail's object-list GC is an explicit no-object-list fixture boundary. No voice,
object, render, game-tick admission, or complete campaign result is supplied.
"""
import argparse, hashlib, json, struct, sys
from pathlib import Path
sys.path.insert(0, str(Path.cwd()/'tools'))
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--xbe',default=str(NEW_DEFAULT),help='owned original English TU1.5 executable')
parser.add_argument('--output-root',type=Path,required=True)
args=parser.parse_args()
ROOT=args.output_root;ROOT.mkdir(parents=True,exist_ok=True)
NONE=0xffffffff
OPS={'begin':0,'=':13,'sleep':19,'sleep_forever':20,'sleep_until':21,'wake':22,'print':27}
class Program:
 def __init__(self):self.nodes=[];self.scripts=[];self.strings=bytearray();self.prints={}
 def string(self,s):
  at=len(self.strings);self.strings.extend(s.encode()+b'\0');return at
 def add(self,op,type,flags,next=NONE,string=0,value=0):
  i=len(self.nodes);self.nodes.append([0xe000+i,op,type,flags,next,string,value]);return ((0xe000+i)<<16)|i
 def literal(self,value,type=7):return self.add(type,type,9,value=value&NONE)
 def call(self,name,*args):
  children=[a if isinstance(a,int) else self.expr(a) for a in args]
  for a,b in zip(children,children[1:]):self.nodes[a&65535][4]=b
  offset=self.string(name);op=OPS[name]
  fn=self.add(op,2,9,next=children[0] if children else NONE,string=offset)
  return self.add(op,5 if name=='=' else 4,8,string=offset,value=fn)
 def expr(self,e):
  name,*args=e
  if name=='short':return self.literal(args[0])
  if name=='bool':return self.literal(args[0],5)
  if name=='script':return self.literal(args[0],21)
  if name=='begin_bool':
   result=self.call('begin',*args);self.nodes[result&65535][2]=5;return result
  if name=='print':
   offset=self.string(args[0]);arg=self.add(9,9,9,string=offset)
   result=self.call(name,arg);self.prints[result]=args[0];return result
  return self.call(name,*args)
 def script(self,name,type,*expressions):
  root=self.expr(('begin',*expressions));self.scripts.append(dict(name=name,type=type,root=root))
 def binary(self):
  strings=bytes(self.strings)
  return (b'HSW1'+struct.pack('<III',len(self.nodes),len(self.scripts),len(strings))+
   b''.join(struct.pack('<HHHHIII',*n) for n in self.nodes)+
   b''.join(s['name'].encode().ljust(32,b'\0')+struct.pack('<HHI',s['type'],4,s['root']) for s in self.scripts)+strings)

def case(name,rows,ticks=105):
 p=Program()
 for row in rows:p.script(*row)
 return dict(name=name,program=p,ticks=ticks)
S=lambda n:('sleep',('short',n))
W=lambda n:('wake',('script',n))
E=lambda n,target:('sleep',('short',n),('script',target))
P=lambda name:('print',name)
CASES=[
 case('wake_none', [('driver',0,W(-1),P('driver'))],3),
 case('wake_absent_ordinal', [('driver',0,W(32767),P('driver'))],3),
 case('external_none_target', [('driver',0,E(3,-1),P('driver'))],8),
 case('external_absent_ordinal', [('driver',0,E(3,32767),P('driver'))],8),
 case('later_dormant', [('driver',0,W(1),P('driver')),('target',1,P('target'))],3),
 case('earlier_dormant', [('target',1,P('target')),('driver',0,W(0),P('driver'))],3),
 case('later_dormant_sleep', [('driver',0,W(1),P('driver')),('target',1,S(5),P('target'))],8),
 case('absent_static', [('driver',0,W(1),P('driver')),('target',3,P('target'))],3),
 case('completed_startup', [('target',0,P('target')),('driver',0,S(1),W(0),P('driver'))],4),
 case('external_restore_wait', [('target',0,S(100),P('target')),('driver',0,S(1),E(3,0),W(0),P('driver'))]),
 case('external_restore_dormant', [('target',1,P('target')),('driver',0,S(1),E(3,0),W(0),P('driver'))],8),
 case('external_repeated_restore', [('target',0,S(100),P('target')),('driver',0,S(1),E(3,0),E(6,0),W(0),P('driver'))]),
 case('external_delayed_restore_wait', [('target',0,S(100),P('target')),('driver',0,S(1),E(3,0),S(1),W(0),P('driver'))]),
 case('external_delayed_restore_dormant', [('target',1,P('target')),('driver',0,S(1),E(3,0),S(1),W(0),P('driver'))],10),
 case('external_expire_wait', [('target',0,S(100),P('target')),('driver',0,S(1),E(3,0),P('driver'))],8),
 case('external_expire_dormant', [('target',1,P('target')),('driver',0,S(1),E(3,0),P('driver'))],8),
 case('external_own_target', [('driver',0,E(3,0),P('driver'))],8),
 case('external_completed', [('target',0,P('target')),('driver',0,S(1),E(3,0),W(0),P('driver'))],8),
 case('sleep_forever_wake', [('target',0,('sleep_forever',),P('target')),('driver',0,S(1),W(0),P('driver'))],5),
 case('sleep_until_wake', [('target',0,('sleep_until',('bool',0),('short',5)),P('target')),('driver',0,S(1),W(0),P('driver'))],10),
 case('sleep_until_external_restore', [('target',0,('sleep_until',('bool',0),('short',5)),P('target')),('driver',0,S(1),E(3,0),W(0),P('driver'))],10),
 case('sleep_until_predicate_wake', [('target',0,('sleep_until',('=',('short',0),('short',1)),('short',5)),P('target')),('driver',0,S(1),W(0),P('driver'))],10),
 case('sleep_until_self_wake', [('target',0,('sleep_until',('begin_bool',W(0),('bool',0)),('short',5)),P('target'))],10),
]
x=Xbe(args.xbe);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
POOL,EXPRPOOL,THREADS,EXPRS,SCENARIO,SCRIPTS,TIME,GAME,BITMAP,STACK,STOP=(
 0x1000000,0x1001000,0x1010000,0x1080000,0x10c0000,0x10c1000,
 0x10c4000,0x10c5000,0x10c7000,0x11fe000,0x11ff000)
results=[]
for c in CASES:
 p=c['program'];g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
 g.mem_map(0x1000000,0x200000)
 def w(a,v):g.mem_write(a,struct.pack('<I',v&NONE))
 def u(a):return struct.unpack('<I',g.mem_read(a,4))[0]
 def signed(v):return v if v<0x80000000 else v-0x100000000
 w(0x505e5c,POOL);w(POOL+0x44,THREADS);w(POOL+0x24,0x418)
 threads=[s for s in p.scripts if s['type']<=2]
 w(POOL+0x38,len(threads));w(POOL+0x48,BITMAP);w(BITMAP,(1<<len(threads))-1)
 w(0x505e6c,EXPRPOOL);w(EXPRPOOL+0x44,EXPRS)
 g.mem_write(EXPRS,b''.join(struct.pack('<HHHHIII',*n) for n in p.nodes))
 w(0x4ecc20,SCENARIO);w(SCENARIO+0x1bc,SCRIPTS)
 g.mem_write(SCRIPTS,b''.join(s['name'].encode().ljust(32,b'\0')+struct.pack('<HHI',s['type'],4,s['root']) for s in p.scripts))
 w(0x51df54,TIME);g.mem_write(TIME+2,struct.pack('<h',30))
 w(0x4f3218,GAME);g.mem_write(GAME+0x1120,b'\x01');g.mem_write(0x505e60,b'\x01')
 for i,s in enumerate(threads):
  row=THREADS+i*0x418;script=p.scripts.index(s)
  g.mem_write(row,struct.pack('<HBBIII',1,0,0,script,-2&NONE if s['type']==1 else 0,0))
  w(row+0x10,row+0x18);w(row+0x18,0);w(row+0x1c,NONE)
 def rows():
  return [dict(script=u(THREADS+i*0x418+4),flags=g.mem_read(THREADS+i*0x418+3,1)[0],
   wake=signed(u(THREADS+i*0x418+8)),saved_wake=signed(u(THREADS+i*0x418+12)),
   frame_offset=u(THREADS+i*0x418+16)-(THREADS+i*0x418)) for i in range(len(threads))]
 events=[];transitions=[];tick=0;wake_pending=[]
 def hook(machine,address,size,unused):
  if address==0x211f40:
   sp=machine.reg_read(R.UC_X86_REG_ESP);handle=u(sp+4);wake_pending.append((handle,rows()))
  elif address==0x210f63 and wake_pending:
   handle,before=wake_pending.pop();transitions.append(dict(tick=tick,kind='wake',target=handle&65535,before=before,after=rows()))
  elif address==0x210c5e:
   transitions.append(dict(tick=tick,kind='sleep_completed',rows=rows()))
  elif address==0x1e7180:
   # No object lists exist in these declared programs. Timing dispatch and
   # full return-stack cleanup have completed before this unrelated GC tail.
   sp=machine.reg_read(R.UC_X86_REG_ESP)
   machine.reg_write(R.UC_X86_REG_EIP,u(sp));machine.reg_write(R.UC_X86_REG_ESP,sp+4)
  elif address==0x2134d0:raise AssertionError('unexpected evaluator fault')
  elif address==print_entry:
   sp=machine.reg_read(R.UC_X86_REG_ESP);ret=u(sp);assert u(sp+4)==27
   handle=u(sp+8);row=THREADS+(handle&65535)*0x418;frame=u(row+0x10);node=u(frame+4)
   events.append(dict(tick=tick,script=u(row+4),label=p.prints[node]))
   w(sp+12,ret);machine.reg_write(R.UC_X86_REG_ESP,sp+12)
   machine.reg_write(R.UC_X86_REG_ECX,handle);machine.reg_write(R.UC_X86_REG_EAX,0)
   machine.reg_write(R.UC_X86_REG_EIP,0x211da0)
 print_entry=struct.unpack('<I',x.read(struct.unpack('<I',x.read(0x480c00+27*4,4))[0]+4,4))[0]
 g.hook_add(UC_HOOK_CODE,hook)
 states=[]
 for tick in range(c['ticks']):
  w(TIME+8,tick);w(STACK,STOP)
  g.reg_write(R.UC_X86_REG_ESP,STACK);g.reg_write(R.UC_X86_REG_FPCW,0x027f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
  try:g.emu_start(0x2115d0,STOP,count=500000)
  except Exception:
   print(c['name'],tick,hex(g.reg_read(R.UC_X86_REG_EIP)),rows());raise
  assert g.reg_read(R.UC_X86_REG_EIP)==STOP
  assert g.reg_read(R.UC_X86_REG_ESP)==STACK+4
  states.append(dict(tick=tick,rows=rows()))
 assert not wake_pending
 data=p.binary();(ROOT/(c['name']+'.hsw')).write_bytes(data)
 (ROOT/(c['name']+'.events')).write_text(''.join(f"{e['tick']} {e['label']}\n" for e in events))
 (ROOT/(c['name']+'.states')).write_text(''.join(f"{s['tick']} {r['script']} {r['wake']} {int(bool(r['flags']&2))} {r['saved_wake']}\n" for s in states for r in s['rows']))
 result=dict(name=c['name'],ticks=c['ticks'],program_sha256=hashlib.sha256(data).hexdigest(),events=events,transitions=transitions,states=states)
 results.append(result)
 print(c['name'],[(e['tick'],e['label']) for e in events])
out=dict(scope=__doc__,xbe_sha256=NEW_PIN,original_entries=['002115d0','0016ea60','00211b10','00211cb0','00211da0','00210aa0','00210a10','00210f00','00212130','00211f40'],instruction_patches=[],cases=results)
(ROOT/'original-results.json').write_text(json.dumps(out,indent=2)+'\n')
(ROOT/'cases.tsv').write_text(''.join(f"{c['name']} {c['ticks']}\n"for c in results))
