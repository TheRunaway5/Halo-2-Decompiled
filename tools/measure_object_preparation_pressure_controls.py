#!/usr/bin/env python3
"""Pair complete original pressure/preparation bodies on declared ABI controls.

Natural entries are exported verbatim. Additional raw heap/flag/tick inputs and
lower reclaim outcomes are explicit kernel controls, never world readiness.
"""
import argparse,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import OLD,NEW,PINS,Xbe,sha,save,encoded,_Code,_trace

STACK,STOP=0x02008000,0x03000000
SLOTS=(0x4eb114,0x486563,0x486562,0x480d8c,0x480da4,0x518444,0x480da0,0x480da8)
OWN=(0xddac0,0x148360,0x148740,0x148510,0x22ecb0)

def put(cells,p,raw):
    for i,b in enumerate(raw):cells[p+i]=b
def regions(cells):
    out=[]
    for p in sorted(cells):
        if out and p==out[-1][0]+len(out[-1][1]):out[-1][1].append(cells[p])
        else:out.append((p,bytearray([cells[p]])))
    return out
def words(cells,p,n=4):return int.from_bytes(bytes(cells[p+i]for i in range(n)),'little')
def raw_cases(report):
    cases=[]
    for c in report['calls']:
        entry=int(c['entry'],16)
        if entry not in OWN:continue
        cells={};q=c.get('descriptor',0)
        for r in report['reads']:
            if not c['order']<=r['order']<c['return_order']:continue
            p=int(r['address'],16)
            if 0x2000000<=p<0x2200000 and not(q<=p and p+r['bytes']<=q+196):continue
            for i,b in enumerate(bytes.fromhex(r['raw_hex'])):cells.setdefault(p+i,b)
        before={};after={}
        for k in ('globals','heap_fields'):
            for p,raw in c['before'][k].items():put(cells,int(p,16),bytes.fromhex(raw))
        clock=c['before']['clock'];put(cells,int(clock['address'],16),bytes.fromhex(clock['raw12']))
        # Keep complete literal request and retained mutable globals, including
        # untouched bytes. The other large world buffers remain in the owning
        # report, rather than copying unrelated live Unit bodies into a kernel.
        for p,raw in c['before']['globals'].items():before[int(p,16)]=raw
        for p,raw in c['after']['globals'].items():after[int(p,16)]=raw
        if q:
            before[q]=c['before']['D'];after[q]=c['after']['D'];put(cells,q,bytes.fromhex(before[q]))
        stores=[]
        for r in report['stores']:
            if not c['order']<=r['order']<c['return_order']:continue
            p=int(r['address'],16)
            if not any(a<=p and p+r['bytes']<=a+len(bytes.fromhex(b))for a,b in before.items()):continue
            stores.append(dict(instruction=r['instruction'],address=r['address'],bytes=r['bytes'],before=r['before'],after=r['after']))
        cases.append(dict(name='owning_'+str(c['id'])+'_'+c['phase'],entry=c['entry'],input=f'{q if entry==0xddac0 else int(c["entry_cpu"]["registers"]["eax"],16):08x}',
            slots=[f'{p:08x}'for p in SLOTS],mxcsr=c['entry_cpu']['mxcsr'],return_mxcsr=c['return_cpu']['mxcsr'],return_eax=c['return_cpu']['registers']['eax'],
            return_xmm0=c['return_cpu']['xmm128'][0][-8:],regions=[dict(address=f'{p:08x}',before=b.hex(),after=bytes.fromhex(after[p]).hex()if p in after else b.hex())for p,b in regions(cells)],
            mutable=[dict(address=f'{p:08x}',before=b,after=after[p])for p,b in before.items()],stores=stores,lower_calls=[],boundary='Unchanged owning original entry; coupled native pressure/statistic bodies execute, no lower result replay.'))
    return cases

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--owning',type=Path,required=True);p.add_argument('--pair',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);report=json.loads(a.owning.read_text());paired=json.loads(a.pair.read_text())
    natural=raw_cases(report);base=next(c for c in natural if int(c['entry'],16)==0x148360);prep=next(c for c in natural if int(c['entry'],16)==0xddac0)
    owners=paired['owners'];targets={int(v['old_entry'],16):int(v['u5_entry'],16)for v in owners}
    slotmap={};callsmap={};pcmaps=[{},{}];allowed=[set(),set()]
    for owner in owners:
        for m in owner['image_memory_operands']:
            old,new=int(m['old_va'],16),int(m['u5_va'],16)
            if old in slotmap:assert slotmap[old]==new
            slotmap[old]=new
        for c in owner['direct_calls']:callsmap[int(c['old_target'],16)]=int(c['u5_target'],16)
        rows=[(a.pair.parent/'instructions'/f'{owner["old_entry"]}-{side}.asm').read_text().splitlines()for side in ('old','u5')]
        for left,right in zip(*rows,strict=True):
            old,new=int(left.split()[0],16),int(right.split()[0],16)
            for side,pc in enumerate((old,new)):pcmaps[side][pc]=old;allowed[side].add(pc)
    # Global+adjacent-byte mappings are derived from actual complete operands.
    def relocate(p,side):
        if not side:return p
        for old,new in slotmap.items():
            if old<=p<old+4:return new+p-old
        return p
    def initial(template,side):
        cells={}
        for r in template['regions']:
            p=int(r['address'],16)
            for i,b in enumerate(bytes.fromhex(r['before'])):cells[relocate(p+i,side)]=b
        return cells
    canonical={}
    for r in base['regions']:put(canonical,int(r['address'],16),bytes.fromhex(r['before']))
    pool=words(canonical,0x480da0);first=words(canonical,0x480da4)
    specs=[]
    # No new gameplay fields: first heap statistic triple and actual counters
    # are deliberately varied solely to cover complete query widths/branches.
    for value in (0,1,600000,700000,800000,900000,0x7fffffff,0x80000000,0xffffffff):
        for mx in (0x1f80,0x3f80,0x5f80,0x7f80,0x1fc0):
            specs.append((0x22ecb0,f'stat_{value:08x}_{mx:04x}',[(pool+0xf0384,4,value),(pool+0xf03c8,4,0),(pool+0xa8,4,1)],mx,None))
            specs.append((0x148740,f'query_{value:08x}_{mx:04x}',[(pool+0xf0384,4,value),(pool+0xf03c8,4,0),(pool+0xa8,4,1)],mx,None))
    for v in(0,0x7fffffff,0x80000000,0xffffffff):
        specs.append((0x148740,f'main_count_{v:08x}',[(pool+0xf0340,4,v)],0x1f80,None))
        specs.append((0x148740,f'overflow_count_{v:08x}',[(first+0x20,4,v)],0x1f80,None))
    specs.append((0x148740,'no_main',[(0x480da0,4,0)],0x1f80,None))
    specs.append((0x22ecb0,'wrapped_product',[(pool+0xf0384,4,0xffffffff),(pool+0xf03c8,4,1),(pool+0xa8,4,0x80000001)],0x1f80,None))
    for busy in(1,255):specs.append((0x148360,f'busy_{busy}',[(0x486563,1,busy)],0x1f80,dict(pressure=[3],reclaim=False,drop=False)))
    for last,tick in((0xffffffff,0),(0,0),(0,9),(0,10),(0xfffffff8,2),(2,0xfffffff8)):
        specs.append((0x148360,f'failed_reclaim_last_{last:08x}_tick_{tick:08x}',[(0x480d8c,4,last),(0x1016008,4,tick)],0x1f80,dict(pressure=[2],reclaim=False,drop=False)))
    for pressure,reclaim,drop,name in(([3],False,False,'high_failed'),([2,2,1],True,False,'reclaim_clears_pressure'),([2],True,False,'full_retry_bound'),([2],True,True,'overflow_disappears')):
        specs.append((0x148360,name,[],0x1f80,dict(pressure=pressure,reclaim=reclaim,drop=drop)))
    for had,drop in((False,False),(True,False),(True,True)):
        specs.append((0x148510,f'collect_had_{had}_drop_{drop}',[(0x480da4,4,first if had else 0)],0x1f80,dict(pressure=[],reclaim=False,drop=drop)))
    # Execute the actual positive preparation branch on both unchanged images.
    for c in natural:
        if int(c['entry'],16)==0xddac0:
            specs.append((0xddac0,'paired_'+c['name'],[],int(c['mxcsr'],16),c))
    cases=[];lower_abi=[]
    for side,path in enumerate((OLD,NEW)):
        assert sha(path)==PINS[str(path)];image=Xbe(path);code_image=_Code(image,'.text')
        for old,pop in ((0x1c7970,12),(0x1c7ec0,4)):
            entry=callsmap[old]if side else old;fn=_trace(code_image,entry,{entry});ret=[]
            for pc in sorted(fn.code):
                ins=code_image.at(pc)
                if ins.mnemonic=='ret':
                    assert ins.op_str==hex(pop)or ins.op_str==str(pop)
                    ret.append(dict(address=f'{pc:08x}',raw_hex=ins.bytes.hex()))
            assert ret;lower_abi.append(dict(image=side+1,canonical_entry=f'{old:08x}',entry=f'{entry:08x}',callee_stack_pop=pop,original_return_instructions=ret))
        for entry,name,changes,mx,control in specs:
            template=control if entry==0xddac0 else base;cells=initial(template,side)
            for addr,n,value in changes:put(cells,relocate(addr,side),value.to_bytes(n,'little'))
            if entry==0xddac0:input_value=int(template['input'],16)
            else:input_value=pool if entry==0x22ecb0 else 0
            g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
            for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
            pages=set(range(0x10000,0x810000,0x1000))
            for addr,n in [(STACK-0x1000,0x2000),(STOP,0x1000)]+[(p,len(b))for p,b in regions(cells)]:
                for page in range(addr&~4095,(addr+n+4095)&~4095,4096):
                    if page not in pages:g.mem_map(page,4096);pages.add(page)
            for addr,raw in regions(cells):g.mem_write(addr,bytes(raw))
            g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_EAX,input_value);g.reg_write(UC_X86_REG_MXCSR,mx)
            g.mem_write(STACK,struct.pack('<II',STOP,input_value));writes=[];lower=[];query_index=0
            replay=control if entry in(0x148360,0x148510)else None
            boundary_targets={callsmap.get(x,x)if side else x:x for x in(0x1c6100,0x1c5ff0,0x1482f0,0x1c7ec0,0x1c7970)}
            if replay:boundary_targets[targets[0x148740]if side else 0x148740]=0x148740
            def memory_before():return [dict(address=f'{p:08x}',raw=bytes(g.mem_read(p,len(b))).hex())for p,b in regions(cells)]
            def code(_g,address,n,user):
                nonlocal query_index
                if replay and address in boundary_targets:
                    target=boundary_targets[address];esp=g.reg_read(UC_X86_REG_ESP);ret=int.from_bytes(g.mem_read(esp,4),'little')
                    pc=0x148542 if ret==STOP and target==0x1482f0 else pcmaps[side].get(ret-5)
                    assert pc is not None,(name,hex(ret),hex(target))
                    argc=3 if target==0x1c7970 else 1 if target==0x1c7ec0 else 0
                    args=[int.from_bytes(g.mem_read(esp+4+i*4,4),'little')for i in range(argc)]
                    before=memory_before();eax=0
                    if target==0x148740:
                        values=replay['pressure'];eax=values[min(query_index,len(values)-1)];query_index+=1
                    elif target==0x1c7970:eax=int(replay['reclaim'])
                    elif target==0x1c5ff0 and replay['drop']:g.mem_write(relocate(0x480da4,side),bytes(4))
                    lower.append(dict(instruction=f'{pc:08x}',entry=f'{target:08x}',ecx=f'{g.reg_read(UC_X86_REG_ECX):08x}',al=f'{g.reg_read(UC_X86_REG_EAX)&255:02x}',arguments=[f'{v:08x}'for v in args],return_eax=f'{eax:08x}',before=before,after=memory_before()))
                    g.reg_write(UC_X86_REG_EAX,eax);g.reg_write(UC_X86_REG_ESP,esp+4+argc*4);g.reg_write(UC_X86_REG_EIP,ret);return
                assert address in allowed[side],('Unexpected unimplemented owner',name,hex(address))
            def write(_g,kind,address,n,value,user):
                if STACK-0x1000<=address<STACK+0x1000:return
                assert all(address+i in cells for i in range(n)),(name,hex(address),n)
                writes.append(dict(instruction=f'{pcmaps[side][g.reg_read(UC_X86_REG_EIP)]:08x}',address=f'{address:08x}',bytes=n,before=bytes(g.mem_read(address,n)).hex(),after=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
            g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,write)
            g.emu_start(targets[entry]if side else entry,STOP,count=2000000);assert g.reg_read(UC_X86_REG_EIP)==STOP
            # Image-backed constants are imported as literal inputs, not C++
            # payload. The addresses come from complete mapped operands.
            operands=[]
            for old in (0x4650a4,0x451ff8,0x4650a0,0x46509c,0x465098,0x465094):operands.append(image.read(relocate(old,side),4).hex())
            output=[dict(address=f'{p:08x}',before=b.hex(),after=bytes(g.mem_read(p,len(b))).hex())for p,b in regions(cells)]
            cases.append(dict(image=side+1,name=name,entry=f'{entry:08x}',input=f'{input_value:08x}',slots=[f'{relocate(p,side):08x}'for p in SLOTS],mxcsr=f'{mx:08x}',return_mxcsr=f'{g.reg_read(UC_X86_REG_MXCSR):08x}',return_eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',return_xmm0=f'{g.reg_read(UC_X86_REG_XMM0)&0xffffffff:08x}',operands=operands,regions=output,mutable=output,stores=writes,lower_calls=lower,
                boundary='Declared kernel raw fields and, for collection controls, explicit test-only pressure/reclaim/world outcomes. No live allocation, collection or readiness claim.'))
    half=len(cases)//2
    for old,new in zip(cases[:half],cases[half:],strict=True):
        assert old['name']==new['name']and old['return_mxcsr']==new['return_mxcsr']
        if old['entry']=='00148740':assert old['return_eax']==new['return_eax']
        if old['entry']=='0022ecb0':assert old['return_xmm0']==new['return_xmm0']
        assert [r['after']for r in old['regions']]==[r['after']for r in new['regions']]
        assert [(r['instruction'],r['bytes'],r['before'],r['after'])for r in old['stores']]==[(r['instruction'],r['bytes'],r['before'],r['after'])for r in new['stores']]
    result=dict(lower_abi=lower_abi,paired_retained_outputs_equal=True,source_arithmetic_status_boundary='Unicorn preserves literal observed MXCSR but does not reliably set SSE sticky exception bits. Native result bits and control bits are compared; real x86 operations retain their actual sticky status and restore the host environment. No Xbox hardware sticky-status equivalence is claimed.',schema='object-preparation-pressure-controls-v1',tool_sha256=sha(__file__),owning_sha256=sha(a.owning),paired_sha256=sha(a.pair),natural=natural,cases=cases,instruction_patches=[],production_unit_copies=[],scope='Original complete-body controls plus unchanged natural owning entries; all test-only lower callback outputs are separately retained.')
    save(out/'report.json',encoded(result));print(json.dumps(dict(report_sha256=sha(out/'report.json'),natural=len(natural),cases=len(cases),stores=sum(len(c['stores'])for c in cases),callbacks=sum(len(c['lower_calls'])for c in cases))))
if __name__=='__main__':main()
