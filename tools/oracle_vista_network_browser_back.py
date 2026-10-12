#!/usr/bin/env python3
"""Execute unchanged PC13 Back, reason2 dispatcher and actual PC6 factory.

Actual widget virtual channel/window, lookup, profile initialization, allocator,
base/list construction, presence, registration and optional dialog are declared
external services. No native screen, account, loader or rendering is invented.
"""
from pathlib import Path
import hashlib,itertools,json,struct,sys
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/browser-back-original'
sys.path.insert(0,str(ROOT/'analysis/frontend/vista-session-flow-audit/local-session-inputs-original'))
from audit import PE,EXE,EXE_SHA
CHANNEL=STOP+0x100;WINDOW=STOP+0x110;VTABLE=OBJ+0x3000
EVENT=OBJ+0x4000;SCREEN=OBJ+0x5000;DESCRIPTOR=OBJ+0x7000

def execute(image,q):
 u=machine(image);u.mem_map(0,0x1000);events=[];visits=0
 w32(u,OBJ,VTABLE);w32(u,VTABLE+0x80,CHANNEL);w32(u,VTABLE+0x84,WINDOW)
 u.mem_write(OBJ+0x1f00,bytes([q['modal']]))
 u.mem_write(EVENT,struct.pack('<III',5,0,q['component']))
 u.mem_write(0xd718e4,bytes([q['initialized']]));w32(u,0xd71904,q['first_clock'])
 w32(u,0xd71900,q['clock']);u.mem_write(0xd73a8d,bytes([q['special']]))
 u.mem_write(0xd718e8,b'\0');u.mem_write(0xd758e0,b'\0')
 for i,flags in enumerate(q['flags']):w32(u,0xd6c858+i*0x127c,flags)
 u.mem_write(0x86ee9f,bytes([q['report']]));u.mem_write(0x86eea0,b'\x83')
 w32(u,0x86eee0,0x81234567);u.mem_write(0x86eea4,bytes(range(60)))
 u.mem_write(0x86ee9d,b'\x81');u.mem_write(0x882200,b'\x82')
 u.mem_write(0x8b00e8,bytes((i*37+11)&255 for i in range(0x840)))
 def ret(v=None,pop=0):
  sp=u.reg_read(UC_X86_REG_ESP)
  if v is not None:u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def descriptor(at):return [u32(u,at+i*4) for i in range(8)]
 def hook(_u,pc,_n,_):
  nonlocal visits
  visits+=1;sp=u.reg_read(UC_X86_REG_ESP);this=u.reg_read(UC_X86_REG_ECX)
  if pc==STOP:u.emu_stop();return
  if pc==CHANNEL:events.append(['channel',q['channel']]);ret(q['channel']);return
  if pc==WINDOW:events.append(['window',q['window']]);ret(q['window']);return
  if pc==0x609850:
   events.append(['present',*[u32(u,sp+4+i*4)for i in range(3)],q['present']]);ret(q['present']);return
  if pc==0x6096da:events.append(['close',u32(u,sp+4),u32(u,sp+8)]);ret();return
  if pc==0x60eebe:events.append(['base',q['base']]);ret(q['base'],4);return
  if pc==0x60ce70:
   events.append(['reason',u32(u,sp+4)])
   if q['op']=='back':ret();return
  if pc==0x608690:events.append(['initialize_profiles']);ret();return
  if pc==0x6073ae:events.append(['count']) #Actual complete controller count.
  if pc==0x60758d:events.append(['mask']) #Actual complete mask.
  if pc in (0x40b7b9,0x63f6b7):
   events.append(['factory',*descriptor(u32(u,sp+4))])
   if q['op']!='joined':ret();return
   assert pc==0x40b7b9
  if pc==0x60d2d8:events.append(['allocate',u32(u,sp+4),u32(u,sp+8)]);ret(SCREEN);return
  if pc==0x611159:
   args=[u32(u,sp+4+i*4)for i in range(5)]
   events.append(['screen_construct',*args[:4],args[4]-SCREEN]);ret(this,20);return
  if pc==0x40b5a8:events.append(['list_construct',u32(u,sp+4)]);ret(this,4);return
  if pc==0x5b07a0:events.append(['presence',u32(u,sp+4)]);ret();return
  if pc==0x60b8c3:
   widget=u32(u,sp+4);events.append(['register',widget-SCREEN,*descriptor(u32(u,sp+8)),u.mem_read(widget+0x6c,1)[0]]);ret();return
  if pc==0x6161e1:
   events.append(['report_dialog',u32(u,sp+4)-0x86eea4,u32(u,sp+8),u32(u,sp+12)]);ret();return
  if pc==0x464551:events.append(['clear_pregame']) #Actual caller and arguments execute.
  if pc==0x687ba9:
   dst,value,n=[u32(u,sp+4+i*4)for i in range(3)];assert (dst,value,n)==(0x8b00e8,0,0x840)
   u.mem_write(dst,bytes([value&255])*n);ret(dst);return
  if pc==0x439457:events.append(['input_flag',u32(u,sp+4),u32(u,sp+8)]) #Actual store executes.
 u.hook_add(UC_HOOK_CODE,hook)
 sp=STACK+0x18000;w32(u,sp,STOP)
 if q['op']=='dispatch':w32(u,sp+4,2);entry=0x60ce70
 else:w32(u,sp+4,EVENT);entry=0x619da0
 u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,OBJ)
 u.emu_start(entry,STOP+1,count=100000)
 assert u.reg_read(UC_X86_REG_EIP)==STOP
 return dict(input=q,events=events,returned=u.reg_read(UC_X86_REG_EAX)&255,
  initialized=u.mem_read(0xd718e4,1)[0],first_clock=u32(u,0xd71904),
  special=u.mem_read(0xd73a8d,1)[0],report=u.mem_read(0x86ee9f,1)[0],
  report_argument=u.mem_read(0x86eea0,1)[0],menu_flag=u.mem_read(0x86ee9d,1)[0],
  input_flag=u.mem_read(0x882200,1)[0],
  pregame_sha256=hashlib.sha256(bytes(u.mem_read(0x8b00e8,0x840))).hexdigest(),visits=visits)

def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
 rows=[]
 def add(**args):
  q=dict(op='back',modal=0,component=1,channel=5,window=4,present=0,base=0,
   initialized=1,first_clock=0x12345678,clock=0x87654321,special=0x80,
   flags=[8,0,0,0],report=0);q.update(args);rows.append(execute(image,q))
 for component,modal,channel,window,present,base in itertools.product((1,13),(0,1,0x80),(1,5),(2,4),(0,0x80),(0,1,0x80)):
  add(component=component,modal=modal,channel=channel,window=window,present=present,base=base)
 for mask,initialized,clock,special in itertools.product(range(16),(0,1,0x80),(0,0x80000001,0xffffffff),(0,0x80)):
  add(op='dispatch',initialized=initialized,clock=clock,special=special,
   flags=[0x80000001|(8 if mask&(1<<i)else 0)for i in range(4)])
 for mask,initialized,report,component in itertools.product((1,2,4,8),(0,1,0x80),(0,1,0x80),(1,13)):
  add(op='joined',initialized=initialized,report=report,component=component,
   flags=[0x80000001|(8 if mask&(1<<i)else 0)for i in range(4)])
 OUT.mkdir(parents=True,exist_ok=True)
 f=ROOT/'tests/fixtures/vista_network_browser_back_original.json'
 f.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 pe=PE(EXE,EXE_SHA)
 spans={name:pe.span(a,b)for name,a,b in (
  ('browser_event',0x619da0,0x619f5d),('reason_dispatch',0x60ce70,0x60d010),
  ('mainmenu_constructor',0x40b6ca,0x40b73e),('mainmenu_factory',0x40b7b9,0x40b88c),
  ('pregame_clear',0x464551,0x464566),('input_flag',0x439457,0x439466))}
 for name,span in spans.items():
  (OUT/(name+'.bin')).write_bytes(bytes.fromhex(span['bytes_hex']))
  (OUT/(name+'.asm')).write_text('\n'.join(f"{i['address']} {i['mnemonic']} {i['operands']}"for i in span['instructions'])+'\n')
 report=dict(source_sha256=SHA,cases=len(rows),instructions=sum(r['visits']for r in rows),
  groups={op:sum(r['input']['op']==op for r in rows)for op in ('back','dispatch','joined')},
  fixture_sha256=hashlib.sha256(f.read_bytes()).hexdigest(),spans=spans,
  limits=__doc__,factory_screen=6,constructor_call='40B70A push6 ->611159',
  event_mapping='619DA0 type5,component1/13; eventtype6 is selection branch619E0E',
  native_scope='Back and reason2 single-controller descriptor only; actual factory allocation/rendering and external services remain separately owned')
 (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:report[k]for k in ('cases','instructions','groups','fixture_sha256','factory_screen')}))
if __name__=='__main__':main()
