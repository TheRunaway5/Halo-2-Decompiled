#!/usr/bin/env python3
"""Authenticated scope and unwind ownership for remaining activation owners."""
import argparse,hashlib,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');rows={}
 for owner in(0x3232e0,0x323230,0x326100,0x325bd0,0x325c40,0x315e10,0x315d10,0x316dc0,0x316130,0x31f660,0x325af0,0x2f7a10,0x31b730,0x31e820,0x31e760,0x1e10a0,0x281f10,0x1d7530,0x22d460):
  body=complete_body(code,owner);listing=[];calls=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(dict(instruction=pc,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str))
   if i.mnemonic in('call','jmp')and(i.mnemonic=='call'or i.op_str.startswith('0x')):calls.append(listing[-1])
  text=''.join(f'{i["instruction"]:x} {i["bytes"]} {i["mnemonic"]} {i["operands"]}\n'for i in listing);rows[hex(owner)]=dict(sites=len(body.code),calls=calls,listing_sha256=hashlib.sha256(text.encode()).hexdigest(),listing=listing)
 unwind={}
 for owner,address in((0x3232e0,0x46d1c8),(0x323230,0x46d1a4)):
  words=struct.unpack('<9I',image.read(address,36));count,table=words[1:3];assert words[0]==0x19930520 and count<=16;entries=[]
  for n in range(count):
   to_state,action=struct.unpack('<iI',image.read(table+n*8,8));pc=action;listing=[]
   for _ in range(8):
    i=code.at(pc);listing.append(dict(instruction=pc,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str));pc+=i.size
    if i.mnemonic in('ret','jmp'):break
   entries.append(dict(state=n,to_state=to_state,action=action,listing=listing))
  unwind[hex(owner)]=dict(function_info=address,words=words,states=entries)
 save(a.output/'report.json',dict(passed=True,image_sha256=NEW_PIN,owners=rows,unwind=unwind,actual_binding_residual_cohort={'0x22d460':30,'0x31b730':30,'0x281f10':10,'0x3232e0':10},inventory_only=True,physics_parity_claimed=False,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],source_sha256=sha(Path(__file__))));print('Authenticated activation residual inventory and manager constructor unwind ownership retained.');return 0
if __name__=='__main__':raise SystemExit(main())
