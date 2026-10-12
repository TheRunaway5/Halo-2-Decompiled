"""Observe a genuine complete382032 spatial commit, including device writes.

All original kernel/algorithm/device code executes normally. Hardware execution
breakpoints observe real call arguments, retained inputs/outputs and pending
device write values. No guest instruction, memory, register, FIFO result or
callee return is supplied. This is emulator evidence, not physical DSP proof.
"""
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import gdb

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

image_path=ROOT/'assets/disc/default.xbe'
sha=lambda raw:hashlib.sha256(raw).hexdigest()
assert sha(image_path.read_bytes())=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image=Xbe(image_path);inferior=gdb.selected_inferior()
read=lambda address,size:bytes(inferior.read_memory(address,size))
reg=lambda name:int(gdb.parse_and_eval('$'+name))&0xffffffff
word=lambda address:struct.unpack('<I',read(address,4))[0]
assert reg('eip')==0x382032
skipped=[]
for attempt in range(256):
    voice=reg('ecx');data=word(voice+0x70)
    if read(voice+0x12,1)[0]&3==3 and word(data+0xb4) and word(data+0xb8):break
    skipped.append(dict(voice=f'{voice:08x}',flags=read(voice+0x12,1)[0]))
    gdb.execute('continue');assert reg('eip')==0x382032
else:raise RuntimeError('No initialized active spatial commit selected in256 observed entries')
output=Path(os.environ['H2_SOUND_COMMIT_OUTPUT']);output.mkdir(parents=True,exist_ok=False)
stack=reg('esp');return_pc=word(stack)
device=word(voice+8);device_data=word(device+0xc)
source=word(data+0xb4);parameters=word(data+0xb8)
regions=[]


def backing(name,address,size):
    if address:regions.append(dict(name=name,address=f'{address:08x}',size=size,before_hex=read(address,size).hex()))


backing('voice',voice,0x74);backing('data',data,0xbc)
backing('device',device,0x10);backing('device_data',device_data,0xb4)
backing('source',source,0xa4);backing('parameters',parameters,0x38)
curve=word(source+0x50);curve_count=word(source+0x54)
if curve and curve_count:
    assert curve_count<=65536
    backing('distance_curve',curve,curve_count*4)
backing('speaker_config',0x386b04,4)
parent=word(data+0xb0);backing('parent_link',parent,0x24)
if parent:
    owner=word(parent+0x20);backing('parent_owner',owner,0x84)
    if owner:
        parent_data=word(owner+0x80);backing('parent_data',parent_data,0xbc)
        backing('parent_parameters',word(parent_data+0xb8),0x38)
verified=[]
for entry in (0x382032,0x381e2a,0x37ca0a,0x37ae57,0x381ce4,0x381d7c,0x3812e0,0x381710):
    for line in (ROOT/f'analysis/audio-spatial-20261011/sdk-image-pairing-v9/instructions/{entry:08x}-base.asm').read_text().splitlines():
        fields=line.split();pc=int(fields[0],16);raw=bytes.fromhex(fields[1])
        assert raw==image.read(pc,len(raw))==read(pc,len(raw))
        verified.append(dict(pc=fields[0],bytes=fields[1]))
report=dict(boundary=__doc__,image_sha256=sha(image_path.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),
    voice_address=f'{voice:08x}',registers={name:reg(name) for name in ('eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags','fctrl','fstat','ftag','mxcsr')},
    regions=regions,skipped_entries=skipped,selected_sdk_table_hex=read(0x3871c8,44).hex(),
    unchanged_instructions=verified,calls=[],dispatch_tests=[],observed_device_writes=[],
    caller_stores=[],instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],
    supplied_callee_returns=[],native_or_physical_DSP_claimed=False)
(output/'entry.json').write_text(json.dumps(report,indent=2)+'\n')


def reach(pc):
    bp=gdb.Breakpoint(f'*0x{pc:x}',type=gdb.BP_HARDWARE_BREAKPOINT,temporary=True)
    gdb.execute('continue');assert reg('eip')==pc,(hex(reg('eip')),hex(pc))
    if bp.is_valid():bp.delete()


def snapshot():
    return {r['name']:read(int(r['address'],16),r['size']).hex() for r in regions}


def call_entry(owner,entry,argument_count):
    reach(entry);sp=reg('esp')
    call=dict(owner=owner,entry=f'{entry:08x}',stack=f'{sp:08x}',this=f'{reg("ecx"):08x}',
        return_pc=f'{word(sp):08x}',arguments=[f'{v:08x}' for v in struct.unpack('<'+'I'*argument_count,read(sp+4,argument_count*4))] if argument_count else [],
        before=snapshot(),device_write_begin=len(report['observed_device_writes']))
    report['calls'].append(call);return call


def call_return(call):
    reach(int(call['return_pc'],16));assert reg('esp')==int(call['stack'],16)+4+len(call['arguments'])*4
    call.update(after=snapshot(),device_write_end=len(report['observed_device_writes']),returned_normally=True)


def pending(owner,pc,address,value):
    reach(pc)
    report['observed_device_writes'].append(dict(owner=owner,pc=f'{pc:08x}',address=f'{address:08x}',
        value=f'{reg(value) if isinstance(value,str) else value:08x}'))


def device_call(owner,entry,argument_count):
    call=call_entry(owner,entry,argument_count);assert reg('ecx')==voice
    if owner=='hrtf':
        pending(owner,0x38139f,0xfe820160,'eax')
        for i in range(15):
            reach(0x3813ec);assert reg('ebx')==0xfe820400+i*4
            report['observed_device_writes'].append(dict(owner=owner,pc='003813ec',address=f'{reg("ebx"):08x}',value=f'{reg("edx"):08x}'))
        for pc,address in ((0x381427,0xfe82043c),(0x381433,0xfe8202f8),(0x38143e,0xfe82031c)):
            pending(owner,pc,address,'eax')
    else:
        if owner=='pitch':assert not word(data+8)&0x182000,'Recursive initialized pitch needs a separate command observer'
        specs={
            'gain':[(0x381d2c,0xfe8202f8,'edx'),(0x381d36,0xfe8202fc,1),
                    (0x381d40,0xfe820360,'edx'),(0x381d4a,0xfe820364,'edx'),
                    (0x381d54,0xfe820368,'edx'),(0x381d5a,0xfe8202fc,0)],
            'pitch':[(0x381de6,0xfe8202f8,'edx'),(0x381def,0xfe82037c,'edx')],
            'filter':[(0x38180d,0xfe8202f8,'esi'),(0x381813,0xfe8202fc,'edx'),
                      (0x38181c,0xfe820318,'esi'),(0x381825,0xfe820374,'esi'),
                      (0x38182e,0xfe820378,'esi'),(0x381834,0xfe8202fc,0)]}
        if read(voice+0x12,1)[0]&1:
            count=read(voice+0x64,1)[0];assert 1<=count<=3
            for i in range(count):
                for pc,address,value in specs[owner]:pending(owner,pc,address,value)
    call_return(call)


def finish_commit_capture():
    calculate=call_entry('calculate',0x37ca0a,2)
    assert [int(v,16) for v in calculate['arguments']]==[device_data+0x30,source]
    call_return(calculate)
    voice_data=call_entry('voice_data',0x37ae57,5)
    assert [int(v,16) for v in voice_data['arguments']]==[device_data+0x30,source,device_data+0x80,source+0x7c,parameters]
    call_return(voice_data)
    for pc,address in ((0x382095,source),(0x3820a1,source+0x7c)):
        reach(pc);assert reg('eax')==source
        report['caller_stores'].append(dict(pc=f'{pc:08x}',address=f'{address:08x}',before=f'{word(address):08x}',after='00000000'))
    dispatch=call_entry('dispatch',0x381e2a,0)
    for pc,mask,owner,entry,count in ((0x381e36,0x1f,'gain',0x381ce4,0),
        (0x381e49,0x20,'pitch',0x381d7c,0),(0x381e5e,0x40,'hrtf',0x3812e0,2),
        (0x381e77,0x80,'filter',0x381710,1)):
        reach(pc);pointer=reg('eax');flags=read(pointer,1)[0]
        assert pointer==word(word(voice+0x70)+0xb8)
        report['dispatch_tests'].append(dict(pc=f'{pc:08x}',mask=mask,flags=flags,parameters=f'{pointer:08x}',called=bool(flags&mask)))
        if flags&mask:device_call(owner,entry,count)
    reach(0x381e8e);pointer=reg('eax')
    report['caller_stores'].append(dict(pc='00381e8e',address=f'{pointer:08x}',before=f'{word(pointer):08x}',after='00000000'))
    call_return(dispatch)
    reach(return_pc);assert reg('esp')==stack+4
    for region in regions:region['after_hex']=read(int(region['address'],16),region['size']).hex()
    report.update(actual_return_pc=f'{return_pc:08x}',actual_return_esp=f'{reg("esp"):08x}',
                  original_whole_commit_returned_normally=True,device_writes_executed_normally=True)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Original complete spatial commit returned normally; {len(report["observed_device_writes"])} actual device writes')
