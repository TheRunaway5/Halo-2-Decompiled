#!/usr/bin/env python3
"""Execute unchanged original scenario screen-effect admission at13870.

The actual mainmenu.scnr named functions and screen-effect records are retained
at their cache addresses. Render-view/environment gates are supplied test inputs;
16B10 graphics reset is recorded as a no-op and2D140 records the selected effect
rather than rendering. This proves selection under those gates, not that a live
Appearance47 frame has those gate values, nor framebuffer parity.
"""
from pathlib import Path
import json,struct,hashlib
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail,ROOT,PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
cache=Cache(ROOT/'assets/disc/maps/mainmenu.map');retail=Retail();g=retail.g
_,off=cache.tag(cache.scenario,'scnr');tag=cache.tags[cache.scenario]
g.mem_map(0x80000000,0x4000000);g.mem_write(tag['address'],bytes(cache.data[off:off+tag['size']]))
retail.w(0x4e0350,tag['address']);states=retail.alloc(bytes(0x204));retail.w(0x4e6740,states)
rows=[];effects=[];count={}
def supplied_ret(value=0,args=0):
 sp=g.reg_read(UC_X86_REG_ESP);retail.ret(value);g.reg_write(UC_X86_REG_ESP,sp+4+args)
def hook(_g,address,size,_):
 count[address]=count.get(address,0)+1
 if address==0x16b10:supplied_ret()
 elif address==0x2d140:
  sp=g.reg_read(UC_X86_REG_ESP);p=retail.r(sp+4)
  effects.append(dict(effect=f'{g.reg_read(UC_X86_REG_EAX):08x}',inputs_words=[f'{retail.r(p+i*4):08x}'for i in range(4)],mode=retail.r(sp+8),layer=retail.r(sp+12)))
  supplied_ret(args=16)
g.hook_add(UC_HOOK_CODE,hook)
base=dict(mode=0,flag485607=0,flag4850c8=0,viewcount=1,viewport=[0,0,480,640],state_value=0)
captured=json.loads((ROOT/'analysis/frontend/customization/world-filter-audit/original-appearance-world-filter.json').read_text())
cases=[('ordinary-world',{}),('actual-appearance47',{k:captured[k]for k in ['flag485607','flag4850c8','viewcount','viewport']}|dict(mode=captured['view_mode'])),('state-one',dict(state_value=0x3f800000)),('excluded-mode5',dict(mode=5)),('excluded-mode6',dict(mode=6)),('excluded-mode7',dict(mode=7)),('offscreen-flag',dict(flag485607=1)),('offscreen-context',dict(flag4850c8=1)),('multiple-views',dict(viewcount=2)),('partial-width',dict(viewport=[0,0,480,320])),('partial-origin',dict(viewport=[0,1,480,640]))]
for label,changes in cases:
 values=base|changes;retail.w(0x485898,values['mode']);g.mem_write(0x485607,bytes([values['flag485607']]));retail.w(0x4850c8,values['flag4850c8']);retail.w(0x4ba04c,values['viewcount']);g.mem_write(0x485648,struct.pack('<4h',*values['viewport']));g.mem_write(0x4b9dd0,struct.pack('<4h',0,0,480,640))
 for i in range(2):retail.w(states+4+i*32,values['state_value'])
 effects.clear();count.clear();retail.run(0x13870,argv=[0,0])
 expected=[dict(effect='e4380218',inputs_words=['3f800000','00000000','00000000','00000000'],mode=2,layer=2)] if label in ['ordinary-world','actual-appearance47','state-one'] else []
 if label=='state-one':expected=[dict(effect='e4320212',inputs_words=['3f800000','00000000','00000000','00000000'],mode=2,layer=2)]+expected
 assert effects==expected,(label,effects)
 rows.append(dict(label=label,supplied_gates=values,selected_effects=list(effects),source_calls={f'{a:08x}':n for a,n in count.items() if a in [0x13870,0x22710,0x134fc0,0x134fe0,0x13b390,0x13bb90,0x2d140]}))
assets=[]
for i in range(cache.u32(off+0x3d0)):
 p=cache.u32(off+0x3d4)-cache.magic+i*36;indices=struct.unpack_from('<2h',cache.data,p+32);sid=cache.u32(p+24);name=cache.string(cache.u32(368)+cache.u32(cache.u32(364)+(sid&65535)*4),100)
 effect=cache.u32(p+20);interp=cache.u32(off+0x3c4)-cache.magic+indices[0]*24;size=cache.u32(interp+12);function=cache.u32(interp+16)-cache.magic
 assets.append(dict(effect=f'{effect:08x}',name=cache.tags[effect]['name'],record_hex=bytes(cache.data[p:p+36]).hex(),function_name=name,indices=list(indices),function_hex=bytes(cache.data[function:function+size]).hex()))
bound=json.loads((ROOT/'analysis/frontend/customization/world-filter-audit/original-appearance-world-filter-bound.json').read_text())
assert bound['shader']=='e4390219' and bound['implementation_index']==0
original=[0]*60;original[8]=0x0c010000;original[9]=0x80;original[10]=0x0080b333;original[34]=0x08010000;original[43]=0x001a2340;original[45]=0x20c0;original[53]=0x11101;original[54]=1
assert list(struct.unpack('<60I',bytes.fromhex(bound['pixel_program_hex'])))==original
assert captured['draws']==[dict(effect='e4380218',input_words=['3f800000','00000000','00000000','00000000'],layer=2,mode=2,quad_hex=captured['draws'][0]['quad_hex'],ui_clock_ms=captured['ui_clock_ms'])] and captured['same_ui_clock']
report=dict(xbe_sha256=PIN,scope=__doc__,scenario=f'{cache.scenario:08x}',assets=assets,cases=rows,live_appearance_admission_verified=True,live_admission='analysis/frontend/customization/world-filter-audit/original-appearance-world-filter.json',live_bound_program='analysis/frontend/customization/world-filter-audit/original-appearance-world-filter-bound.json',bound_title_program_words_equal=60,framebuffer_verified=False)
out=ROOT/'analysis/frontend/customization/appearance-world-filter-original.json';out.write_text(json.dumps(report,indent=2)+'\n');print('11 source admission cases pass; actual47 bound program equals60 title words; fixture',out,'sha256',hashlib.sha256(out.read_bytes()).hexdigest())
