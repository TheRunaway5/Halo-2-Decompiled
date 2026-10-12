#!/usr/bin/env python3
"""Original1EB150/1EA310 on frozen source-produced player/view/clock records.

The full local setter/constructors are not repeated or replaced. Their actual
complete records are imported as declared source fixtures. Callback operands
come from the unchanged admitted Q/nested-action dispatcher. Clock stays tick0.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,ROOT,PIN,_Code,_trace,STOP,STACK
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def encode(x):return(json.dumps(x,sort_keys=True,indent=2)+'\n').encode()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,p
 else:p.write_bytes(b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--dispatch-report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 lp=ROOT/'analysis/multiplayer/integration/inventory-source/smg-local-effect-owner/audio-unavailable/report.json';l=json.loads(lp.read_text())
 d=json.loads(a.dispatch_report.read_text());st=next(s for s in l['stages']if s['name']=='declared_local_view_request_owner');source=l['states'][st['after_state']]
 view=next(x for x in source['local_owner_storage']if x['name']=='view');vr=bytes.fromhex(view['raw_hex']);v=int(view['pointer'],16)
 player=source['player'];pr=bytes.fromhex(player['raw21c_hex']);pp=int(player['pointer'],16)
 pool=next(x['state']for x in st['services']if x['state'].get('player_pool_header_hex'));poolp=int(pool['player_pool'],16);poolraw=bytes.fromhex(pool['player_pool_header_hex']);assert int.from_bytes(poolraw[0x44:0x48],'little')==pp
 clk=source['clock'];cp=int(clk['pointer'],16);cr=bytes.fromhex(clk['raw24_hex']);assert len(cr)==0x24 and int.from_bytes(cr[8:12],'little')==0
 assert int.from_bytes(pr[0x28:0x2a],'little')==0
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN
 g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
 mapped=set(range(0x10000,0x810000,0x1000))
 def backing(at,raw):
  for page in range(at&~4095,(at+len(raw)+4095)&~4095,4096):
   if page not in mapped:g.mem_map(page,4096);mapped.add(page)
  g.mem_write(at,raw)
 backing(poolp,poolraw);backing(pp,pr);backing(v,vr);backing(cp,cr);backing(STACK-0x10000,bytes(0x20000));backing(STOP,b'\xf4')
 for at,ptr in ((0x4f01f4,poolp),(0x526250,v),(0x518444,cp)):g.mem_write(at,struct.pack('<I',ptr))
 code=_Code(image,'.text');owners={e:_trace(code,e,{0x1eb150,0x1ea310})for e in (0x1eb150,0x1ea310)};addresses={pc for r in owners.values()for pc in r.code};stores=[];calls=[];case=None
 def hook(uc,pc,n,user):
  if pc==0x1ea310:calls.append(dict(case=case,entry=f'{pc:08x}',eax=f'{uc.reg_read(UC_X86_REG_EAX):08x}'))
 def write(uc,access,at,n,value,user):
  pc=uc.reg_read(UC_X86_REG_EIP)
  if pc in addresses and v<=at and at+n<=v+len(vr):stores.append(dict(case=case,instruction=f'{pc:08x}',offset=at-v,size=n,before_hex=bytes(uc.mem_read(at,n)).hex(),written_hex=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
 g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
 inputs=[c for c in d['calls']if c['entry']=='001eb150'];assert len(inputs)==2;rows=[]
 for i,c in enumerate(inputs+inputs):
  case=i;args=[int(x,16)for x in c['stack24_words'][:2]];before=bytes(g.mem_read(v,len(vr)));player_before=bytes(g.mem_read(pp,len(pr)))
  g.mem_write(STACK,struct.pack('<III',STOP,*args));g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_EAX,int(player['datum'],16))
  g.emu_start(0x1eb150,STOP,count=2000);assert g.reg_read(UC_X86_REG_EIP)==STOP and g.reg_read(UC_X86_REG_ESP)==STACK+12
  rows.append(dict(index=i,type=args[0],result_word=f'{args[1]:08x}',defined_result_byte=args[1]&255,
   player_datum=player['datum'],before_player_hex=player_before.hex(),after_player_hex=bytes(g.mem_read(pp,len(pr))).hex(),before_view_hex=before.hex(),after_view_hex=bytes(g.mem_read(v,len(vr))).hex(),raw_clock_hex=cr.hex(),return_esp_delta=12))
 listings=[]
 for e,r in owners.items():
  name=f'instructions/{e:08x}.asm';raw=''.join(f'{q.address:08x} {q.bytes.hex()} {q.mnemonic} {q.op_str}\n'for q in(code.at(pc)for pc in sorted(r.code))).encode();save(out/name,raw);listings.append(dict(file=name,sha256=sha(out/name)))
 value=dict(schema='source-owned-unit-action-local-notification-v1',tool_sha256=sha(__file__),source_xbe_sha256=PIN,local_fixture_sha256=sha(lp),local_fixture=str(lp.relative_to(ROOT)),dispatcher_fixture_sha256=sha(a.dispatch_report),
  source_pool=dict(pointer=f'{poolp:08x}',raw4c_hex=poolraw.hex()),source_view=view,source_player=player,source_clock=clk,cases=rows,stores=stores,calls=calls,instructions=listings,new_gameplay_fields=[],new_source_returns=[],instruction_patches=[],
  boundaries=['Complete original1EB150 tail-jumped1EA310 instructions execute on complete literal source records from actual1874D0/1EA3D0/156A30 constructors/resets and1508B0 local setter. This is a new bounded callback composition, not natural local-machine/controller admission.',
   'Two actual source dispatcher callback operands (nested type19 false and grenade type22 true) are delivered twice without modifying source clock or source records. The source clock remains tick0; minimum ties therefore retain slot0.',
   'Backing is mapped host fixture storage at the recorded original opaque addresses. Original player pool/header/body/view/clock bytes are copied intact; there is no generated gameplay state or inferred allocation/readiness.',
   'Upper result-word bytes are retained as original operands, but1EA310 consumes only lowbyte and writes zero-extended word. Native action notifier takes that defined lowbyte only.'])
 save(out/'report.json',encode(value));print(json.dumps(dict(report=str(out/'report.json'),sha256=sha(out/'report.json'),cases=len(rows),stores=len(stores))))
if __name__=='__main__':main()
