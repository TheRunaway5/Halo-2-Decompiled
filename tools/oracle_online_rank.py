#!/usr/bin/env python3
"""Research controls: execute unchanged retail XBE rank routines with fixtures.
No native rank implementation is imported; no original routine/call is stubbed.
The original network initializer supplies all configuration and table bytes.
Fixture collections are explicit synthetic inputs, not captured live results.
"""
import hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_game_variants import Source,VERSIONS,VARIANT
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI
s=Source(VERSIONS['retail'])
s.call(0x66330)
BASE=0x4ce040

def table(off,fmt,count):return list(struct.unpack('<'+fmt*count,s.g.mem_read(BASE+off,struct.calcsize(fmt)*count)))
def pair(own,other,place,other_place):
 return s.call(0x746b0,registers=((UC_X86_REG_EBX,other),(UC_X86_REG_ESI,own),(UC_X86_REG_EDX,place),(UC_X86_REG_EAX,other_place)))
def level(xp,previous):return s.call(0x74970,registers=((UC_X86_REG_ESI,xp),(UC_X86_REG_EDI,previous)))
def collection(players,team_mode=False,team_places=None):
 s.g.mem_write(VARIANT,bytes(0xf44))
 s.g.mem_write(VARIANT+4,struct.pack('<i',3))
 s.g.mem_write(VARIANT+0x12b,bytes([int(team_mode)]))
 for i,p in enumerate(players):
  a=VARIANT+0x384+i*0xa4
  s.g.mem_write(a,b'\x01');s.g.mem_write(a+0xa,bytes([p.get('flags',0)]))
  s.g.mem_write(a+0x8c,struct.pack('<b',p.get('team',i)))
  s.g.mem_write(a+0x94,struct.pack('<i',p.get('key',3)))
  s.g.mem_write(a+0x98,struct.pack('<h',p['skill']))
  s.g.mem_write(a+0x9c,struct.pack('<i',p.get('xp',5000)))
  s.g.mem_write(a+0xa0,struct.pack('<b',p['place']))
 for i,p in enumerate(team_places or []):
  s.g.mem_write(VARIANT+0xdc4+i*24,b'\x01'+struct.pack('<b',p))
 return [s.call(0x74800,(VARIANT,i)) for i in range(len(players))]
checks=[]
def check(name,actual,expected):
 assert actual==expected,(name,actual,expected)
 checks.append(dict(name=name,actual=actual,expected=expected))
check('equal skill win',pair(15,15,0,1),100)
check('equal skill loss',pair(15,15,1,0),-100)
check('equal standings',pair(15,15,0,0),0)
check('both no standing',pair(15,15,-1,-1),0)
check('opponent no standing',pair(15,15,0,-1),100)
check('own no standing',pair(15,15,-1,0),-100)
check('higher skill win',pair(16,15,0,1),90)
check('higher skill loss',pair(16,15,1,0),-110)
check('negative fraction truncates toward zero',pair(8,7,1,0),-71)
check('low loss pair scaling',pair(7,10,1,0),-42)
check('positive high rank fraction truncates toward zero',pair(41,40,0,1),85)
base=[dict(skill=skill,place=i) for i,skill in enumerate([20,17,18,15])]
check('four player base averaging truncates signed result',collection(base),[66,40,-40,-66])
low=[dict(skill=skill,place=i) for i,skill in enumerate([10,7,8,5])]
low_actual=collection(low)
check('four player low mitigation applied pairwise',low_actual,[66,49,-17,-29])
check('tied opponent included in average denominator',collection([dict(skill=15,place=0),dict(skill=15,place=0),dict(skill=15,place=1)]),[50,50,-100])
check('same team ignored and team standings used',collection([dict(skill=15,place=9,team=0),dict(skill=15,place=9,team=0),dict(skill=15,place=0,team=1)],True,[0,1]),[100,100,-100])
check('bottom XP clamp',collection([dict(skill=15,place=1,xp=3),dict(skill=15,place=0,xp=5000)])[0],-3)
check('top XP clamp',collection([dict(skill=15,place=0,xp=0x3fffffff-3),dict(skill=15,place=1,xp=5000)])[0],3)
check('mismatched playlist key excluded',collection([dict(skill=15,place=0),dict(skill=15,place=1,key=7)])[0],0)
check('flagged guest excluded',collection([dict(skill=15,place=0),dict(skill=15,place=1,flags=1)])[0],0)
for xp,previous,expected in [(0,0,0),(99,0,0),(100,0,1),(2999,10,10),(2750,10,9),(2751,10,10),(2499,10,8),(3000,0,10),(44500,0,49)]:
 check(f'level xp={xp} previous={previous}',level(xp,previous),expected)
# Distinct controls: mutate fixture configuration only to prove configured delta clamps,
# then restore original initialized data. All executable bytes stay unchanged.
s.g.mem_write(BASE+0xc78,struct.pack('<ii',7,-9))
check('configured maximum delta',collection([dict(skill=15,place=0),dict(skill=15,place=1)])[0],7)
check('configured minimum delta',collection([dict(skill=15,place=0),dict(skill=15,place=1)])[1],-9)
s.call(0x66330)
# Built-in original preset routine has EDI=preset, ESI=variant.
presets=[]
for preset in [0,2,7,9]:
 s.g.mem_write(VARIANT,bytes(0x130))
 s.call(0x212f40,registers=((UC_X86_REG_EDI,preset),(UC_X86_REG_ESI,VARIANT)))
 raw=bytes(s.g.mem_read(VARIANT,0x130))
 presets.append(dict(preset=preset,name=raw[4:68].decode('utf-16-le').split('\x00')[0],raw=raw.hex()))
result=dict(scope=__doc__,xbe=dict(path=VERSIONS['retail']['path'],sha256=VERSIONS['retail']['pin']),
 routines={hex(a):hashlib.sha256(bytes(s.g.mem_read(a,n))).hexdigest() for a,n in [(0x746b0,0x6c),(0x74720,0xdc),(0x74800,0x162),(0x74970,0x3e),(0x66330,0xa80),(0x212f40,0x3c8)]},
 tables=dict(win_exchange=table(0x378,'h',255),loss_exchange=table(0x576,'h',255),xp_thresholds=table(0x774,'i',128),loss_percent=table(0x974,'B',128),win_percent=table(0x9f4,'B',128),loss_offsets=table(0xa74,'h',128),win_offsets=table(0xb74,'h',128),delta_bounds=table(0xc78,'i',2)),
 controls=checks,presets=presets,limits='Retail executable defaults only. Online network_configuration.dat can replace tables. No live Live-service, authenticated match result, title-update comparison, captured TMS file or end-to-end online parity demonstrated.')
print(json.dumps(result,indent=2))
