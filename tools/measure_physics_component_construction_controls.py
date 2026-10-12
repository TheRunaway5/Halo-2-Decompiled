#!/usr/bin/env python3
"""Execute complete original component and raw pool owners on retained inputs."""
import argparse,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as reg
from oracle_mp_title_update_display import OLD,NEW,PINS,Xbe,sha,save,encoded

STACK,STOP=0x02008000,0x03000000
SLOTS=(0x526248,0x4e78d0,0x526230,0x518444)

def read(regions,address,n=4):
    return next(int.from_bytes(b[address-p:address-p+n],'little')for p,b in regions if p<=address and address+n<=p+len(b))

def write(regions,address,n,value):
    for p,b in regions:
        if p<=address and address+n<=p+len(b):b[address-p:address-p+n]=value.to_bytes(n,'little');return
    raise AssertionError(('Declared control escaped owning storage',hex(address)))

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--pair',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    original=json.loads(a.capture.read_text());pair=json.loads(a.pair.read_text());owners=pair['owners']
    assert original['entire_factory_and_retained_slot_sequence_identical']and not original['instruction_patches']and not original['supplied_callee_results']
    base=[(int(r['address'],16),bytearray.fromhex(r['raw_hex']))for r in original['regions_before']]
    pool=read(base,SLOTS[0]);element=read(base,pool+0x44);object_=int(original['object'],16);clock=read(base,SLOTS[3])
    # Recover exact initializer input by applying observed original stores only
    # until its first instruction. This is a direct-owner test input, never a
    # native live allocator result or final body injection.
    initialized=[(p,bytearray(b))for p,b in base]
    for s in original['stores']:
        if s['instruction']=='001d3343':break
        write(initialized,int(s['address'],16),s['bytes'],int.from_bytes(bytes.fromhex(s['after_hex']),'little'))
    initial_element=next(bytes(b[element-p:element-p+160])for p,b in initialized if p<=element and element+160<=p+len(b))
    initial_clock=next(bytes(b[clock-p:clock-p+4])for p,b in initialized if p<=clock and clock+4<=p+len(b))
    actual_cw=int(original['floating_point']['x87_control_word'],16);actual_mx=int(original['floating_point']['mxcsr'],16)
    specs=[(0x1d2aa0,'actual_owned_input',base,[],1,object_,actual_cw,actual_mx),
           (0x16d2c0,'declared_sequence_to_actual_capacity_and_none',[(p,b)for p,b in base if p==pool],[],read(base,pool+0x20)+1,pool,actual_cw,actual_mx)]
    # The complete component bank contains header, elements and bitmap. These
    # explicit scalar controls retain its actual capacity and data pointers.
    for salt in(0,0x7fff,0xfffe,0xffff):
        specs.append((0x16d2c0,f'explicit_salt_{salt:04x}',[(p,b)for p,b in base if p==pool],[(pool+0x40,2,salt)],2,pool,actual_cw,actual_mx))
    for stride in(2,3,5,24,160):
        specs.append((0x16d2c0,f'explicit_stride_{stride}',[(p,b)for p,b in base if p==pool],[(pool+0x24,4,stride)],2,pool,actual_cw,actual_mx))
    bitmap=read(base,pool+0x48)
    for hole in(0,31,32,511):
        changes=[(pool+0x34,4,0),(pool+0x38,4,512),(pool+0x3c,4,511)]
        changes.extend((bitmap+i*4,4,(0xffffffff^(1<<(hole&31)))if i==hole//32 else 0xffffffff)for i in range(16))
        specs.append((0x16d2c0,f'explicit_bitmap_hole_{hole}',[(p,b)for p,b in base if p==pool],changes,2,pool,actual_cw,actual_mx))
    for cw in(0x027f,0x067f,0x0a7f,0x0e7f):
        for mx in(0x1f80,0x3f80,0x5f80,0x7f80):
            for tick in(-32768,-32767,-101,-21,-10,-3,-1,0,1,3,10,21,101,32766,32767):
                fields=[(element,bytearray(initial_element)),(SLOTS[3],bytearray(struct.pack('<I',clock))),(clock,bytearray(initial_clock))]
                specs.append((0x1d3340,f'explicit_tick_{tick}_cw_{cw:04x}_mx_{mx:04x}',fields,[(clock+2,2,tick&65535)],1,element,cw,mx))
    cases=[];canonical={int(o['old'],16):o for o in owners}
    for side,path in enumerate((OLD,NEW)):
        assert sha(path)==PINS[str(path)];image=Xbe(path);pcmap={};allowed=set();slotmap={};scale_address=None
        for owner in owners:
            lines=[(a.pair.parent/'instructions'/f'{owner["old"]}-{tag}.asm').read_text().splitlines()for tag in('old','u5')]
            for left,right in zip(*lines,strict=True):
                pc=int((left if side==0 else right).split()[0],16);raw=(left if side==0 else right).split()[1]
                assert image.read(pc,len(raw)//2).hex()==raw;pcmap[pc]=int(left.split()[0],16);allowed.add(pc)
            for m in owner['memory']:
                old=int(m['old_va'],16);new=int(m['old_va'if side==0 else'new_va'],16)
                if old in slotmap:assert slotmap[old]==new
                slotmap[old]=new
        scale=image.read(slotmap[0x465198],4);assert scale.hex()==original['tick_scale_bits']
        for entry,name,fields,changes,repetitions,arg,cw,mx in specs:
            backing=[(slotmap.get(p,p),bytearray(b))for p,b in fields]
            for p,n,v in changes:write(backing,slotmap.get(p,p),n,v)
            g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
            for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
            pages=set(range(0x10000,0x810000,0x1000))
            for address,n in[(STACK-0x1000,0x2000),(STOP,0x1000)]+[(s,len(b))for s,b in backing]:
                for page in range(address&~4095,(address+n+4095)&~4095,4096):
                    if page not in pages:g.mem_map(page,4096);pages.add(page)
            for address,b in backing:g.mem_write(address,bytes(b))
            stores=[];returns=[]
            def code(_g,address,n,user):assert address in allowed,('Unexpected original callee',name,hex(address))
            def store(_g,kind,address,n,value,user):
                if STACK-0x1000<=address<STACK+0x1000:return
                assert n in(1,2,4)and any(s<=address and address+n<=s+len(b)for s,b in backing),hex(address)
                stores.append(dict(instruction=f'{pcmap[g.reg_read(reg.UC_X86_REG_EIP)]:08x}',address=f'{address:08x}',bytes=n,
                    before_hex=bytes(g.mem_read(address,n)).hex(),after_hex=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
            g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,store)
            target=int(canonical[entry]['old'if side==0 else'u5'],16)
            for repeat in range(repetitions):
                g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(reg.UC_X86_REG_ESP,STACK)
                g.reg_write(reg.UC_X86_REG_EDX,arg if entry==0x16d2c0 else object_)
                g.reg_write(reg.UC_X86_REG_EAX,arg);g.reg_write(reg.UC_X86_REG_FPCW,cw);g.reg_write(reg.UC_X86_REG_MXCSR,mx)
                g.emu_start(target,STOP,count=100000);assert g.reg_read(reg.UC_X86_REG_EIP)==STOP
                returns.append(f'{g.reg_read(reg.UC_X86_REG_EAX):08x}')
            regions=[dict(address=f'{s:08x}',before=b.hex(),after=bytes(g.mem_read(s,len(b))).hex())for s,b in backing]
            if name=='actual_owned_input':
                assert returns==[original['return_eax']]and stores==original['stores']
                for source,actual in zip(original['regions_after'],regions,strict=True):assert source['raw_hex']==actual['after']
            cases.append(dict(image=side+1,entry=f'{entry:08x}',name=name,argument=f'{arg:08x}',object=f'{object_:08x}',
                slots=[f'{slotmap[s]:08x}'for s in SLOTS],tick_scale_bits=scale.hex(),x87_control_word=f'{cw:04x}',mxcsr=f'{mx:08x}',
                returns=returns,regions=regions,stores=stores,
                declared_input_changes=[dict(address=f'{slotmap.get(p,p):08x}',bytes=n,value=f'{v:08x}')for p,n,v in changes]))
    report=dict(schema='physics-component-construction-controls-v1',oracle_sha256=sha(Path(__file__)),capture_sha256=sha(a.capture),paired_report_sha256=sha(a.pair),
        cases=cases,instruction_patches=[],supplied_callee_results=[],private_x86_abi_stack_excluded=True,
        scope='Whole original allocator/component/init on actual old retained input and declared pool/clock/FP controls; u5 executes those direct ABI inputs, not full u5 startup/world. No solver, activation, final Unit restoration or live collision.')
    save(out/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(out/'report.json'),cases=len(cases),calls=sum(len(c['returns'])for c in cases),stores=sum(len(c['stores'])for c in cases))))

if __name__=='__main__':main()
