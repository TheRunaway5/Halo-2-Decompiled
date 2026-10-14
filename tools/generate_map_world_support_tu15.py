#!/usr/bin/env python3
"""Lower complete original TU1.5 callbacks reached by authored map construction."""
import argparse,hashlib,json,struct
from pathlib import Path
from types import SimpleNamespace
from generate_physics_live_support_tu15 import generate
from oracle_smg_firing_1_1 import _Code,_trace,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ENTRIES=(0x10b5c0,0x117870,0x10c0b0,0x1888a0,0x381710,0x381730,0x381750,0x381770,0x158340,0x166a80,0x166ae0,0x323730,0x323430,0x37c5d0,0x1e6e80,0xc3ab0,0xc55d0,0xc3820,0xc3a20,0xc3960,0xc39b0,0x1e7330,0x181cb0,0x183c10,0x181da0,0x182bb0,0x144e90,0x17f990,0x180830,0x180c10,0x180790,0x1800b0,0x43880,0x11f910,0x2439e0,0x123150,0x17fe30,0x17fcf0,0x17fc90,0x1839f0,0x17ff90,0x183af0,0x439c0,0x180170,0xd6d10,0x14d790,0xd3c10,0xd42b0,0xd5130)
LIGHTING_ENTRIES=(1511440, 1184192, 873680, 74592, 89264, 118672, 119504, 119952, 147872, 148592, 219328, 867104, 867184, 867264, 873216, 874000, 874240, 874720, 875232, 875376, 1263968, 1278816, 1279248, 1279440, 1279712, 1279872, 1281056, 1281424, 1281904, 1281984, 1311952, 1312016, 1511664, 2271360, 2271456, 2271824, 2271888, 2375456)
RENDER_MAP_ENTRIES=(0x3ddc0,0x3dd00,0x2c390,0x323560,0xc5b00,0x2f3670,0x2f1cc0,0x2b480,0x2b520,0x3d260,0x3b890,0x16780,0x50680,0x3235f0)
BASE_ENTRIES=(*ENTRIES,*RENDER_MAP_ENTRIES,0x43810,0x43840,0x1806a0,0x1806f0,0x130d20,0x22a790)
ENTRIES=tuple(dict.fromkeys((*BASE_ENTRIES,*LIGHTING_ENTRIES)))
def main():
 global ENTRIES
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--output',type=Path,required=True);a.add_argument('--manifest',type=Path,required=True);a=a.parse_args()
 assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
 x=Xbe(NEW_DEFAULT);code=_Code(x,'.text')
 assert x.read(0x10db94,2)==bytes.fromhex('ffd1')
 assert x.read(0x10e155,2)==bytes.fromhex('ffd0')
 assert x.read(0x10e142,3)==bytes.fromhex('8b406c')
 assert x.read(0x10db82,3)==bytes.fromhex('8b492c')
 #10DBC0 consumes each actual type descriptor's callback+2C. Preserve the
 #literal data registration instead of inferring a successful initializer.
 bodies={entry:_trace(code,entry,set(ENTRIES))for entry in ENTRIES}
 reached=set(BASE_ENTRIES)
 while True:
  old=len(reached)
  for entry in tuple(reached):
   for pc in bodies[entry].code:
    i=code.at(pc)
    if i.mnemonic=='call' and i.operands[0].type==2 and i.operands[0].imm in bodies:reached.add(i.operands[0].imm)
  if len(reached)==old:break
 ENTRIES=tuple(p for p in ENTRIES if p in reached)
 bodies={p:bodies[p]for p in ENTRIES}
 direct_callers={}
 for body in bodies.values():
  for pc in sorted(body.code):
   i=code.at(pc)
   if i.mnemonic=="call" and i.operands[0].type==2:direct_callers.setdefault(i.operands[0].imm,pc)
 registrations=[]
 for entry in ENTRIES:
  found=[]
  for section in x.sections:
   data=x.section_bytes(section);needle=struct.pack('<I',entry);start=0
   while (index:=data.find(needle,start))!=-1:
    address=section.va+index
    if address>=0x400000 and address%4==0:
     row=dict(slot=f'{address:08x}',callback=f'{entry:08x}')
     if entry in [0x10b5c0,0x117870,0x10c0b0]:
      offset=0x6c if entry==0x10c0b0 else 0x2c
      row.update(kind='type-callback',descriptor=f'{address-offset:08x}',source_call='0010e155'if entry==0x10c0b0 else'0010db94',callback_offset=offset)
     elif entry in (0x180170,0x2b520):row.update(kind='new-map-function-pointer',native_request='PhysicsLiveWorldTu15 new-map lifecycle; original whole map scheduling remains separate')
     elif entry in [0x1888a0,0x381710,0x381730,0x381750,0x381770,0x1800b0,0x180170,0x2b480]:row.update(kind='startup-function-pointer',native_request='PhysicsLiveWorldTu15 constructor; original whole startup scheduling remains separate')
     else:row.update(kind='original-function-pointer',native_request='Reached retained original world routine; whole scheduling remains separate')
     found.append(row)
    start=index+1
  if not found and entry in (0x2f3670,0x2f1cc0):
   pc=0x2f55d3 if entry==0x2f3670 else 0x2f1d43;i=code.at(pc)
   assert i.mnemonic=='mov' and i.operands[-1].imm==entry
   found.append(dict(callback=f'{entry:08x}',kind='source-callback-registration-argument',original_source_instruction=f'{pc:08x}',source_instruction=bytes(i.bytes).hex()))
  if not found and entry in RENDER_MAP_ENTRIES:
   pc=0x17e4aa if entry==0x3ddc0 else direct_callers[entry];i=code.at(pc)
   assert i.mnemonic=='call' and i.operands[0].imm==entry
   found.append(dict(callback=f'{entry:08x}',kind='native-reached-original-function',original_source_call=f'{pc:08x}',source_instruction=bytes(i.bytes).hex()))
  if not found and entry==0x130d20:
   i=code.at(0x13044c);assert i.mnemonic=="push" and i.operands[0].imm==entry
   found.append(dict(callback=f"{entry:08x}",kind="cache-callback-constructor-argument",original_source_instruction="0013044c",source_instruction=bytes(i.bytes).hex()))
  if not found and entry in (0x43810,0x43840):
   pc=0x4391d if entry==0x43810 else 0x43918;i=code.at(pc)
   assert i.mnemonic=='push' and i.operands[0].imm==entry
   found.append(dict(callback=f'{entry:08x}',kind='cache-callback-constructor-argument',original_source_instruction=f'{pc:08x}',source_instruction=bytes(i.bytes).hex()))
  if not found and entry in (0x1806a0,0x1806f0,0x22a790):
   pc=direct_callers[entry];i=code.at(pc);assert i.mnemonic=='call' and i.operands[0].imm==entry
   found.append(dict(callback=f'{entry:08x}',kind='native-reached-original-function',original_source_call=f'{pc:08x}',source_instruction=bytes(i.bytes).hex()))
  if not found and entry in LIGHTING_ENTRIES:
   source_site=direct_callers.get(entry)
   if source_site is None:
    # An entry may belong to a different branch of the same original lighting
    # graph; retain its literal direct caller, never a guessed invocation.
    raise AssertionError(f"No retained original direct caller for {entry:08x}")
   call=code.at(source_site)
   assert call.mnemonic=='call' and call.operands[0].imm==entry
   found.append(dict(callback=f'{entry:08x}',kind='native-reached-original-function',original_source_call=f'{source_site:08x}',source_instruction=bytes(call.bytes).hex()))
  if not found:
   assert entry in [0x158340,0x166a80,0x166ae0,0x323430,0x37c5d0,0x1e6e80,0xc3ab0,0xc55d0,0xc3820,0xc3a20,0xc3960,0xc39b0,0x1e7330,0x181cb0,0x183c10,0x181da0,0x182bb0,0x144e90,0x17f990,0x180830,0x180c10,0x180790,0x43880,0x11f910,0x2439e0,0x123150,0x17fe30,0x17fcf0,0x17fc90,0x1839f0,0x17ff90,0x183af0,0x439c0,0xd6d10,0x14d790,0xd3c10,0xd42b0,0xd5130]
   source_site={0x158340:0x15834d,0x166a80:0x1888d1,0x166ae0:0x166a96,0x323430:0x3237ee,0x37c5d0:0x323825,0x1e6e80:0x1e6e37,0xc3ab0:0xc4a0d,0xc55d0:0xc4cd9,0xc3820:0xc5608,0xc3a20:0xc389b,0xc3960:0xc3a74,0xc39b0:0xc3a7f,0x1e7330:0xc4cf1,0x181cb0:0x1816aa,0x183c10:0x181d74,0x181da0:0x181d30,0x182bb0:0x181d57,0x144e90:0x18222c,0x17f990:0x1829dd,0x180830:0x182b49,0x180c10:0x182c4f,0x180790:0x182f03,0x43880:0x180127,0x11f910:0x180c72,0x2439e0:0x180e78,0x123150:0x1810e3,0x17fe30:0x183de7,0x17fcf0:0x17ff25,0x17fc90:0x17ff49,0x1839f0:0x184649,0x17ff90:0x183aad,0x183af0:0x183fd8,0x439c0:0x183e17,0xd6d10:0x183e5a,0x14d790:0xd6d43,0xd3c10:0xd6d56,0xd42b0:0xd3c3a,0xd5130:0xd3cb5}[entry]
   call=code.at(source_site)
   assert call.mnemonic=='call'
   assert call.operands[0].imm=={0x158340:0xbcdf0,0x166a80:0x166a80,0x166ae0:0x166ae0,0x323430:0x323430,0x37c5d0:0x37c5d0,0x1e6e80:0x1e6e80,0xc3ab0:0xc3ab0,0xc55d0:0xc55d0,0xc3820:0xc3820,0xc3a20:0xc3a20,0xc3960:0xc3960,0xc39b0:0xc39b0,0x1e7330:0x1e7330,0x181cb0:0x181cb0,0x183c10:0x183c10,0x181da0:0x181da0,0x182bb0:0x182bb0,0x144e90:0x144e90,0x17f990:0x17f990,0x180830:0x180830,0x180c10:0x180c10,0x180790:0x180790,0x43880:0x43880,0x11f910:0x11f910,0x2439e0:0x2439e0,0x123150:0x123150,0x17fe30:0x17fe30,0x17fcf0:0x17fcf0,0x17fc90:0x17fc90,0x1839f0:0x1839f0,0x17ff90:0x17ff90,0x183af0:0x183af0,0x439c0:0x439c0,0xd6d10:0xd6d10,0x14d790:0x14d790,0xd3c10:0xd3c10,0xd42b0:0xd42b0,0xd5130:0xd5130}[entry]
   found.append(dict(callback=f'{entry:08x}',kind="native-reached-original-function",original_source_call=f'{source_site:08x}',source_instruction=bytes(call.bytes).hex()))
  registrations.extend(found)
 sites={};instructions={};addresses={};routines=[]
 for entry in ENTRIES:
  body=bodies[entry];assert body.code
  assert all(w in (1,4)for _,w,_ in body.tables)
  sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,original_tail_targets=sorted(body.tail_jumps),tables=[dict(disc=p,count=n)for p,w,n in body.tables if w==4],byte_tables=[dict(address=p,width=w,count=n,raw_hex=x.read(p,w*n).hex())for p,w,n in body.tables if w==1]))
  for pc in sorted(body.code):
   i=code.at(pc);assert x.read(pc,i.size)==bytes(i.bytes);instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
   for operand in i.operands:
    if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
    if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
 paired=dict(image='English TU1.5',original_sha256=NEW_PIN,complete_bodies=len(ENTRIES),entries={f'{pc:08x}':pc for pc in ENTRIES},body_sites=sites,instruction_map=instructions,address_map={f'{pc:08x}':v for pc,v in addresses.items()},routines=routines)
 a.manifest.parent.mkdir(parents=True,exist_ok=True);a.correspondence=a.manifest.with_name('map-bodies.json');a.correspondence.write_text(json.dumps(paired,indent=2)+'\n');a.additional_entries=a.manifest.with_name('registrations.json');a.additional_entries.write_text(json.dumps(dict(original_sha256=NEW_PIN,source_call='0010db94',registrations=registrations),indent=2)+'\n')
 generate(a,paired,runtime_header='halo2/source_map_world_support_tu15.hpp')
 #Rename the public dispatcher/ownership exports; instruction lowering and
 #original-address labels are unchanged from the shared generator's output.
 text=a.output.read_text().replace('source_run_physics_live_support_tu15','source_run_map_world_support_tu15').replace('source_physics_live_support_tu15_owns_entry','source_map_world_support_tu15_owns_entry');a.output.write_text(text)
 report=json.loads(a.manifest.read_text());report['generated_cpp_sha256']=hashlib.sha256(a.output.read_bytes()).hexdigest();report['map_generator_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();report['renamed_exports_only']=True;a.manifest.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(entries=[f'{pc:08x}'for pc in ENTRIES],instructions=len(instructions),registrations=registrations,generated_cpp_sha256=report['generated_cpp_sha256'])))
if __name__=='__main__':main()
