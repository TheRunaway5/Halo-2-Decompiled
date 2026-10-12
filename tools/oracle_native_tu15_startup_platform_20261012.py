#!/usr/bin/env python3
"""Fresh unchanged TU1.5 startup with declared native desktop platform inputs.

Uses independently compiled primary record/list owners with the declared native
preserving void ABI. The application owns its power preference and desktop
capability record. Only these native query/status calls are adapted; the timer
remains a real required call. No BIOS or EEPROM resource is read. This validates
the native adapter contract, not original Xbox kernel clobbers or hardware.
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
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    tool_pin=sha(__file__)
    address=0x90000000;raw_hardware=bytes(8)
    # Native desktop is not an Xbox arcade machine. Remaining ABI fields are
    # unconsumed reserved storage, not claimed Xbox GPU/MCP revisions.
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
    settings_manifest=ROOT/'analysis/multiplayer/sandbox-source/tu15-settings-startup-20261012/generated-v1.json'
    settings=json.loads(settings_manifest.read_text());assert settings['original_sha256']==NEW_PIN
    for key,row in settings['source_instructions'].items():
        p=int(key,16);assert image.read(p,len(bytes.fromhex(row['raw'])))==bytes.fromhex(row['raw'])
        byte_rows[key]=dict(row,canonical=p);inverse[p]=p
    assert len(byte_rows)==182
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
         for automatic_power_down in (False,True):
            g=machine();sp=SP+shift;g.mem_write(sp,struct.pack('<I',STOP));load(g,cpu(sp,flags,top))
            before=abi(g);active=0;current=None;trace=[];stores=[];calls=[];boundary=None;native_calls=[]
            def hook(uc,p,n,user):
                nonlocal active,current,boundary
                if current is not None:
                    if p==STOP+0x100:
                        load(uc,current['before_abi']);uc.reg_write(R.UC_X86_REG_ESP,current['frame_address']+current['width'])
                        uc.reg_write(R.UC_X86_REG_EIP,current['return_address']);current=None;return
                    base,raw=(registry['text'],list_code) if current['target']==0x8000002f else (TEXT,records)
                    assert base<=p<p+n<=base+len(raw) and bytes(uc.mem_read(p,n))==raw[p-base:p-base+n]
                    return
                if p==0x80000095:
                    esp=uc.reg_read(R.UC_X86_REG_ESP);frame=bytes(uc.mem_read(esp,20))
                    assert active==0x2e0096 and list(struct.unpack('<5I',frame))==[0x2e009c,0x56ede0,0xb5659000,0xffffffcd,0x56edc0]
                    boundary=dict(kind='required_timer_owner',instruction=active,canonical_instruction=inverse[active],
                        target=p,frame_address=esp,frame=frame.hex(),width=20)
                    uc.emu_stop();return
                if p in (0x80000018,0x8000012d):
                    esp=uc.reg_read(R.UC_X86_REG_ESP);width=24 if p==0x80000018 else 8
                    frame=bytes(uc.mem_read(esp,width));words=struct.unpack('<'+'I'*(width//4),frame)
                    event=dict(instruction=active,target=p,frame_address=esp,frame=frame.hex(),
                        before=abi(uc),trace_index=len(trace),stores=[])
                    if p==0x80000018:
                        bp=uc.reg_read(R.UC_X86_REG_EBP)
                        assert active==0x2e101c and words[0]==0x2e0ff9 and words[1]==17 and words[4]==4 and esp==bp-24
                        assert list(words[1:])==list(struct.unpack('<5I',bytes(uc.mem_read(bp+8,20))))
                        for target,value in ((words[5],4),(words[2],4),(words[3],int(automatic_power_down))):
                            event['stores'].append(dict(instruction=active,address=target,before=bytes(uc.mem_read(target,4)).hex(),written=struct.pack('<I',value).hex()))
                            uc.mem_write(target,struct.pack('<I',value))
                        uc.reg_write(R.UC_X86_REG_EAX,0)
                    else:
                        assert active==0x2e0ffa and words==(0x2e1000,0) and uc.reg_read(R.UC_X86_REG_EAX)==0
                    uc.reg_write(R.UC_X86_REG_ESP,esp+width);uc.reg_write(R.UC_X86_REG_EIP,words[0]);event['after']=abi(uc)
                    native_calls.append(event);return
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
            cases.append(dict(automatic_power_down=automatic_power_down,native_calls=native_calls,image='1.5',entry=entry,canonical_entry=inverse[entry],stack_pointer=sp,before=before,after=abi(g),boundary=boundary,
                instructions=trace,stores=stores,platform_calls=calls,final_banks=[dict(address=p,hex=bytes(g.mem_read(p,n)).hex()) for p,n in banks]))
    result=dict(schema=1,parity_target='English TU1.5 only',tool_sha256=tool_pin,
        helper_tool_sha256=sha(ROOT/'tools/oracle_tu15_startup_records_20261011.py'),original_path=str(NEW_DEFAULT),original_sha256=NEW_PIN,
        generated_parent_manifest_sha256=sha(GENERATED),source_bodies=bodies,source_instructions=byte_rows,
        primary_record_owner=primary,primary_registry_owner=registry,stack_address=STACK,initial_stack_hex=stack.hex(),cases=cases,
        native_environment=dict(hardware_address=address,hardware_hex=raw_hardware.hex(),desktop_arcade=False,
            consumed_hardware_mask=8,settings='explicit native automatic_power_down preference; XC_MISC bit0',
            status_domain=[0],status_result=[0],bios_input_used=False,eeprom_input_used=False,kernel_tables_used=False),
        loader_data_bindings=[dict(cell=0x41b66c,ordinal=322,before='42010080',written=struct.pack('<I',address).hex())],
        source_instruction_patches=[],older_image_executed=False,captured_world_input_used=False,native_expected_data_used=False,
        native_platform_abi='void stdcall, exact actual TU1.5 frame cleanup, other live CPU state preserved',
        hardware_data_supplied=True,native_configuration_queried=True,timer_armed=False,process_heap_installed=False,
        full_startup_complete=False,live_physics_parity=False,physical_xbox_proof=False)
    assert sha(__file__)==tool_pin
    out=a.output/'fixture.json';out.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),source_instructions=sum(len(c['instructions']) for c in cases),
        source_writes=sum(len(c['stores']) for c in cases),platform_writes=sum(len(v['stores']) for c in cases for v in c['platform_calls']),
        native_queries=len(cases),required_timer=hex(0x80000095),fixture_sha256=sha(out))))

if __name__=='__main__':main()
