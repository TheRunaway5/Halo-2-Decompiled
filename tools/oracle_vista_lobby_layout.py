#!/usr/bin/env python3
"""Unchanged PC14 registration, pane selection and button visibility owners.

Only declared UI child, focus, host/countdown/count and factory flag services
are controlled. No native UI, live session, renderer, audio or account runs.
"""
from pathlib import Path
import hashlib,itertools,json,struct,sys
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBP,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_ESI
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/pc14-layout-original'
WIDGETS=(0xa5c,0xb5c,0xc5c,0x12a4,0xd5c)
TEXT=OBJ+0x9000
FAVORITE=OBJ+0x9100
VTABLE=OBJ+0x9200
FOCUS_SERVICE=OBJ+0x9300

def signed(v):return struct.unpack('<i',struct.pack('<I',v&0xffffffff))[0]
def half(u,a):return struct.unpack('<h',u.mem_read(a,2))[0]
def rect(u,a):return list(struct.unpack('<4h',u.mem_read(a,8)))
def execute(image,q):
 u=machine(image);events=[];visits=0;focus=q['focus'];registration=None;caption=None
 sp=STACK+0x18000
 u.mem_write(OBJ+0x42,bytes([q['flags']]))
 u.mem_write(OBJ+0x3484,struct.pack('<h',q['pane']))
 u.mem_write(OBJ+0x1c6a,bytes([q['chat']]))
 u.mem_write(OBJ+0x13a4,bytes([q['before_allowed']]))
 w32(u,OBJ+0x13a8,0 if q['remembered'] is None else OBJ+WIDGETS[q['remembered']])
 w32(u,OBJ,VTABLE);w32(u,VTABLE+0x24,FOCUS_SERVICE)
 for i,off in enumerate(WIDGETS):
  w32(u,OBJ+off+4,3)
  u.mem_write(OBJ+off+0x6e,bytes([q['visible'][i],q['enabled'][i]]))
  u.mem_write(OBJ+off+0x20,struct.pack('<4h',*q['widget_bounds'][i]))
 for i,e in enumerate(q['edits']):u.mem_write(OBJ+0x1c6c+i*4,bytes([e]))
 u.mem_write(TEXT+0x6e,bytes([q['text_visible']]))
 u.mem_write(TEXT+0x20,struct.pack('<4h',*q['text_bounds']))
 u.mem_write(OBJ+0x3584,struct.pack('<4h',*q['normal_bounds']))
 u.mem_write(OBJ+0x358c,struct.pack('<4h',*q['host_bounds']))
 u.mem_write(0xd73abc,bytes([q['factory']]))
 u.mem_write(FAVORITE+0x6e,bytes([q['favorite_visible']]))
 def widget(a):
  if a==OBJ:return 5
  assert a-OBJ in WIDGETS,hex(a)
  return WIDGETS.index(a-OBJ)
 def ret(v=0,pop=0):
  s=u.reg_read(UC_X86_REG_ESP)
  u.reg_write(UC_X86_REG_EAX,v&0xffffffff);u.reg_write(UC_X86_REG_EIP,u32(u,s));u.reg_write(UC_X86_REG_ESP,s+4+pop)
 def hook(_u,pc,_n,_):
  nonlocal visits,focus,registration,caption
  visits+=1;s=u.reg_read(UC_X86_REG_ESP);c=u.reg_read(UC_X86_REG_ECX)
  if pc==stop:u.emu_stop()
  elif pc==0x6152b0:events.append(['host',q['host']]);ret(q['host'])
  elif pc==0x616ba2:events.append(['active',q['active']]);ret(q['active'])
  elif pc==0x60b365:events.append(['factory',q['factory']]) # Actual getter executes.
  elif pc==0x61525a:events.append(['count',q['count']]);ret(q['count'])
  elif pc==0x611973:
   i=signed(u32(u,s+4));events.append(['text',i,q['text_exists']]);ret(TEXT if q['text_exists'] else 0,4)
  elif pc==0x6116d2:
   events.append(['bounds',widget(c) if c!=TEXT else 6,rect(u,u32(u,s+4))]) # Actual setter executes.
  elif pc==0x612152:
   i=widget(c);assert u32(u,s+4)==0
   events.append(['focused',i,int(focus==i)]);ret(int(focus==i),4)
  elif pc==0x612660:
   i=widget(c);a=[u32(u,s+4),u32(u,s+8)&255];events.append(['focus_change',i,*a])
   if a[1]:focus=i
   elif focus==i:focus=None
   ret(0,8)
  elif pc==FOCUS_SERVICE:
   i=widget(u32(u,s+4));events.append(['activate',i]);focus=i;ret(0,4)
  elif pc==0x615ea9:events.append(['enum',q['enum']]);ret(q['enum'])
  elif pc==0x610227:
   i=signed(u32(u,u32(u,s+4)));events.append(['pane',i]);ret(0,4)
  elif pc==0x6119f7:
   i=signed(u32(u,s+4));assert i==41
   events.append(['bitmap',i,q['favorite_exists']]);ret(FAVORITE if q['favorite_exists'] else 0,4)
  elif pc==0x6156b9:events.append(['favorite',q['favorite']]);ret(q['favorite'])
  elif pc==0x687ba9:
   dst,value,size=[u32(u,s+4+i*4) for i in range(3)]
   u.mem_write(dst,bytes([value&255])*size);ret(dst)
  elif pc==0x610a44:
   d=u32(u,s+8);count=u32(u,d+8);items=u32(u,d+12)
   registration=[widget(u32(u,items+i*4)) for i in range(count)]
   assert [u32(u,d),u32(u,d+4)]==[0,1]
   ret(0,8)
  elif pc==0x4381fd:events.append(['language',q['language']]);ret(q['language'])
  elif pc==0x63f743:
   va=u32(u,s+4)
   # Source strings are in mapped PE sections; inspect actual emulated bytes.
   data=bytearray()
   while u.mem_read(va+len(data),2)!=b'\0\0':data.extend(u.mem_read(va+len(data),2))
   caption=data.decode('utf-16le');events.append(['caption',widget(c),caption]);ret(0,4)
  elif pc==0x6875d3:ret(0) # Stack-cookie boundary only.
 u.hook_add(UC_HOOK_CODE,hook)
 if q['op']=='registration':
  # Start after the original constructor's external chat-provider query. The
  # remainder of64404E..644176 builds and passes the exact registration table.
  u.mem_write(sp+0x13,bytes([q['chat']]))
  w32(u,sp+0xc4,0x1234);u.reg_write(UC_X86_REG_ESI,OBJ);u.reg_write(UC_X86_REG_EBX,0)
  entry=0x64404e;stop=0x644177
 elif q['op']=='caption':
  u.reg_write(UC_X86_REG_ESI,OBJ);u.reg_write(UC_X86_REG_EDI,OBJ+WIDGETS[3]);u.reg_write(UC_X86_REG_EBX,0)
  u.reg_write(UC_X86_REG_EBP,OBJ+WIDGETS[0]);u.mem_write(sp,struct.pack('<I',STOP))
  entry=0x644177;stop=0x6441b0
 else:
  entry=0x64300d if q['op']=='pane' else 0x643632;stop=STOP
  u.mem_write(sp,struct.pack('<I',STOP));u.reg_write(UC_X86_REG_ECX,OBJ)
 u.reg_write(UC_X86_REG_ESP,sp);u.emu_start(entry,stop+1,count=100000)
 assert u.reg_read(UC_X86_REG_EIP)==stop,(q,hex(u.reg_read(UC_X86_REG_EIP)))
 remembered=u32(u,OBJ+0x13a8)
 return dict(input=q,events=events,registration=registration,caption=caption,focus=focus,
  state=dict(visible=[u.mem_read(OBJ+off+0x6e,1)[0] for off in WIDGETS],enabled=[u.mem_read(OBJ+off+0x6f,1)[0] for off in WIDGETS],
   widget_bounds=[rect(u,OBJ+off+0x20) for off in WIDGETS],text_visible=u.mem_read(TEXT+0x6e,1)[0],text_bounds=rect(u,TEXT+0x20),
   allowed=u.mem_read(OBJ+0x13a4,1)[0],remembered=None if not remembered else widget(remembered),pane=half(u,OBJ+0x3484),
   favorite_visible=u.mem_read(FAVORITE+0x6e,1)[0]),visits=visits)

def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
 rows=[]
 def add(**args):
  q=dict(op='visibility',flags=0,pane=0,chat=0,factory=0,host=0x80,active=0,count=1,edits=[0]*4,before_allowed=1,
    remembered=0,focus=0,visible=[1]*5,enabled=[1]*5,widget_bounds=[[480,-690,440,-360],[480,-320,440,8],[410,-690,370,-360],[410,-320,370,8],[-410,130,-440,670]],
    normal_bounds=[480,-690,440,-360],host_bounds=[410,-320,370,8],text_exists=1,text_visible=1,text_bounds=[50,-515,-30,-30],
    enum=3,favorite_exists=1,favorite_visible=1,favorite=0,language=0)
  q.update(args);rows.append(execute(image,q))
 for factory,chat in itertools.product((0,1,0x80),(0,1,0x80)):add(op='registration',factory=factory,chat=chat)
 for factory,language in itertools.product((0,0x80),range(9)):add(op='caption',factory=factory,language=language)
 for enum,pane,exists,favorite in itertools.product((-1,0,1,2,3,4,5,6,7,2147483647),(-1,0,1,2),(0,1),(0,0x80)):
  add(op='pane',enum=enum,pane=pane,favorite_exists=exists,favorite=favorite)
 for flags,host,factory,active,edit,pane in itertools.product((0,2),(0,0x80),(0,0x80),(0,0x80),(0,3),(-1,0,1,2)):
  add(flags=flags,host=host,factory=factory,active=active,edits=[0,0,edit,0],pane=pane)
 for host,factory,count,focus,chat,before in itertools.product((0,0x80),(0,0x80),(-2147483648,0,1,2,2147483647),(None,0,2,3,4),(0,0x80),(0,1)):
  add(host=host,factory=factory,count=count,focus=focus,chat=chat,before_allowed=before)
 for pane,exists,remembered,focus,active in itertools.product((0,1,2),(0,1),(None,0,4),(None,4),(0,0x80)):
  add(host=0,pane=pane,text_exists=exists,remembered=remembered,focus=focus,chat=0x80,active=active,before_allowed=1)
 OUT.mkdir(parents=True,exist_ok=True)
 fixture=ROOT/'tests/fixtures/vista_lobby_layout_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'),ensure_ascii=False)+'\n')
 sys.path.insert(0,str(ROOT/'analysis/frontend/vista-session-flow-audit/local-session-inputs-original'))
 from audit import PE,EXE,EXE_SHA
 p=PE(EXE,EXE_SHA);spans={k:p.span(a,b) for k,a,b in [('registration',0x64404e,0x644177),('caption',0x644177,0x6441b0),('pane',0x64300d,0x6430a4),('visibility',0x643632,0x6438fe)]}
 report=dict(source_sha256=SHA,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=len(rows),instructions=sum(r['visits'] for r in rows),
  fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),source_spans=spans,
  limits=['Registration begins after the external chat-provider query, with its exact supplied AL result. Source table construction and original global factory getter execute unchanged.',
   'Full643632 and64300D execute unchanged. Child lookup, host/count/countdown and focus services are explicit boundaries; source visibility, enabled and rectangle setters execute.',
   'Caption executes the original language table and rectangle getter, and captures actual PE UTF16. The widget string setter is a declared service.',
   'No native UI rendering, genuine account/factory-flag binding, roster/status owner or complete Vista transition parity is established.'])
 (OUT/'original.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='source_spans'}))

if __name__=='__main__':main()
