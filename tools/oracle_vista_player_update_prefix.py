#!/usr/bin/env python3
"""Execute unchanged TU1.1 player-update prefix through actual activation.

Stops at14EAEC after14EAE0's actual first call14E9E0, before respawn iteration.
The imported oracle's named profile/binding/delivery services remain external.
No respawn selection or successful whole-world tick is claimed here.
"""
import hashlib,json,struct
from pathlib import Path
from oracle_vista_player_birth import Original,r16,r32,hx
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
ROOT=Path(__file__).resolve().parents[1]
def main():
 original=Original('tu11');u=original.u;rows=[]
 for kind in ('local','mismatched_machine','film','simulation_not_initialized'):
  for left in (0,1):
   q=dict(slot=0,salt=0x1234,new_byte=1,left=left,user=0,controller=0,world=kind)
   original.setup(q);original.phase='actual_native_birth'
   original.call(original.meta['birth'],(original.datum,1,original.creation));born=r16(u,original.player+2)
   original.fragment_stop=0x14eaec
   before=original.instructions
   for pass_index in (0,1):
    original.phase=f'actual_player_update_prefix_{pass_index}'
    u.mem_write(original.stack,struct.pack('<II',original.stop,original.data+0x18000))
    u.reg_write(UC_X86_REG_ESP,original.stack);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
    u.emu_start(0x14eae0,0x14eaed,count=100000)
    assert u.reg_read(UC_X86_REG_EIP)==0x14eaec
   current=r32(u,original.player+0x2c);flags=r16(u,original.player+2)
   events=[call for call in original.calls if call['call']=='local_delivery_external']
   admitted=not left and kind not in ('mismatched_machine','film')
   assert current==0xffffffff and flags==born|int(admitted) and len(events)==int(admitted)
   assert all(call['phase']=='actual_player_update_prefix_0'for call in events)
   rows.append(dict(input=q,born_flags=born,after_prefix_flags=flags,current_unit=hx(current),
       calls=original.calls,stores=original.stores,instructions=original.instructions-before))
 output=ROOT/'analysis/frontend/vista-session-flow-audit/player-birth-original'
 evidence=dict(source=str(original.path),source_sha256=hashlib.sha256(original.raw).hexdigest(),
  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),entry='0014eae0',end_exclusive='0014eaec',
  cases=len(rows),instructions=original.instructions,scope=__doc__,rows=rows)
 (output/'native-player-update-prefix.json').write_text(json.dumps(evidence,indent=2)+'\n')
 (output/'native-player-update-prefix-executed.asm').write_text(original.disasm())
 print(f'PASS {len(rows)} actual TU1.1 birth/player-update-prefix cases; activation precedes respawn iteration; {original.instructions} instructions')
if __name__=='__main__':main()
