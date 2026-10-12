#!/usr/bin/env python3
"""Original singleton netgame births, item scale and weapon timer/control.

All source bytes are unchanged. Cold storage services are inherited from the
independent constructor scaffold; no selector, pose, scale or timer is hooked.
The game engine mode callback is an explicit input. Effect/looping-sound/light and
physics capacity0 banks are explicit for full levitated constructor controls.
Full construction is bounded to literal no-animation-graph, non-spinning rows;
other rows retain complete original descriptor, scale and timer coverage.
"""
from pathlib import Path
import argparse,gc,hashlib,json,random,struct,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_dynamic_model_shader_assets import MAPS
from inspect_multiplayer import Cache
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def words(d):return list(struct.unpack('<'+'I'*(len(d)//4),d))
def pack(v):return struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v))
def bits(f):return words(struct.pack('<f',f))[0]
def sha(d):return hashlib.sha256(d).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);a=ap.parse_args()
 scaffold=ROOT/'analysis/multiplayer-shading/object-cold-constructor-original.py'
 prefix=scaffold.read_text().split('expected=json.loads')[0]
 fixtures=[];births=[];cache_hashes={};seen={};timer_count=0;scale_count=0;update_count=0;constructor_count=0
 for name in MAPS:
  path=ROOT/'assets/disc/maps'/f'{name}.map'
  if not path.exists():path=ROOT/'assets/map-pack/maps'/f'{name}.map'
  env={'path':path}
  exec(compile(prefix.replace("ROOT/'assets/disc/maps/zanzibar.map'","path"),str(scaffold),'exec'),env)
  g=env['u'];x=env['x'];c=env['c'];assert sha(x.data)==PIN
  cache_hashes[name]=sha(c.data)
  put,read,run=env['put'],env['read'],env['run'];BASE,HEAP,HANDLES,HEADER,OBJGLOBALS=[env[k]for k in ['BASE','HEAP','HANDLES','HEADER','OBJGLOBALS']]
  ENTRY=BASE+0xb000;CLOCK=BASE+0x8000;ENGINE=BASE+0xd000;CALLBACK=env['STOP']+0x100
  put(0x51e9a0,BASE+0xa000);put(BASE+0xa000,0)
  put(0x4ea93c,BASE+0xc000);put(0x4ed28c,BASE+0xc100);put(0x4e030c,BASE+0xc200)
  put(0x4e9ae8,ENGINE);put(ENGINE+0xc14,2);put(0x55e4d0+8,ENGINE+0xc20)
  put(ENGINE+0xc20,ENGINE+0xc30);put(ENGINE+0xc30,CALLBACK)
  engine_mode=[2];stage=[''];output=[None];request=[False];observed=[]
  def observe(emu,pc,size,_):
   if pc==CALLBACK:
    env['ret'](emu,value=engine_mode[0]);return
   assert bytes(emu.mem_read(pc,size))==x.read(pc,size),(name,hex(pc),'changed source bytes')
   seen[pc]=size
   if pc==0xb7b40:
    ptr=read(emu.reg_read(UC_X86_REG_ESP)+4);observed.append(words(bytes(emu.mem_read(ptr,196))))
    if stage[0]=='descriptor':output[0]=observed[-1];emu.emu_stop()
   if pc==0xb7680 and stage[0]=='scale':
    output[0]=emu.reg_read(UC_X86_REG_XMM3)&0xffffffff;emu.emu_stop()
   if pc==0x15aad0 and stage[0]=='timer':request[0]=True;emu.emu_stop()
  g.hook_add(UC_HOOK_CODE,observe)
  env['mode']='netgame'
  def reset():
   env['nxt']=0;env['calls'].clear();env['writes'].clear();observed.clear();output[0]=None;request[0]=False
   for ptr,size in [(HEAP,0x100000),(HANDLES,4096),(OBJGLOBALS,132),(ENTRY,800)]:g.mem_write(ptr,bytes(size))
   put(HEADER+0x38,0);put(CLOCK,0x001e0001);put(CLOCK+4,0x3d088889);put(CLOCK+8,5)
   for i in range(100):g.mem_write(ENTRY+i*8,struct.pack('<Ihh',0xffffffff,-1,0))
  n,p=struct.unpack_from('<II',c.data,c.scenario+0x120);records=p-c.magic
  for ordinal in range(n):
   r=records+ordinal*0x90;raw=bytes(c.data[r:r+0x90]);classification=raw[0x14]
   collection=words(raw[0x5c:0x60])[0]
   if classification!=0 or collection==0xffffffff or words(raw[0x58:0x5c])[0]!=0x69746d63:continue
   col=c.tags[collection];q=col['offset'];count,items=struct.unpack_from('<II',c.data,q)
   if count!=1:continue
   item=bytes(c.data[items-c.magic:items-c.magic+16]);definition=words(item[8:12])[0]
   if c.tags[definition]['group']!='weap':continue
   reset();put(ENTRY+ordinal*8,0xffffffff);stage[0]='descriptor';run(0x15aad0,[ENTRY,ordinal])
   descriptor=output[0];assert descriptor and descriptor[0]==definition
   d=c.tags[definition]['offset'];metadata=bytes(c.data[d:d+0xcc]);scale_input=words(metadata[0xc4:0xcc])
   body=HEAP;put(HANDLES+8,body);put(body,definition);put(body+0x12c,0)
   stage[0]='scale';g.reg_write(UC_X86_REG_EAX,0)
   #run resets registers: call10DAD0 through a stack wrapper preservingEAX0.
   run(0x10dad0);scale=output[0];assert scale is not None
   model=words(metadata[0x38:0x3c])[0]
   animation_graph=words(c.data[c.tags[model]['offset']+0x14:c.tags[model]['offset']+0x18])[0]if model!=0xffffffff else 0xffffffff
   widget_count,widget_ptr=struct.unpack_from('<II',metadata,0x94)
   widget_groups=[struct.unpack_from('<I',c.data,widget_ptr-c.magic+i*24)[0]for i in range(widget_count)]
   root=None;update=None;cold_state=None;constructor_scope='not levitated'
   if words(raw[:4])[0]&1:
    constructor_scope='animation graph owner unresolved'if animation_graph!=0xffffffff else 'source spin owner unresolved'if words(metadata[0xbc:0xc0])[0]&1 else 'complete cold no-graph levitated constructor with explicit empty widget pools'
   if words(raw[:4])[0]&1 and animation_graph==0xffffffff and not(words(metadata[0xbc:0xc0])[0]&1):
    assert set(widget_groups)<={0x65666665,0x6c736e64,0x6c696768},(name,ordinal,'unsupported widget pool',widget_groups)
    reset();stage[0]='constructor'
    try:run(0x15aad0,[ENTRY,ordinal])
    except Exception:
     print('constructor failure',name,ordinal,hex(g.reg_read(UC_X86_REG_EIP)),[hex(v)for v in env['calls']],flush=True);raise
    handle=read(ENTRY+ordinal*8);assert handle==0x12340000
    ptr=read(HANDLES+8);off=struct.unpack('<h',g.mem_read(ptr+0x116,2))[0]
    root=words(bytes(g.mem_read(ptr+off,52)));assert len(root)==13 and root[0]==scale
    born=bytes(g.mem_read(ptr,struct.unpack('<H',g.mem_read(HANDLES+6,2))[0]))
    #Full original constructor fields used by the native bounded cold state.
    #12C is a byte,12E a separate signed timer;16C/184 are shader inputs.
    cold_state=[read(ptr+4),read(ptr+0x14),born[0xc1],born[0x10a],
      struct.unpack_from('<h',born,0x112)[0]&65535,born[0x12c],
      struct.unpack_from('<h',born,0x12e)[0]&65535,read(ptr+0x14c),read(ptr+0x154),
      read(ptr+0x16c),read(ptr+0x184),read(ptr+0xc8)]
    constructor_count+=1
    #Actual activated sourceitem path, explicit registered bit8; no registry
    #membership/location or activation service is inferred by this control.
    put(ptr+4,read(ptr+4)|0x100);before=bytes(g.mem_read(ptr,len(born)))
    run(0x10b4d0,[handle]);after=bytes(g.mem_read(ptr,len(born)))
    assert before==after and (g.reg_read(UC_X86_REG_EAX)&255)==0,(name,ordinal,'levitated update mutated')
    update=words(before[4:8])+[words(before[0x14:0x18])[0],before[0x12c],
       words(before[0x14c:0x150])[0],words(before[0x154:0x158])[0],before[0xc1],
       struct.unpack_from('<h',before,0x12e)[0]&65535,struct.unpack_from('<h',before,0x112)[0]&65535,read(ptr+0xc8)]
    fixtures.append(('U',name,ordinal,[definition,*update],[0,*update]));update_count+=1
    #Independent source controls for10B850's registered/collision flag. Only
    #explicit caller state changes; the entire item body is checked for writes.
    for registered in [0,1]:
     for collision in [-1,0]:
      control_body=bytearray(born)
      initial_flags=cold_state[0]|(0x100 if registered else 0)
      struct.pack_into('<I',control_body,4,initial_flags);struct.pack_into('<h',control_body,0x112,collision)
      g.mem_write(ptr,bytes(control_body));run(0x10b4d0,[handle])
      final_flags=read(ptr+4)
      allowed=bytearray(control_body);struct.pack_into('<I',allowed,4,final_flags)
      assert bytes(g.mem_read(ptr,len(born)))==bytes(allowed),(name,ordinal,'collision control mutated other item fields')
      assert(g.reg_read(UC_X86_REG_EAX)&255)==0
      controlled=update.copy();controlled[0]=initial_flags;controlled[7]=collision&65535
      resulting=controlled.copy();resulting[0]=final_flags
      fixtures.append(('U',name,ordinal,[definition,*controlled],[0,*resulting]));update_count+=1
   births.append(dict(map=name,row=ordinal,definition=definition,raw_row=raw.hex(),item=item.hex(),
      scale=scale,descriptor=descriptor,node0=root,constructor_metadata=metadata.hex(),
      animation_graph=animation_graph,widget_groups=widget_groups,cold_state=cold_state,constructor_scope=constructor_scope))
   fixtures.append(('B',name,ordinal,[definition,*words(raw[:4]),*words(item),*scale_input,
       animation_graph,widget_count,*widget_groups],descriptor+[scale]+(root or [])+(cold_state or [])))
  #Original10DAD0 clamp/held/game-type branches, explicit synthetic metadata.
  definition=births[-1]['definition'];d=c.tags[definition]['offset'];definition_va=d+c.magic
  original_scale=bytes(g.mem_read(definition_va+0xc4,8))
  values=[-2,0,.0001,.29999998,.3,.30000004,1,1.25,2,2.00000024,4]
  for i,v in enumerate(values):
   for game_type in [0,1,2]:
    for held in [0,1]:
     reset();stage[0]='scale';put(HANDLES+8,HEAP);put(HEAP,definition);put(HEAP+0x12c,held)
     put(env['GAME']+8,game_type);put(definition_va+0xc4,bits(v));put(definition_va+0xc8,bits(values[-1-i]))
     run(0x10dad0);fixtures.append(('S',name,0,[game_type,held,bits(v),bits(values[-1-i])],[output[0]]));scale_count+=1
  g.mem_write(definition_va+0xc4,original_scale);put(env['GAME']+8,2)
  #Complete15AE70 timer branch, actual literal row copied to controlledordinal.
  #All other rows have timer-1. Engine query is the explicit callback above.
  choices=[b for b in births if b['map']==name]
  rng=random.Random(0x15ae70+len(births))
  for case in range(48):
   b=rng.choice(choices);ordinal=b['row'];raw=bytearray.fromhex(b['raw_row']);definition=b['definition']
   reset();stage[0]='timer';bound=case%5!=0;timer=rng.choice([-1,0,1,2,32767]);empty=rng.choice([0,1,32767])
   playback=case%13==0;option=case%5;rate=rng.choice([15,30,60]);tick=ordinal+rate*rng.randrange(8)+(case%7==0)
   mode=rng.choice([0,1,2,9]);engine_mode[0]=mode;available=case%11!=0
   put(0x55e4d0+8,ENGINE+0xc20 if available else 0);g.mem_write(env['GAME']+0xc,bytes([4 if playback else 1]));g.mem_write(env['GAME']+0x211,bytes([option]))
   put(CLOCK,1|(rate<<16));put(CLOCK+8,tick)
   timer_start=case%3;struct.pack_into('<h',raw,0x12,timer_start)
   #Use original row memory for the caller, preserving allotherauthoredfields.
   row_va=records+c.magic+ordinal*0x90;saved=bytes(g.mem_read(row_va,0x90));g.mem_write(row_va,bytes(raw))
   handle=0x12340000 if bound else 0xffffffff;g.mem_write(ENTRY+ordinal*8,struct.pack('<Ihh',handle,timer,empty))
   pos=words(raw[0x40:0x4c]);position=list(struct.unpack('<3f',raw[0x40:0x4c]));position[0]+=rng.choice([0,.5,1,1.125])
   itemflags=rng.choice([0,1,2]);control=4 if case%4==0 else 0
   if bound:
    put(HANDLES+8,HEAP);put(HEAP,definition);g.mem_write(HEAP+0xaa,b'\x02');put(HEAP+0x12c,itemflags)
    g.mem_write(HEAP+0x10a,bytes([control]));g.mem_write(HEAP+0x30,struct.pack('<3f',*position))
   before=[handle,timer&65535,empty&65535]
   run(0x15ae70,[ENTRY]);after=words(bytes(g.mem_read(ENTRY+ordinal*8,4)))+list(struct.unpack('<HH',g.mem_read(ENTRY+ordinal*8+4,4)))
   inputs=[ordinal,tick,rate,playback,available,mode,option,*before,timer_start,itemflags,control,*map(bits,position),*pos,
       struct.unpack_from('<h',raw,0xe)[0]&65535,c.tags[words(raw[0x5c:0x60])[0]]['offset']]
   #Fixture retains raw row; native decoder controls collectiondelay directly.
   delay=struct.unpack_from('<h',c.data,inputs[-1]+8)[0];inputs[-1]=delay&65535
   fixtures.append(('T',name,ordinal,inputs,[int(request[0]),*after]));timer_count+=1
   g.mem_write(row_va,saved)
  del env,g;gc.collect();print(name,'original source rows complete',flush=True)
 fixture=ROOT/'tests/fixtures/retail_source_netgame_weapon.tsv'
 fixture.write_text('# kind map ordinal inputwords | untouched original outputwords\n'+''.join(
   f'{kind} {name} {row} '+' '.join(f'{v&0xffffffff:08X}'for v in inp)+' | '+' '.join(f'{v&0xffffffff:08X}'for v in out)+'\n'
   for kind,name,row,inp,out in fixtures))
 report=dict(xbe_sha256=PIN,cache_sha256=cache_hashes,births=births,fixtures=len(fixtures),timer_cases=timer_count,
    scale_cases=scale_count,levitated_constructors=constructor_count,levitated_updates=update_count,checked_words=sum(len(r[-1])for r in fixtures),
    scaffold_sha256=sha(scaffold.read_bytes()),fixture_sha256=sha(fixture.read_bytes()),executed_instructions=len(seen),
    instruction_sha256=sha(b''.join(x.read(pc,n)for pc,n in sorted(seen.items()))),
    service_boundaries=['BC280 explicit successful zeroed object allocation/header','18E340 extension storage success without relocation',
      'BF380 no pressure','gameengine mode callback explicit input; not a computed engine producer',
      'full levitated controls: g4EA93C effe, g4ED28C lsnd and g4E030C ligh widget pools have explicit capacity0; original allocators execute and returnNONE',
      'full levitated controls: physics capacity0; cold registry disabled; authored animation_graphNONE and item spin flag0clear',
      'timer request stops at original15AAD0 entry; no allocation return substituted'],
    scope='Singleton default stockclass0 weap descriptors across21 caches; authored no-graph non-spinning levitated fullconstruction and unchanged10B4D0/10B850. Graph owners retain descriptor/scale/timer coverage but full construction is unresolved. No general item settling, live global registration chronology or successful native activation proof.')
 if a.native:
  result=subprocess.run([str(a.native.resolve())],cwd=ROOT,text=True,capture_output=True)
  print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr);assert result.returncode==0
  report['native_validation']={'binary_sha256':sha(a.native.read_bytes()),'return_code':result.returncode,
      'stdout':result.stdout.strip(),'scope':'CPU frozen fixture comparison; no GPU or guest run'}
 target=ROOT/'analysis/multiplayer-shading/source-netgame-weapon-original.json';target.write_text(json.dumps(report,indent=2)+'\n')
 fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k!='births'},indent=2)+'\n')
 print(f'{len(fixtures)} cases, {report["checked_words"]} original words; {len(births)} singleton births, {update_count} levitated complete updates')
if __name__=='__main__':main()
