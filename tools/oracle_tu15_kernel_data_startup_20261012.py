#!/usr/bin/env python3
"""Fresh unchanged TU1.5 startup with actual observed kernel export322 input.

Uses independently compiled primary record/list owners with the declared native
preserving void ABI. Stops at the actual settings callee, supplies no settings
return, and does not construct a heap, world, body, contact or physics result.
The kernel input comes from the named modified BIOS in xemu, not physical Xbox.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_tu15_startup_records_20261011 import (
    ROOT,NEW_DEFAULT,NEW_PIN,GENERATED,Xbe,_Code,_trace,compile_records,
    compile_registry,abi,load,cpu,TEXT,STACK,SP,STOP,PRIVATE,PSP)

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--kernel-report',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    tool_pin=sha(__file__)
    kernel=json.loads(a.kernel_report.read_text());hardware=kernel['records']['322']
    raw_hardware=(a.kernel_report.parent/'XboxHardwareInfo.bin').read_bytes()
    assert len(raw_hardware)==8 and raw_hardware.hex()==hardware['hex']
    assert sha(a.kernel_report.parent/'XboxHardwareInfo.bin')==hardware['sha256']
    assert not kernel['guest_memory_writes'] and not kernel['guest_register_writes']
    assert not kernel['instruction_patches'] and not kernel['physical_xbox_proof']
    assert kernel['tool_sha256']==sha(ROOT/'tools/capture_xbox_kernel_hardware_20261012.py')
    address=hardware['address'];assert 0x80010000<=address<=0xfffffff8
    # This is the observed resource, not a selected branch-forcing profile.
    assert not (int.from_bytes(raw_hardware[:4],'little')&8)
    boot=a.kernel_report.parent.parent
    boot_validation=json.loads((boot/'validation.json').read_text())
    inputs=json.loads((boot/'inputs.json').read_text())
    assert boot_validation['kernel_report_sha256']==sha(a.kernel_report)
    assert boot_validation['original_resources_unchanged'] and not boot_validation['title_executed']
    primary=compile_records(a.output/'primary');registry=compile_registry(a.output/'registry')
    assert sha(NEW_DEFAULT)==NEW_PIN
    generated=json.loads(GENERATED.read_text());assert generated['later_original_sha256']==NEW_PIN
    correspondence=GENERATED.with_name('normalized-correspondence.json')
    assert sha(correspondence)==generated['correspondence_sha256']
    inverse={q:int(p,16) for p,q in json.loads(correspondence.read_text())['instruction_map'].items()}
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');bodies={};byte_rows={}
    starts={0x2dba3e,0x2e00af,0x2e0082}
    for p in sorted(starts):
        trace=_trace(code,p,starts);bodies[f'{p:08x}']=sorted(trace.code)
        for q in sorted(trace.code):
            i=code.at(q);assert q in inverse and image.read(q,len(i.bytes))==i.bytes
            byte_rows[f'{q:08x}']=dict(raw=i.bytes.hex(),mnemonic=i.mnemonic,operand=i.op_str,canonical=inverse[q])
    assert len(byte_rows)==169 and image.read(0x41b66c,4).hex()=='42010080'
    assert code.at(0x2e00fc).mnemonic=='call' and code.at(0x2e00fc).operands[0].imm==0x2e0fe2
    records=(a.output/'primary/records.bin').read_bytes();list_code=(a.output/'registry/registry.bin').read_bytes()
    stack=bytes((i*13+7)&255 for i in range(0x10000));cases=[]
    def machine():
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        raw=NEW_DEFAULT.read_bytes();header_size=struct.unpack_from('<I',raw,0x108)[0]
        g.mem_write(0x10000,raw[:header_size])
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        for p in (STACK,PRIVATE):g.mem_map(p,0x10000)
        g.mem_write(STACK,stack);g.mem_map(STOP,0x1000);g.mem_map(0x80000000,0x1000)
        g.mem_map(address&~0xfff,0x2000 if (address&0xfff)>0xff8 else 0x1000)
        g.mem_write(address,raw_hardware);g.mem_write(0x41b66c,struct.pack('<I',address))
        g.mem_map(TEXT,0x1000);g.mem_write(TEXT,records)
        g.mem_map(registry['text'],0x1000);g.mem_write(registry['text'],list_code)
        g.mem_map(registry['head'],0x1000);g.mem_write(registry['head'],(a.output/'registry/head.bin').read_bytes())
        g.mem_write(0,struct.pack('<I',0xffffffff));return g
    for entry in (0x2dba3e,0x2e00af):
      for flags in (2,0xcd7):
       for shift in (0,4,8):
        for top in (0,3):
            g=machine();sp=SP+shift;g.mem_write(sp,struct.pack('<I',STOP));load(g,cpu(sp,flags,top))
            before=abi(g);active=0;current=None;trace=[];stores=[];calls=[];boundary=None
            def hook(uc,p,n,user):
                nonlocal active,current,boundary
                if current is not None:
                    if p==STOP+0x100:
                        load(uc,current['before_abi']);uc.reg_write(R.UC_X86_REG_ESP,current['frame_address']+current['width'])
                        uc.reg_write(R.UC_X86_REG_EIP,current['return_address']);current=None;return
                    base,raw=(registry['text'],list_code) if current['target']==0x8000002f else (TEXT,records)
                    assert base<=p<p+n<=base+len(raw) and bytes(uc.mem_read(p,n))==raw[p-base:p-base+n]
                    return
                if p>=0x80000000:
                    width={0x8000002f:12,0x8000006b:16,0x80000071:12}[p]
                    esp=uc.reg_read(R.UC_X86_REG_ESP);frame=bytes(uc.mem_read(esp,width));words=struct.unpack('<'+'I'*(width//4),frame)
                    expected={0x8000002f:(0x2dba55,[0x2dba5b,0x46e2e8,1]),
                        0x8000006b:(0x2e00c1,[0x2e00c7,0x56edc0,0x2e009d,0]),
                        0x80000071:(0x2e00ce,[0x2e00d4,0x56ede0,0])}[p]
                    assert active==expected[0] and list(words)==expected[1]
                    current=dict(instruction=active,canonical_instruction=inverse[active],target=p,
                        frame_address=esp,frame=frame.hex(),return_address=words[0],width=width,before_abi=abi(uc),trace_index=len(trace),stores=[])
                    calls.append(current)
                    uc.mem_write(PSP,struct.pack('<'+'I'*(width//4),STOP+0x100,*words[1:]));uc.reg_write(R.UC_X86_REG_ESP,PSP)
                    target=registry['entry'] if p==0x8000002f else primary['entries']['primary_dpc' if p==0x8000006b else 'primary_timer']
                    uc.reg_write(R.UC_X86_REG_EIP,target);return
                if p==0x2e0fe2:
                    esp=uc.reg_read(R.UC_X86_REG_ESP);ebp=uc.reg_read(R.UC_X86_REG_EBP)
                    frame=bytes(uc.mem_read(esp,24));assert active==0x2e00fc
                    assert list(struct.unpack('<6I',frame))==[0x2e0101,0x11,ebp-12,ebp-4,4,ebp-8]
                    boundary=dict(kind='required_settings_owner',instruction=active,canonical_instruction=inverse[active],
                        target=p,canonical_target=0x2dd532,frame_address=esp,frame=frame.hex(),width=24)
                    uc.emu_stop();return
                assert p in inverse and bytes(uc.mem_read(p,n))==image.read(p,n),hex(p)
                active=p;trace.append(p)
            def write(uc,access,p,n,value,user):
                change=dict(instruction=active,trace_index=len(trace),address=p,before=bytes(uc.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex())
                if current is not None:
                    if PRIVATE<=p<p+n<=PRIVATE+0x10000:return
                    assert (registry['head']<=p<p+n<=registry['head']+8 or 0x46e2e8<=p<p+n<=0x46e2f8 or 0x56edc0<=p<p+n<=0x56ee08)
                    current['stores'].append(change)
                else:change['canonical_instruction']=inverse[active];stores.append(change)
            g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
            g.emu_start(entry,STOP,count=10000);assert boundary and current is None
            assert bytes(g.mem_read(image.section('.text').va,image.section('.text').rsize))==image.section_bytes(image.section('.text'))
            assert bytes(g.mem_read(address,8))==raw_hardware
            banks=[(sp-0x100,0x140),(0,4),(0x56edc0,0x70),(0x56ee28,4),(0x46e2e8,16),(registry['head'],8),(address,8),(0x41b66c,4)]
            cases.append(dict(image='1.5',entry=entry,canonical_entry=inverse[entry],stack_pointer=sp,before=before,after=abi(g),boundary=boundary,
                instructions=trace,stores=stores,platform_calls=calls,final_banks=[dict(address=p,hex=bytes(g.mem_read(p,n)).hex()) for p,n in banks]))
    result=dict(schema=1,parity_target='English TU1.5 only',tool_sha256=tool_pin,
        helper_tool_sha256=sha(ROOT/'tools/oracle_tu15_startup_records_20261011.py'),original_path=str(NEW_DEFAULT),original_sha256=NEW_PIN,
        generated_parent_manifest_sha256=sha(GENERATED),source_bodies=bodies,source_instructions=byte_rows,
        primary_record_owner=primary,primary_registry_owner=registry,stack_address=STACK,initial_stack_hex=stack.hex(),cases=cases,
        kernel_input=dict(record=hardware,report_sha256=sha(a.kernel_report),capture_tool_sha256=kernel['tool_sha256'],
            boot_validation_sha256=sha(boot/'validation.json'),resources=inputs,reference=boot_validation['reference']),
        loader_data_bindings=[dict(cell=0x41b66c,ordinal=322,before='42010080',written=struct.pack('<I',address).hex())],
        source_instruction_patches=[],older_image_executed=False,captured_world_input_used=False,native_expected_data_used=False,
        native_platform_abi='void stdcall, exact actual TU1.5 frame cleanup, other live CPU state preserved',
        hardware_data_supplied=True,settings_result_supplied=False,timer_armed=False,process_heap_installed=False,
        full_startup_complete=False,live_physics_parity=False,physical_xbox_proof=False)
    assert sha(__file__)==tool_pin
    out=a.output/'fixture.json';out.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),source_instructions=sum(len(c['instructions']) for c in cases),
        source_writes=sum(len(c['stores']) for c in cases),platform_writes=sum(len(v['stores']) for c in cases for v in c['platform_calls']),
        settings_owner=hex(0x2e0fe2),fixture_sha256=sha(out))))

if __name__=='__main__':main()
