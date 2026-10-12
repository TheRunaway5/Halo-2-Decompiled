#!/usr/bin/env python3
"""Fresh unchanged TU1.5 startup with declared native desktop platform inputs.

Uses independently compiled primary record/list owners with the declared native
preserving void ABI. The application owns its power preference and desktop
capability record. Only these native query/status calls are adapted; the timer
is a declared native one-shot operation backed by an owned timer resource.
The original heap constructor runs over real reserved/committed host pages.
No BIOS or EEPROM resource is read. This validates
the native adapter contract, not original Xbox kernel clobbers or hardware.
"""
import argparse
import ctypes
import shutil
import hashlib
import json
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
import oracle_mp_spawn_kernel_sync_world as sync
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
    critical_dir=a.output/'critical';critical_dir.mkdir()
    prior=sync.OUT
    for p in prior.iterdir():
        if p.is_file() and p.name not in ('initializer.c','initializer.o','initializer.elf','initializer.bin'):
            (critical_dir/p.name).symlink_to(p.resolve())
    sync.OUT=critical_dir;sync.COMPILED=0x9200000;critical=sync.compile_initializer();sync.OUT=prior
    critical_code=(critical_dir/'initializer.bin').read_bytes()
    libc=ctypes.CDLL(None,use_errno=True)
    libc.mmap.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_long];libc.mmap.restype=ctypes.c_void_p
    libc.mprotect.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int];libc.munmap.argtypes=[ctypes.c_void_p,ctypes.c_size_t]
    clock=0x102030405060000;head=0x90001000

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
    for p,canonical in inverse.items():
        i=code.at(p);assert image.read(p,len(i.bytes))==i.bytes
        byte_rows[f'{p:08x}']=dict(raw=i.bytes.hex(),mnemonic=i.mnemonic,operand=i.op_str,canonical=canonical)
    for p in (0x2e009d,0x2e00a4,0x2e00a6,0x2e00ac):
        i=code.at(p);inverse[p]=p;byte_rows[f'{p:08x}']=dict(raw=i.bytes.hex(),mnemonic=i.mnemonic,operand=i.op_str,canonical=p)

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
        g.mem_map(head,0x1000);g.mem_write(head,struct.pack('<2I',head,head))
        g.mem_map(0x9200000,0x1000);g.mem_write(0x9200000,critical_code)
        g.mem_write(address,raw_hardware);g.mem_write(0x41b66c,struct.pack('<I',address))
        g.mem_map(TEXT,0x1000);g.mem_write(TEXT,records)
        g.mem_map(registry['text'],0x1000);g.mem_write(registry['text'],list_code)
        g.mem_map(registry['head'],0x1000);g.mem_write(registry['head'],(a.output/'registry/head.bin').read_bytes())
        g.mem_write(0,struct.pack('<I',0xffffffff));return g
    for entry in (0x2dba3e,0x2e00af):
      for flags in (2,0x8d7):
       for shift in (0,4,8):
        for top in (0,3):
         for automatic_power_down in (False,True):
            g=machine();sp=SP+shift;g.mem_write(sp,struct.pack('<I',STOP));load(g,cpu(sp,flags,top))
            before=abi(g);active=0;current=None;trace=[];stores=[];calls=[];boundary=None;native_calls=[];resources=[];reservation=None;committed=set()
            def hook(uc,p,n,user):
                nonlocal active,current,boundary,reservation
                if current is not None:
                    if p==STOP+0x100:
                        load(uc,current['before_abi']);uc.reg_write(R.UC_X86_REG_ESP,current['frame_address']+current['width'])
                        uc.reg_write(R.UC_X86_REG_EIP,current['return_address']);current=None;return
                    base,raw=(0x9200000,critical_code) if current['target']==0x80000123 else (registry['text'],list_code) if current['target']==0x8000002f else (TEXT,records)
                    assert base<=p<p+n<=base+len(raw) and bytes(uc.mem_read(p,n))==raw[p-base:p-base+n]
                    return
                if p==0x80000168:
                    esp=uc.reg_read(R.UC_X86_REG_ESP);frame=bytes(uc.mem_read(esp,4))
                    assert active==0x2e00a6 and frame.hex()=='ac002e00'
                    boundary=dict(kind='required_shutdown_owner',instruction=active,target=p,frame_address=esp,frame=frame.hex(),width=4)
                    uc.emu_stop();return
                if p==0x2dba2d:
                    esp=uc.reg_read(R.UC_X86_REG_ESP);frame=bytes(uc.mem_read(esp,8))
                    assert active==0x2dbaa3 and struct.unpack('<2I',frame)==(0x2dbaa8,0x46de38)
                    boundary=dict(kind='required_post_heap_owner',instruction=active,canonical_instruction=inverse[active],target=p,
                        frame_address=esp,frame=frame.hex(),width=8)
                    uc.emu_stop();return
                if p==0x800000b8:
                    esp=uc.reg_read(R.UC_X86_REG_ESP);words=struct.unpack('<6I',bytes(uc.mem_read(esp,24)))
                    ret,bp,zero,np,flags,protection=words;base=struct.unpack('<I',bytes(uc.mem_read(bp,4)))[0];size=struct.unpack('<I',bytes(uc.mem_read(np,4)))[0]
                    assert zero==0 and protection==4 and size and size%4096==0
                    initial=None
                    if flags==0x2000:
                        assert base==0 and reservation is None
                        host=libc.mmap(None,size,0,0x22,-1,0);assert host!=ctypes.c_void_p(-1).value
                        reservation=(host,size);base=0x10000000;uc.mem_map_ptr(base,size,0,host)
                    else:
                        assert flags==0x1000 and reservation and base>=0x10000000 and base+size<=0x10000000+reservation[1]
                        host=reservation[0]+base-0x10000000;assert libc.mprotect(host,size,3)==0
                        uc.mem_protect(base,size,3);initial=ctypes.string_at(host,size).hex();committed.update(range(base,base+size,4096))
                    resources.append(dict(instruction=active,request=[struct.unpack('<I',bytes(uc.mem_read(bp,4)))[0],zero,size,flags,protection],result=[0,base,size],initial_hex=initial))
                    uc.mem_write(bp,struct.pack('<I',base));uc.mem_write(np,struct.pack('<I',size));uc.reg_write(R.UC_X86_REG_EAX,0)
                    uc.reg_write(R.UC_X86_REG_ESP,esp+24);uc.reg_write(R.UC_X86_REG_EIP,ret);return
                if p==0x80000095:
                    esp=uc.reg_read(R.UC_X86_REG_ESP);frame=bytes(uc.mem_read(esp,20));words=struct.unpack('<5I',frame)
                    assert active==0x2e0096 and words== (0x2e009c,0x56ede0,0xb5659000,0xffffffcd,0x56edc0)
                    due=(clock+216000000000)&0xffffffffffffffff
                    event=dict(instruction=active,target=p,frame_address=esp,frame=frame.hex(),before=abi(uc),trace_index=len(trace),stores=[],clock=clock,due_time=due)
                    for target,width,value in ((0x56ee00,4,0x56edc0),(0x56ee04,4,0),(0x56ede1,1,0),(0x56edf0,4,due&0xffffffff),(0x56edf4,4,due>>32),
                        (0x56ede3,1,1),(0x56ede4,4,0),(0x56edf8,4,head),(0x56edfc,4,head),(head,4,0x56edf8),(head+4,4,0x56edf8)):
                        raw=value.to_bytes(width,'little');event['stores'].append(dict(instruction=active,address=target,before=bytes(uc.mem_read(target,width)).hex(),written=raw.hex()))
                        uc.mem_write(target,raw)
                    uc.reg_write(R.UC_X86_REG_EAX,uc.reg_read(R.UC_X86_REG_EAX)&0xffffff00)
                    uc.reg_write(R.UC_X86_REG_ESP,esp+20);uc.reg_write(R.UC_X86_REG_EIP,words[0]);event['after']=abi(uc);native_calls.append(event);return
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
                    width={0x8000002f:12,0x8000006b:16,0x80000071:12,0x80000123:8}[p]
                    esp=uc.reg_read(R.UC_X86_REG_ESP);frame=bytes(uc.mem_read(esp,width));words=struct.unpack('<'+'I'*(width//4),frame)
                    expected={0x80000123:(0x2de666,[0x2de66c,0x10000610]),0x8000002f:(0x2dba55,[0x2dba5b,0x46e2e8,1]),
                        0x8000006b:(0x2e00c1,[0x2e00c7,0x56edc0,0x2e009d,0]),
                        0x80000071:(0x2e00ce,[0x2e00d4,0x56ede0,0])}[p]
                    assert active==expected[0] and list(words)==expected[1]
                    current=dict(instruction=active,canonical_instruction=inverse[active],target=p,
                        frame_address=esp,frame=frame.hex(),return_address=words[0],width=width,before_abi=abi(uc),trace_index=len(trace),stores=[])
                    calls.append(current)
                    uc.mem_write(PSP,struct.pack('<'+'I'*(width//4),STOP+0x100,*words[1:]));uc.reg_write(R.UC_X86_REG_ESP,PSP)
                    target=int(critical['entry'],16) if p==0x80000123 else registry['entry'] if p==0x8000002f else primary['entries']['primary_dpc' if p==0x8000006b else 'primary_timer']
                    uc.reg_write(R.UC_X86_REG_EIP,target);return
                assert p in inverse and bytes(uc.mem_read(p,n))==image.read(p,n),dict(pc=hex(p),trace=[hex(v)for v in trace[-24:]],cpu=abi(uc),stack=bytes(uc.mem_read(uc.reg_read(R.UC_X86_REG_ESP),40)).hex())
                active=p;trace.append(p)
            def write(uc,access,p,n,value,user):
                change=dict(instruction=active,trace_index=len(trace),address=p,before=bytes(uc.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex())
                if current is not None:
                    if PRIVATE<=p<p+n<=PRIVATE+0x10000:return
                    assert (0x10000000<=p<p+n<=0x11000000 or registry['head']<=p<p+n<=registry['head']+8 or 0x46e2e8<=p<p+n<=0x46e2f8 or 0x56edc0<=p<p+n<=0x56ee08)
                    current['stores'].append(change)
                else:change['canonical_instruction']=inverse[active];stores.append(change)
            g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
            g.emu_start(entry,STOP,count=100000);assert current is None
            if entry==0x2e00af:
                assert g.reg_read(R.UC_X86_REG_EIP)==STOP
                boundary=dict(kind='returned_setup',target=STOP,width=0,frame_address=0,frame='')
            assert boundary

            assert bytes(g.mem_read(image.section('.text').va,image.section('.text').rsize))==image.section_bytes(image.section('.text'))
            assert bytes(g.mem_read(address,8))==raw_hardware
            banks=[(sp-0x100,0x140),(0,4),(0x56edc0,0x70),(0x56ee28,4),(0x46e2e8,16),(registry['head'],8),(address,8),(0x41b66c,4),(head,8)]+[(p,4096)for p in sorted(committed)]
            cases.append(dict(resources=resources,heap_pointer=struct.unpack('<I',bytes(g.mem_read(0x56ee28,4)))[0],automatic_power_down=automatic_power_down,native_calls=native_calls,image='1.5',entry=entry,canonical_entry=inverse[entry],stack_pointer=sp,before=before,after=abi(g),boundary=boundary,
                instructions=trace,stores=stores,platform_calls=calls,final_banks=[dict(address=p,hex=bytes(g.mem_read(p,n)).hex()) for p,n in banks]))
            # Execute the original DPC over the flag produced by this very
            # startup, rather than loading a captured settings/world output.
            active=0;trace=[];stores=[];boundary=None;native_calls=[]
            callback_frame=struct.pack('<5I',STOP,0x56edc0,0,0x05060708,0x01020304)
            g.mem_write(sp,callback_frame);load(g,cpu(sp,flags,top));callback_before=abi(g)
            g.emu_start(0x2e009d,STOP,count=100)
            if automatic_power_down:assert boundary and boundary['target']==0x80000168
            else:assert not boundary and g.reg_read(R.UC_X86_REG_EIP)==STOP
            cases[-1]['power_dpc']=dict(before=callback_before,after=abi(g),frame=callback_frame.hex(),frame_address=sp,
                boundary=boundary,instructions=trace,stores=stores,final_stack=bytes(g.mem_read(sp-4,24)).hex())
            if reservation:assert libc.munmap(reservation[0],reservation[1])==0
    result=dict(schema=1,parity_target='English TU1.5 only',tool_sha256=tool_pin,
        helper_tool_sha256=sha(ROOT/'tools/oracle_tu15_startup_records_20261011.py'),original_path=str(NEW_DEFAULT),original_sha256=NEW_PIN,
        generated_parent_manifest_sha256=sha(GENERATED),source_bodies=bodies,source_instructions=byte_rows,
        primary_record_owner=primary,primary_registry_owner=registry,primary_critical_owner=critical,stack_address=STACK,initial_stack_hex=stack.hex(),cases=cases,
        native_environment=dict(hardware_address=address,hardware_hex=raw_hardware.hex(),desktop_arcade=False,
            consumed_hardware_mask=8,settings='explicit native automatic_power_down preference; XC_MISC bit0',
            status_domain=[0],status_result=[0],timer_list_head=head,procedure_interrupt_time=clock,bios_input_used=False,eeprom_input_used=False,kernel_tables_used=False),
        loader_data_bindings=[dict(cell=0x41b66c,ordinal=322,before='42010080',written=struct.pack('<I',address).hex())],
        source_instruction_patches=[],older_image_executed=False,captured_world_input_used=False,native_expected_data_used=False,
        native_platform_abi='void stdcall, exact actual TU1.5 frame cleanup, other live CPU state preserved',
        hardware_data_supplied=True,native_configuration_queried=True,timer_armed=True,process_heap_installed_in_full_startup=True,
        full_startup_complete=False,live_physics_parity=False,physical_xbox_proof=False)
    assert sha(__file__)==tool_pin
    out=a.output/'fixture.json';out.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),source_instructions=sum(len(c['instructions']) for c in cases),
        source_writes=sum(len(c['stores']) for c in cases),platform_writes=sum(len(v['stores']) for c in cases for v in c['platform_calls']),
        native_queries=len(cases),required_post_heap=hex(0x2dba2d),fixture_sha256=sha(out))))

if __name__=='__main__':main()
