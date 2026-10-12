#!/usr/bin/env python3
"""Run original signed dispatch prefixes to their actual required lower call."""
import argparse,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as reg
from oracle_mp_title_update_display import OLD,NEW,PINS,Xbe,_Code,sha,save,encoded

STACK,STOP=0x02008000,0x03000000

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--pair',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    capture=json.loads(a.capture.read_text());paired=json.loads(a.pair.read_text())
    regions=[(int(r['address'],16),bytes.fromhex(r['raw_hex']))for r in capture['states'][capture['initial_state']]['regions']]
    def raw(address,n):return next(b[address-s:address-s+n]for s,b in regions if s<=address and address+n<=s+len(b))
    def word(address):return int.from_bytes(raw(address,4),'little')
    pool=word(0x526248);objects=word(0x4e78d0);datum=int(capture['root']['datum'],16)
    component=word(pool+0x44)+(datum&65535)*160;obj=word(component+8);record=word(objects+0x44)+(obj&65535)*12;body=word(record+8)
    mapping={int(o['old'],16):o for o in paired['owners']};cases=[];packets=[]
    for side,path in enumerate((OLD,NEW)):
        assert sha(path)==PINS[str(path)];image=Xbe(path);code=_Code(image,'.text');pcmap={};allowed=set()
        rebuild=mapping[0x1d2b10];entry=int(rebuild['old'if side==0 else'u5'],16)
        remap=next(v.mem.disp for v in code.at(entry+0x3f).operands if v.type==3)
        table=next(v.mem.disp for v in code.at(entry+0x4c).operands if v.type==3)
        table_bytes=image.read(table,24);type_bytes=image.read(remap,13)
        labels=[int.from_bytes(table_bytes[i:i+4],'little')for i in range(0,20,4)]
        assert type_bytes.hex()=='00010505050505020505050304'and table_bytes[20:]==bytes(4)
        # Full branch labels and their actual instruction bytes, not a native
        # object-type enum. This also authenticates the zero sixth destination.
        for old_offset,label in zip((0x5a,0x86,0x6a,0x53,0x63),labels,strict=True):assert label==entry+old_offset
        packets.append(dict(image=side+1,path=str(path),sha256=sha(path),remap=f'{remap:08x}',table=f'{table:08x}',
            remap_hex=type_bytes.hex(),table_hex=table_bytes.hex(),labels=[f'{v:08x}'for v in labels]))
        slots={}
        for owner in paired['owners'][:2]:
            lines=[(a.pair.parent/'instructions'/f'{owner["old"]}-{tag}.asm').read_text().splitlines()for tag in('old','u5')]
            for old,new in zip(*lines,strict=True):
                current=old if side==0 else new;pc=int(current.split()[0],16)
                assert image.read(pc,len(current.split()[1])//2).hex()==current.split()[1]
                allowed.add(pc);pcmap[pc]=int(old.split()[0],16)
            for m in owner['memory']:slots[int(m['old_va'],16)]=int(m['old_va'if side==0 else'new_va'],16)
        bounds={int(o['old'if side==0 else'u5'],16):int(o['old'],16)for o in paired['owners'][2:]}
        for typ in(0,1,7,11,12,255):
            for active in(0,1,255):
                for flags in(0,0x20000,0xffffffff):
                    # Small exact retained regions needed by these prefixes.
                    before=[(slots[0x526248],struct.pack('<I',pool)),(slots[0x4e78d0],struct.pack('<I',objects)),
                        (pool,raw(pool,0x4c)),(objects,raw(objects,0x4c)),(component,raw(component,160)),
                        (record,raw(record,12)),(body,raw(body,0x382)),(remap-128,image.read(remap-128,256))]
                    before=[(p,bytearray(b))for p,b in before]
                    def set_word(address,n,value):
                        for p,b in before:
                            if p<=address and address+n<=p+len(b):b[address-p:address-p+n]=value.to_bytes(n,'little');return
                        raise AssertionError(hex(address))
                    set_word(body+0xaa,1,typ);set_word(record+2,1,active);set_word(component+4,4,flags)
                    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
                    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
                    pages=set(range(0x10000,0x810000,4096))
                    for address,n in[(STACK-4096,8192),(STOP,4096)]+[(p,len(b))for p,b in before]:
                        for page in range(address&~4095,(address+n+4095)&~4095,4096):
                            if page not in pages:g.mem_map(page,4096);pages.add(page)
                    for address,b in before:g.mem_write(address,bytes(b))
                    g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(reg.UC_X86_REG_ESP,STACK);g.reg_write(reg.UC_X86_REG_EAX,datum)
                    lower=[];stores=[]
                    def hook(_g,pc,n,user):
                        if pc in bounds:
                            target=bounds[pc];esp=g.reg_read(reg.UC_X86_REG_ESP)
                            ret=int.from_bytes(g.mem_read(esp,4),'little');site=pcmap[ret-5]
                            count=4 if target==0x1d9330 else 0
                            lower.append(dict(instruction=f'{site:08x}',entry=f'{target:08x}',component=f'{component:08x}',
                                object='00000000',linear='00000000',angular='00000000',arguments=[f'{int.from_bytes(g.mem_read(esp+4+i*4,4),"little"):08x}'for i in range(count)]))
                            g.emu_stop();return
                        assert pc in allowed,('Unexpected branch',typ,hex(pc))
                    def store(_g,kind,address,n,value,user):
                        if STACK-4096<=address<STACK+4096:return
                        assert n in(1,4)and any(s<=address and address+n<=s+len(b)for s,b in before)
                        stores.append(dict(instruction=f'{pcmap[g.reg_read(reg.UC_X86_REG_EIP)]:08x}',address=f'{address:08x}',bytes=n,
                            before_hex=bytes(g.mem_read(address,n)).hex(),after_hex=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
                    g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,store);g.emu_start(entry,STOP,count=1000)
                    assert len(lower)==1
                    cases.append(dict(image=side+1,datum=f'{datum:08x}',type=typ,header_flags=active,component_flags=f'{flags:08x}',
                        slots=[f'{slots[0x526248]:08x}',f'{slots[0x4e78d0]:08x}',f'{remap:08x}',f'{table:08x}'],branches=packets[-1]['labels'],
                        required_lower=lower[0],stores=stores,regions=[dict(address=f'{p:08x}',before=b.hex(),after=bytes(g.mem_read(p,len(b))).hex())for p,b in before]))
    save(out/'operands.json',encoded(dict(schema=1,images=packets,scope='Verified indexed table and branch labels only; no mutable component/world state.')))
    result=dict(schema='physics-component-rebuild-prefix-controls-v1',oracle_sha256=sha(Path(__file__)),input_pins={str(p):sha(p)for p in(a.capture,a.pair)},
        cases=cases,instruction_patches=[],supplied_callee_results=[],
        scope='Original1.1/u5 declared dispatch/header/flag input prefixes. Each stops before required type/grouping owner; no successful lower response or native world completion.')
    save(out/'report.json',encoded(result));print(json.dumps(dict(report_sha256=sha(out/'report.json'),cases=len(cases),stores=sum(len(c['stores'])for c in cases))))

if __name__=='__main__':main()
