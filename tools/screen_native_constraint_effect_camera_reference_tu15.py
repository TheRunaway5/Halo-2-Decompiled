#!/usr/bin/env python3
"""Screen a retained original TU1.5 RAM reference for authentic view DATA.

This is offline read-only reference extraction, never a physics execution
backend. A candidate page directory must translate the entire unchanged
TU1.5 text, not just contain plausible camera/player words.
"""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from validate_native_contact_agent_dispatch_tu15 import sha,save
RAM=ROOT/'analysis/camera/vehicle-tu15-completion-20261015/reference-scorpion-v2/approach-state/physical-ram.bin'
RAM_SHA='905fc8afb1215727f2319aa0f12775725de4c97cc10fa4cf21e20ab9eadfea97'
TEXT_SHA='d9c6fd7e7475862b1f19c2da80d7f2f6dc41da53619991002e9546e5f9e629fe'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 assert sha(NEW_DEFAULT)==NEW_PIN and sha(RAM)==RAM_SHA;ram=RAM.read_bytes();image=Xbe(NEW_DEFAULT)
 def physical_word(at):
  if at<0 or at+4>len(ram):raise ValueError('Unmapped physical reference')
  return struct.unpack_from('<I',ram,at)[0]
 def translate(directory,address):
  pde=physical_word(directory+(address>>22)*4)
  if not pde&1:raise ValueError('Unmapped PDE')
  if pde&0x80:return (pde&0xffc00000)+(address&0x3fffff)
  pte=physical_word((pde&0xfffff000)+((address>>12)&0x3ff)*4)
  if not pte&1:raise ValueError('Unmapped PTE')
  return (pte&0xfffff000)+(address&0xfff)
 def read(directory,address,size):
  result=bytearray()
  while size:
   physical=translate(directory,address);count=min(size,4096-(address&4095))
   if physical+count>len(ram):raise ValueError('Unmapped physical reference bank')
   result.extend(ram[physical:physical+count]);address+=count;size-=count
  return bytes(result)
 first=image.read(0x12000,4096);candidates=[]
 for directory in range(0,len(ram),4096):
  try:
   if read(directory,0x12000,4096)!=first:continue
   text=read(directory,0x12000,3611948)
   if hashlib.sha256(text).hexdigest()!=TEXT_SHA:continue
   word=lambda address:int.from_bytes(read(directory,address,4),'little')
   players=word(0x4f54f0);slots=[word(players+0xc+n*4)for n in range(4)]
   bsp=int.from_bytes(read(directory,0x474c24,2),'little')
   cameras=[dict(slot=n,player=datum,position_words=[word(0x4f655c+n*0x358+k*4)for k in range(3)],
     scale_word=word(0x4f655c+n*0x358+0x50))for n,datum in enumerate(slots)if datum!=0xffffffff]
   clock=word(0x51df54);clock_bytes=read(directory,clock,36);clock_file=RAM.parent/'game-clock.bin'
   checks=dict(game_clock_snapshot_equal=clock_file.exists()and clock_bytes==clock_file.read_bytes())
   state= json.loads((RAM.parent/'state.json').read_text())
   object_checks=[]
   for obj in state['objects'][:8]:
    body=obj['body_address'];original=RAM.parent/('object-'+format(obj['object'],'08x')+'.bin')
    if original.exists():object_checks.append(read(directory,body,original.stat().st_size)==original.read_bytes())
   checks['first_eight_object_snapshots_equal']=object_checks
   candidates.append(dict(directory_physical=directory,full_text_sha256=TEXT_SHA,players_pointer=players,
    local_player_slots=slots,active_bsp=bsp,cameras=cameras,snapshot_checks=checks))
  except (ValueError,KeyError,AssertionError):continue
 save(a.output/'report.json',dict(read_only=True,physics_execution_backend=False,manufactured_rows=False,
  reference=dict(path=str(RAM),sha256=RAM_SHA,bytes=len(ram)),original_sha256=NEW_PIN,candidates=candidates,
  unique_virtual_view_closures=len({json.dumps({k:r[k]for k in ('players_pointer','local_player_slots','active_bsp','cameras')},sort_keys=True)for r in candidates}),
  qualification_claimed=False,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 print([(hex(r['directory_physical']),r['local_player_slots'],r['active_bsp'],r['snapshot_checks'])for r in candidates]);return 0
if __name__=='__main__':raise SystemExit(main())
