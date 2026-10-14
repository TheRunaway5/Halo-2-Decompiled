#!/usr/bin/env python3
"""Execute unchanged TU1.5 input bodies with measured native SDL API inputs.

This is an offline reference test, not a runtime Xbox input dependency. SDL
resources are owned by the native test; this runner replays its complete named
platform-boundary receipts through original caller instructions. It imports no
constructed controller bindings, preset fields, dead-zone results or gameplay
state. Source GP/defined flags, XMM/FP, ordered stores and complete declared
input banks are checked by the native comparison executable.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native-observation',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 native=json.loads(a.native_observation.read_text());assert native['schema']=='H2_NATIVE_TU15_INPUT_NATIVE_V1' and not native['runtime_active']
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
 body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/input-startup/input-startup-bodies.json'
 bodies=json.loads(body_path.read_text());assert bodies['original_sha256']==NEW_PIN
 allowed=set(bodies['instruction_map'].values());registers=[getattr(R,'UC_X86_REG_'+n)for n in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
 cases=[]
 for case in native['cases']:
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
  for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
  g.mem_map(0x60000000,0x10000);g.mem_map(0x60200000,0x1000);g.mem_map(0x60212000,0x1000)
  g.mem_write(0x41b774,struct.pack('<I',0x60212000)) # Explicit native tick binding, not game state.
  site=[0];tick_index=[0];call_index=[0];phase=[None];sites=[];stores=[]
  u=lambda p,n=4:int.from_bytes(g.mem_read(p,n),'little')
  def cpu():
   assert g.reg_read(R.UC_X86_REG_FPTAG)==0xffff
   return dict(gp=[g.reg_read(r)for r in registers],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
    xmm=[g.reg_read(getattr(R,f'UC_X86_REG_XMM{i}')).to_bytes(16,'little').hex()for i in range(8)],
    x87_control=g.reg_read(R.UC_X86_REG_FPCW),x87_status=g.reg_read(R.UC_X86_REG_FPSW),mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),x87_depth=0)
  def write(uc,access,p,n,value,user):
   if 0x60000000<=p<0x60010000:return
   assert n in (1,2,4,8)
   stores.append(dict(instruction=site[0],address=p,before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
  def hook(uc,p,n,user):
   if p in (0x4139a0,0x4139a5,0x413a12,0x413a68,0x413a74,0x413c4c,0x413cbf):
    row=phase[0]['native_calls'][call_index[0]];call_index[0]+=1
    sp=g.reg_read(R.UC_X86_REG_ESP)
    assert p==row['target'] and site[0]==row['instruction'] and sp==row['entry_esp'] and u(sp)==row['instruction']+5
    assert [u(sp+4+i*4)for i in range(len(row['arguments']))]==row['arguments']
    assert row['return_esp']==sp+4+len(row['arguments'])*4
    if p in (0x4139a0,0x413a68):assert row['return_eax']==g.reg_read(R.UC_X86_REG_EAX),'native void API changed EAX'
    for output in row['writes']:g.mem_write(output['address'],bytes.fromhex(output['bytes']))
    g.reg_write(R.UC_X86_REG_EAX,row['return_eax']);g.reg_write(R.UC_X86_REG_ESP,row['return_esp']);g.reg_write(R.UC_X86_REG_EIP,u(sp));return
   assert p in allowed,hex(p)
   assert bytes(g.mem_read(p,n))==image.read(p,n),'original instruction bytes changed'
   if p==0x33b595:
    tick=case['ticks'][tick_index[0]];tick_index[0]+=1;g.mem_write(0x60212000,struct.pack('<I',tick))
   sites.append(p);site[0]=p
  g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
  results=[]
  for row in case['phases']:
   phase[0]=row;call_index[0]=0;sites.clear();stores.clear()
   for r in registers:g.reg_write(r,0)
   for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),0)
   g.reg_write(R.UC_X86_REG_ESP,0x6000f000);g.mem_write(0x6000f000,struct.pack('<I',0x60200000))
   g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
   g.emu_start(row['entry'],0x60200000,count=1000000)
   assert g.reg_read(R.UC_X86_REG_EIP)==0x60200000 and call_index[0]==len(row['native_calls'])
   result=dict(name=row['name'],entry=row['entry'],cpu=cpu(),instructions=list(sites),stores=list(stores),
    banks=[dict(address=b['address'],bytes=bytes(g.mem_read(b['address'],len(bytes.fromhex(b['bytes'])))).hex())for b in row['banks']],native_calls=row['native_calls'])
   for key in ('instructions','stores','banks'):
    assert result[key]==row[key],(case['mode'],row['name'],key,'native lowering differs from original')
   assert result['cpu']['gp']==row['cpu']['gp'],(case['mode'],row['name'],'GP')
   assert not((result['cpu']['eflags']^row['cpu']['eflags'])&0xcc5),(case['mode'],row['name'],'defined flags')
   results.append(result)
  assert tick_index[0]==len(case['ticks'])
  cases.append(dict(mode=case['mode'],phases=results,ticks=case['ticks']))
 report=dict(schema='H2_NATIVE_TU15_INPUT_ORIGINAL_V1',source_sha256=NEW_PIN,oracle_sha256=sha(__file__),
  native_observation_sha256=sha(a.native_observation),body_manifest_sha256=sha(body_path),instruction_patches=[],supplied_gameplay_results=[],cases=cases,
  declared_platform_boundaries='Measured native SDL controller resources and monotonic clock, no Xbox kernel/USB emulation',
  total_instructions=sum(len(p['instructions'])for c in cases for p in c['phases']),total_nonstack_stores=sum(len(p['stores'])for c in cases for p in c['phases']))
 a.output.parent.mkdir(parents=True,exist_ok=True);assert not a.output.exists();a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps({k:report[k]for k in ('schema','total_instructions','total_nonstack_stores')}))
if __name__=='__main__':main()
