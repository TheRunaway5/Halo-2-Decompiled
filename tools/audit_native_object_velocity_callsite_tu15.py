#!/usr/bin/env python3
"""Authenticate the internal vehicle B9800 void-call continuation contract."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');body=complete_body(code,0xf1dd0);assert 0xf2091 in body.code;call=code.at(0xf2091);assert call.mnemonic=='call'and call.op_str=='0xb9800'
 expected=[(0xf2096,'inc','byte ptr [edi + 0x34e]'),(0xf209c,'pop','edi'),(0xf209d,'pop','esi'),(0xf209e,'mov','al, 1'),(0xf20a0,'pop','ebp'),(0xf20a1,'add','esp, 0x2c'),(0xf20a4,'ret','')];tail=[]
 for pc,mnemonic,operands in expected:
  assert pc in body.code;i=code.at(pc);assert(i.mnemonic,i.op_str)==(mnemonic,operands),(hex(pc),i.mnemonic,i.op_str);reads,writes=i.regs_access();tail.append(dict(instruction=pc,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str,register_reads=[i.reg_name(r)for r in reads],register_writes=[i.reg_name(r)for r in writes],eflags=i.eflags))
 packet=[]
 for pc,mnemonic,operands in((0xf1de4,'mov','esi, eax'),(0xf2088,'lea','eax, [esp + 0x14]'),(0xf208c,'push','eax'),(0xf208d,'lea','eax, [esp + 0x30]'),(0xf2091,'call','0xb9800')):
  assert pc in body.code;i=code.at(pc);assert(i.mnemonic,i.op_str)==(mnemonic,operands);packet.append(dict(instruction=pc,bytes=i.bytes.hex(),mnemonic=mnemonic,operands=operands))
 callee=complete_body(code,0xb9800);returns=[]
 for pc in sorted(callee.code):
  i=code.at(pc)
  if i.mnemonic=='ret':assert i.op_str=='4';returns.append(dict(instruction=pc,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str))
 assert returns
 assert all(not any(name in row['register_reads']for name in('eax','ax','al','ah','ecx','cx','cl','ch','edx','dx','dl','dh',*(f'xmm{n}'for n in range(8))))for row in tail)
 section=image.section('.text');text=image.section_bytes(section);offset=0;candidates=[]
 while True:
  offset=text.find(b'\xe8',offset)
  if offset<0:break
  if offset+5<=len(text)and(section.va+offset+5+struct.unpack_from('<i',text,offset+1)[0])&0xffffffff==0xb9800:candidates.append(section.va+offset)
  offset+=1
 outer=complete_body(code,0xf2340);assert 0xf24d8 in outer.code;outer_call=code.at(0xf24d8);assert(outer_call.mnemonic,outer_call.op_str)==('call','0xf1dd0')
 outer_expected=[(0xf24dd,'mov','dl, byte ptr [esp + 0x12]'),(0xf24e1,'or','dl, al'),(0xf24e3,'mov','al, byte ptr [edi + 0xc1]'),(0xf24e9,'and','al, 1'),(0xf24eb,'mov','byte ptr [esp + 0x12], dl'),(0xf24ef,'mov','byte ptr [esp + 0x13], al'),(0xf24f3,'je','0xf257e'),(0xf24f9,'mov','eax, ebp'),(0xf257e,'lea','eax, [esp + 0x104]')];outer_tail=[]
 for pc,mnemonic,operands in outer_expected:
  assert pc in outer.code;i=code.at(pc);assert(i.mnemonic,i.op_str)==(mnemonic,operands);outer_tail.append(dict(instruction=pc,bytes=i.bytes.hex(),mnemonic=mnemonic,operands=operands))
 assert(code.at(0xf20c0).mnemonic,code.at(0xf20c0).op_str)==('mov','ecx, dword ptr [0x4ecbd0]');assert(code.at(0xf20c6).mnemonic,code.at(0xf20c6).op_str)==('mov','edx, dword ptr [ecx + 0x44]');assert(code.at(0xf2c20).mnemonic,code.at(0xf2c20).op_str)==('lea','ecx, [eax + 0xbc0]');assert(code.at(0xf2c26).mnemonic,code.at(0xf2c26).op_str)==('mov','edx, 0x10')
 listing=''.join(f'{row["instruction"]:x} {row["bytes"]} {row["mnemonic"]} {row["operands"]}\n'for row in tail);files=[ROOT/'src/physics/vehicles/physics_vehicle_update_generated.cpp',ROOT/'src/physics/physics_phantom_world_tu15_generated.cpp',Path(__file__)];report=dict(passed=True,image_sha256=NEW_PIN,caller=0xf1dd0,canonical_vehicle_caller=0xf0eb0,callee=0xb9800,canonical_vehicle_callee=0xb86c0,actual_callsite=0xf2091,actual_return_site=0xf2096,canonical_callsite=0xf1171,canonical_return_site=0xf1176,generated_internal_call_line=716,tail=tail,tail_listing_sha256=hashlib.sha256(listing.encode()).hexdigest(),unread_volatile_registers_before_local_return=['ecx','edx',*(f'xmm{n}'for n in range(8))],flags_before_first_use='INC writes arithmetic flags exceptCF;ADD ESP,2C overwrites remaining arithmetic flags before any flag read',caller_boolean_low_byte='MOV AL,1 overwrites AL before local return',eax_upper_bits_at_caller_return='Unspecified locally; the authenticated direct F2340 caller reads only AL atF24E1 and overwrites fullEAX on both successor paths atF24F9/F257E before any upper-bit read',outer_caller=0xf2340,outer_caller_sites=len(outer.code),outer_callsite=0xf24d8,outer_return_width_use=outer_tail,outer_ecx_edx='Not consumed by F24DD..F24F3; next F20C0/F2C20 helpers overwrite ECX andEDX before use',generic_guest_register_postimage_equivalence_qualified=False,scoped_typed_void_continuation_qualified=True,known_direct_outer_caller_return_width_qualified=True,direct_relative_call_byte_candidates=candidates,complete_caller_inventory_qualified=False,candidate_inventory_limit='These are E8 displacement byte matches, not a complete instruction-boundary/indirect-call inventory.',source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},instruction_patches=[],supplied_original_outcomes=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 generated=(ROOT/'src/physics/vehicles/physics_vehicle_update_generated.cpp').read_text();needle='if(!handoff_native_object_velocity(s))run_b86c0(s);';assert generated.count(needle)==1
 report.update(generated_internal_call_line=generated[:generated.index(needle)].count('\n')+1,authored_argument_packet=packet,callee_return_sites=returns,actual_entry_contract=dict(object='ESI',linear='word(entrySP+4)==entrySP+1C',angular='EAX==entrySP+34',return_address='word(entrySP)==F2096',source_return_pop=4,total_entry_to_caller_stack_delta=8),packet_derivation='If P is ESP at F2088, PUSH stores linear=P+14 at P-4; CALL stores F2096 at P-8; angular=(P-4)+30=P+2C. Thus entrySP=P-8 gives linear=SP+1C and angular=SP+34.',incoming_x87_depth_admission='Every depth 0..5 independently qualified with physical original B9800 and genuine nested owners in native-object-velocity-live-transport-tu15; no empty-stack assumption is inferred from the caller.')
 save(a.output/'report.json',report);print('Authenticated F2091->B9800 typedvoid continuation andF2340 directcaller AL-only return use; no guestvolatile postimage supplied. Full caller inventory remains open.');return 0
if __name__=='__main__':raise SystemExit(main())
