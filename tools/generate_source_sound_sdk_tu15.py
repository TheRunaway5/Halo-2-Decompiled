#!/usr/bin/env python3
"""Compile authenticated whole TU1.5 SDK factory/accounting CPU bodies.

Native synchronization/storage/settings/hardware remain reached required owners.
No original positive settings/device/DSP result or free-counter value is supplied.
"""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
CORE=(0x387877,0x383f7e,0x38780a,0x383d5f,0x387a32,0x38796c,0x384212,0x3876d1,0x386d4a,0x3841f0,0x37da9a,0x2e101c,0x33c29d,0x387baa,0x387bb5,0x38a988,0x38a993)
class Code:
 def __init__(self,image):self.text=_Code(image,'.text');self.sound=_Code(image,'DSOUND')
 def inside(self,p):return self.text.inside(p)or self.sound.inside(p)
 def at(self,p):return(self.sound if self.sound.lo<=p<self.sound.hi else self.text).at(p)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=Path('src/source_sound_sdk_tu15_generated.cpp'));p.add_argument('--directory',type=Path,required=True);a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
 x=Xbe(NEW_DEFAULT);code=Code(x);paired=json.loads(Path('analysis/multiplayer/integration/sound-capabilities-native/paired-v2/report.json').read_text())
 entries=sorted(set(CORE)|{int(row['u5'],16)for row in paired['owners']});assert len(paired['owners'])==27
 for row,entry in ((0x46d7e4,0x387baa),(0x46d7e8,0x387bb5),(0x46d7ec,0x38a988),(0x46d7f0,0x38a993)):
  assert int.from_bytes(x.read(row,4),'little')==entry
 a.directory.mkdir(parents=True,exist_ok=True);sites={};instructions={};addresses={};routines=[]
 for entry in entries:
  body=_trace(code.sound if code.sound.lo<=entry<code.sound.hi else code.text,entry,set(entries));assert body.code and all(width==4 for _,width,_ in body.tables),(hex(entry),body)
  sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[dict(disc=q,count=n)for q,_,n in body.tables],byte_tables=[]));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);assert bytes(i.bytes)==x.read(pc,i.size);listing.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
   for o in i.operands:
    if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
    elif o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
  (a.directory/f'{entry:08x}.asm').write_text(''.join(listing))
 correspondence=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{e:08x}':e for e in entries},body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':v for p,v in addresses.items()},routines=routines)
 path=a.directory/'bodies.json';path.write_text(json.dumps(correspondence,indent=2,sort_keys=True)+'\n')
 def required_platform_data(i):
  if i.address!=0x37daba:return None
  assert i.mnemonic=='call'and i.operands[0].type==2 and i.operands[0].imm==0x33c29d and i.size==5
  # Preserve the full actual ABI at the data-dependent reader. Its complete
  # original body remains available, but no current boot-mode input is invented.
  return 's.push(0x37dabau,0x37dabfu);s.required(0x37dabau,0x33c29du,false);'
 generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=path,additional_entries=Path(__file__)),correspondence,code_override=code,extra_instruction=required_platform_data)
 s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_source_sound_sdk_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_sound_sdk_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_sound_sdk_tu15').replace('source_physics_live_support_tu15_owns_entry','source_sound_sdk_tu15_owns_entry');a.output.write_text(s)
 report=json.loads((a.directory/'generation.json').read_text());report.update(generated_cpp_sha256=sha(a.output),sdk_generator_sha256=sha(Path(__file__)),core_entries=list(CORE),required_platform_data_call=dict(instruction=0x37daba,target=0x33c29d,complete_reader_available=True,current_native_value_supplied=False),accounting_correspondence_sha256=sha(Path('analysis/multiplayer/integration/sound-capabilities-native/paired-v2/report.json')));(a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(bodies=len(entries),instructions=len(instructions))))
if __name__=='__main__':main()
