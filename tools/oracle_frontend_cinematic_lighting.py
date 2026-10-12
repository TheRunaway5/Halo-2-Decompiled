#!/usr/bin/env python3
"""Freeze unchanged BBFC0/BC070/2A2349 lighting writes and BBF80/3DB00 selection.

The only environment is an allocated lighting global, one object datum, and
literal arguments; no rendering, math, lighting producer or selector is stubbed.
The ambient wrapper is entered after HS argument evaluation and stopped before
HS completion. Cases retain a nonzero bank to expose untouched fields.
"""
import hashlib,json,random,struct
from oracle_multiplayer_hud_positions import Retail,ROOT,PIN
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX
r=Retail();g=r.g
bank=r.alloc(bytes(0x80));r.w(0x4de2f4,bank)
pool=r.alloc(bytes(0x50));table=r.alloc(bytes(24));obj=r.alloc(bytes(0x100))
r.w(0x4e0300,pool);r.w(pool+0x44,table);r.w(table+20,obj)
words=lambda data:list(struct.unpack('<'+str(len(data)//4)+'I',data))
bits=lambda v:struct.unpack('<I',struct.pack('<f',v))[0]
seed=[bits(i/32) for i in range(20)]
cases=[];rng=random.Random(0xBBFC0)
angles=[(45,240),(-45,30),(0,0),(90,180),(-90,360),(180,-180)]+[(rng.uniform(-720,720),rng.uniform(-720,720)) for _ in range(128)]
for operation,address in [(0,0xbbfc0),(1,0xbc070),(2,0x2a2349)]:
 for a,b in angles:
  args=[bits(a),bits(b),bits(.45),bits(.1),bits(.8)]
  g.mem_write(bank+0x1c,struct.pack('<20I',*seed));r.w(bank+0x6c,0x12345678)
  if operation==2:
   ptr=r.alloc(struct.pack('<3I',*args[2:]));r.run(address,stop=0x2a2368,registers=[(UC_X86_REG_EAX,ptr)])
  else:r.run(address,argv=args)
  cases.append(dict(operation=operation,args=args,before=seed,after=words(bytes(g.mem_read(bank+0x1c,80))),metadata=r.r(bank+0x6c)))
# Same sequence as authored mainmenu startup; all inputs come from script capture.
g.mem_write(bank+0x1c,bytes(84))
for address,args in [(0xbbfc0,[45,240,.45,.45,.45]),(0xbc070,[-45,30,.1,.1,.1])]:r.run(address,argv=[bits(a) for a in args])
ptr=r.alloc(struct.pack('<3f',.1,.1,.1));r.run(0x2a2349,stop=0x2a2368,registers=[(UC_X86_REG_EAX,ptr)])
startup=words(bytes(g.mem_read(bank+0x1c,84)))
selection=[]
for flags in (0,0x00889b00,0xffffffff,0x01089b00):
 for enabled in (0,1):
  r.w(obj+4,flags);r.run(0xbbf80,argv=[enabled],registers=[(UC_X86_REG_EAX,0x10001)])
  after=r.r(obj+4);selected=None
  if enabled:selected=r.run(0x3db00,argv=[bits(1)],registers=[(UC_X86_REG_EAX,0x10001)])-bank
  selection.append(dict(flags=flags,enabled=enabled,after=after,selected_offset=selected))
out=ROOT/'analysis/frontend/customization';out.mkdir(parents=True,exist_ok=True)
report=dict(original_xbe_sha256=PIN,boundary=__doc__,cases=cases,startup=startup,selection=selection)
p=out/'cinematic-lighting-original.json';p.write_text(json.dumps(report,indent=2)+'\n')
with (out/'cinematic-lighting-original.tsv').open('w') as f:
 for c in cases:f.write(str(c['operation'])+' '+' '.join(f'{v:08x}' for v in c['args']+c['before']+c['after'])+'\n')
print(len(cases),'cases',hashlib.sha256(p.read_bytes()).hexdigest());print('startup',startup)
