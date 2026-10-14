#!/usr/bin/env python3
"""Execute complete original light-volume sprite generation for actual plasma bolts.

The rendering submission and renderer-state entry are unavailable hardware
boundaries. Original radius/curve/aspect/color/CRT callees execute unchanged.
Controlled marker/camera/color inputs do not establish projectile attachment
residency, original-world placement or rendered framebuffer parity.
"""
import argparse,csv,hashlib,json,math,random,struct,subprocess
from pathlib import Path
from audit_rendering import Cache
from weapon_reference_versions import images
from oracle_smg_firing_1_1 import ROOT,_Code,_trace
from research_mp_inventory_switch_u5 import raw_prefix_candidates,norm
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
STATE,STACK,STOP,TABLE=0x2100000,0x21fff00,0x3000000,0x2300000
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--inputs',type=Path,required=True);p.add_argument('--expected',type=Path,required=True);p.add_argument('--native-binary',type=Path);a=p.parse_args()
 bank={label:x for label,x,_ in images()};old=bank['retail'];old_code=_Code(old,'.text')
 old_starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/retail/functions.tsv').open(),delimiter='\t')}
 cache=Cache(ROOT/'assets/disc/maps/lockout.map');definitions=[t for t in cache.tags.values() if t['group']=='MGS2' and t['name'] in (
  'effects\\objects\\weapons\\pistol\\plasma_pistol\\plasma_bolt',
  'effects\\objects\\weapons\\pistol\\plasma_pistol\\plasma_charged_bolt')];assert len(definitions)==2
 controls=[];rng=random.Random(0x4f25a)
 for tag in definitions:
  for distance in (1.,8.,25.,89.,95.,101.):
   for forward in ((1.,0.,0.),(0.,1.,0.),(0.,0.,1.),(.6,0.,.8)):
    controls.append(dict(datum=tag['datum'],marker=[1.,*forward,0.,1.,0.,0.,0.,1.,distance,0.,0.],opacity=1.,color=[1.,1.,1.],eye=[0.,0.,0.],view=[1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.],projection=[2.5,3.]))
  for _ in range(32):
   forward=[rng.uniform(-1,1) for _ in range(3)];m=math.sqrt(sum(v*v for v in forward));forward=[f32(v/m) for v in forward]
   controls.append(dict(datum=tag['datum'],marker=[f32(rng.uniform(.2,2)),*forward,0.,1.,0.,0.,0.,1.,*[f32(rng.uniform(-40,40)) for _ in range(3)]],opacity=f32(rng.uniform(.01,1.2)),color=[f32(rng.uniform(0,1.2)) for _ in range(3)],eye=[f32(rng.uniform(-5,5)) for _ in range(3)],view=[1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.],projection=[2.5,3.]))
 input_bytes=bytearray(struct.pack('<I',len(controls)))
 for c in controls:input_bytes+=struct.pack('<I35f',c['datum'],*c['marker'],c['opacity'],*c['color'],*c['eye'],*c['view'],*c['projection'])
 a.inputs.write_bytes(input_bytes);versions=[];reference=None
 for label,inventory in [('retail','analysis/retail/functions.tsv'),('TU1.1','analysis/map-pack-update/functions.tsv'),('TU1.5','analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')]:
  x=bank[label];code=_Code(x,'.text');starts={int(r['address'],16) for r in csv.DictReader((ROOT/inventory).open(),delimiter='\t')};owners=[];absolute={};calls={}
  for entry in (0x4f010,0x4ff50,0x30830,0x30bf0):
   f=_trace(old_code,entry,old_starts);seq=[old_code.at(at) for at in sorted(f.code)];pos={i.address:n for n,i in enumerate(seq)};expected=[norm(i,old,pos) for i in seq];matches=[]
   for at in raw_prefix_candidates(old_code,old,code,entry):
    g=_trace(code,at,starts);other=[code.at(pc) for pc in sorted(g.code)];np={i.address:n for n,i in enumerate(other)}
    if [norm(i,x,np) for i in other]==expected:matches.append((at,other))
   assert len(matches)==1,(label,hex(entry),matches);at,other=matches[0]
   operands=[]
   for before,after in zip(seq,other):
    if before.mnemonic=='call' and before.operands[0].type==2:calls[before.operands[0].imm]=after.operands[0].imm
    for u,v in zip(before.operands,after.operands):
     if u.type==v.type==2 and 0x400000<=u.imm<0x600000:absolute[u.imm]=v.imm
     if u.type==v.type==3 and u.mem.base==u.mem.index==v.mem.base==v.mem.index==0 and u.mem.disp and v.mem.disp:
      absolute[u.mem.disp]=v.mem.disp;operands.append(dict(old_va=f'{u.mem.disp:08x}',new_va=f'{v.mem.disp:08x}',size=u.size,old_hex=old.read(u.mem.disp,u.size).hex(),new_hex=x.read(v.mem.disp,v.size).hex()))
   owners.append(dict(retail_entry=f'{entry:08x}',entry=f'{at:08x}',instruction_count=len(seq),normalized_complete_body_equal=True,absolute_operands=operands))
  guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x800000)
  for section in x.sections:guest.mem_write(section.va,x.section_bytes(section))
  guest.mem_map(STATE,0x100000);guest.mem_map(TABLE,0x100000);guest.mem_map(STOP,4096)
  page=cache.magic&~4095;guest.mem_map(page,(cache.magic+len(cache.data)-page+4095)&~4095);guest.mem_write(cache.magic,bytes(cache.data))
  guest.mem_write(absolute[0x4e3b44],struct.pack('<I',TABLE))
  for tag in cache.tags.values():guest.mem_write(TABLE+(tag['datum']&65535)*16+8,struct.pack('<I',tag['address']))
  captured=[];reached={};draw=calls[0x4f7e0];begin=calls[0x16b10]
  def hook(g,pc,size,_):
   reached.setdefault(pc,bytes(g.mem_read(pc,size)).hex())
   if pc==draw:
    sp=g.reg_read(UC_X86_REG_ESP);words=struct.unpack('<9I',g.mem_read(sp,36));records=[bytes(g.mem_read(words[1]+i*24,24)).hex() for i in range(words[2])]
    captured.append(dict(bitmap=g.reg_read(UC_X86_REG_ECX),cosine_bits=words[4],sine_bits=words[5],fuzzy=words[8]&255,records=records))
    g.reg_write(UC_X86_REG_ESP,sp+36);g.reg_write(UC_X86_REG_EIP,words[0])
   elif pc==begin:
    sp=g.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',g.mem_read(sp,4))[0];g.reg_write(UC_X86_REG_ESP,sp+4);g.reg_write(UC_X86_REG_EIP,ret)
  guest.hook_add(UC_HOOK_CODE,hook);outputs=[]
  # Literal camera/projection roots used by the original30830 aspect callee.
  # Their relocation comes from complete corresponding instruction operands.
  for c in controls:
   eye=absolute[0x4b9da0];guest.mem_write(eye,struct.pack('<3f',*c['eye']))
   guest.mem_write(absolute[0x4b9e14],struct.pack('<13f',*c['view']));guest.mem_write(absolute[0x4b9ecc],struct.pack('<2f',*c['projection']))
   marker=c['marker'];state=struct.pack('<2I14f',0,c['datum'],*marker[10:13],*marker[1:4],0.,0.,1.,marker[0],c['opacity'],*c['color'])
   guest.mem_write(STATE,state);guest.mem_write(STACK,struct.pack('<2I',STOP,STATE));guest.reg_write(UC_X86_REG_ESP,STACK);guest.reg_write(UC_X86_REG_FPCW,0x027f);guest.reg_write(UC_X86_REG_MXCSR,0x1f80);captured.clear()
   try:guest.emu_start(int(owners[0]['entry'],16),STOP,count=2000000)
   except Exception as e:raise RuntimeError(f'{label} light-volume fault at {guest.reg_read(UC_X86_REG_EIP):08x}') from e
   assert guest.reg_read(UC_X86_REG_EIP)==STOP;outputs.append(list(captured))
  if reference is None:reference=outputs
  else:assert outputs==reference,label+' original light-volume outputs differ'
  for pc,raw in reached.items():assert x.read(pc,len(bytes.fromhex(raw)))==bytes.fromhex(raw)
  versions.append(dict(label=label,xbe_sha256=hashlib.sha256(x.data).hexdigest(),owners=owners,hardware_boundaries=[f'{begin:08x}',f'{draw:08x}'],reached_instruction_bytes={f'{pc:08x}':raw for pc,raw in sorted(reached.items())}))
  print(label,len(controls),'original light-volume inputs',flush=True)
 report=dict(scope=__doc__,versions=versions,cases=[dict(input=c,batches=b) for c,b in zip(controls,reference)],cache_sha256=hashlib.sha256(cache.data).hexdigest(),inputs_sha256=hashlib.sha256(a.inputs.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),instruction_patches=[])
 expected=bytearray()
 for batches in reference:
  expected+=struct.pack('<I',len(batches))
  for b in batches:
   expected+=struct.pack('<5I',b['bitmap'],b['fuzzy'],len(b['records']),b['cosine_bits'],b['sine_bits'])
   for row in b['records']:expected+=bytes.fromhex(row)
 a.expected.write_bytes(expected);report['expected_sha256']=hashlib.sha256(expected).hexdigest()
 if a.native_binary:
  native=a.output.with_suffix('.native.bin');xbe=Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'
  subprocess.run([str(a.native_binary.resolve()),str(cache.path.resolve()),str(ROOT/'assets/disc/maps'),str(xbe),str(a.inputs.resolve()),str(native.resolve()),str(a.expected.resolve())],check=True)
  assert native.read_bytes()==expected
  report['native_comparison']=dict(cases=len(controls),sprites=sum(len(b['records']) for batches in reference for b in batches),bit_identical=True,
    executable_sha256=hashlib.sha256(a.native_binary.read_bytes()).hexdigest(),output_sha256=hashlib.sha256(native.read_bytes()).hexdigest())
 a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
