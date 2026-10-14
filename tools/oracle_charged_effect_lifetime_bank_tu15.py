#!/usr/bin/env python3
"""Compare an actual charge-effect consumer with unchanged TU1.5 instructions.

The input bank is exported from native construction. The retained input has an actually constructed/equipped source player and pistol
after60 neutral source ticks. Native caller ABI is explicit. This checks the full
effect consumer, not Xbox boot or every earlier constructor. Native
uncontended locks and allocation notifications are explicit platform policies.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from validate_charge_effect_bank_tu15 import validate

def regions(path):
    with path.open('rb') as f:
        while header := f.read(8):
            p,n=struct.unpack('<II',header);b=f.read(n);assert len(b)==n
            yield p,b

def stores(path):
    with path.open('rb') as f:
        while header := f.read(12):
            pc,p,n=struct.unpack('<III',header)
            before,after=f.read(n),f.read(n);assert len(after)==n
            if not 0x60000000<=p<0x60200000:
                yield pc,p,before,after

def segment(base, access):
    return struct.pack('<II',0xffff|((base&0xffff)<<16),
                       ((base>>16)&255)|(access<<8)|0xcf0000|(base&0xff000000))

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--case',type=Path,required=True);ap.add_argument('--output',type=Path);a=ap.parse_args()
    metadata=json.loads((a.case/'native.json').read_text())
    before=list(regions(a.case/'before.bin'));context=validate(a.case,metadata,before)
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    g=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set()
    def map_range(p,n):
        pending=[q for q in range(p&~4095,(p+n+4095)&~4095,4096) if q not in mapped]
        start=0
        while start<len(pending):
            end=start+1
            while end<len(pending) and pending[end]==pending[end-1]+4096:end+=1
            g.mem_map(pending[start],(end-start)*4096);start=end
        mapped.update(pending)
    for p,b in before:
        map_range(p,len(b));g.mem_write(p,b)
    for p in (0x70000000,0x60300000,0x80000000,0xf7000000):map_range(p,4096)
    g.mem_write(0x70000008,segment(0,0x9b)+segment(0,0x93)+segment(0x60200000,0x93))
    g.reg_write(R.UC_X86_REG_GDTR,(0,0x70000000,0xfff,0))
    for reg,val in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),
                    (R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):
        g.reg_write(reg,val)
    assert g.reg_read(R.UC_X86_REG_FS_BASE)==0x60200000
    registers=[getattr(R,'UC_X86_REG_'+s) for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
    raw_input=(a.case/'native-input.bin').read_bytes();assert len(raw_input)==168
    for reg,val in zip(registers,struct.unpack_from('<8I',raw_input)):g.reg_write(reg,val)
    for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),int.from_bytes(raw_input[32+i*16:48+i*16],'little'))
    eflags,fs=struct.unpack_from('<2I',raw_input,160);assert fs==g.reg_read(R.UC_X86_REG_FS_BASE)
    g.reg_write(R.UC_X86_REG_EFLAGS,eflags);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
    g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
    g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
    def u(p,n=4):return int.from_bytes(g.mem_read(p,n),'little')
    def w(p,v,n=4):g.mem_write(p,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
    assert u(0x600f0000)==0x60300000
    def ret(n):
        sp=g.reg_read(R.UC_X86_REG_ESP);target=u(sp)
        g.reg_write(R.UC_X86_REG_ESP,sp+4+n);g.reg_write(R.UC_X86_REG_EIP,target)
    native=list(stores(a.case/'native-stores.bin'));position=0;pc=0;count=0;services=[];reached={};floating=[];calls=[];pending=[]
    entries={0x108a70:'weapon_effect_request',0x179610:'effect_creator',0x17afb0:'effect_allocate',0x17c140:'initial_event_advance',0x17b130:'event_select',0x17a4a0:'effect_retire',0x17a380:'effect_free',0x17a420:'effect_cleanup',0x157d70:'player_response',0x17a0f0:'effect_stop',0x17a030:'effect_stop_beams'}
    def hook(uc,p,n,user):
        nonlocal pc,count
        sp=g.reg_read(R.UC_X86_REG_ESP)
        if p in (0x80000115,0x80000126):
            q=u(sp+4);services.append(dict(target=p,ret=u(sp),pointer=q))
            if p==0x80000115:
                current=(u(q+16)+1)&0xffffffff;w(q+16,current)
                if current and u(q+24)==1:w(q+20,u(q+20)+1)
                else:
                    assert not current or not u(q+24),'contended native lock requires explicit schedule'
                    w(q+24,1);w(q+20,1)
            else:
                depth=(u(q+20)-1)&0xffffffff;current=(u(q+16)-1)&0xffffffff
                w(q+20,depth);w(q+16,current)
                if not depth:
                    assert current&0x80000000,'wake requires explicit schedule'
                    w(q+24,0)
            ret(4);return
        if p==0xf7000001:
            assert u(sp)==0x2dee63 and u(sp+4)==10
            services.append(dict(target=p,ret=u(sp)));ret(8);return
        assert code.inside(p) and bytes(g.mem_read(p,n))==image.read(p,n),hex(p)
        pc=p;count+=1;reached.setdefault(p,bytes(g.mem_read(p,n)).hex())
        for row in pending[:]:
            if row['return_pc']==p and row['entry_sp']<sp<=row['entry_sp']+0x100:
                row['return_order']=count;row['return_eax']=g.reg_read(R.UC_X86_REG_EAX);row['return_sp']=sp;pending.remove(row)
        if p in entries:
            row=dict(name=entries[p],entry=p,order=count,entry_sp=sp,return_pc=u(sp),
                gp=[g.reg_read(reg) for reg in registers],stack_words=[u(sp+4+i*4) for i in range(6)])
            calls.append(row);pending.append(row)
        instruction=code.at(p)
        if instruction.mnemonic.startswith('f'):floating.append(dict(pc=f'{p:08x}',instruction=instruction.mnemonic+' '+instruction.op_str,status=g.reg_read(R.UC_X86_REG_FPSW),control=g.reg_read(R.UC_X86_REG_FPCW)))
    def write(uc,access,p,n,value,user):
        nonlocal position
        if 0x60000000<=p<0x60200000:return
        before=bytes(g.mem_read(p,n));after=(value&((1<<(8*n))-1)).to_bytes(n,'little')
        # Unicorn splits a 16-byte SSE store into two ordered eight-byte writes.
        expected=bytearray();old=bytearray();start=position
        while len(expected)<n and position<len(native):
            npc,np,nb,na=native[position]
            if np!=p+len(expected):break
            expected.extend(na);old.extend(nb);position+=1
        assert bytes(expected)==after and bytes(old)==before,(hex(pc),hex(p),n,
            before.hex(),after.hex(),'native',native[start:start+3])
    g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
    error=None;compared_bytes=0;exit_compared=False
    try:
        g.emu_start(metadata['entry'],0x60300000,count=10000000)
        assert g.reg_read(R.UC_X86_REG_EIP)==0x60300000,'charging effect consumer did not return'
        assert position==len(native),('unconsumed native source writes',position,len(native))
        for p,b in regions(a.case/'after.bin'):
            if 0x60000000<=p<0x60200000:continue
            actual=bytes(g.mem_read(p,len(b)))
            if actual!=b:
                first=next(i for i,(x,y) in enumerate(zip(actual,b)) if x!=y)
                raise AssertionError(('final retained bank differs',hex(p+first),actual[first:first+16].hex(),b[first:first+16].hex()))
            compared_bytes+=len(b)
        if (a.case/'native-exit.bin').exists():
            gp=tuple(g.reg_read(r) for r in registers)
            xmm=b''.join(g.reg_read(getattr(R,f'UC_X86_REG_XMM{i}')).to_bytes(16,'little') for i in range(8))
            status=g.reg_read(R.UC_X86_REG_FPSW)
            tags=g.reg_read(R.UC_X86_REG_FPTAG)
            depth=sum(((tags>>(2*i))&3)!=3 for i in range(8))
            actual=struct.pack('<8I',*gp)+xmm+struct.pack('<IIIHHI',
                g.reg_read(R.UC_X86_REG_EFLAGS),g.reg_read(R.UC_X86_REG_FS_BASE),depth,status,
                g.reg_read(R.UC_X86_REG_FPCW),g.reg_read(R.UC_X86_REG_MXCSR))
            expected=(a.case/'native-exit.bin').read_bytes()
            assert actual==expected,('world exit ABI differs',actual.hex(),expected.hex())
            exit_compared=True
    except (UcError,AssertionError) as e:error=str(e)
    report=dict(scope=__doc__,native_metadata=metadata,validated_source_context=context,original_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),case_before_sha256=hashlib.sha256((a.case/'before.bin').read_bytes()).hexdigest(),
                original_instructions=count,matched_native_stores=position,total_native_stores=len(native),
                original_pc=g.reg_read(R.UC_X86_REG_EIP),services=services,error=error,
                final_retained_bytes_compared=compared_bytes,final_abi_compared=exit_compared,
                whole_boot_proof=False,full_weapon_parity=False,instruction_patches=[],original_calls=calls,last_floating_instructions=floating[-32:],executed_instruction_bytes={f'{pc:08x}':raw for pc,raw in sorted(reached.items())})
    (a.output or a.case/'original-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('original_instructions','matched_native_stores','total_native_stores','final_retained_bytes_compared','final_abi_compared','services','error')}));return 1 if error else 0
if __name__=='__main__':raise SystemExit(main())
