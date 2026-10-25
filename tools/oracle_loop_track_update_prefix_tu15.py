#!/usr/bin/env python3
"""Measure original per-track requests up to their first playback operation.

The entry declares only the real per-track caller ABI. Execution stops before
the first backend operation; no call result, source voice or availability is
supplied. This qualifies decision prefixes, not completed looping playback.
"""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    cpu.mem_map(0x1000000,0x20000)
    authored=0x1001000;track=0x1002000;parent=0x1003000;definition=0x1004000
    pool=0x1005000;voice=0x1006000;table=0x1007000;gestalt=0x1008000;playbacks=0x1009000
    scales=0x100a000;stack=0x101e000
    def put(at,value):cpu.mem_write(at,struct.pack('<I',value&0xffffffff))
    def word(at):return struct.unpack('<I',cpu.mem_read(at,4))[0]
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    put(0x4f2c4c,pool);put(pool+0x44,voice);put(0x4f0414,table);put(0x52d664,gestalt)
    put(gestalt+4,playbacks);put(gestalt+0xc,scales)
    for n in range(7):put(table+n*16+8,0x100b000+n*0x20)
    current={};instructions={}
    def register(reg):return cpu.reg_read(reg)&0xffffffff
    def optional(value):return -1 if value==0xffffffff else value
    def native_voice(value):
        assert value in (0x12340000,0xffffffff),hex(value)
        return 1 if value!=0xffffffff else 0
    def observe(_cpu,pc,size,_):
        insn=code.at(pc);assert insn and bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=insn.bytes.hex();sp=register(R.UC_X86_REG_ESP)
        action=None
        if pc==0x221539:action=['none']
        elif pc==0x221535:action=['stopping',1]
        elif insn.mnemonic=='call':
            target=insn.operands[0].imm
            if target==0x222280:action=['find',register(R.UC_X86_REG_EBX),optional(word(sp))]
            elif target==0x222320:action=['find',register(R.UC_X86_REG_EAX),optional(register(R.UC_X86_REG_EDI))]
            elif target==0x222350:action=['create',word(sp+4),optional(word(sp+8)),word(sp+16)]
            elif target==0x21f960:action=['pitch_range',register(R.UC_X86_REG_EBX),word(track+0x10)]
            elif target==0x222710:action=['fade_out',native_voice(word(sp+4)),word(sp)]
            elif target==0x129900:action=['crossfade',native_voice(register(R.UC_X86_REG_EAX)),native_voice(register(R.UC_X86_REG_EDI)),word(sp)]
            else:raise AssertionError((hex(pc),hex(target)))
        if action is not None:current.update(action=action,stop_pc=f'{pc:08x}');cpu.emu_stop()
    cpu.hook_add(UC_HOOK_CODE,observe)
    state=0x1522ab91;rows=[]
    def draw():
        nonlocal state
        state=(state*1664525+1013904223)&0xffffffff;return state>>8
    for index in range(2400):
        mask=draw()%128;flags=draw()%32;phase=draw()%7;pending=draw()%7;command=draw()%3
        alternate=bool(draw()%2);created=bool(draw()%2);has_primary=bool(draw()%2)
        voice_definition=0x76540000+draw()%7;has_output=bool(draw()%2)
        definition_flags=2*(draw()%2);stopped=bool(draw()%2);was_stopping=bool(draw()%2)
        fade=bits(.25 if draw()%3==0 else 0);control=bits(.08 if index%2 else 1)
        raw=bytearray(0x58);struct.pack_into('<I',raw,4,flags)
        for n,offset in enumerate((0x18,0x20,0x28,0x44,0x30,0x4c,0x38)):
            struct.pack_into('<I',raw,offset,0x76540000+n if mask&(1<<n) else 0xffffffff)
        for offset,value in ((0xc,1.5),(0x10,.2),(0x50,.75),(0x54,.3)):struct.pack_into('<f',raw,offset,value)
        cpu.mem_write(authored,bytes(raw));cpu.mem_write(track,bytes(0x20));cpu.mem_write(parent,bytes(0x60));cpu.mem_write(stack,bytes(0x100))
        put(track,0x12340000 if has_primary else 0xffffffff);cpu.mem_write(track+12,bytes((phase,pending)))
        put(track+0x10,control);put(parent+0x10,control);cpu.mem_write(parent+0x50,bytes((8 if stopped else 0,0,2 if was_stopping else 1)))
        put(definition,definition_flags);put(voice+0xc,voice_definition);cpu.mem_write(voice+0xac,struct.pack('<H',0 if has_output else 0xffff))
        for offset,value in ((0x13,int(alternate)),(0x14,0x33330000),(0x18,definition),(0x20,parent),
                             (0x50,0),(0x54,authored),(0x58,int(created)),(0x5c,0x100d000),(0x64,command),(0x6c,fade)):
            if offset in (0x13,0x58):cpu.mem_write(stack+offset,bytes((value,)))
            else:put(stack+offset,value)
        for register_id,value in ((R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_EBP,track),(R.UC_X86_REG_ECX,parent),
                                  (R.UC_X86_REG_EDX,authored),(R.UC_X86_REG_EBX,command),(R.UC_X86_REG_EAX,0)):
            cpu.reg_write(register_id,value)
        current.clear();cpu.reg_write(R.UC_X86_REG_EFLAGS,2);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        cpu.emu_start(0x220fbd,0,count=1000);assert 'action' in current
        rows.append(dict(mask=mask,flags=flags,phase=phase,pending=pending,state=command,alternate=alternate,
            created=created,has_primary=has_primary,voice_definition=voice_definition,has_output=has_output,
            definition_flags=definition_flags,stopped=stopped,was_stopping=was_stopping,fade_bits=fade,control_bits=control,
            action=current['action'],out_primary=word(track)!=0xffffffff,
            out_phase=cpu.mem_read(track+12,1)[0],out_pending=cpu.mem_read(track+13,1)[0],stop_pc=current['stop_pc']))
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,entry='00220fbd',cases=rows,
        executed_original_instructions=instructions,source_instruction_patches=[],supplied_callee_results=[],completed_playback_verified=False)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,separators=(',',':'))+'\n')
    print(json.dumps(dict(cases=len(rows),bytes=args.output.stat().st_size,sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())),flush=True)
if __name__=='__main__':main()
