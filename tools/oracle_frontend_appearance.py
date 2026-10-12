#!/usr/bin/env python3
"""Execute original profile colors and child-page preview/cancel producers.

The original pinned XBE routines execute unchanged. The actual mainmenu matg
palette is copied into a ready tag-global environment with pointer relocation.
Child/widget lookup, list focus, unrelated base update, text expansion, emblem
renderer, window-return virtuals and final object-color sinks are supplied seams.
No color, focus write, snapshot restoration or species policy is native-derived.
"""
import hashlib, json, struct
from pathlib import Path
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from audit_rendering import Cache
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EBP

OUT=ROOT/'analysis/frontend/profile-regression'
r=Retail();g=r.g;cache=Cache(ROOT/'assets/disc/maps/mainmenu.map')
t=next(t for t in cache.tags.values() if t['group']=='matg');p=t['address']-cache.magic
palette=cache.block(p+0x160,12);assert len(palette)==18
palette_bytes=b''.join(cache.data[a:a+12] for a in palette)
header=r.alloc(bytes(cache.data[p:p+0x168]));bank=r.alloc(palette_bytes)
r.w(header+0x164,bank);r.w(0x4e034c,header);r.w(0x4e0350,1)
profile=r.alloc(bytes(0x1e0));r.run(0x1a0750,registers=[(UC_X86_REG_EDX,profile)])
default=bytes(g.mem_read(profile+0x118,16));assert default[:8]==bytes([10,0,0,10,0,0,0,0])
screen=r.alloc(bytes(0xab0));models=[r.alloc(bytes(0x100)) for _ in range(2)]
vtable=r.alloc(bytes(0x60));r.w(screen+0x7f8,vtable);r.w(vtable+0x2c,0x3000100);r.w(vtable+0x30,0x3000200)
focus=0;calls=[];counts={};write_sinks=[]
functions=[0x1a06a0,0x7f790,0x2b10a3,0x2c8d7c,0x2cb350,0x2cb4e2,0x2cb174,0x2cb1d4,0x2c87fe,0x2cb30a,0x2cb4a2,0x153850]
def ret(value=0,args=0):
 sp=g.reg_read(UC_X86_REG_ESP);r.ret(value);g.reg_write(UC_X86_REG_ESP,sp+4+args)
def hook(_g,a,_size,_ctx):
 if a in functions:counts[a]=counts.get(a,0)+1
 sp=g.reg_read(UC_X86_REG_ESP)
 if a==0x22e391:ret()
 elif a==0x24c447:ret(focus)
 elif a==0x22edb8:
  kind,index,recurse=r.read(sp+4,'<3I');assert not recurse
  ret(models[index] if kind==7 else 0,12)
 elif a==0x22f042:ret(args=8)
 elif a==0x2b102f:
  ptr=r.r(sp+4);calls.append(('emblem',bytes(g.mem_read(ptr,4)).hex()));ret(args=4)
 elif a==0x2b0f76:
  widget,slot,ptr=r.read(sp+4,'<3I');calls.append(('color',models.index(widget),slot,bytes(g.mem_read(ptr,12)).hex()));ret(args=12)
 elif a==0x2b12ca:ret(args=4)
 elif a==0x2300ea:ret(1,4)
 elif a==0x3000100:ret(5)
 elif a==0x3000200:ret(4)
 elif a==0x14800c:write_sinks.append([g.reg_read(UC_X86_REG_ECX),g.reg_read(UC_X86_REG_EAX)]);ret()
g.hook_add(UC_HOOK_CODE,hook)
color_cases=[]
for species in (0,1,2):
 for i in range(-1,19):
  appearance=bytearray(default);appearance[:4]=bytes([i&255]*4);appearance[4]=species;appearance[5:9]=bytes([7,3,1,0])
  ptr=r.alloc(appearance);calls.clear();counts.clear()
  for model in models:g.mem_write(model+0x6e,b'\x01')
  for kind,model in enumerate(models):r.run(0x2b10a3,argv=[kind],registers=[(UC_X86_REG_EAX,ptr),(UC_X86_REG_EBX,model)])
  visibility=[bool(r.read(m+0x6e,'<B')[0]) for m in models]
  colors=[c[3] for c in calls if c[0]=='color']
  assert len(colors)==(4 if species in (0,1) else 0)
  color_cases.append(dict(species=species,index=i,appearance=appearance.hex(),visibility=visibility,color_hex=colors,emblem=[c[1] for c in calls if c[0]=='emblem']))
parent_cases=[]
for species in (0,1,2):
 appearance=bytes([8,12,9,1,species,26,9,0])+bytes(8)
 g.mem_write(0x54e6f0,appearance);calls.clear();counts.clear()
 r.run(0x2c8d7c,registers=[(UC_X86_REG_ECX,screen)])
 parent_cases.append(dict(species=species,appearance=appearance.hex(),visibility=[bool(r.read(m+0x6e,'<B')[0]) for m in models],color_hex=[c[3] for c in calls if c[0]=='color'],executed={f'{a:08x}':n for a,n in counts.items()}))
assert parent_cases[0]['visibility']==[True,False] and parent_cases[1]['visibility']==[False,True] and parent_cases[2]['visibility']==[False,False]
assert all(len(c['color_hex'])==(4 if c['species']<2 else 0) for c in parent_cases)
# Original name-group color corridor; input is the raw resolved primary.
name_cases=[];frame=r.alloc(bytes(0x200))+0x100;widget=r.alloc(bytes(0x100))
for i in range(18):
 primary=palette_bytes[i*12:(i+1)*12]
 g.mem_write(frame-0x6c,primary);r.w(frame+8,widget)
 r.run(0x2b00f0,stop=0x2b0130,registers=[(UC_X86_REG_EBP,frame),(UC_X86_REG_EDI,frame-0x30)])
 name_cases.append(dict(index=i,primary_hex=primary.hex(),result_hex=bytes(g.mem_read(widget+0x28,12)).hex()))
preview=[]
for slot in range(4):
 for f in list(range(18))+[-2,-1,18]:
  for gate in (0,2):
   g.mem_write(0x54e6f0,default);r.w(screen+0xa9c,slot);g.mem_write(screen+0x42,bytes([gate]));focus=f&0xffffffff;calls.clear()
   r.run(0x2cb350,registers=[(UC_X86_REG_ECX,screen)])
   preview.append(dict(page=50,slot=slot,focus=f,animation_flags=gate,result=bytes(g.mem_read(0x54e6f0,16)).hex()))
for page,entry in ((52,0x2cb4e2),(49,0x2cb174),(233,0x2cb174)):
 for f in (-1,0,1,17):
  for gate in (0,2):
   g.mem_write(0x54e6f0,default);r.w(screen+0xa9c,int(page==233));g.mem_write(screen+0x42,bytes([gate]));focus=f&0xffffffff
   r.run(entry,registers=[(UC_X86_REG_ECX,screen)])
   preview.append(dict(page=page,slot=0,focus=f,animation_flags=gate,result=bytes(g.mem_read(0x54e6f0,16)).hex()))
# Snapshot restoration and foreground-only X toggle operate on original settings.
cancel=[];event=r.alloc(struct.pack('<3I',5,0,1))
for page in (49,233,50,52):
 for button in (1,13,2):
  for changed in (0,1):
   g.mem_write(screen+0x614,bytes([changed]));g.mem_write(screen+0x618,bytes(g.mem_read(profile,0x1e0)))
   g.mem_write(0x54e6f0,bytes([1,2,3,4,1,7,8,1])+bytes(8));r.w(screen+0xa9c,int(page==233));r.w(event+8,button)
   r.run(0x2cb1d4 if page in (49,233) else 0x2c87fe,argv=[event],registers=[(UC_X86_REG_ECX,screen)])
   cancel.append(dict(page=page,param=button,changed=changed,result=bytes(g.mem_read(0x54e6f0,16)).hex()))
accept=[];item=r.alloc(bytes(4));list_ptr=screen+0x7f8
for page,entry in ((50,0x2cb30a),(52,0x2cb4a2)):
 for slot in range(4 if page==50 else 1):
  for value in (-2,-1,0,1,17,18):
   g.mem_write(0x54e6f0,default);r.w(list_ptr+0x2a0,slot);r.w(item,value)
   r.run(entry,argv=[0,item],registers=[(UC_X86_REG_ECX,list_ptr)])
   accept.append(dict(page=page,slot=slot,value=value,result=bytes(g.mem_read(0x54e6f0,16)).hex()))
fixture=dict(original_xbe_sha256=PIN,map_sha256=hashlib.sha256(cache.data).hexdigest(),palette_hex=palette_bytes.hex(),default_appearance=default.hex(),name_cases=name_cases,parent_cases=parent_cases,color_cases=color_cases,preview_cases=preview,cancel_cases=cancel,accept_cases=accept,boundaries=__doc__.splitlines()[2:])
OUT.mkdir(exist_ok=True,parents=True);path=OUT/'frontend-appearance-original.json';path.write_text(json.dumps(fixture,indent=2)+'\n')
with (OUT/'frontend-appearance-original.tsv').open('w') as out:
 out.write('# Original XBE color resolution; source bytes9 followed by 12 output words, or hidden.\n')
 for c in color_cases:
  if c['color_hex']:
   words=struct.unpack('<12I',bytes.fromhex(''.join(c['color_hex'])))
   out.write(c['appearance'][:18]+' '+' '.join(f'{w:08x}' for w in words)+'\n')
 for c in parent_cases[:1]:out.write(c['appearance'][:18]+' '+' '.join(f'{w:08x}' for w in struct.unpack('<12I',bytes.fromhex(''.join(c['color_hex']))))+'\n')
 out.write('# Literal mainmenu palette words\n')
 for i in range(18):out.write('palette '+str(i)+' '+' '.join(f'{w:08x}' for w in struct.unpack_from('<3I',palette_bytes,i*12))+'\n')
 out.write('default '+default.hex()[:18]+'\n')
 for c in name_cases:out.write('name '+str(c['index'])+' '+' '.join(f'{w:08x}' for w in struct.unpack('<3I',bytes.fromhex(c['result_hex'])))+'\n')
 for c in preview:out.write(f"preview {c['page']} {c['slot']} {c['focus']} {c['animation_flags']} {default.hex()[:18]} {c['result'][:18]}\n")
print(f'{len(color_cases)} original color/species cases, {len(preview)} preview, {len(cancel)} cancel/toggle, {len(accept)} accepted writes')
print('fixture SHA256 '+hashlib.sha256(path.read_bytes()).hexdigest())
