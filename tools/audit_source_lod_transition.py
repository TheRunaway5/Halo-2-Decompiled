#!/usr/bin/env python3
"""Verify the original geometry-transition output is dead after 4C450 returns."""
import hashlib,json,sys
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_OP_MEM
from capstone.x86_const import X86_REG_ESP
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
x=Xbe(ROOT/'assets/disc/default.xbe')
assert hashlib.sha256(x.data).hexdigest()==PIN
cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
s=next(s for s in x.sections if s.va<=0x4be41<s.va+len(x.section_bytes(s)))
data=x.section_bytes(s)
def decoded(a,b):return list(cs.disasm(data[a-s.va:b-s.va],a))
producer=decoded(0x4be41,0x4be52)
assert [(i.mnemonic,i.op_str) for i in producer]==[
 ('mov','edx, dword ptr [esp + 0x1c]'),('lea','ecx, [esi + 0x28]'),
 ('push','edx'),('lea','ebx, [esp + 0x24]'),('call','0x4c450')]
# After the stdcall ret4, the fraction occupies original [esp+20], the
# caller's consumed distance argument. Every later stack access/address is
# checked with the literal push/callee cleanup adjustments.
offset=0;references=[];calls=[]
for i in decoded(0x4be52,0x4c0ab):
 for op in i.operands:
  if op.type==CS_OP_MEM and op.mem.base==X86_REG_ESP:
   absolute=offset+op.mem.disp
   assert absolute!=0x20,f'transition escapes/consumed at {i.address:x}'
   references.append({'instruction':f'{i.address:08X}','caller_stack_offset':absolute,'caller_stack_offset_hex':f'0x{absolute:X}'})
 if i.mnemonic=='push':offset-=4
 elif i.mnemonic=='call':
  target=int(i.op_str,16);calls.append(f'{target:08X}')
  cleanup={0xbab40:12,0x4c0d0:4}[target];offset+=cleanup
assert offset==0
report={'original_executable_sha256':PIN,'fraction_call':'0004BE4D',
 'output_pointer':'EBX = pre-push ESP+0x20, consumed caller distance argument',
 'post_return_instructions':len(decoded(0x4be52,0x4c0ab)),
 'post_return_stack_references':references,'calls_after_return':calls,
 'transition_references_after_return':0,
 'scope':'4C450 uses the fraction internally for hysteresis. 4BCC0 stores only the capped integer LOD in its draw context; no fraction survives to 133F70/2CBF0. No geometry morph, opacity, stream blend or texture override is implied by this scalar.'}
p=ROOT/'analysis/multiplayer-shading/lod-transition-consumer-proof.json';p.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
