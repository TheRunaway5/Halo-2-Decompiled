#!/usr/bin/env python3
"""Original41980 attachment collection through no-model B8D30 and42850 queues.

Actual plasma-bolt metadata, free object/no model/no mirror inputs and completed
root matrices are declared ABI storage. Both complete update-image consumers
execute without instruction patches or supplied services. This is attachment/
queue evidence, not original projectile birth, visibility or GPU execution.
"""
import argparse,csv,hashlib,json,math,random,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Trace,Cache,ROOT,WEAPON,WI,_Code,_trace
from oracle_weapon_graph_timing_tu15 import Original
from weapon_reference_versions import images
from research_mp_inventory_switch_u5 import raw_prefix_candidates,norm
QUEUE=0x2400000
def mapping(label):
 bank={n:x for n,x,_ in images()};old=bank['retail'];x=bank[label];oc=_Code(old,'.text');code=_Code(x,'.text')
 old_starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/retail/functions.tsv').open(),delimiter='\t')}
 inventory=ROOT/('analysis/map-pack-update/functions.tsv' if label=='TU1.1' else 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')
 starts={int(r['address'],16) for r in csv.DictReader(inventory.open(),delimiter='\t')};owners=[];absolute={};entries={}
 for entry in (0x41980,0x42850,0xb8ca0,0xb8d30,0xbe6d0):
  f=_trace(oc,entry,old_starts);seq=[oc.at(pc) for pc in sorted(f.code)];pos={i.address:n for n,i in enumerate(seq)};expected=[norm(i,old,pos) for i in seq];matches=[]
  for pc in raw_prefix_candidates(oc,old,code,entry):
   g=_trace(code,pc,starts);other=[code.at(a) for a in sorted(g.code)];np={i.address:n for n,i in enumerate(other)}
   if [norm(i,x,np) for i in other]==expected:matches.append((pc,other))
  assert len(matches)==1,(label,hex(entry),[hex(v[0]) for v in matches]);pc,other=matches[0];entries[entry]=pc;operands=[]
  for u,v in zip(seq,other):
   for a,b in zip(u.operands,v.operands):
    if a.type==b.type==2 and 0x400000<=a.imm<0x600000:absolute[a.imm]=b.imm
    if a.type==b.type==3 and a.mem.base==a.mem.index==b.mem.base==b.mem.index==0 and a.mem.disp and b.mem.disp:
     absolute[a.mem.disp]=b.mem.disp;operands.append(dict(old_va=f'{a.mem.disp:08x}',new_va=f'{b.mem.disp:08x}',size=a.size,old_hex=old.read(a.mem.disp,a.size).hex(),new_hex=x.read(b.mem.disp,b.size).hex()))
  owners.append(dict(retail_entry=f'{entry:08x}',entry=f'{pc:08x}',instruction_count=len(seq),normalized_complete_body_equal=True,absolute_operands=operands))
 return entries,absolute,owners
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 cache=Cache(ROOT/'assets/disc/maps/lockout.map');tags=[t for t in cache.tags.values() if t['group']=='proj' and t['name'].split('\\')[-1] in ('plasma_pistol_bolt','plasma_pistol_charged_bolt')];assert len(tags)==2
 cases=[];rng=random.Random(0x42850)
 for tag in tags:
  assert cache.u(tag['offset']+0x38)==0xffffffff and not (cache.u(tag['offset']+2)&0x200)
  for sample in range(16):
   yaw=rng.uniform(-3,3);co,si=math.cos(yaw),math.sin(yaw)
   root=[rng.uniform(.2,3),co,si,0.,-si,co,0.,0.,0.,1.,*[rng.uniform(-20,20) for _ in range(3)]]
   eye=[rng.uniform(-10,10) for _ in range(3)];forward=[co,si,0.]
   root=list(struct.unpack('<13f',struct.pack('<13f',*root)));eye=list(struct.unpack('<3f',struct.pack('<3f',*eye)));forward=list(struct.unpack('<3f',struct.pack('<3f',*forward)))
   seed=bytes(rng.randrange(256) for _ in range(112))
   for mode in (0,1,3):
    for count in (0,1,1023,1024):cases.append(dict(datum=int(tag['datum'],16),mode=mode,count=count,root=root,eye=eye,forward=forward,seed_hex=seed.hex()))
 lines=['# image datum mode count notice root13 eye3 forward3 seed28 expected_count expected_notice expected28'];reports=[];reference=None
 for label,version,cls in [('TU1.1',11,Trace),('TU1.5',15,Original)]:
  entries,absolute,owners=mapping(label);t=cls();t.g.mem_map(QUEUE,0x20000);reached={}
  t.g.hook_add(4,lambda g,pc,size,u:reached.setdefault(pc,bytes(g.mem_read(pc,size)).hex()))
  # Free, unmirrored projectile body with its actual completed single root.
  t.w(WEAPON+4,0);t.w(WEAPON+0x14,0xffffffff);t.g.mem_write(WEAPON+0xaa,b'\x05');t.sh(WEAPON+0x116,0x500)
  t.w(absolute[0x4b6288],QUEUE);outputs=[]
  for n,c in enumerate(cases):
   t.w(WEAPON,c['datum']);t.g.mem_write(WEAPON+0x500,struct.pack('<13f',*c['root']))
   t.g.mem_write(absolute[0x4b9da0],struct.pack('<3f',*c['eye']));t.g.mem_write(absolute[0x4b9dac],struct.pack('<3f',*c['forward']))
   t.w(absolute[0x4b6280],c['count']);t.g.mem_write(absolute[0x4b6291],b'\x01')
   at=QUEUE+min(c['count'],1023)*112;t.g.mem_write(at,bytes.fromhex(c['seed_hex']))
   t.run(entries[0x41980],(c['mode'],WI));count=t.u(absolute[0x4b6280]);notice=t.u(absolute[0x4b6291],1);raw=bytearray(t.g.mem_read(at,112))
   authentic=raw.hex()
   if count>c['count']:
    # The callback is an explicitly normalized port token, as in the existing
    # transparent draw bindings. Authentic original pointer remains in report.
    assert int.from_bytes(raw[32:36],'little')==0x4f000;raw[32:36]=(0x4f010).to_bytes(4,'little')
   words=[version,c['datum'],c['mode'],c['count'],1,*struct.unpack('<13I',struct.pack('<13f',*c['root'])),*struct.unpack('<3I',struct.pack('<3f',*c['eye'])),*struct.unpack('<3I',struct.pack('<3f',*c['forward'])),*struct.unpack('<28I',bytes.fromhex(c['seed_hex'])),count,notice,*struct.unpack('<28I',raw)]
   lines.append(' '.join(f'{v:08x}' for v in words));outputs.append(dict(count=count,notice=notice,authentic_command_hex=authentic,normalized_command_hex=raw.hex()))
  if reference is None:reference=outputs
  else:assert outputs==reference
  for pc,raw in reached.items():assert t.image.read(pc,len(bytes.fromhex(raw)))==bytes.fromhex(raw)
  reports.append(dict(label=label,xbe_sha256=hashlib.sha256(t.image.data).hexdigest(),owners=owners,outputs=outputs,reached_instruction_bytes={f'{pc:08x}':raw for pc,raw in sorted(reached.items())}));print(label,len(cases),'complete original attachment/queue cases',flush=True)
 a.fixture.write_text('\n'.join(lines)+'\n');a.output.write_text(json.dumps(dict(scope=__doc__,cases=cases,versions=reports,cache_sha256=hashlib.sha256(cache.data).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),instruction_patches=[],supplied_services=[]),indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
