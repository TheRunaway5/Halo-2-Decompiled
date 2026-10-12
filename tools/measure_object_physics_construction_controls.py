#!/usr/bin/env python3
"""Run complete original wrapper/leaf controls on explicitly declared raw inputs."""
import argparse,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_mp_title_update_display import OLD,NEW,PINS,Xbe,sha,save,encoded

STACK,STOP=0x02008000,0x03000000
SLOTS=(0x4e78d0,0x526248,0x486560,0x526230)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--pair',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    original=json.loads(a.capture.read_text());paired=json.loads(a.pair.read_text());owners=paired['owners']
    assert original['actual_factory_identical']and not original['instruction_patches']and not original['supplied_lower_results']
    canonical={int(o['old'],16):o for o in owners};obj=int(original['root']['object'],16)
    source=original['states'][original['initial_state']]['regions']
    regions=[(int(r['address'],16),bytes.fromhex(r['raw_hex']))for r in source if r['address']not in('06000000','06200000')]
    read=lambda address,n=4:next(int.from_bytes(b[address-p:address-p+n],'little')for p,b in regions if p<=address and address+n<=p+len(b))
    pool=read(SLOTS[0]);heads=read(pool+0x44);record=heads+(obj&65535)*12;body=read(record+8);counter=read(SLOTS[3])
    specs=[]
    for typ in (0,1,2,5,7,11,12,13,31,32,33,64,255):
        specs.append((0x1c6f50,f'explicit_type_{typ}',[(SLOTS[2],1,0),(record+3,1,typ)],obj,0,0))
    specs.extend([
        (0x1c6f50,'explicit_header_rejection',[(record+2,1,read(record+2,1)|16)],obj,0,0),
        (0x1c6f50,'explicit_ancestor_invalid',[(record+4,2,65535)],obj,0,0),
        (0x1c6f50,'explicit_context_zero_mark',[(SLOTS[2],1,0),(body+0xc0,2,0x10fe)],obj,0,0)])
    for bit in(0,12,15,16,31,32,63,255):
        for request in(0,1,0x100,0xffffffff):
            specs.append((0xc0d10,f'explicit_word_bit_{bit}_{request:08x}',[],body+0xc0,bit,request))
    for flags in(0,1,0xfffe,0xffff):
        for count in(0,0x7fffffff,0xffffffff):
            specs.append((0x1c72e0,f'explicit_mark_{flags:04x}_{count:08x}',[(body+0xc0,2,flags),(counter,4,count)],obj,0,0))
    for value in(obj,0xffffffff,(obj&65535)|0x00010000,(obj&65535)|0xffff0000):
        specs.append((0xbbe70,f'explicit_root_datum_{value:08x}',[],value,0,0))
    for child in(0xffffffff,obj):
        specs.append((0x1c7050,f'explicit_existing_component_child_{child:08x}',[(body+0xb4,4,0),(body+0x10,4,child)],obj,obj,0))
    cases=[]
    for side,path in enumerate((OLD,NEW)):
        assert sha(path)==PINS[str(path)];image=Xbe(path);pcmap={};allowed=set();slotmap={}
        for owner in owners:
            lines=[]
            for tag in('old','u5'):
                file=a.pair.parent/'instructions'/f'{owner["old"]}-{tag}.asm';lines.append(file.read_text().splitlines())
            for left,right in zip(*lines,strict=True):
                oldpc=int(left.split()[0],16);newpc=int(right.split()[0],16)
                pc=oldpc if side==0 else newpc;raw=(left if side==0 else right).split()[1]
                assert image.read(pc,len(raw)//2).hex()==raw;pcmap[pc]=oldpc
                if int(owner['old'],16)in(0x1c7050,0x1c6f50,0x1c72e0,0xbbe70,0xc0d10):allowed.add(pc)
            for m in owner['memory']:
                old=int(m['old_va'],16);new=int(m['old_va'if side==0 else'new_va'],16)
                if old in slotmap:assert slotmap[old]==new
                slotmap[old]=new
        for entry,name,changes,first,second,request in specs:
            backing=[(slotmap.get(p,p),bytearray(b))for p,b in regions]
            def write(address,n,value):
                address=slotmap.get(address,address)
                for start,raw in backing:
                    if start<=address and address+n<=start+len(raw):raw[address-start:address-start+n]=value.to_bytes(n,'little');return
                raise AssertionError(('Declared control escaped input',hex(address)))
            for address,n,value in changes:write(address,n,value)
            g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
            for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
            pages=set(range(0x10000,0x810000,0x1000))
            for address,n in[(STACK-0x1000,0x2000),(STOP,0x1000)]+[(s,len(b))for s,b in backing]:
                for page in range(address&~4095,(address+n+4095)&~4095,4096):
                    if page not in pages:g.mem_map(page,4096);pages.add(page)
            for address,raw in backing:g.mem_write(address,bytes(raw))
            g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_EAX,first);g.reg_write(UC_X86_REG_ECX,second if entry==0xc0d10 else first)
            args=[first]if entry==0x1c6f50 else[first,second]if entry==0x1c7050 else[request]if entry==0xc0d10 else[]
            g.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));stores=[]
            def code(_g,address,n,user):assert address in allowed,('Unexpected lower owner',name,hex(address))
            def store(_g,kind,address,n,value,user):
                if STACK-0x1000<=address<STACK+0x1000:return
                assert n in(1,2,4)and any(s<=address and address+n<=s+len(b)for s,b in backing)
                stores.append(dict(instruction=f'{pcmap[g.reg_read(UC_X86_REG_EIP)]:08x}',address=f'{address:08x}',bytes=n,
                    before_hex=bytes(g.mem_read(address,n)).hex(),after_hex=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
            g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,store)
            target=int(canonical[entry]['old'if side==0 else'u5'],16);g.emu_start(target,STOP,count=10000)
            assert g.reg_read(UC_X86_REG_EIP)==STOP
            cases.append(dict(image=side+1,name=name,entry=f'{entry:08x}',first=f'{first:08x}',second=f'{second:08x}',request=f'{request:08x}',
                state_slots=[f'{slotmap[q]:08x}'for q in SLOTS],return_eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',
                declared_raw_input_changes=[dict(address=f'{slotmap.get(p,p):08x}',bytes=n,value=f'{v:08x}')for p,n,v in changes],
                regions=[dict(address=f'{s:08x}',before=b.hex(),after=bytes(g.mem_read(s,len(b))).hex())for s,b in backing],stores=stores))
    report=dict(schema='object-physics-construction-control-v1',oracle_sha256=sha(Path(__file__)),capture_sha256=sha(a.capture),paired_report_sha256=sha(a.pair),
        cases=cases,instruction_patches=[],supplied_lower_results=[],
        scope='Whole original leaf and upper-owner controls with explicit raw type/flag/counter/bit/datum inputs; no required lower calls are reached and no positive live physics construction is implied.')
    save(out/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(out/'report.json'),cases=len(cases),stores=sum(len(c['stores'])for c in cases))))

if __name__=='__main__':main()
