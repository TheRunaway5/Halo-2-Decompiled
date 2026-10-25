#!/usr/bin/env python3
"""Execute the complete original pre-creation track transition slice.

All seven current/pending phases and authored flag combinations are declared
component inputs. The CPU stops before a voice-creation call, supplies no
callee result, and retains only a compact input/result matrix and digest.
"""
import hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1]
def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(0x1000000,0x10000);track=0x1001000;phase=0x1002000;stack=0x100e000
    tables={name:list(struct.unpack('<7i',image.read(address,28))) for name,address in
        (('normal',0x455ba8),('alternate',0x455bc4),('reference_offsets',0x455be0))}
    assert tables['reference_offsets']==[0x14,0x1c,0x24,0x40,0x2c,0x48,0x34]
    instructions={};stop={}
    def observe(_g,pc,size,_):
        if pc in (0x22121b,0x22123b):stop['pc']=pc;g.emu_stop();return
        i=code.at(pc);assert i and i.mnemonic!='call' and bytes(g.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=i.bytes.hex()
    g.hook_add(UC_HOOK_CODE,observe)
    rows=[]
    # Full reference presence masks cover detail-only/no-track and every
    # normal/alternate transition shape without supplying a voice result.
    for mask in range(128):
      raw=bytearray(0x58)
      for n,offset in enumerate(tables['reference_offsets']):struct.pack_into('<I',raw,offset+4,0x12340000+n if mask&(1<<n) else 0xffffffff)
      for flags in range(32):
       struct.pack_into('<I',raw,4,flags);g.mem_write(track,bytes(raw))
       for current in range(7):
        for pending in range(7):
         for alternate in (0,1):
            g.mem_write(phase,bytes(0x20));g.mem_write(phase+12,bytes((current,pending)))
            g.mem_write(stack,bytes(0x100));g.mem_write(stack+0x60,struct.pack('<I',alternate))
            for register in (R.UC_X86_REG_EAX,R.UC_X86_REG_EBX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDI):g.reg_write(register,0)
            g.reg_write(R.UC_X86_REG_EDX,track);g.reg_write(R.UC_X86_REG_EBP,phase)
            g.reg_write(R.UC_X86_REG_ESI,0xffffffff);g.reg_write(R.UC_X86_REG_ESP,stack)
            stop.clear();g.emu_start(0x221122,0,count=500);assert stop.get('pc') in (0x22121b,0x22123b)
            rows.append([mask,flags,current,pending,alternate,stop['pc'],g.reg_read(R.UC_X86_REG_EBX),
                struct.unpack('<I',g.mem_read(stack+0x24,4))[0],g.mem_read(phase+13,1)[0]])
    packed=b''.join(struct.pack('<9I',*row) for row in rows)
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,entry=0x221122,
        tables=tables,cases=len(rows),results_sha256=hashlib.sha256(packed).hexdigest(),
        input_order=['reference_presence_mask','track_flags','current_phase','pending_phase','alternate'],
        output_order=['stop_pc','next_sound','impulse_sound','pending_phase'],
        executed_original_instructions=instructions,source_instruction_patches=[],supplied_callee_results=[])
    # The exhaustive rows stay disposable. A native regression can regenerate
    # them from the declared grid and compare this independently measured hash.
    (scratch/'track-transition-rows.bin').write_bytes(packed)
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio/track-transition-original.json'
    out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('executed_original_instructions','scope')}),flush=True)
if __name__=='__main__':main()
