#!/usr/bin/env python3
"""Own original1.5 control/action correspondence, including true callbacks.

The action dispatcher table is recovered from the actual paired E7F30 bodies.
Its nonzero callback fields seed actual entry correspondence even when an
unseeded disassembly inventory missed a callback. Body/data comparison is still
required; a table entry or annotation is not action-success evidence.
"""
import csv,struct
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
from oracle_mp_combat_postprocess_updates import OLD,sha
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,full_tokens
from capstone.x86 import X86_OP_MEM
import research_mp_live_combat_updates as comparison

def main():
 assert sha(OLD)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 images=(Xbe(OLD),Xbe(NEW_DEFAULT));codes=[_Code(x,'.text') for x in images]
 entries=(0xe7f30,0xe8e50);functions=[]
 for side,p in enumerate(entries):
  inv=ROOT/('analysis/map-pack-update/functions.tsv' if not side else 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')
  starts={int(r['address'],16) for r in csv.DictReader(inv.open(),delimiter='\t')}
  f=_trace(codes[side],p,starts);functions.append([codes[side].at(q)for q in sorted(f.code)])
 assert full_tokens(functions[0],images[0])==full_tokens(functions[1],images[1])
 tables=[];sites=[]
 for side,ins in enumerate(functions):
  hit=[(i,o)for i in ins for o in i.operands if o.type==X86_OP_MEM and not o.mem.base and o.mem.index and o.mem.scale==4]
  assert len(hit)==1
  i,o=hit[0];tables.append(o.mem.disp);sites.append(i.address)
 assert tables[0]==0x46ec68
 seeds={};callbacks=[]
 for action in (8,18,22,27):
  pointers=[struct.unpack('<I',x.read(p+action*4,4))[0]for x,p in zip(images,tables)]
  buffers=[x.read(p,16)for x,p in zip(images,pointers)]
  for field in range(4):
   p,q=[struct.unpack_from('<I',b,field*4)[0]for b in buffers]
   assert bool(p)==bool(q)
   if not p:continue
   why={'kind':'actual_original_action_dispatch_callback_table','action_type':action,'descriptor_field':field*4,
    'disc_dispatch':f'{entries[0]:08x}','later_dispatch':f'{entries[1]:08x}',
    'disc_table_instruction':f'{sites[0]:08x}','later_table_instruction':f'{sites[1]:08x}',
    'disc_table':f'{tables[0]:08x}','later_table':f'{tables[1]:08x}',
    'disc_descriptor':f'{pointers[0]:08x}','later_descriptor':f'{pointers[1]:08x}',
    'disc_descriptor_hex':buffers[0].hex(),'later_descriptor_hex':buffers[1].hex()}
   if p in seeds:assert seeds[p][0]==q
   else:seeds[p]=(q,why)
   callbacks.append(p)
 groups={'device_and_request_generation':(0x126350,0x219d50,0x21a530,0x192666,0x187610,0x1884e0),
  'player_and_unit_command':(0x14e780,0x14e7e0,0x150dd0,0x150f00,0x151c90,0xc8140,0xc6c50),
  'zoom_and_selectors':(0xc9920,0xc9cc0,0x1051c0,0x104a30,0xcf300,0xcf440,0xcd310,0x112270),
  'action_dispatch_and_admission':(0xe7f30,0xe7ef0,0xe7e60,0xe7ff0,0xe8610,0xeb080),
  'switch_melee_grenade_callbacks':tuple(dict.fromkeys(callbacks))}
 comparison.main(groups=groups,seed_pairs=seeds,source_files=('src/gameplay/units/unit_input.cpp','src/gameplay/units/player_command.cpp','src/gameplay/units/unit_inventory.cpp'),
  configuration_tool={'path':'tools/research_mp_requested_controls_updates.py','sha256':sha(__file__),
   'source_dispatcher_pair':{'disc':'000e7f30','later':'000e8e50','complete_body_and_branches_equal':True}})
if __name__=='__main__':main()
