#!/usr/bin/env python3
"""Offline original processor graph/program inventory; no runtime program export.

Execute the pure original38F544 selector, authenticate complete x86 bodies,
and compare whole identified program extents with official vendor ELF data.
Byte matches identify research leads only, never native DSP/effect readiness.
"""
import argparse,hashlib,importlib.util,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

ROOT=Path(__file__).resolve().parents[1]
ENTRIES=[0x38e7d5,0x38e1a1,0x38e2e0,0x388275,0x388294,0x38f6f4,
         0x38f544,0x38f5e2,0x38f5f3,0x38f5ef,0x38f66a,0x38f692,
         0x38dd0c,0x38dd54,0x38de59,0x38e6df,0x38de07,0x38850a,0x38851e]
GP=[getattr(R,'UC_X86_REG_'+n)for n in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']]
def selector(image,index):
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x380000,0x10000);cpu.mem_map(0x2000000,0x2000)
    cpu.mem_write(0x38f000,image.read(0x38f000,4096));cpu.mem_write(0x2000000,bytes([0xa5])*0x2000)
    gp=[0x41000000+i*0x111111 for i in range(8)];gp[4]=0x2001000
    for reg,value in zip(GP,gp):cpu.reg_write(reg,value)
    cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202);cpu.mem_write(gp[4],struct.pack('<4I',0x2001f00,index,0x2000100,0x2000104))
    instructions=[];stores=[]
    def hook(m,pc,size,_):assert bytes(m.mem_read(pc,size))==image.read(pc,size);instructions.append(pc)
    def store(m,kind,address,size,value,_):stores.append([m.reg_read(R.UC_X86_REG_EIP),address,size,value&((1<<(size*8))-1)])
    cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store);cpu.emu_start(0x38f544,0x2001f00,count=1000)
    assert cpu.reg_read(R.UC_X86_REG_EIP)==0x2001f00 and cpu.reg_read(R.UC_X86_REG_ESP)==gp[4]+16
    pointer,words=struct.unpack('<2I',cpu.mem_read(0x2000100,8));return dict(index=index,pointer=pointer,storage_words=words,storage_bytes=words*4,instructions=instructions,source_stores=stores,returned_gp=[cpu.reg_read(r)for r in GP],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'DSOUND');a.output.mkdir(parents=True)
    bodies={entry:_trace(code,entry,set(ENTRIES))for entry in ENTRIES};rows=[];instruction_bytes={}
    for entry,body in bodies.items():
        listing=[];calls=[];absolute_reads=[];absolute_writes=[]
        for pc in sorted(body.code):
            op=code.at(pc);raw=image.read(pc,op.size);assert raw==bytes(op.bytes);instruction_bytes[f'{pc:08x}']=raw.hex()
            listing.append(f'{pc:08x} {raw.hex()} {op.mnemonic} {op.op_str}')
            if op.mnemonic=='call'and op.operands:
                arg=op.operands[0]
                if arg.type==2:calls.append(dict(instruction=pc,target=arg.imm&0xffffffff))
                elif arg.type==3 and not arg.mem.base and not arg.mem.index:calls.append(dict(instruction=pc,import_slot=arg.mem.disp&0xffffffff,import_target=struct.unpack('<I',image.read(arg.mem.disp&0xffffffff,4))[0]))
            for i,arg in enumerate(op.operands):
                if arg.type==3 and not arg.mem.base and not arg.mem.index:
                    (absolute_writes if i==0 and op.mnemonic in ['mov','and','or','add','sub']else absolute_reads).append(dict(instruction=pc,address=arg.mem.disp&0xffffffff,bytes=arg.size,operation=op.mnemonic))
        name=f'original-{entry:08x}.asm';(a.output/name).write_text('\n'.join(listing)+'\n')
        rows.append(dict(entry=entry,sites=len(body.code),calls=calls,absolute_reads=absolute_reads,absolute_writes=absolute_writes,listing=name,listing_sha256=hashlib.sha256((a.output/name).read_bytes()).hexdigest()))
    selections=[selector(image,i)for i in range(8)];assert all(not row['pointer']and not row['storage_words']for row in selections[6:])
    expected=[(0x574180,0x2f2),(0x574d48,0x862),(0x576ed0,0x652),(0x578818,0x6d3),(0x57a368,0xbc),(0x57a658,0xc9)]
    assert [(r['pointer'],r['storage_words'])for r in selections[:6]]==expected
    spec=importlib.util.spec_from_file_location('existing_program_inventory',ROOT/'analysis/asset-setup/mcc-native-20261015/native-audio-program-reference/inventory.py')
    vendor_module=importlib.util.module_from_spec(spec);spec.loader.exec_module(vendor_module)
    vendor=vendor_module.VENDOR;raw=vendor.read_bytes();sections,symbols=vendor_module.object_sections(raw)
    data_sections=[s for s in sections if s['type']==1 and s['flags']&2 and not s['flags']&4 and s['bytes']]
    blocks=[dict(name='gp_initial_390C18',source=0x390c18,storage_bytes=0x5cc,source_copy_call=0x388301,source_destination_byte_offset=0),dict(name='ep_initial_573860',source=0x573860,storage_bytes=0x320,source_copy_call=0x38f741,source_destination_byte_offset=0),dict(name='ep_loader_573B80',source=0x573b80,storage_bytes=0x5fc,source_copy_call=0x38f762,source_destination_byte_offset=0x600)]
    cursor=0x300
    for r in selections[:6]:
        blocks.append(dict(name=f'ep_selected_{r["index"]}',source=r['pointer'],storage_bytes=r['storage_bytes'],source_copy_call=0x38f79a,source_destination_byte_offset=cursor*4));cursor+=r['storage_words']
    blocks.append(dict(name='ep_config_57A980',source=0x57a980,storage_bytes=0x60,source_copy_call=0x38f7cb,source_destination_byte_offset=cursor*4))
    for block in blocks:
        data=image.read(block['source'],block['storage_bytes']);words=struct.unpack('<'+'I'*(len(data)//4),data)
        matches=[];named_runs=[]
        for section in data_sections:
            content=raw[section['offset']:section['offset']+section['bytes']];position=content.find(data)
            while position>=0:
                owners=[s for s in symbols if s['section']==section['index']and s['value']<=position and s['bytes']and position+len(data)<=s['value']+s['bytes']]
                matches.append(dict(section=section['name'],section_offset=position,bytes=len(data),containing_symbols=owners));position=content.find(data,position+1)
        # Longest nontrivial aligned run inside the named corresponding
        # vendor program. This does not promote partial matches to parity.
        name='execsh4'if block['name'].startswith('gp_')else ('AC3Dolby'+str(int(block['name'].rsplit('_',1)[1])+1)if block['name'].startswith('ep_selected_')else {'ep_initial_573860':'AC3SuperExec','ep_loader_573B80':'AC3Loader','ep_config_57A980':'AC3ConfigTable'}[block['name']])
        for symbol in [s for s in symbols if s['name']==name and s['bytes']]:
            section=sections[symbol['section']];vendor_data=raw[section['offset']+symbol['value']:section['offset']+symbol['value']+symbol['bytes']]
            best=(0,0,0)
            for start in range(0,len(data)-31,4):
                chunk=data[start:start+32];test=struct.unpack('<8I',chunk)
                if len(set(test))<4 or sum(bool(v)for v in test)<6:continue
                at=vendor_data.find(chunk)
                while at>=0:
                    if at%4:at=vendor_data.find(chunk,at+1);continue
                    count=32
                    while start+count+4<=len(data)and at+count+4<=len(vendor_data)and data[start+count:start+count+4]==vendor_data[at+count:at+count+4]:count+=4
                    if count>best[2]:best=(start,at,count)
                    at=vendor_data.find(chunk,at+1)
            named_runs.append(dict(symbol=name,symbol_bytes=symbol['bytes'],source_offset=best[0],symbol_offset=best[1],common_bytes=best[2],complete_match=bool(len(data)==len(vendor_data)and data==vendor_data)))
        block.update(sha256=hashlib.sha256(data).hexdigest(),storage_words=len(words),non24bit_storage_words=sum(w>0xffffff for w in words),whole_vendor_payload_matches=matches,named_corresponding_vendor_runs=named_runs,native_algorithm_implemented=False)
    report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),original_bodies=rows,executed_selector_cases=selections,program_blocks=blocks,original_instruction_bytes=instruction_bytes,vendor_object_sha256=hashlib.sha256(raw).hexdigest(),ep_selected_program_end_word=cursor,ep_config_end_word=cursor+24,runtime_program_bytes_exported=False,source_processor_ready=False,native_algorithm_implemented=False,source_pcm_produced=False,full_startup_completed=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(bodies=len(rows),sites=len(instruction_bytes),selectors=len(selections),programs=[dict(name=b['name'],bytes=b['storage_bytes'],full_match=bool(b['whole_vendor_payload_matches']),named_runs=b['named_corresponding_vendor_runs'])for b in blocks]),indent=2))
if __name__=='__main__':main()
