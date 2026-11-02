#!/usr/bin/env python3
"""Exhaust unchanged TU1.5 D83F0..D8426 for every pending-response word.

Entry is the real per-section loop boundary, with a declared current pending
pointer in the caller's stack. Stop at its unchanged Waiting/Idle continuation
or before Due reads the object pool. No eligibility, damage, effect, retirement,
clock or callee result is supplied. Numeric references contain no raw banks.
"""
import argparse,gzip,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_PROT_READ,UC_PROT_EXEC
from unicorn import x86_const as R

BANK,EXTENT,STACK,PENDING=0x2100000,256,0x2100080,0x2100020
ENTRY,DUE,CONTINUE=0xd83f0,0xd842c,0xd84dc


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--vectors',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if a.output.exists() or a.vectors.exists():p.error('Preserve prior original evidence')
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
    u.mem_protect(0x10000,0x800000,UC_PROT_READ|UC_PROT_EXEC);u.mem_map(BANK,4096)
    registers=[getattr(R,'UC_X86_REG_'+n)for n in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
    current={};sites={};total=0
    def instruction(_,pc,size,__):
        if pc in(DUE,CONTINUE):current['stop']=pc;u.emu_stop();return
        assert ENTRY<=pc<0xd842c and bytes(u.mem_read(pc,size))==image.read(pc,size)
        i=code.at(pc);assert i.size==size and i.mnemonic!='call';sites[f'{pc:08x}']=bytes(i.bytes).hex()
        current['pc']=pc;current['steps']+=1
        if current['record']:current['trace'].append(pc)
    def access(_,kind,address,size,value,__):
        assert BANK<=address and address+size<=BANK+EXTENT
        if current['record']:
            current['accesses'].append(dict(pc=current['pc'],kind='write' if kind==17 else 'read',address=address,size=size))
    u.hook_add(UC_HOOK_CODE,instruction);u.hook_add(UC_HOOK_MEM_READ,access);u.hook_add(UC_HOOK_MEM_WRITE,access)
    selected=set(range(24))|{31,32,63,64,127,128,255,256,511,512,1023,1024,4095,4096,8191,8192,16383,16384,32767,32768,32769,65527,65528,65534,65535}
    rows=[];raw=bytearray(b'PND15V01'+struct.pack('<I',65536));counts=[0,0,0]
    for word in range(65536):
        before=bytearray([0xa5]*EXTENT);struct.pack_into('<H',before,PENDING-BANK,word);struct.pack_into('<I',before,STACK-BANK+0x1c,PENDING)
        u.mem_write(BANK,bytes(before));gp=[(0x61834a79*(j+1)+word*0x19660d)&0xffffffff for j in range(8)];gp[4]=STACK
        for register,value in zip(registers,gp):u.reg_write(register,value)
        flags=0x202|(word&0xcd5);u.reg_write(R.UC_X86_REG_EFLAGS,flags)
        current.clear();current.update(record=word in selected,steps=0,trace=[],accesses=[])
        u.emu_start(ENTRY,0,count=32);assert current.get('stop')in(DUE,CONTINUE);total+=current['steps']
        after=bytes(u.mem_read(BANK,EXTENT));new=struct.unpack_from('<H',after,PENDING-BANK)[0]
        wrote=after[STACK-BANK+0x13]!=before[STACK-BANK+0x13]
        status=2 if current['stop']==DUE else 1 if wrote else 0
        count=u.reg_read(R.UC_X86_REG_EAX) if not wrote else (word>>3)
        # Only the output word/status are interpreted. The input countdown
        # field is explicit reference metadata, not an assumed original result.
        raw.extend(struct.pack('<HHBB',new,count,word&7,status));counts[status]+=1
        if word in selected:rows.append(dict(word=word,before_gp=gp,before_eflags=flags,before_bank_hex=before.hex(),
            after_gp=[u.reg_read(reg)&0xffffffff for reg in registers],after_eflags=u.reg_read(R.UC_X86_REG_EFLAGS),
            after_bank_hex=after.hex(),trace=current['trace'],accesses=current['accesses'],status=status,stop=current['stop']))
    a.vectors.parent.mkdir(parents=True,exist_ok=True);packed=gzip.compress(bytes(raw),mtime=0);a.vectors.write_bytes(packed)
    result=dict(passed=True,scope=__doc__,xbe_sha256=NEW_PIN,entry=ENTRY,due_boundary=DUE,continue_boundary=CONTINUE,
        bank=BANK,extent=EXTENT,stack=STACK,pending=PENDING,word_cases=65536,status_counts=counts,abi_cases=rows,
        original_instructions=total,original_instruction_sites=sites,defined_flags_mask=0x8c5,
        vector_sha256=hashlib.sha256(packed).hexdigest(),vector_raw_sha256=hashlib.sha256(raw).hexdigest(),
        vectors_bytes=len(packed),vectors_raw_bytes=len(raw),original_code_patches=[],supplied_callee_results=[],
        declared_loop_entry=True,full_pending_parent_or_event_delivery_verified=False,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),retained_large_artifacts=[])
    text=json.dumps(result,separators=(',',':'))+'\n';assert len(text)<1024*1024
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(text)
    print(json.dumps(dict(passed=True,word_cases=65536,abi_cases=len(rows),status_counts=counts,instructions=total,vector_bytes=len(packed))),flush=True)


if __name__=='__main__':main()
