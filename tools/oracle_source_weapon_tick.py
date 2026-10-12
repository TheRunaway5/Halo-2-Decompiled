#!/usr/bin/env python3
"""Read-only Retail cold weapon update and header scheduling boundary.

Reconstruct the31 proved no-graph levitated weapons, record literal header
producers, and run complete108C60/BC470/B6F10/B7150. The genuine B7290
activation controls receive a declared cluster; registry/location production
is not inferred. No GPU, guest, native producer or retained state is used.
"""
from pathlib import Path
import collections,gc,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_multiplayer_hud_positions import PIN
def sha(data):return hashlib.sha256(data).hexdigest()
def pack(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))
def main():
 proof=ROOT/'analysis/multiplayer-shading/source-netgame-weapon-original.json'
 original=json.loads(proof.read_text());births=[b for b in original['births']if b['cold_state']]
 assert len(births)==31 and original['xbe_sha256']==PIN
 scaffold=ROOT/'analysis/multiplayer-shading/object-cold-constructor-original.py'
 prefix=scaffold.read_text().split('expected=json.loads')[0]
 env=None;prior=None;captures=[];fixtures=[];executed={};entries=collections.Counter()
 watched=[0xb7b40,0xfdad0,0x1058b0,0xb7360,0xb7290,0xb6f10,0xb7150,0xbc470,0x108c60,0x10b4d0,0xfdfb0,0xfdd50,0x105e80,0xff5f0,0xfd910]
 phase=['constructor'];stores=[];header_stores=[];wake_caller=[None]
 for b in births:
  name=b['map']
  if name!=prior:
   if env:del env;gc.collect()
   path=ROOT/'assets/disc/maps'/f'{name}.map'
   if not path.exists():path=ROOT/'assets/map-pack/maps'/f'{name}.map'
   env={'path':path};exec(compile(prefix.replace("ROOT/'assets/disc/maps/zanzibar.map'","path"),str(scaffold),'exec'),env)
   g,x,c,put,read,run=[env[k]for k in['u','x','c','put','read','run']]
   BASE,HEAP,HANDLES,HEADER,OBJGLOBALS=[env[k]for k in['BASE','HEAP','HANDLES','HEADER','OBJGLOBALS']]
   assert sha(x.data)==PIN
   def trace(_g,pc,size,_):
    assert bytes(_g.mem_read(pc,size))==x.read(pc,size),(hex(pc),'changed source instructions')
    executed[pc]=size
    if pc in watched:entries[f'{pc:X}']+=1
    if pc==0xb7360:wake_caller[0]=f'{read(_g.reg_read(UC_X86_REG_ESP)):08X}'
   g.hook_add(UC_HOOK_CODE,trace)
   def body_write(_g,_access,address,size,value,_):
    if phase[0]!='constructor':stores.append(dict(instruction=f'{_g.reg_read(UC_X86_REG_EIP):08X}',offset=address-HEAP,size=size,value=f'{value&((1<<(size*8))-1):X}'))
   def header_write(_g,_access,address,size,value,_):
    if address<=HANDLES+3 and address+size>HANDLES+2:
     header_stores.append(dict(phase=phase[0],instruction=f'{_g.reg_read(UC_X86_REG_EIP):08X}',offset=address-HANDLES,size=size,value=f'{value&((1<<(size*8))-1):X}',wake_caller=wake_caller[0]))
   g.hook_add(UC_HOOK_MEM_WRITE,body_write,begin=HEAP,end=HEAP+0xfff)
   g.hook_add(UC_HOOK_MEM_WRITE,header_write,begin=HANDLES,end=HANDLES+11);prior=name
  env['mode']='netgame';env['nxt']=0;phase[0]='constructor';header_stores.clear();wake_caller[0]=None
  for ptr,size in[(HEAP,0x100000),(HANDLES,4096),(OBJGLOBALS,132),(BASE+0xb000,800)]:g.mem_write(ptr,bytes(size))
  put(HEADER+0x24,12);put(HEADER+0x38,0)
  for address,offset in[(0x51e9a0,0xa000),(0x4ea93c,0xc000),(0x4ed28c,0xc100),(0x4e030c,0xc200)]:put(address,BASE+offset);put(BASE+offset,0)
  put(BASE+0x8000,0x001e0001);put(BASE+0x8004,0x3d088889);put(BASE+0x8008,5)
  entry=BASE+0xb000;put(entry+b['row']*8,0xffffffff);run(0x15aad0,[entry,b['row']]);assert g.reg_read(UC_X86_REG_EIP)==env['STOP']
  handle=read(entry+b['row']*8);assert handle==0x12340000
  body=read(HANDLES+8);size=struct.unpack('<H',g.mem_read(HANDLES+6,2))[0]
  cold=bytes(g.mem_read(body,size));cold_header=bytes(g.mem_read(HANDLES,12));assert cold_header[2:4]==b'\x0a\x02'
  assert cold[0xc1]==1 and cold[0x12c]==0 and read(body+0x14)==0xffffffff
  assert read(body+0x16c)==0 and read(body+0x184)==0 and struct.unpack_from('<h',cold,0x19e)[0]==0
  ctor_stores=list(header_stores)
  #The inherited allocation produced one live header. These storage facts
  #complete its literal iterator bitmap, without changing flags or location.
  put(HEADER+0x48,BASE+0xf000);put(BASE+0xf000,1)
  put(0x5107f0,env['TABLE']+0xf0000);put(env['TABLE']+0xf0000+0x1d80,0)
  def restore():g.mem_write(body,cold);g.mem_write(HANDLES,cold_header)
  def snapshot():
   raw=bytes(g.mem_read(body,size));return [g.mem_read(HANDLES+2,1)[0],raw[0xc1],raw[0x12c],read(body+0x16c),read(body+0x184),struct.unpack_from('<h',raw,0x19e)[0]&65535,read(body+0x210)]
  def capture(kind,tick,function,args=()):
   phase[0]=kind;stores.clear();header_stores.clear();before=bytes(g.mem_read(body,size));inputs=[function,*snapshot()]
   calls_before=entries['FDFB0'];run(function,args);assert g.reg_read(UC_X86_REG_EIP)==env['STOP']
   callback_count=entries['FDFB0']-calls_before;result=(g.reg_read(UC_X86_REG_EAX)&255)if function in[0x108c60,0xbc470]else 0
   after=bytes(g.mem_read(body,size));actual_stores=list(stores);actual_header_stores=list(header_stores)
   out,active=BASE+0xe100,BASE+0xe104;put(out,0xdeadbeef);put(active,0xdeadbeef)
   run(0xff5f0,[handle,0x09000568,out,active]);assert g.reg_read(UC_X86_REG_EIP)==env['STOP']
   assert read(out)==0x3f800000 and read(active)&255==1 and after==bytes(g.mem_read(body,size))
   outputs=[result,callback_count,*snapshot(),read(out),read(active)&255]
   fixtures.append((kind,name,b['row'],tick,inputs,outputs))
   changed=[dict(offset=i,before=before[i],after=after[i])for i in range(size)if before[i]!=after[i]]
   return dict(kind=kind,tick=tick,function=f'{function:08X}',input_words=inputs,output_words=outputs,
     body_sha256=sha(after),changed_bytes=changed,body_stores=actual_stores,header_stores=actual_header_stores)
  restore();dispatch=[capture('D',tick,0x108c60,[handle])for tick in range(1,5)]
  assert dispatch[0]['output_words'][0:2]==[1,1] and all(row['output_words'][0:2]==[0,1]for row in dispatch[1:])
  assert all(row['output_words'][5:8]==[32,0,65535]for row in dispatch)
  restore();cold_schedule=[capture('N',1,0xb6f10),capture('N',2,0xb7150)]
  assert all(row['output_words'][1]==0 and row['output_words'][2:9]==[10,1,0,0,0,0,struct.unpack_from('<I',cold,0x210)[0]]for row in cold_schedule)
  #Original activation with NONE cluster is a no-op. Then supply the cluster
  #as a declared control and let untouchedB7290 itself set active01.
  restore();phase[0]='activation';g.reg_write(UC_X86_REG_EDI,handle);g.reg_write(UC_X86_REG_ESP,env['STACK']);put(env['STACK'],env['STOP'])
  g.emu_start(0xb7290,env['STOP'],count=10000);assert g.reg_read(UC_X86_REG_EIP)==env['STOP'] and g.mem_read(HANDLES+2,1)[0]==10
  g.mem_write(HANDLES+4,struct.pack('<h',0));header_stores.clear()
  g.reg_write(UC_X86_REG_EDI,handle);g.reg_write(UC_X86_REG_ESP,env['STACK']);put(env['STACK'],env['STOP']);g.emu_start(0xb7290,env['STOP'],count=10000)
  assert g.reg_read(UC_X86_REG_EIP)==env['STOP'] and g.mem_read(HANDLES+2,1)[0]==11
  activation_stores=list(header_stores)
  activated=[capture('A',1,0xb6f10),capture('A',2,0xb7150),capture('A',3,0xb6f10),capture('A',4,0xb6f10)]
  assert [row['output_words'][1]for row in activated]==[0,1,1,0]
  assert [row['output_words'][2]for row in activated]==[11,3,1,1]
  assert [row['output_words'][5]for row in activated]==[0,32,32,32]
  #BC470's entry gating is distinct from scheduler gating.08 is accepted,
  #20 defers, no awake02 skips; C1 physicsdisabled is absent from admission.
  controls=[]
  for header_flags in[0,8,10,2,34,42]:
   for disabled in[0,1]:
    restore();g.mem_write(HANDLES+2,bytes([header_flags]));g.mem_write(body+0xc1,bytes([disabled]))
    row=capture('H',len(controls),0xbc470,[handle]);expected=int(bool(header_flags&2)and not(header_flags&32))
    assert row['output_words'][1]==expected and row['output_words'][5]==(32 if expected else 0)
    assert row['output_words'][3]==disabled;controls.append(row)
  captures.append(dict(map=name,row=b['row'],definition=b['definition'],cold_body_sha256=sha(cold),cold_header_hex=cold_header.hex(),
    constructor_header_stores=ctor_stores,declared_cluster_activation_stores=activation_stores,
    full_callbacks=dispatch,cold_scheduler=cold_schedule,declared_cluster_scheduler=activated,header_controls=controls))
  print('original weapon tick',name,b['row'],'cold header0A; first callback20; power0; scheduler boundary exact',flush=True)
 fixture=ROOT/'tests/fixtures/retail_source_weapon_tick.tsv'
 fixture.write_text('# kind map row tick function,before(header,C1,item12C,weapon16C,power184,countdown19E,trigger210) | return(bool callbacks only),callbackcount,after7,turned_on,active\n'+''.join(f'{kind} {name} {row} {tick} '+' '.join(f'{word:08X}'for word in inputs)+' | '+' '.join(f'{word:08X}'for word in outputs)+'\n'for kind,name,row,tick,inputs,outputs in fixtures))
 report=dict(xbe_sha256=PIN,tool_sha256=sha(Path(__file__).read_bytes()),scaffold_sha256=sha(scaffold.read_bytes()),constructor_proof_sha256=sha(proof.read_bytes()),
   cache_sha256={name:original['cache_sha256'][name]for name in sorted({b['map']for b in births})},
   fixture_sha256=sha(fixture.read_bytes()),cases=len(fixtures),original_output_words=sum(len(v[-1])for v in fixtures),constructors=31,
   source_entries=dict(entries),unique_instructions=len(executed),instruction_sha256=sha(b''.join(x.read(pc,size)for pc,size in sorted(executed.items()))),
   source_type2_definition='467CD0: inherit4678E8/467C08/467CD0; handler40=0/10B4D0/FDFB0',
   header_boundary='B7C0E produces08; B7398 produces02 from1058B0 wake. B6F10 needsactive01,awake02,!deferred08. B7150 needs08,01,02,!10 and clears08 beforeBC470. BC470 needs02,!20. C1 is not admission.',
   source_power_producers=[dict(function='101740',stores=['10188E','101894'],caller='E8DE0',rule='Same-definition weapon transfer swaps184 when first resource value is greater than second'),
     dict(function='101A10',stores=['101A72'],caller='1E2A90',rule='Declared fraction setter:101D20 true stores1-clamp(fraction,0,1)'),
     dict(function='101DB0',stores=['101E44','101E52'],callers=['101C80','102FB0'],rule='Resource consumption only withitem12Cbit3,184<1,and no player disable. AddcallerXMM0,cap1,playdefinition+2FCeffect and optionally replace definition+304')],
   services_replaced=['Inherited BC280 zeroed allocation,18E340 nonrelocating extension andBF380 explicit no-pressure boundary only; all update/shader routines unchanged'],
   declared_inputs=['Original coldregistry disabled, all widget/physics pools empty as in constructor proof','One allocated header occupancy bit and empty object platform list for literal iterators',
     'Cluster0 for independent B7290 activation controls; source registry/BSP producer is not recovered here','Headerflags/C1 for independent BC470 admission controls'],
   scope='Complete original31coldconstructors and bounded item/weapon/object callback and scheduler paths. Actual cold0Aheaders do not tick while inactive. Firsttick20 is proved only after admission, never inferred fromretainedstate. Power184 stays0 in this cold input scope. Laterweaponinput,damage,activation/location,animation,effects andlivegameplay remain separate.',captures=captures)
 out=ROOT/'analysis/multiplayer-shading/source-weapon-tick-original.json';out.write_text(json.dumps(report,indent=2)+'\n')
 provenance={key:value for key,value in report.items()if key!='captures'}
 fixture.with_suffix('.provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
 print(json.dumps({key:report[key]for key in['cases','original_output_words','unique_instructions','source_entries']},indent=2))
if __name__=='__main__':main()
