#!/usr/bin/env python3
"""Recover original title-effect installation/description as offline evidence.

Execute the original pure38DFFF transformation on reference-only words.
Export metadata/hashes, never decrypted executable programs for runtime use.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ENTRIES=[0x1972c0,0x225610,0x1975b0,0x38594d,0x384369,0x3841f0,0x383f9c,0x388576,0x388309,0x38de81,0x38df59,0x38dfd2,0x38dfff]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;codes={};a.output.mkdir(parents=True)
    pairing_path=Path(__file__).resolve().parents[1]/'analysis/multiplayer/integration/sound-driver-update-native/paired/report.json'
    pairing=json.loads(pairing_path.read_text());assert next(r for r in pairing['owners']if r['old']=='00220d10')['u5']=='00225610'
    assert next(r for r in pairing['owners']if r['old']=='00193020')['u5']=='001975b0'
    rows=[];source_instructions={}
    for entry in ENTRIES:
        section=next(s for s in image.sections if s.va<=entry<s.va+s.vsize)
        if section.name not in codes:codes[section.name]=_Code(image,section.name)
        code=codes[section.name]
        body=_trace(code,entry,set(ENTRIES));listing=[];calls=[]
        for pc in sorted(body.code):
            op=code.at(pc);raw=image.read(pc,op.size);assert raw==bytes(op.bytes);source_instructions[f'{pc:08x}']=raw.hex();listing.append(f'{pc:08x} {raw.hex()} {op.mnemonic} {op.op_str}')
            if op.mnemonic=='call':
                arg=op.operands[0]
                if arg.type==2:calls.append(dict(instruction=pc,target=arg.imm&0xffffffff))
                else:calls.append(dict(instruction=pc,indirect=op.op_str))
        name=f'original-{entry:08x}.asm';(a.output/name).write_text('\n'.join(listing)+'\n');rows.append(dict(entry=entry,instruction_sites=len(body.code),calls=calls,listing=name,listing_sha256=hashlib.sha256((a.output/name).read_bytes()).hexdigest()))
    base,size=0x58d9e0,28768;payload=image.read(base,size);header=struct.unpack_from('<6I',payload,0x800)
    description=base+0x818+(header[1]+header[3])*4;count,scratch=struct.unpack('<2I',image.read(description,8));extent=8+count*0x20;keys=description+extent
    assert count==15 and scratch==774144 and keys+count*8==base+size
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x380000,0x20000);cpu.mem_map(0x580000,0x20000);cpu.mem_map(0x2000000,0x20000)
    dsound=next(section for section in image.sections if section.name=='DSOUND')
    cpu.mem_write(dsound.va,image.section_bytes(dsound));cpu.mem_write(base,payload);cpu.mem_write(0x2000000,bytes(0x20000));executed={};operations=[]
    def hook(m,pc,n,_):raw=bytes(m.mem_read(pc,n));assert raw==image.read(pc,n),(hex(pc),raw.hex(),image.read(pc,n).hex());executed[f'{pc:08x}']=raw.hex()
    cpu.hook_add(UC_HOOK_CODE,hook)
    def transform(seed,source,bytes_,destination):
        stack=0x2010000;cpu.mem_write(stack,struct.pack('<6I',0x201f000,seed,source,bytes_,destination,0));cpu.reg_write(R.UC_X86_REG_ESP,stack);cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202)
        cpu.emu_start(0x38dfff,0x201f000,count=10000000);assert cpu.reg_read(R.UC_X86_REG_EIP)==0x201f000 and cpu.reg_read(R.UC_X86_REG_EAX)==0 and cpu.reg_read(R.UC_X86_REG_ESP)==stack+24
        operations.append(dict(entry=0x38dfff,seed=seed,source=source,bytes=bytes_,destination=destination,source_returned_hresult=0))
    cpu.mem_write(0x2000100,bytes(8));cpu.mem_write(0x2010000,struct.pack('<2I',0x201f000,0x2000100));cpu.reg_write(R.UC_X86_REG_ESP,0x2010000)
    cpu.emu_start(0x38dfd2,0x201f000,count=100000);assert cpu.reg_read(R.UC_X86_REG_EIP)==0x201f000
    transform(0x2000100,keys,count*8,keys)
    program_cursor=base+0x818;maps=[]
    for i in range(count):
        pointer=description+8+i*0x20;record=struct.unpack('<8I',cpu.mem_read(pointer,0x20));program_size=record[1]
        assert program_size>0 and program_cursor+program_size<=description
        transform(keys+i*8,program_cursor,program_size,program_cursor)
        maps.append(dict(index=i,raw_map_words=list(record),program_source=program_cursor,program_bytes=program_size,transformed_program_sha256=hashlib.sha256(cpu.mem_read(program_cursor,program_size)).hexdigest(),native_algorithm_identified=False))
        program_cursor+=program_size
    code_end=base+0x818+header[1]*4;assert program_cursor<=code_end==base+header[2]
    untouched_code_gap=bytes(cpu.mem_read(program_cursor,code_end-program_cursor))
    report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),paired_driver_reference_sha256=hashlib.sha256(pairing_path.read_bytes()).hexdigest(),driver_entries=dict(canonical11_update=0x220d10,tu15_update=0x225610,canonical11_reload=0x193020,tu15_reload=0x1975b0),original_bodies=rows,original_instruction_bytes=source_instructions,executed_transformation_instruction_bytes=executed,source_image_sha256=hashlib.sha256(payload).hexdigest(),source_image_bytes=size,source_header_words=list(header),description_source=description,effect_count=count,description_bytes=extent,key_bytes=count*8,scratch_request_bytes=scratch,effect_maps=maps,transformations=operations,code_extent_bytes=header[1]*4,transformed_program_bytes=sum(r['program_bytes']for r in maps),untransformed_code_gap_bytes=len(untouched_code_gap),untransformed_code_gap_sha256=hashlib.sha256(untouched_code_gap).hexdigest(),
      source_installation_contract=dict(location_copy_entry=0x3841f0,wrapper_entry=0x383f9c,device_success_flag_offset=0x80,device_success_flag_mask=1,download_entry=0x388576,description_builder=0x388309,header_copy_destination_gp_scratch=0x800,header_words=6,source_command_commit_instruction=0x38846e,source_command_word_offset=0x810,source_command_request=3,source_acknowledgement_poll=0x388470,source_required_ack_value=0,source_effect_description_owner_offset=0x20,description_record_stride=0x20),
      runtime_program_bytes_exported=False,native_algorithm_implemented=False,source_installation_executed=False,source_acknowledgement_supplied=False,source_processor_ready=False,full_startup_completed=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(bodies=len(rows),sites=len(source_instructions),effect_maps=count,scratch_request_bytes=scratch,program_bytes=sum(r['program_bytes']for r in maps),transformations=len(operations),native_algorithm_implemented=False)))
if __name__=='__main__':main()
