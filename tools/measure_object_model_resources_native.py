#!/usr/bin/env python3
"""Execute whole original C0890/C19D0 on retained owning query inputs."""
import argparse,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_mp_title_update_display import OLD,NEW,PINS,Xbe,sha,save,encoded
from freeze_mp_native_sound_storage import ROOT
PAIR=ROOT/'analysis/multiplayer/integration/spawn-source/unit-pose-requests/body-next-u5'
STACK,STOP=0x02008000,0x03000000

def backing(capture):
    cells={}
    for address,text in capture['regions_before'].items():
        for offset,value in enumerate(bytes.fromhex(text)):
            p=int(address,16)+offset;assert p not in cells or cells[p]==value;cells[p]=value
    regions=[]
    for p,value in sorted(cells.items()):
        if regions and p==regions[-1][0]+len(regions[-1][1]):regions[-1][1].append(value)
        else:regions.append((p,bytearray([value])))
    return regions

def store(regions,address,value):
    for start,data in regions:
        if start<=address and address+4<=start+len(data):
            data[address-start:address-start+4]=struct.pack('<I',value);return
    raise ValueError('Declared control escapes retained field backing')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    captured=json.loads(a.capture.read_text());pair=json.loads((PAIR/'report.json').read_text())
    owner=next(q for q in pair['owners']if q['old_entry']=='000c0890')
    assert owner['old_rows']==49 and owner['full_body_matches']==['000c19d0']
    pcs={};lists=[]
    for side in ('old','u5'):
        path=PAIR/'instructions'/f'000c0890-{side}.asm';lines=path.read_text().splitlines();lists.append(lines)
        assert len(lines)==49 and sha(path)==next(q['sha256']for q in owner['listings']if q['file'].endswith(f'-{side}.asm'))
    for left,right in zip(*lists,strict=True):pcs[int(right.split()[0],16)]=int(left.split()[0],16)
    cases=[];instructions={}
    for side,path in enumerate((OLD,NEW)):
        assert sha(path)==PINS[str(path)];image=Xbe(path)
        for line in lists[side]:
            address,raw,*_=line.split();assert image.read(int(address,16),len(raw)//2).hex()==raw
        for index,c in enumerate(captured['calls']):
            info=c['details'];base=backing(c);outputs=[int(q,16)for q in c['outputs']]
            specs=[('actual_owning_input',[],outputs)]
            specs.append(('explicit_hlmt_none',[(int(info['definition_body'],16)+0x38,0xffffffff)],outputs))
            if 'model'in info:
                model=int(info['model'],16)
                specs.extend([('explicit_mode_none',[(model+4,0xffffffff)],outputs),
                              ('explicit_collision_none',[(model+0xc,0xffffffff)],outputs)])
            if 'mode_body'in info:
                mode=int(info['mode_body'],16)
                for first,second in ((0,1),(1,0),(1,1),(0xffffffff,1),(1,0xffffffff),(0x80000000,1),(1,0x80000000),(0x7fffffff,1),(1,0x7fffffff)):
                    specs.append((f'explicit_signed_counts_{first:08x}_{second:08x}',[(mode+0x74,first),(mode+0x7c,second)],outputs))
                specs.append(('explicit_aliased_outputs',[],[outputs[0],outputs[0]]))
                specs.append(('explicit_first_output_aliases_collision_reference',[],[int(info['model'],16)+0xc,outputs[1]]))
            for name,changes,output in specs:
                regions=[(start,bytearray(data))for start,data in base]
                for address,value in changes:store(regions,address,value)
                g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
                for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
                pages=set(range(0x10000,0x810000,0x1000))
                for address,length in [(STACK-0x1000,0x2000),(STOP,0x1000)]+[(s,len(b))for s,b in regions]:
                    for page in range(address&~0xfff,(address+length+0xfff)&~0xfff,0x1000):
                        if page not in pages:g.mem_map(page,0x1000);pages.add(page)
                for address,data in regions:g.mem_write(address,bytes(data))
                globals_={int(q['old_va'],16):int(q['old_va'if side==0 else'u5_va'],16)for q in owner['image_memory_operands']}
                g.mem_write(globals_[0x4e78d0],struct.pack('<I',int(c['pool'],16)))
                g.mem_write(globals_[0x4eb114],struct.pack('<I',int(c['tag_table'],16)))
                g.mem_write(STACK,struct.pack('<III',STOP,*output));g.reg_write(UC_X86_REG_ESP,STACK)
                g.reg_write(UC_X86_REG_ECX,int(c['object'],16));writes=[];executed=set()
                original_pcs={int(q.split()[0],16)for q in lists[side]}
                def code(_g,address,length,user):assert address in original_pcs,hex(address);executed.add(address)
                def write(_g,kind,address,length,value,user):
                    if STACK-0x1000<=address<STACK+0x1000:return
                    assert length==4 and any(s<=address and address+length<=s+len(b)for s,b in regions),hex(address)
                    pc=g.reg_read(UC_X86_REG_EIP);pc=pc if side==0 else pcs[pc]
                    writes.append(dict(instruction=f'{pc:08x}',address=f'{address:08x}',bytes=4,before=bytes(g.mem_read(address,4)).hex(),value=f'{value&0xffffffff:08x}'))
                g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,write)
                entry=int(owner['old_entry'if side==0 else'u5_entry'],16);g.emu_start(entry,STOP,count=1000)
                assert g.reg_read(UC_X86_REG_EIP)==STOP
                result=g.reg_read(UC_X86_REG_EAX)&255
                if name=='actual_owning_input':
                    assert result==(int(c['return_eax'],16)&255)
                    assert [bytes(g.mem_read(q,4)).hex()for q in output]==c['outputs_after']
                    # The native CPU owner does not reproduce the private ABI
                    # PUSH ESI stack slot. Its two caller-owned stores match.
                    assert writes==[q for q in c['writes']if q['instruction']!='000c08d2']
                cases.append(dict(image=side+1,owning_call=index,name=name,pool=c['pool'],tag_table=c['tag_table'],object=c['object'],
                    outputs=[f'{q:08x}'for q in output],declared_input_changes=[dict(address=f'{p:08x}',value=f'{v:08x}')for p,v in changes],
                    regions=[dict(address=f'{s:08x}',before=b.hex(),after=bytes(g.mem_read(s,len(b))).hex())for s,b in regions],
                    stores=writes,return_al=result,instructions=[f'{q:08x}'for q in sorted(executed)]))
                instructions.setdefault(side,set()).update(executed)
    report=dict(schema='object-model-resource-original-controls-v1',oracle_sha256=sha(Path(__file__)),
        capture_sha256=sha(a.capture),paired_report_sha256=sha(PAIR/'report.json'),cases=cases,
        instruction_patches=[],supplied_callee_results=[],
        scope='Whole original query on genuine retained biped/weapon input plus separately declared raw NONE/signed-count/output-alias controls; no full u5 world or positive live resource readiness claim.')
    save(out/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(out/'report.json'),cases=len(cases),
        stores=sum(len(c['stores'])for c in cases),full_output_bytes=sum(len(r['after'])//2 for c in cases for r in c['regions']))))

if __name__=='__main__':main()
