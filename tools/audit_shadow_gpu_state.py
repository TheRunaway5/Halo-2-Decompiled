#!/usr/bin/env python3
"""Run original shadow target/viewport/clear/state producers up to D3D submission.

Only D3D command submission endpoints and unrelated 15370 stencil policy are
recorded services. Target selection, metadata/GetDesc, clear flags, alpha-AA
selection and shadow raster state execute untouched pinned retail code.
"""
from pathlib import Path
import struct, json
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
exec((ROOT/'analysis/multiplayer-shading/shadow-selection-original.py').read_text().split('rng=random')[0])
OUT=ROOT/'analysis/multiplayer-shading/original-xemu/shadow-gpu-audit'
commands=[]
ENDPOINTS={0x3f7870:('SetRenderTarget',8),0x3f7b30:('SetRenderTargetFast',4),
 0x3f8100:('SetViewport',4),0x3f9fc0:('Clear',24),
 0x3f5cf0:('SetRenderStateSimple',0),0x3f5a60:('MultiSampleMode',4),
 0x3f5b20:('SampleAlpha',4),0x3f59b0:('ZBias',4),
 0x3f5330:('ShadowFunc',4),0x3f53d0:('CullMode',4),
 0x15370:('StencilPolicyBoundary',0)}
def hook(uc,pc,size,_):
 if pc not in ENDPOINTS:return
 name,count=ENDPOINTS[pc];sp=uc.reg_read(UC_X86_REG_ESP)
 args=list(struct.unpack('<'+'I'*(count//4),uc.mem_read(sp+4,count))) if count else []
 row={'endpoint':f'{pc:08X}','name':name,'args':args}
 if name=='SetRenderStateSimple':row.update(header=uc.reg_read(UC_X86_REG_ECX),value=uc.reg_read(UC_X86_REG_EDX))
 if name=='SetViewport':row['viewport_words']=list(struct.unpack('<6I',uc.mem_read(args[0],24)))
 commands.append(row)
 uc.reg_write(UC_X86_REG_EAX,0)
 uc.reg_write(UC_X86_REG_EIP,struct.unpack('<I',uc.mem_read(sp,4))[0])
 uc.reg_write(UC_X86_REG_ESP,sp+4+count)
u.hook_add(UC_HOOK_CODE,hook)
u.mem_write(0x407488,struct.pack('<I',BANK+0x4000))
u.mem_write(BANK+0x4000+0x1a14,struct.pack('<I',BANK+0x6000))
u.mem_write(BANK+0x6000+4,struct.pack('<I',0x12345678))
# Original target metadata initializer, without a physical GPU allocation.
for target,width,height,depth,offset in [(10,128,128,0,0x8000),(9,512,512,1,0x9000)]:
 call(0x1df20,[1,width,height,depth,1,1,BANK+offset],target)
 assert u.reg_read(UC_X86_REG_EAX)&255==1
# Admit authored generation layer in all groups; actual layergroup remains source.
for p in (0x4c1a48,0x4c1a60,0x4c1a78):u.mem_write(p+8,struct.pack('<I',1<<10))
cases=[]
for physical_width,physical_height in [(640,480),(1280,720),(1920,1080),(853,479)]:
 for mask in (0x14,):
  for aa in (0,4,7):
   for ms in (0,1):
    u.mem_write(0x4ba014,struct.pack('<I',mask))
    u.mem_write(0x485898,struct.pack('<I',aa))
    u.mem_write(0x47fe84,bytes([ms]))
    u.mem_write(0x485602,b'\x00\x00')
    u.mem_write(0x485607,b'\x00')
    u.mem_write(0x51f3f4,b'\0'*8)
    # Explicit represented main presentation dimensions, consumed only if chosen.
    u.mem_write(0x485618,struct.pack('<4H',0,0,physical_width,physical_height))
    commands.clear();u.reg_write(UC_X86_REG_EDI,10);call(0x44370,[])
    assert u.reg_read(UC_X86_REG_EAX)&255==1
    generation=list(commands);selected=struct.unpack('<I',u.mem_read(0x4858b4,4))[0]
    expected=10 if mask&16 and not mask&32 else 9
    assert selected==expected,(mask,selected)
    viewport=next(c['viewport_words'] for c in generation if c['name']=='SetViewport')
    dimensions=128 if selected==10 else 512
    assert viewport==[0,0,dimensions,dimensions,0,0x3f800000],viewport
    binding=next(c for c in generation if c['name']=='SetRenderTarget')
    assert binding['args']==[0x4b4b70+selected*0x98,0],binding
    clear=next(c for c in generation if c['name']=='Clear')
    assert clear['args']==([0,0,0xf0,0x00ffffff,0,0] if selected==10 else [0,0,3,0,0x3f800000,0]),clear
    commands.clear();call(0x496a0,[]);generation_restore=list(commands)
    commands.clear();u.mem_write(0x5093b4,struct.pack('<I',0x900));call(0x496f0,[]);receiver=list(commands)
    commands.clear();call(0x49740,[]);receiver_restore=list(commands)
    cases.append({'presentation':[physical_width,physical_height],'feature_mask':mask,'aa_mode':aa,
     'multisample_flag':ms,'target':selected,'generation':generation,
     'generation_restore':generation_restore,'receiver':receiver,'receiver_restore':receiver_restore})
result={'xbe_sha256':pin,'source_entries':['0001DF20','00044370','00014BC0','0001DCF0','0001CD30',
 '003FDB20','003FB690','003FD9C0','00014B60','00015680','000495E0','000496A0','000496F0','00049740'],
 'fixtures':len(cases),'errors':[],'cases':cases,
 'scope':'Untouched retail producers with explicit resident target metadata, camera/AA/feature inputs; D3D submission is recorded, not GPU-executed. 15370 stencil policy is an explicit unexecuted boundary. No live camera/feature/caster selection producer claim.'}
(OUT/'shadow-gpu-state-original.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'fixtures':len(cases),'targets':sorted(set(c['target']for c in cases)),
 'fixed_color_extent':[128,128],'color_depth_attachment':0,'errors':[],'scope':result['scope']},indent=2))
