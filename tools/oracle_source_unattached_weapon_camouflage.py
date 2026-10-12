#!/usr/bin/env python3
"""Untouched weapon owner camouflage and literal carbine-display alpha remap.

Reconstruct all31 frozen original no-graph cold weapons, then execute
336F0 index37 ->33850->33670. Declared unit/parent controls are independent
inputs. The actual carbine_display remap runs unchanged19CA0 on a declared
poisoned program bank; no GPU, guest, pass assembly or live owner is inferred.
The three actual cold carbines additionally run complete BAB40/108D90/BA8C0
and both display overlays through unchanged BE8B0/1B230. Magazine controls
are explicitly declared inputs on disposable emulated metadata/body storage.
"""
from pathlib import Path
import argparse,collections,gc,hashlib,json,struct,sys,subprocess
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_multiplayer_hud_positions import Retail,PIN
from audit_rendering import Cache
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
def pack(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))
def words(data):return list(struct.unpack('<'+'I'*(len(data)//4),data))
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);a=ap.parse_args()
 proof=ROOT/'analysis/multiplayer-shading/source-netgame-weapon-original.json';original=json.loads(proof.read_text())
 births=[b for b in original['births']if b['cold_state']];assert len(births)==31 and original['xbe_sha256']==PIN
 scaffold=ROOT/'analysis/multiplayer-shading/object-cold-constructor-original.py';prefix=scaffold.read_text().split('expected=json.loads')[0]
 records=[];captures=[];ammo=[];ammo_records=[];magazine_records=[];magazine_constructors=[];constructor_writes=[];executed={};entries=collections.Counter();env=None;prior=None
 watched=[0x336f0,0x33850,0x33670,0x19ca0,0xfdad0,0xff5f0,0xbab40,0x108d90,0xba8c0,0xbe8b0,0x1b230,0x13bb90,0x13b390]
 for b in births:
  name=b['map']
  if name!=prior:
   if env:del env;gc.collect()
   path=ROOT/'assets/disc/maps'/f'{name}.map'
   if not path.exists():path=ROOT/'assets/map-pack/maps'/f'{name}.map'
   env={'path':path};exec(compile(prefix.replace("ROOT/'assets/disc/maps/zanzibar.map'","path"),str(scaffold),'exec'),env)
   g,x,put,read=[env[k]for k in ['u','x','put','read']];BASE,HEAP,HANDLES,HEADER,OBJGLOBALS=[env[k]for k in ['BASE','HEAP','HANDLES','HEADER','OBJGLOBALS']]
   assert sha(x.data)==PIN
   def observe(_g,pc,size,_):
    assert bytes(_g.mem_read(pc,size))==x.read(pc,size),(hex(pc),'changed source instructions')
    executed[pc]=size
    if pc in watched:entries[f'{pc:X}']+=1
   g.hook_add(UC_HOOK_CODE,observe);prior=name
   def magazine_write(_g,_access,address,count,value,_):
    constructor_writes.append(dict(instruction=f'{_g.reg_read(UC_X86_REG_EIP):08X}',offset=address-HEAP,size=count,value=f'{value:08X}'))
   g.hook_add(UC_HOOK_MEM_WRITE,magazine_write,begin=HEAP+0x22c,end=HEAP+0x24d)
  env['mode']='netgame';env['nxt']=0
  for ptr,size in [(HEAP,0x100000),(HANDLES,4096),(OBJGLOBALS,132),(BASE+0xb000,800)]:g.mem_write(ptr,bytes(size))
  put(HEADER+0x24,12);put(HEADER+0x38,0)
  for address,offset in [(0x51e9a0,0xa000),(0x4ea93c,0xc000),(0x4ed28c,0xc100),(0x4e030c,0xc200)]:put(address,BASE+offset);put(BASE+offset,0)
  put(BASE+0x8000,0x001e0001);put(BASE+0x8004,0x3d088889);put(BASE+0x8008,5)
  constructor_writes.clear();entry=BASE+0xb000;put(entry+b['row']*8,0xffffffff);env['run'](0x15aad0,[entry,b['row']])
  assert g.reg_read(UC_X86_REG_EIP)==env['STOP']
  handle=read(entry+b['row']*8);assert handle==0x12340000
  body=read(HANDLES+8);size=struct.unpack('<H',g.mem_read(HANDLES+6,2))[0];raw=bytes(g.mem_read(body,size))
  offset=struct.unpack_from('<h',raw,0x116)[0];assert words(raw[offset:offset+52])==b['node0']
  assert raw[0xaa]==2 and struct.unpack_from('<I',raw,0x14)[0]==0xffffffff
  context=BASE+0xe000;put(0x485a58,context);put(context,handle)
  #Original run() has no register-input interface: this is the literal caller
  #EAX37 input, using its same source stack and floating-point environment.
  def scalar():
   for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:g.reg_write(reg,0)
   g.reg_write(UC_X86_REG_EAX,37);g.reg_write(UC_X86_REG_ESP,env['STACK']);g.mem_write(env['STACK'],pack([env['STOP']]))
   g.reg_write(UC_X86_REG_EFLAGS,2);g.reg_write(UC_X86_REG_FPCW,0x27f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
   g.emu_start(0x336f0,env['STOP'],count=10000);assert g.reg_read(UC_X86_REG_EIP)==env['STOP']
   return g.reg_read(UC_X86_REG_XMM0)&0xffffffff
  result=scalar();assert result==0 and bytes(g.mem_read(body,size))==raw
  fraction=struct.unpack_from('<I',raw,0x2b0)[0]if size>=0x2b4 else 0
  records.append(('O',name,b['row'],[handle,raw[0xaa],0xffffffff,fraction],[result]))
  captures.append(dict(map=name,row=b['row'],definition=b['definition'],body_sha256=sha(raw),
      header_hex=bytes(g.mem_read(HANDLES,12)).hex(),type=raw[0xaa],parent=0xffffffff,source_word=result))
  c=env['c'];weapon=c.tags[b['definition']]['offset'];count,ptr=struct.unpack_from('<II',c.data,weapon+0x2c0)
  assert 0<=count<=2
  magazine_constructors.append(dict(map=name,row=b['row'],definition=b['definition'],name=c.tags[b['definition']]['name'],magazine_count=count,
    magazines=[dict(record_hex=bytes(c.data[ptr-c.magic+i*0x5c:ptr-c.magic+(i+1)*0x5c]).hex(),
      loaded_rounds=struct.unpack_from('<h',raw,0x22c+i*16)[0],maximum_loaded_rounds=struct.unpack_from('<h',c.data,ptr-c.magic+i*0x5c+0xa)[0])for i in range(count)],
    inactive_first_loaded_rounds=struct.unpack_from('<h',raw,0x22c)[0]if not count else None,constructor_writes=list(constructor_writes)))
  def magazine_capture(kind,count,initial,capacity,reserve):
   out,active=BASE+0xe100,BASE+0xe104;put(out,0xdeadbeef);put(active,0xdeadbeef)
   env['run'](0xff5f0,[handle,0x1200056a,out,active]);assert g.reg_read(UC_X86_REG_EIP)==env['STOP']
   actual=struct.unpack('<h',g.mem_read(body+0x22c,2))[0];stored=struct.unpack('<h',g.mem_read(body+0x22a,2))[0]
   magazine_records.append((kind,name,b['row'],[b['definition'],count,initial&65535,capacity&65535,reserve&65535],
     [actual&65535,stored&65535,g.reg_read(UC_X86_REG_EAX)&255,read(out),read(active)&255]))
  if count:initial,capacity,reserve=[struct.unpack_from('<h',c.data,ptr-c.magic+at)[0]for at in [6,0xa,0xc]]
  else:initial=capacity=reserve=0
  magazine_capture('K',count,initial,capacity,reserve)
  if env['c'].tags[b['definition']]['name'].endswith('covenant_carbine\\covenant_carbine'):
   c=env['c'];definition=c.tags[b['definition']]['offset'];fn_count,fn_ptr=struct.unpack_from('<II',c.data,definition+0x64);assert fn_count==1
   function=bytes(c.data[fn_ptr-c.magic:fn_ptr-c.magic+32]);input_name,export_name=struct.unpack_from('<II',function,4)
   material=next(t for t in c.tags.values()if t['group']=='shad'and t['name'].endswith('\\carbine_display'))
   shader=material['offset'];definition_count,definitions=struct.unpack_from('<II',c.data,shader+32);assert definition_count==1
   overlays_count,overlays_ptr=struct.unpack_from('<II',c.data,definitions-c.magic+60);assert overlays_count==2
   overlay_records=[bytes(c.data[overlays_ptr-c.magic+i*20:overlays_ptr-c.magic+(i+1)*20])for i in range(2)]
   assert all(struct.unpack_from('<I',overlay)[0]==export_name for overlay in overlay_records)
   out,active=BASE+0xe100,BASE+0xe104
   magazines,mag_ptr=struct.unpack_from('<II',c.data,definition+0x2c0)
   curve_size,curve_ptr=struct.unpack_from('<II',function,0x14)
   maximum=struct.unpack_from('<h',c.data,mag_ptr-c.magic+0xa)[0];loaded=struct.unpack_from('<h',raw,0x22c)[0]
   curve=bytes(c.data[curve_ptr-c.magic:curve_ptr-c.magic+curve_size]);assert len(curve)==20
   def call(entry,arguments):
    env['run'](entry,arguments);assert g.reg_read(UC_X86_REG_EIP)==env['STOP'],hex(entry)
    return g.reg_read(UC_X86_REG_EAX)&255
   def functions(kind,count,capacity,rounds):
    # Originals/assets stay unchanged. These named fields are explicit test
    # inputs in the disposable mapped cache/body; restore all of them below.
    put(definition+c.magic+0x2c0,count);g.mem_write(mag_ptr+0xa,struct.pack('<h',capacity));g.mem_write(body+0x22c,struct.pack('<h',rounds))
    put(out,0xdeadbeef);put(active,0xdeadbeef);direct=[call(0xff5f0,[handle,export_name,out,active]),read(out),read(active)]
    put(out,0xdeadbeef);put(active,0xdeadbeef);builtin=[call(0xff5f0,[handle,input_name,out,active]),read(out),read(active)&255]
    put(out,0xdeadbeef);resolved=[call(0xbab40,[handle,export_name,out]),read(out)]
    # Bind the literal original BE8B0 caller adapter. No callback code or
    # computed result is supplied; it invokes original BAB40 for both names.
    put(context+4,0xbe8b0);overlay_outputs=[]
    for i,overlay in enumerate(overlay_records):
     source,range_name,period=struct.unpack_from('<III',overlay)
     for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:g.reg_write(reg,0)
     g.reg_write(UC_X86_REG_EAX,overlays_ptr+i*20+12);g.reg_write(UC_X86_REG_ESI,source)
     g.reg_write(UC_X86_REG_ESP,env['STACK']);g.mem_write(env['STACK'],pack([env['STOP'],range_name,period]))
     g.reg_write(UC_X86_REG_EFLAGS,2);g.reg_write(UC_X86_REG_FPCW,0x27f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
     g.reg_write(UC_X86_REG_FPTAG,0xffff);g.reg_write(UC_X86_REG_FPSW,0)
     g.emu_start(0x1b230,env['STOP'],count=200000);assert g.reg_read(UC_X86_REG_EIP)==env['STOP']
     overlay_outputs.append(g.reg_read(UC_X86_REG_XMM0)&0xffffffff)
    put(context+4,0)
    fn=words(function);inputs=[b['definition'],int(material['datum'],16),count,capacity&65535,rounds&65535,*fn[:5],fn[7],*words(curve)]
    outputs=[*direct,*builtin,*resolved,*overlay_outputs]
    ammo_records.append((kind,name,b['row'],inputs,outputs));return direct,builtin,resolved,overlay_outputs
   direct,builtin,resolved,overlay_outputs=functions('C',magazines,maximum,loaded)
   assert builtin==[1,0x3f800000,1]and resolved==[1,0x3f800000]
   row=dict(map=name,row=b['row'],definition=b['definition'],input_name=f'{input_name:08X}',export_name=f'{export_name:08X}',function_hex=function.hex(),
     curve_hex=curve.hex(),magazines=magazines,body_hex=raw.hex(),body_sha256=sha(raw),material=material['datum'],
     overlay_records_hex=[overlay.hex()for overlay in overlay_records],
     authored_magazine_hex=bytes(c.data[mag_ptr-c.magic:mag_ptr-c.magic+0x5c]).hex(),loaded_rounds=struct.unpack_from('<h',raw,0x22c)[0],
     maximum_loaded_rounds=maximum,direct_export=direct,builtin_input=builtin,generic_export=resolved,overlay_outputs=overlay_outputs)
   ammo.append(row);print('original carbine ammo',name,b['row'],loaded,maximum,builtin,resolved,overlay_outputs,flush=True)
   for count,capacity,rounds in [(1,18,0),(1,18,9),(1,18,1),(1,18,-1),(1,18,19),(1,0,18),(0,18,18),(1,-18,18),(1,-18,-9),(1,32767,32767),(1,-32768,-32768)]:
    functions('M',count,capacity,rounds)
   put(definition+c.magic+0x2c0,magazines);g.mem_write(mag_ptr+0xa,struct.pack('<h',maximum));g.mem_write(body,raw)
   assert bytes(g.mem_read(body,size))==raw
   if name=='headlong':
    for initial,capacity,reserve in [(0,18,72),(9,18,72),(36,18,10),(36,18,0),(-1,18,72),(36,-18,72),(32767,-32768,32767),(-32768,32767,32767),(36,0,72)]:
     g.mem_write(body,raw);g.mem_write(mag_ptr+6,struct.pack('<h',initial));g.mem_write(mag_ptr+0xa,struct.pack('<hh',capacity,reserve))
     env['run'](0xfdad0,[handle,0,0]);assert g.reg_read(UC_X86_REG_EIP)==env['STOP']
     magazine_capture('D',magazines,initial,capacity,reserve)
    g.mem_write(mag_ptr,bytes(c.data[mag_ptr-c.magic:mag_ptr-c.magic+0x5c]));g.mem_write(body,raw)
 #Declared consumer controls; no shader services are hooked by Retail here.
 r=Retail();x=env['x']
 def trace(_g,pc,size,_):
  assert bytes(_g.mem_read(pc,size))==x.read(pc,size),(hex(pc),'changed source instructions')
  executed[pc]=size
  if pc in watched:entries[f'{pc:X}']+=1
 r.g.hook_add(UC_HOOK_CODE,trace)
 pool=r.alloc(bytes(0x50));table=r.alloc(bytes(36));child=r.alloc(bytes(0x340));parent=r.alloc(bytes(0x340));ctx=r.alloc(bytes(0x80))
 r.w(pool+0x24,12);r.w(pool+0x38,3);r.w(pool+0x44,table);r.w(0x4e0300,pool);r.w(0x485a58,ctx)
 r.g.mem_write(table,struct.pack('<HBBHHI',0x1234,0,2,0xffff,0x340,child));r.g.mem_write(table+12,struct.pack('<HBBHHI',0x1234,0,0,0xffff,0x340,parent))
 r.w(ctx,0x12340000)
 def control(kind,child_type,parent_handle,parent_type,fraction):
  r.g.mem_write(table+3,bytes([child_type]));r.g.mem_write(table+15,bytes([parent_type]))
  r.g.mem_write(child+0xaa,bytes([child_type]));r.w(child+0x14,parent_handle);r.w(child+0x2b0,fraction)
  r.g.mem_write(parent+0xaa,bytes([parent_type]));r.w(parent+0x14,0xffffffff);r.w(parent+0x2b0,fraction)
  r.run(0x336f0,registers=[(UC_X86_REG_EAX,37)]);result=r.g.reg_read(UC_X86_REG_XMM0)&0xffffffff
  records.append((kind,'control',0,[child_type,parent_handle,parent_type,fraction],[result]));return result
 for fraction in [0,0x3f000000,0x3f800000]:
  assert control('N',2,0xffffffff,0,fraction)==0
  assert control('P',2,0x12340001,0,fraction)==fraction
  assert control('Q',2,0x12340001,2,fraction)==0
  assert control('V',0,0xffffffff,0,fraction)==fraction
 assert control('Z',2,0x12340002,0,0x3f000000)==0 #Actual null parent body pointer.
 #Actual literal carbine_display only has scalar37 alpha; no other input is
 #substituted. Original19CA0 binds its authored remap on declared program data.
 for name in ['headlong','containment']:
  path=ROOT/('assets/disc/maps'if name=='headlong'else'assets/map-pack/maps')/f'{name}.map';c=Cache(path)
  tag=next(t for t in c.tags.values()if t['group']=='shad'and t['name'].endswith('\\carbine_display'))
  _,shader=c.tag(tag['datum'],'shad');definition=c.block(shader+32,124)[0];_,stem=c.tag(c.u32(definition),'stem');template=c.block(stem+88,40)[0]
  passes=c.block(template+16,10);assert len(passes)==1
  _,spas=c.tag(c.u32(passes[0]+4),'spas');pp=c.block(spas+28,88)[0];impl=c.block(pp,306)[0]
  packed_range=c.u16(impl+0x122);remaps=c.block(pp+56,4);mapping=c.u32(remaps[packed_range&511]);assert packed_range>>9==1 and mapping==0x25030480 and c.u16(impl+246)==0
  state=r.alloc(bytes(0x1800));pass_pp=r.alloc(bytes(88));implementation=r.alloc(bytes(306));map_ptr=r.alloc(pack([mapping]));range_ptr=r.alloc(struct.pack('<H',0x200))
  r.w(state+0x20,pass_pp);r.w(state+0x24,implementation);r.w(pass_pp+0x3c,map_ptr)
  initial=[0xa5000000+i*0x010203 for i in range(60)]
  for fraction in [0,0x3f000000,0x3f800000]:
   r.w(state+0x5c+37*4,fraction);r.g.mem_write(state+0x1424,pack(initial));r.run(0x19ca0,argv=[range_ptr],registers=[(UC_X86_REG_ESI,state)])
   result=words(bytes(r.g.mem_read(state+0x1424,240)))
   records.append(('A',name,0,[tag['datum'],mapping,fraction,*initial],result))
  c.close()
 fixture=ROOT/'tests/fixtures/retail_source_unattached_weapon_camouflage.tsv'
 fixture.write_text('# kind map ordinal inputwords | unchanged original336F0/33670 or19CA0 outputwords\n'+''.join(f'{kind} {name} {ordinal} '+' '.join(f'{v:08X}'for v in inputs)+' | '+' '.join(f'{v:08X}'for v in outputs)+'\n'for kind,name,ordinal,inputs,outputs in records))
 ammo_fixture=ROOT/'tests/fixtures/retail_source_cold_carbine_ammunition.tsv'
 ammo_fixture.write_text('# C actual cold constructor; M declared magazine/body controls; original FF5F0, BAB40, BE8B0/1B230\n'+''.join(f'{kind} {name} {ordinal} '+' '.join(f'{v:08X}'for v in inputs)+' | '+' '.join(f'{v:08X}'for v in outputs)+'\n'for kind,name,ordinal,inputs,outputs in ammo_records))
 magazine_fixture=ROOT/'tests/fixtures/retail_source_cold_weapon_magazines.tsv'
 magazine_fixture.write_text('# K actual cold constructor; D declared original FDAD0 control; def,count,tag+6,+A,+C | body+22C,+22A,FF5F0return,fraction,active\n'+''.join(f'{kind} {name} {ordinal} '+' '.join(f'{v:08X}'for v in inputs)+' | '+' '.join(f'{v:08X}'for v in outputs)+'\n'for kind,name,ordinal,inputs,outputs in magazine_records))
 report=dict(xbe_sha256=PIN,constructor_proof_sha256=sha(proof.read_bytes()),scaffold_sha256=sha(scaffold.read_bytes()),
   fpcw='027F',mxcsr='1F80',
   tool_sha256=sha(Path(__file__).read_bytes()),fixture_sha256=sha(fixture.read_bytes()),source_entries=dict(entries),
   cases=len(records),constructor_cases=len(captures),original_words=sum(len(row[4])for row in records),
   unique_instructions=len(executed),instruction_sha256=sha(b''.join(x.read(pc,size)for pc,size in sorted(executed.items()))),
   constructors=captures,magazine_constructors=magazine_constructors,carbine_object_functions=ammo,ammunition_cases=len(ammo_records),ammunition_words=sum(len(row[4])for row in ammo_records),
   ammunition_fixture_sha256=sha(ammo_fixture.read_bytes()),cache_sha256={name:original['cache_sha256'][name]for name in sorted({b['map']for b in births})},
   magazine_cases=len(magazine_records),magazine_words=sum(len(row[4])for row in magazine_records),magazine_fixture_sha256=sha(magazine_fixture.read_bytes()),
   services_replaced=['Inherited constructor storage BC280/18E340 and BF380 pressure boundary only; unchanged owner consumer,19CA0 and all ammunition/overlay routines'],
   declared_inputs=['Zero-capacity actual effe/lsnd/ligh/physics pools and disabled registry as in the inherited constructor proof',
     'Explicit original BE8B0 shader callback/context binding; complete original caller registration lifecycle is not claimed',
     'Named M/D cases edit only disposable emulated magazine fields and object loaded shorts; actual cold C/K cache/body inputs are unchanged'],
   scope=__doc__)
 if a.native:
  result=subprocess.run([str(a.native.resolve())],cwd=ROOT,capture_output=True,text=True);print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr);assert result.returncode==0
  report['native_validation']={'binary_sha256':sha(a.native.read_bytes()),'return_code':0,'stdout':result.stdout.strip()}
 output=ROOT/'analysis/multiplayer-shading/source-unattached-weapon-camouflage-original.json';output.write_text(json.dumps(report,indent=2)+'\n')
 fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k!='constructors'},indent=2)+'\n')
 ammo_fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k!='constructors'},indent=2)+'\n')
 magazine_fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k!='constructors'},indent=2)+'\n')
 print(f'{len(captures)} actual original cold owners,13 parent/type controls,6 authored alpha banks; {report["original_words"]} words; '+
   f'{len(magazine_records)} magazine cases/{report["magazine_words"]} words and {len(ammo_records)} full ammo/overlay cases/{report["ammunition_words"]} words')
if __name__=='__main__':main()
