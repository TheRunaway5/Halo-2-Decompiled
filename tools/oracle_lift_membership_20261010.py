#!/usr/bin/env python3
"""Execute original phantom membership, with declared contact/storage services.

No overlap, body/world construction or lift force result is supplied or proven.
"""
import argparse, hashlib, itertools, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_lift_contact_20261010 import (BASE, POOL, COMPONENTS, OBJECTPOOL,
    HEADERS, OWN, OTHER, TAGS, DEFINITION, MODEL, PHMO, SHAPES, RULES,
    STACK, STOP, ROOT, Xbe, PIN, NEW_DEFAULT, NEW_PIN)

ENTITY=BASE+0xc000; PROPERTIES=ENTITY+0x100; DESC=BASE+0xe100; ARRAY=BASE+0xe200
OWN_COMPONENT=COMPONENTS; OWN_DATUM=0xabcd0000; OTHER_DATUM=0xbcde0001
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    cases=[]
    # Source integer/identity edge inputs, not a claimed collision producer.
    configurations=itertools.product((0,0x1000000,0x20),(0,1,11),(-1,0,127,128),(0,255))
    for flags,kind,body,refs in configurations:
        cases.append(dict(flags=flags,kind=kind,body=body,refs=refs,other=OTHER_DATUM,
                          owner=0xffffffff,operations=['add','add','remove','remove']))
    cases += [dict(flags=0,kind=0,body=0,refs=0,other=other,owner=owner,operations=ops)
              for other,owner,ops in [(0xffffffff,0xffffffff,['add','remove']),
                (OWN_DATUM,0xffffffff,['add','remove']),
                (OTHER_DATUM,0x12345678,['remove']),
                (OTHER_DATUM,0xffffffff,['remove'])]]
    banks=[]
    for label,path,pin,entries,slots,grow,allocator in [
        ('1.1',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',PIN,
         (0x1d5830,0x1d5a60),(0x526248,0x4e78d0,0x4eb114),0x2df780,0x487638),
        ('u5',NEW_DEFAULT,NEW_PIN,(0x1da0f0,0x1da320),
         (0x52be5c,0x4ecbd0,0x4f0414),0x2e3250,0x48c938)]:
        assert sha(path)==pin
        x=Xbe(path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
        for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
        g.mem_map(BASE,0x10000)
        alloc_entry=STOP+0x100;free_entry=STOP+0x200;vtable=STOP+0x300;manager=STOP+0x400
        def w(p,v,n=4):g.mem_write(p,int(v&((1<<(n*8))-1)).to_bytes(n,'little'))
        def r(p):return int.from_bytes(g.mem_read(p,4),'little')
        def ret(clean,value=0):
            esp=g.reg_read(R.UC_X86_REG_ESP);target=r(esp)
            g.reg_write(R.UC_X86_REG_ESP,esp+4+clean)
            g.reg_write(R.UC_X86_REG_EAX,value);g.reg_write(R.UC_X86_REG_EIP,target)
        stores=[];events=[];active=False;external=False
        def hook_code(_g,p,_size,_data):
            nonlocal external
            if p not in (alloc_entry,free_entry,grow):return
            external=True
            esp=g.reg_read(R.UC_X86_REG_ESP)
            if p==alloc_entry:
                events.append(['allocate',r(esp+4),r(esp+8),DESC]);ret(8,DESC)
            elif p==free_entry:
                events.append(['release',r(esp+4),r(esp+8),r(esp+12)]);ret(12)
            else:
                descriptor=r(esp+4);stride=r(esp+8)
                events.append(['grow',descriptor,stride]);w(descriptor,ARRAY);w(descriptor+8,8);ret(0)
            external=False
        def hook_write(_g,_access,p,n,v,_data):
            if active and not external and BASE<=p<BASE+0x10000 and not STACK-0x400<=p<STACK+0x100:
                stores.append([p,n,v&((1<<(n*8))-1)])
        g.hook_add(UC_HOOK_CODE,hook_code);g.hook_add(UC_HOOK_MEM_WRITE,hook_write)
        results=[]
        for config in cases:
            g.mem_write(BASE,bytes(0x10000));setup=[]
            def setup_w(p,v,n=4):w(p,v,n);setup.append([p,n,v&((1<<(8*n))-1)])
            for p,v in ((slots[0],POOL),(slots[1],OBJECTPOOL),(slots[2],TAGS),
                (POOL+0x44,COMPONENTS),(OBJECTPOOL+0x44,HEADERS),
                (COMPONENTS+8,0x12340000),(COMPONENTS+0xa8,0x12350001),
                (HEADERS+8,OWN),(HEADERS+20,OTHER),(OWN,0x99990000),
                (TAGS+8,DEFINITION),(TAGS+24,MODEL),(TAGS+40,PHMO),
                (DEFINITION+0x38,0x99990001),(MODEL+0x24,0x99990002),
                (PHMO+0x44,SHAPES),(PHMO+0x2c,RULES),(SHAPES+8,0),
                (OTHER+0x13c,0xffffffff),(OTHER+0x3e4,config['owner']),
                (RULES,config['flags']),(ENTITY+0x30,PROPERTIES),(ENTITY+0x34,3),
                (PROPERTIES,0x7777),(PROPERTIES+8,0x2002),(PROPERTIES+12,config['body']),
                (PROPERTIES+16,0x2001),(PROPERTIES+20,config['other']),
                (allocator,manager),(manager,vtable),(vtable+0x10,alloc_entry),(vtable+0x14,free_entry)):
                setup_w(p,v)
            setup_w(RULES+4,0,1);setup_w(RULES+5,6,1);setup_w(HEADERS+15,config['kind'],1)
            if config['refs']:
                for p,v in ((OWN_COMPONENT+0x94,DESC),(DESC,ARRAY),(DESC+4,2),(DESC+8,8),
                            (ARRAY,0),(ARRAY+4,0x12350001),(ARRAY+8,0),(ARRAY+12,0x87654321)):
                    setup_w(p,v)
                setup_w(ARRAY+1,config['body'],1);setup_w(ARRAY+2,config['refs'],1)
            stores=[];events=[];operation_results=[]
            for operation in config['operations']:
                active=False;w(STACK,STOP)
                args=[OWN_COMPONENT,0,0] if operation=='add' else [OWN_COMPONENT,OWN_DATUM,0,0]
                for i,arg in enumerate(args):w(STACK+4+i*4,arg)
                for register,value in ((R.UC_X86_REG_ESP,STACK),(R.UC_X86_REG_EAX,ENTITY),
                    (R.UC_X86_REG_ECX,OWN_DATUM),(R.UC_X86_REG_EDX,0),(R.UC_X86_REG_EFLAGS,2)):
                    g.reg_write(register,value)
                start=len(stores);event_start=len(events);active=True
                g.emu_start(entries[operation=='remove'],STOP,count=10000)
                active=False;assert g.reg_read(R.UC_X86_REG_EIP)==STOP
                operation_results.append(dict(operation=operation,stores=stores[start:],events=events[event_start:]))
            results.append(dict(configuration=config,setup=setup,operations=operation_results))
        banks.append(dict(image=label,pin=pin,cases=results))
    # Paired source operations must agree after image globals are excluded.
    assert [v['operations']for v in banks[0]['cases']]==[v['operations']for v in banks[1]['cases']]
    p=a.output/'fixture.json';p.write_text(json.dumps(dict(schema=1,base=BASE,size=0x10000,banks=banks),indent=2,sort_keys=True)+'\n')
    report=dict(tool_sha256=sha(__file__),fixture_sha256=sha(p),cases=len(cases)*2,
                direct_operations=sum(len(c['operations'])for c in cases)*2,
                original_memory_stores=sum(len(o['stores'])for b in banks for c in b['cases']for o in c['operations']),
                boundary=__doc__,live_world_contact_producer=False,live_lift_force=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
