#!/usr/bin/env python3
"""Original populated PCR focus and supplied-DLL offline voice owners.

The original list constructor, datum-array constructor/allocation, sixteen row
constructors, list initializer, focus setter and focus query execute unchanged.
Only the UI allocator's memory contract is supplied. A real registered screen
root is an explicit UI-manager input. Original protected40B94B late selection
executes with asset/layout, pane initialization and clock boundaries recorded.
Original XHVCreateEngine from the supplied DLL executes unchanged; no HRESULT
is supplied by the harness. Voice widget lookup executes; frame assignment is
recorded at its already separately tested bitmap-frame owner boundary.
"""
import hashlib,itertools,json,struct,sys
from pathlib import Path
from unicorn.x86_const import *
from oracle_native_vista_menu_services import Original
from oracle_vista_pcr_screen import STOP,STACK,u32,w32
from export_vista_menu_ui import VistaCache
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'analysis/frontend/vista-session-flow-audit/local-session-inputs-original'))
from audit import PE,EXE,EXE_SHA,DLL,DLL_SHA
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/pcr-owner-state-original'
MAP_SHA='eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44'
SCREEN=0x1a00000;LIST=SCREEN+0x289c;DATA=SCREEN+0x10000;SESSION=SCREEN+0x11000
ROW=SCREEN+0x9000;BITMAP=ROW+0x1000;IDENTITY=SCREEN+0x1f000
QUERY=bytes.fromhex('0102030405060708')
class FocusOriginal(Original):
 def hook(self,u,pc,n,x):
  if pc==0x60d30f:
   self.calls.append(['ui_allocate',u32(u,u.reg_read(UC_X86_REG_ESP)+4)])
   self.ret(DATA,pop=4);return
  super().hook(u,pc,n,x)
class PaneOriginal(Original):
 def hook(self,u,pc,n,x):
  if pc in (0x610a44,0x611e23,0x5b3ca0,STOP+0x800,STOP+0x810):
   sp=u.reg_read(UC_X86_REG_ESP)
   self.calls.append([hex(pc),u.reg_read(UC_X86_REG_ECX),u32(u,sp+4)])
   self.ret(1234 if pc==0x5b3ca0 else None,pop=8 if pc==0x610a44 else 4 if pc in (STOP+0x800,STOP+0x810) else 0);return
  super().hook(u,pc,n,x)
class VoiceOriginal(Original):
 def __init__(self,e,d):
  super().__init__(e);size=(max(s['rva']+max(s['virtual_size'],s['raw_size'])for s in d.sections)+4095)&~4095
  self.u.mem_map(d.base,size)
  for s in d.sections:self.u.mem_write(d.base+s['rva'],d.data[s['raw']:s['raw']+s['raw_size']])
  w32(self.u,0x79b034,0x105a4370);self.frames=[];self.xhv=[]
 def hook(self,u,pc,n,x):
  if pc==0x105a4370:
   sp=u.reg_read(UC_X86_REG_ESP);self.xhv.append([u32(u,sp+i*4)for i in (1,2,3)])
  if pc==0x61c300:
   self.frames.append(u32(u,u.reg_read(UC_X86_REG_ESP)+4));self.ret(pop=4);return
  super().hook(u,pc,n,x)
def initial_pane(e,players,teams):
 o=PaneOriginal(e);w32(o.u,SCREEN+0x70,16);w32(o.u,0x86eee8,players);w32(o.u,0x86eef0,teams)
 for off in (0xad0,0x1e40,0x31b0,0x4520,0x5894,0x6c04):w32(o.u,SCREEN+off,STOP+0x900)
 w32(o.u,STOP+0x900+0x58,STOP+0x800);w32(o.u,STOP+0x900+0x24,STOP+0x810)
 assert o.invoke(0x40b94b,SCREEN,[0])['pc']==STOP
 offset=u32(o.u,SCREEN+0xacc)-SCREEN;assert offset in (0xad0,0x1e40)
 return dict(players=players,teams=teams,pane=0 if offset==0xad0 else 1,offset=offset,body=o.body,calls=o.calls,visits=o.visits)
def list_focus(e,count,teams):
 o=FocusOriginal(e);u=o.u
 # The concrete screen root is registered in actual channel3/user4 owner.
 w32(u,SCREEN,0x79c904);w32(u,SCREEN+4,0);w32(u,SCREEN+0x74,3);w32(u,SCREEN+0x78,4)
 w32(u,0xd71acc+4*60+8,SCREEN)
 assert o.invoke(0x40dcab,LIST,[1,count,int(teams),0])['pc']==STOP
 w32(u,LIST,0x79c214 if teams else 0x79c28c);w32(u,LIST+0x10,SCREEN)
 for entry in (0x613bdd,0x613fe5):assert o.invoke(entry,LIST)['pc']==STOP
 pointer=o.invoke(0x609efd,0,[0,3,4])['eax'];handle=o.invoke(0x61383b,LIST)['eax']
 assert pointer==LIST+0xb0 and (handle&0xffff)==(0 if count else 0xffff)
 handles=[u32(u,LIST+0xb0+i*0x84+0x70)for i in range(16)]
 assert sum(v!=0xffffffff for v in handles)==count
 return dict(count=count,teams=teams,handle=handle,row=handle&0xffff if handle!=0xffffffff else handle,
  pointer=pointer,handles=handles,calls=o.calls,visits=o.visits)
def metadata():
 c=VistaCache(ROOT/'Halo 2 Vista installer/maps/mainmenu.map')
 try:
  assert hashlib.sha256(c.data).hexdigest()==MAP_SHA
  tag=next(t for t in c.tags.values()if t['group']=='wgit' and c.u16(c.tag(t['datum'],'wgit')[1]+4)==16)
  p=c.tag(tag['datum'],'wgit')[1];panes=[]
  for q in c.block(p+32,76):
   lists=[dict(flags=c.u32(a),skin=c.u16(a+4),visible=c.u16(a+6),raw=c.data[a:a+24].hex())for a in c.block(q+12,24)]
   assert len(lists)==1 and lists[0]['visible']==16
   panes.append(lists[0])
  assert len(panes)==6 and panes[1]['skin']==16
  return dict(map_sha256=MAP_SHA,screen=tag,panes=panes)
 finally:c.close()
def main():
 e,d=PE(EXE,EXE_SHA),PE(DLL,DLL_SHA);assets=metadata();rows=[]
 panes=[initial_pane(e,p,t)for p,t in itertools.product((0,1,16),(0,1,16))]
 focus=[list_focus(e,n,t)for n,t in itertools.product(range(17),(False,True))]
 byfocus={(r['count'],r['teams']):r for r in focus}
 for p,t in itertools.product(range(17),(0,1,16)):
  selected=0 if t else 1;actual=byfocus[(t if t else p,bool(t))]
  proved=next(x for x in panes if x['players']==(p if p in (0,1,16)else 1)and x['teams']==t)
  assert proved['pane']==selected
  rows.append(dict(kind='focus',players=p,teams=t,pane=selected,row=actual['row']))
 o=VoiceOriginal(e,d)
 # Actual initialized voice-manager gate and actual failed DLL engine owner.
 for mode in (0,1):
  assert o.invoke(0x5b4213,0,[mode])['pc']==STOP
  assert bytes(o.u.mem_read(0x91abd6,1))==b'\1' and bytes(o.u.mem_read(0x91abf0,1))==b'\0'
  assert u32(o.u,0x91ac00)==0xffffffff and bytes(o.u.mem_read(0x91ac04,6))==bytes(6)
  rows.append(dict(kind='voice_initialization',mode=mode,gate=1,engine_available=0,local_slot=0xffffffff,masks=[0,0,0],hresult=0x88780078))
 init=dict(calls=o.xhv,owner=bytes(o.u.mem_read(0x91abd4,0xc0)).hex(),visits=o.visits)
 # Exact original identity and painter chain; only frame leaf is recorded.
 w32(o.u,ROW+0x14,BITMAP);w32(o.u,BITMAP+4,8);o.u.mem_write(IDENTITY,QUERY);o.u.mem_write(0x86eff8,QUERY)
 w32(o.u,0x820fe8,SESSION)
 controls=[(0,[]),(1,[0]),(1<<7,[7]),(1<<15,[15]),(0x8001,[0,15]),(1<<6,[7])]
 for enabled,manager,state,(mask,matches)in itertools.product((0,1),(0,1,3,4,5,6,0xffffffff),(*range(12),0xffffffff),controls):
  o.u.mem_write(SESSION,bytes(0x7b88));w32(o.u,SESSION+0x73a0,state);w32(o.u,SESSION+0x1258,mask)
  for slot in matches:o.u.mem_write(SESSION+0x125c+slot*0x128,QUERY)
  o.u.mem_write(0x820fc0,bytes([enabled]));w32(o.u,0x820fc4,manager)
  assert bytes(o.u.mem_read(IDENTITY,8))==QUERY
  result=o.invoke(0x616cfc,0,[IDENTITY]);assert result['pc']==STOP;slot=result['eax']
  for lobby in (0,1):
   o.frames=[];o.u.mem_write(BITMAP+0x6e,b'\xa5')
   assert o.invoke(0x40c5c4 if lobby else 0x40c5da,0,[ROW,0])['pc']==STOP
   visible=bytes(o.u.mem_read(BITMAP+0x6e,1))[0]
   assert visible in (0,1);frame=o.frames[0]if o.frames else 0
   assert o.frames in ([],[2])
   rows.append(dict(kind='voice',enabled=enabled,manager=manager,state=state,mask=mask,matches=matches,
    identity=QUERY.hex(),slot=slot,lobby=lobby,visible=visible,frame=frame))
 fixture=ROOT/'tests/fixtures/vista_pcr_owner_state_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 spans={'voice_identity':e.span(0x616c85,0x616d34),'voice_session':e.span(0x5ad782,0x5ad7b7),
  'session_admission':e.span(0x5ad423,0x5ad457),'voice_initialize':e.span(0x5e5abd,0x5e5b88),
  'voice_predicates':e.span(0x5b4aa1,0x5b4b3e),'dll_xhv':d.span(0x105a4370,0x105a43a8),
  'list_constructor':e.span(0x40dcab,0x40ddd8),'datum_allocate':e.span(0x4667a0,0x466858),
  'list_initialize':e.span(0x613fe5,0x61401f),'bind_datums':e.span(0x6136ce,0x613719),
  'query_row':e.span(0x61383b,0x61386b),'child_constructor':e.span(0x61ffc9,0x620050)}
 report=dict(exe_sha256=EXE_SHA,dll_sha256=DLL_SHA,assets=assets,panes=panes,list_focus=focus,
  voice_initialization=init,spans=spans,cases=len(rows),voice_visits=o.visits,
  fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),boundaries=__doc__)
 OUT.mkdir(parents=True,exist_ok=True);(OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(cases=len(rows),fixture_sha256=report['fixture_sha256'],voice_instruction_visits=o.visits,
  single_row=byfocus[(1,False)]['handle'],xhv=init['calls'])))
if __name__=='__main__':main()
