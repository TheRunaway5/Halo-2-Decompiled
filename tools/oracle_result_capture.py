#!/usr/bin/env python3
"""Unchanged original weapon/spree/death/lifetime result producers.

Recording bytes, poisoned stat banks, occupied live-pool records, raw game
mode, lifecycle/status and tick/clock are declared API inputs. Actual engine
registration and every reached game routine execute. Replication entities and
local users are NONE, unit parent/seat are NONE, and first-binding notification
is not requested. No original instruction or service/platform return is
replaced. This is not whole combat, retained identity remapping or PCR rendering.
"""
import argparse,json,random,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
from oracle_result_statistics import StatisticsTrace,seed,SPECS as STATS,G,M,P,R,K
from oracle_game_over import SPECS as BASE
from oracle_game_variants import STACK,STOP
from oracle_mp_full_scoreboard_1_5 import ROOT,literal,complete_tokens,references,sha,table_pair

OUT=ROOT/'analysis/multiplayer/game-variants/result-capture';FIXTURE=ROOT/'tests/fixtures/result_capture_original.json'
# weapon-add, cause-map, credited-kill, victim-death, player-phase, active-query,
# unit replication request (only inTU1.1/U5), last-betrayer record owner
OWNERS={
 'retail':dict(weapon=0x196e60,cause=0x196ef0,kill=0x1ea5b0,death=0x1ea770,life=0x15bb20,active=0x15b2f0,betrayer=0x162bf0),
 'tu11':dict(weapon=0x198960,cause=0x1989f0,kill=0x1edf70,death=0x1ee130,life=0x15d160,active=0x15c930,betrayer=0x164290,unit_request=0xa8810),
 'u5':dict(weapon=0x19b860,cause=0x19b8f0,kill=0x1f2890,death=0x1f2a50,life=0x15e540,active=0x15dd10,betrayer=0x165670,unit_request=0xb1c60)}
DATA={'retail':(0x46e098,0x45dc10,0x196ff8),'tu11':(0x475580,0x465080,0x198af8),'u5':(0x47a658,0x46a120,0x19b9f8)}
ATTRIBUTION={'retail':0x1e9fa0,'tu11':0x1ed960,'u5':0x1f2280}
O,C,U,H=0x1016000,0x1019000,0x1017000,0x1016000
def difference(before,after):
 return [dict(word=i,old=struct.unpack_from('<H',before,i*2)[0],new=struct.unpack_from('<H',after,i*2)[0])
  for i in range(len(before)//2) if before[i*2:i*2+2]!=after[i*2:i*2+2]]
class CaptureTrace(StatisticsTrace):
 def __init__(self,name):
  super().__init__(name);self.owners=OWNERS[name];self.g.mem_map(O,0x4000)
  self.w(self.spec['clock'],C)
  # Literal paired MOV owner global, then MOV owner+44 in the actual player
  # phase. Retail omits the later five-second unit replication request.
  ins=literal(self.code,self.owners['life'],self.starts)[1]
  candidates=[x.operands[1].mem.disp for x,y in zip(ins,ins[1:])
   if x.mnemonic=='mov' and x.op_str.startswith('eax, dword ptr [0x') and y.op_str=='eax, dword ptr [eax + 0x44]']
  assert len(candidates)==1,candidates;self.object_pool_global=candidates[0]
  self.w(self.object_pool_global,H);self.w(H+0x44,H+0x100);self.w(H+0x108,U)
 def setup_capture(self,flags,bank,slot,team,mode=1,present=True,lifecycle=1,status=1,tick=0,rate=30,current=True):
  self.setup(flags);self.g.mem_write(M,bytes(0xc18));self.g.mem_write(R,bytes(16*0x21c));self.g.mem_write(G,bytes(0x1200))
  self.w(M+0xc14,2 if present else 0);self.g.mem_write(M+0x6c,struct.pack('<h',lifecycle));self.w(M+0xc04,status)
  self.w(M+0x28,-1)
  for i in range(16):self.w(M+0x2c+i*4,-1)
  self.w(G+0x184,int(team));self.g.mem_write(G+0xc,bytes([mode&255]));self.w(P+0x38,16)
  for i in range(16):
   p=R+i*0x21c;self.g.mem_write(p,struct.pack('<H',0x9abc));self.g.mem_write(p+0xc0,bytes([7 if i==slot and team else 0]))
   self.w(p+0x24,-1);self.g.mem_write(p+0x28,b'\xff\xff');self.w(p+0x1b0,-1)
   self.w(p+0x2c,0x8aaa0000 if current else -1);self.g.mem_write(p+0x1a8,b'\x00')
  self.g.mem_write(U,bytes(0x600));self.w(U+0x14,-1);self.g.mem_write(U+0x1fc,b'\xff\xff');self.w(U+0xd4,-1)
  self.g.mem_write(C+2,struct.pack('<h',rate));self.w(C+8,tick);self.g.mem_write(K,bank)
  self.calls=[]
 def check_calls(self):
  allowed=set(self.owners.values())|set(self.s[k]for k in ('set','add','player','team'))
  assert set(self.calls)<=allowed,(self.name,[hex(e)for e in self.calls if e not in allowed])
 def result(self,initial):
  self.check_calls();return dict(after_bank_hex=bytes(self.g.mem_read(K,0x254)).hex(),retained_updates=difference(seed(),bytes(self.g.mem_read(self.s['bank'],0x4040))),calls=[f'{e:08x}'for e in self.calls])
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--fixture',type=Path,default=FIXTURE);a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=True);a.fixture.parent.mkdir(parents=True,exist_ok=True);sources={};versions={}
 for name in BASE:
  t=CaptureTrace(name);sources[name]=t;rng=random.Random(0x1f2890);mapping=[];kills=[];deaths=[];lives=[];weapons=[];cutoffs=[]
  for cause in range(256):
   t.calls=[];mapping.append(t.call(t.owners['cause'],registers=((UC_X86_REG_ECX,cause),(UC_X86_REG_EDX,0x9abc000f))))
   assert t.g.reg_read(UC_X86_REG_EDX)==0x9abc000f;t.check_calls()
  for n in range(512):
   slot=(0,3,15)[n%3];team=bool(n&1);cause=n&255;tick=(0,1,120,121,0x7fffffff,0x80000000,0xffffffff)[n%7]
   cutoff=rng.choice((-2147483648,-121,-1,0,1,120,2147483647));flags=((0,0),(1,0),(255,0),(1,255))[n%4]
   bank=bytearray(rng.randbytes(0x254));struct.pack_into('<h',bank,4+slot*28+18,(-32768,-1,0,1,254,255,32766,32767)[n%8])
   struct.pack_into('<I',bank,4+slot*28+24,rng.choice((0,120,0x7fffffff,0x80000000,0xffffffff)))
   t.setup_capture(flags,bytes(bank),slot,team,tick=tick)
   t.call(t.owners['kill'],(K,cutoff,cause),((UC_X86_REG_EDX,0x9abc0000+slot),))
   kills.append(dict(slot=slot,team=team,cause=cause,tick=tick,cutoff=cutoff,flags=flags,before_bank_hex=bytes(bank).hex(),**t.result(bank)))
  for n in range(256):
   slot=(0,3,15)[n%3];credit=(-1,0,3,15)[n%4];team=bool(n&4);cause=n;self=bool(n&8);betrayal=bool(n&16);flags=((0,0),(1,0),(255,0),(1,255))[(n//4)%4]
   bank=rng.randbytes(0x254);t.setup_capture(flags,bank,slot,team)
   t.call(t.owners['death'],(K,0xffffffff if credit==-1 else 0x9abc0000+credit,int(self),int(betrayal)),
    ((UC_X86_REG_ECX,cause),(UC_X86_REG_EDI,0x9abc0000+slot)))
   deaths.append(dict(slot=slot,credited=credit,team=team,cause=cause,self=self,betrayal=betrayal,flags=flags,before_bank_hex=bank.hex(),**t.result(bank)))
  ticks=(0,1,14,15,29,30,59,60,119,120,149,150,0x7fffffff,0x80000000,0xffffffff)
  for n in range(512):
   slot=(0,3,15)[n%3];tick=ticks[n%len(ticks)];rate=(15,30,60,-1,-32768)[(n//len(ticks))%5]
   # INT_MIN/-1 and rate0 fault before publication; never supplied a result.
   if tick==0x80000000 and rate==-1:continue
   mode=(1,2,4,-1)[(n//75)%4];current=bool(n&1);present=bool(n&2);lifecycle=1 if n&4 else 2;status=1 if n&8 else 2
   flags=((0,0),(1,0),(255,0),(1,255))[(n//16)%4];bank=rng.randbytes(0x254)
   t.setup_capture(flags,bank,slot,False,mode,present,lifecycle,status,tick,rate,current)
   t.call(t.owners['life'],(0x9abc0000+slot,))
   lives.append(dict(slot=slot,tick=tick,rate=rate,mode=mode,current=current,present=present,lifecycle=lifecycle,status=status,flags=flags,before_bank_hex=bank.hex(),**t.result(bank)))
  for rate in (15,30,60):
   for tick in (0,1,rate-1,rate,rate+1,rate*5):
    for flags in ((0,0),(1,0),(255,0),(1,255)):
     slot=15;bank=rng.randbytes(0x254);t.setup_capture(flags,bank,slot,False,tick=tick,rate=rate)
     t.call(t.owners['life'],(0x9abc0000+slot,))
     lives.append(dict(slot=slot,tick=tick,rate=rate,mode=1,current=True,present=True,lifecycle=1,status=1,flags=flags,before_bank_hex=bank.hex(),**t.result(bank)))
  # Execute the actual damage-owner prefix through its first cutoff store.
  # Its original object lookup receives NONE and returns through its own code;
  # stop before subsequent object/history/medal/event dependencies.
  prefix=literal(t.code,ATTRIBUTION[name],t.starts)[1]
  store=next(i for i in prefix if i.op_str=='dword ptr [esp + 0x30], ecx')
  stop=store.address+store.size;resolver=next(i.operands[0].imm for i in prefix if i.mnemonic=='call')
  for rate in (-32768,-1,0,15,30,32767):
   for tick in (0,1,120,0x7fffffff,0x80000000,0xffffffff):
    bank=bytes([1])+bytes(0x253);t.setup_capture((1,0),bank,0,False,tick=tick,rate=rate)
    t.w(STACK,STOP);t.w(STACK+4,K);t.w(STACK+8,-1)
    t.g.reg_write(UC_X86_REG_ESP,STACK);t.g.reg_write(UC_X86_REG_FPCW,0x27f);t.g.reg_write(UC_X86_REG_MXCSR,0x1f80)
    t.g.emu_start(ATTRIBUTION[name],stop,count=10000);assert t.g.reg_read(UC_X86_REG_EIP)==stop
    assert set(t.calls)<={ATTRIBUTION[name],resolver},[hex(e)for e in t.calls]
    value=struct.unpack('<i',t.g.mem_read(t.g.reg_read(UC_X86_REG_ESP)+0x30,4))[0]
    cutoffs.append(dict(tick=tick,rate=rate,cutoff=value,completion='stopped_after_original_cutoff_store',stop=f'{stop:08x}',calls=[f'{e:08x}'for e in t.calls]))
  for n in range(384):
   flags=((0,0),(1,0),(255,0),(1,255))[n%4];slot=(0,3,15)[n%3];kind=(n//4)%7;weapon=(0,5,8,15,38,40)[(n//28)%6];delta=(-2147483648,-1,0,1,32767,2147483647)[n%6]
   t.setup(flags);t.call(t.owners['weapon'],(weapon,),((UC_X86_REG_ECX,slot),(UC_X86_REG_EAX,kind),(UC_X86_REG_EBX,delta)));t.check_calls()
   weapons.append(dict(slot=slot,kind=kind,weapon=weapon,delta=delta,flags=flags,retained_updates=difference(seed(),bytes(t.g.mem_read(t.s['bank'],0x4040)))))
  raw=t.image.read(DATA[name][0],112)
  versions[name]=dict(image_sha256=BASE[name]['pin'],owners={k:f'{v:08x}'for k,v in t.owners.items()},weapon_descriptors_hex=raw.hex(),cause_mapping=mapping,
   multikill_seconds_bits=t.image.read(DATA[name][1],4).hex(),dispatch_table_hex=t.image.read(DATA[name][2],164).hex(),object_pool_global=f'{t.object_pool_global:08x}',
   kills=kills,deaths=deaths,lifetimes=lives,weapons=weapons,cutoffs=cutoffs)
 pairs=[];listings=[];ld=a.output.parent/'instructions';ld.mkdir(exist_ok=True)
 for name,t in sources.items():
  for key,e in t.owners.items():
   f,ins=literal(t.code,e,t.starts);p=ld/f'{name}-{e:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(file=p.name,sha256=sha(p),instructions=len(ins)))
   if name!='u5' and not(name=='retail' and key=='life'):
    later=sources['u5'];lf,li=literal(later.code,later.owners[key],later.starts)
    assert len(ins)==len(li),(name,key,len(ins),len(li))
    old,new=complete_tokens(ins,t.image),complete_tokens(li,later.image);dif=[i for i in range(len(old))if old[i]!=new[i]]
    if key=='kill':
     assert dif==[68] and ins[68].op_str==f'esi, {hex(STATS[name]["bank"]+24)}' and li[68].op_str=='esi, 0x51ef0c'
    else:assert not dif,(name,key,dif)
    mapping={**{t.owners[k]:later.owners[k]for k in t.owners if k in later.owners},**{STATS[name][k]:STATS['u5'][k]for k in ('set','add','player','team')}}
    for oi,ni in zip(ins,li):
     if oi.mnemonic=='call' and oi.op_str.startswith('0x') and int(oi.op_str,16)in mapping:assert int(ni.op_str,16)==mapping[int(oi.op_str,16)]
    tables=table_pair(f,lf,ins,li,t.image,later.image);assert tables is not None and all(r['topology_equal']for r in tables)
    pairs.append(dict(earlier=name,owner=key,instructions=len(ins),literal_profile_base_relocation=dif,complete_shape_equal_except_declared_base=True,switch_tables=tables,relocated_operands=references(ins,li,t.image,later.image)))
 # Static world/player phase order and cutoff owner; external game services
 # are not declared successful executions by these listings.
 t=sources['u5']
 for e in (0x13aac0,0x15eba0,0x15e870,0x1f2280):
  _,ins=literal(t.code,e,t.starts);p=ld/f'u5-order-{e:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(file=p.name,sha256=sha(p),instructions=len(ins),kind='static_caller'))
 data=dict(schema='original-result-capture-v1',boundary=__doc__,instruction_patches=[],supplied_returns=[],versions=versions)
 a.fixture.write_text(json.dumps(data,separators=(',',':'),sort_keys=True)+'\n');report=dict(**data,fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),complete_source_pairs=pairs,instruction_listings=listings,
  dependencies={p:sha(ROOT/p)for p in ('tools/oracle_result_statistics.py','tools/oracle_game_variants.py','tools/oracle_game_over.py','tools/oracle_mp_full_scoreboard_1_5.py')})
 a.output.write_text(json.dumps(report,separators=(',',':'),sort_keys=True)+'\n');print(json.dumps(dict(fixture_sha256=sha(a.fixture),pairs=len(pairs),cases=sum(sum(len(v[k])for k in ('kills','deaths','lifetimes','weapons','cutoffs'))+len(v['cause_mapping'])for v in versions.values())),indent=2))
if __name__=='__main__':main()
