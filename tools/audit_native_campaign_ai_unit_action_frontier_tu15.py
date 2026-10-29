#!/usr/bin/env python3
"""Finite original order-reset unit-action graph, native support and open begin owners."""
import argparse,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROLES={0xe8e10:'zeroed32-byte action request producer',0xe8e50:'generic action begin dispatch and notification',0xb9390:'object/parent update marking',0xed110:'action1D admission, interrupts and parent/seat transition',0xe8f10:'action interrupt callback and retained action bit clear',0x8fdb0:'actual empty RET8 interrupt',0xed060:'parent/seat transition with authored animation and detach',0x1efa70:'local-player action notification admission',0x1eec30:'oldest-of-four action history publication',0x1c4600:'actor-triggered request69 speech',0x213d30:'speech selection/admission frontier',0xd0520:'interrupt8/12/0/A/1B, begin13, animation owner',0xece70:'unit detach/transition composition',0x111b10:'authored animation request',0xbbc90:'unit animation/local state update',0xb2d40:'object parent-related state transition',0xd37f0:'authored seat-flag-dependent action17 request'}
SUPPORT={0xb9390,0xe8f10,0x8fdb0,0x1efa70,0x1eec30}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);a=p.parse_args();assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir();image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');owners=[];unexpanded=set()
 for entry,role in ROLES.items():
  b=_trace(code,entry,set(ROLES));calls=[]
  for pc in sorted(b.code):
   i=code.at(pc)
   if i.mnemonic!='call':continue
   target=int(i.op_str,16) if i.op_str.startswith('0x') else None;calls.append(dict(site=hex(pc),target=hex(target) if target else i.op_str))
   if target and target not in ROLES:unexpanded.add(target)
  owners.append(dict(entry=hex(entry),semantic_role=role,sites=len(b.code),original_body_sha256=hashlib.sha256(b''.join(bytes(code.at(pc).bytes) for pc in sorted(b.code))).hexdigest(),calls=calls,native_context=('required interrupt1C/1D/1F/20 only; other callbacks unqualified' if entry==0xe8f10 else 'complete support owner') if entry in SUPPORT else None))
 methods=[]
 for action in [0x1d,0x1c,0x1f,0x20,8,0x12,0,0xa,0x1b,0x13,0x17]:
  pointer=0x473d28+action*4;descriptor=int.from_bytes(image.read(pointer,4),'little');methods.append(dict(action=hex(action),directory=hex(pointer),descriptor=hex(descriptor),begin=hex(int.from_bytes(image.read(descriptor,4),'little')),interrupt=hex(int.from_bytes(image.read(descriptor+12,4),'little'))))
 def listing(start,stop):
  rows=[];pc=start
  while pc<stop:
   ins=code.at(pc);rows.append(dict(instruction=hex(pc),bytes=bytes(ins.bytes).hex(),operation=ins.mnemonic,operands=ins.op_str));pc+=ins.size
  assert pc==stop;return rows
 contracts=dict(ordinary_abi='B9390 void(EAXunit)/RET0; E8F10 void(ECXunit,EDItype)/RET0 for admitted families;1EFA70 void(EAXplayer,typeWORD/resultBYTE stack inputs)/RET8;1EEC30 void(EAXhistory,same inputs)/RET8. No scratchGP/EFLAGS result is exported.',mark_caller=listing(0xe8e65,0xe8e69),begin_callback_argument_rebind=listing(0xed110,0xed14b),interrupt_caller=listing(0xed184,0xed1c4),notification_return=listing(0xe8ea2,0xe8ea8),history_tail=listing(0x1efa90,0x1efaaa),liveness={'mark':'E8E50 pushes real requestEDI/unitESI and calls begin using preservedEBX. ED110 reloads unit from the real stack, definesEAX fromEBX,ECX from actual object rows andEDX from component data before use; its firstAND/TEST replaces flags. No mark scratch result is consumed.','interrupt':'ED110 redefinesEDI andECX before each next interruption. After finalED1AF call, ED1B4 definesfullEAX fromunitEBX beforeED060. ED060 begins by definingECX/EDX from object backing and writes flags before Jcc. No interrupt GP/flags result is consumed.','notification':'E8EA2 restoresESI, E8EA3 overwritesAL from the real begin-resultBL, then restoresEBX/ECX and returnsAL only. Notification scratchECX/EDX/flags and upperEAX are not consumed. No begin result is supplied by this module.','history':'1EFA70 tail-jumps1EEC30 and shares its void notification contract; callerE8E50 ignores scratch as above.','FP':'The five support owners contain no x87/SIMD/MMX instructions; actual empty8FDB0 has onlyRET8. Native core preserves caller FP state.'},boundaries=['The proved interrupt selector profile is1C/1D/1F/20; other handlers and arbitrary hidden/computed callers remain unqualified.','Existing typed span/provider void contracts agree; this receipt pins actual required TU1.5 caller continuations.'])
 result=dict(original_xbe_sha256=NEW_PIN,owners=owners,required_selected_descriptor_methods=methods,unexpanded_direct_targets=sorted(hex(x) for x in unexpanded),existing_generic_native_api='src/gameplay/units/unit_action_dispatch.cpp provides typed span/provider dispatch/interrupt/notification semantics; required callback and TU1.5 lazy-bank closure remains separate',runtime_descriptor_owner='src/platform/native_tu15_world_memory.cpp mounts complete OriginalData::native_tu15 unit_action_tables catalog',new_native_support_scope='B9390, required1C/1D/1F/20 interruptions including actual8FDB0,1EFA70/1EEC30; selector-only register admission performs no source reads',full_e8e10_e8e50_native=False,full_ai_set_orders_native=False,remaining_required_families=['ED060 seat/parent transition and authored animation/detach owners','D0520 extra interrupt callbacks and nested action13','D37F0 authored seat policy and nested action17','successful ED110 actor-triggered213D30 speech selection','actual begin-result production; no supplied acknowledgement'],no_runtime_original_execution=True)
 result['required_caller_contracts']=contracts
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(owners=len(owners),unexpanded_direct_targets=len(unexpanded),full_e8e10_e8e50_native=False)))
if __name__=='__main__':main()
