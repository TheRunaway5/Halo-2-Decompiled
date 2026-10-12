"""Observe genuine original position/environment/filter setter calls.

Original wrapper, state synchronization, retained setter and reached device
code execute normally. Hardware breakpoints only observe; no guest instruction,
memory, register, lock/FIFO result or callee return is supplied. Deferred calls
are captured as deferred, not represented as completed spatial device updates.
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

kind=os.environ['H2_SOUND_SETTER_KIND']
entry,caller,count,acquired_pc={
    'position':(0x37c668,0x220ea6,5,0x37c3c8),
    'environment':(0x37c6e5,0x221427,3,0x37c4d8),
    'filter':(0x37b68b,0x22147c,2,0x37b1ec)}[kind]
image_path=ROOT/'assets/disc/default.xbe'
sha=lambda raw:hashlib.sha256(raw).hexdigest()
assert sha(image_path.read_bytes())=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image=Xbe(image_path);inferior=gdb.selected_inferior()
read=lambda address,size:bytes(inferior.read_memory(address,size))
reg=lambda name:int(gdb.parse_and_eval('$'+name))&0xffffffff
word=lambda address:struct.unpack('<I',read(address,4))[0]
assert reg('eip')==entry
stack=reg('esp');assert word(stack)==caller
arguments=list(struct.unpack('<'+'I'*count,read(stack+4,count*4)))
interface=arguments[0];assert interface
owner=(interface-0x1c)&0xffffffff;voice=word(owner+0xc);data=word(owner+0x10)
assert data==word(voice+0x70)
output=Path(os.environ['H2_SOUND_SETTER_OUTPUT']);output.mkdir(parents=True,exist_ok=False)
regions=[]
def backing(name,address,size):
    if address:regions.append(dict(name=name,address=f'{address:08x}',size=size,before_hex=read(address,size).hex()))
backing('object',owner,0x20);backing('voice',voice,0x74);backing('data',data,0xbc)
backing('source',word(data+0xb4),0xa4);backing('parameters',word(data+0xb8),0x38)
backing('busy',0x386b0c,4)
if kind!='position':backing('payload',arguments[1],36 if kind=='environment' else 24)
if kind=='position':assert arguments[-1]==1
if kind=='environment':assert arguments[-1]==1
verified=[]
for directory in ('sdk-setters-base-v1','sdk-setters-deeper-v1'):
    for path in sorted((ROOT/'analysis/audio-spatial-20261011'/directory).glob('*.asm')):
        for line in path.read_text().splitlines():
            fields=line.split();pc=int(fields[0],16);raw=bytes.fromhex(fields[1])
            assert raw==image.read(pc,len(raw))==read(pc,len(raw))
            verified.append(dict(pc=fields[0],bytes=fields[1]))
report=dict(boundary=__doc__,kind=kind,image_sha256=sha(image_path.read_bytes()),
    tool_sha256=sha(Path(__file__).read_bytes()),entry=f'{entry:08x}',caller=f'{caller:08x}',
    registers={name:reg(name) for name in ('eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags','fctrl','fstat','ftag','mxcsr')},
    arguments=[f'{v:08x}'for v in arguments],regions=regions,
    unchanged_instructions=verified,observed_device_writes=[],instruction_patches=[],
    guest_memory_writes=[],guest_register_writes=[],supplied_callee_returns=[],native_or_physical_DSP_claimed=False)
(output/'entry.json').write_text(json.dumps(report,indent=2)+'\n')

def reach(pc):
    bp=gdb.Breakpoint(f'*0x{pc:x}',type=gdb.BP_HARDWARE_BREAKPOINT,temporary=True)
    gdb.execute('continue');assert reg('eip')==pc,(hex(reg('eip')),hex(pc))
    if bp.is_valid():bp.delete()

def finish_setter_capture():
    reach(acquired_pc)
    report['original_lock_acquired']=reg('eax')&0xff
    report['busy_under_lock']=word(0x386b0c)
    assert report['busy_under_lock']==0
    if kind=='filter' and read(voice+0x12,1)[0]&1:
        commands=[(0x38180d,0xfe8202f8,'esi'),(0x381813,0xfe8202fc,1),
            (0x38181c,0xfe820318,'esi'),(0x381825,0xfe820374,'esi'),
            (0x38182e,0xfe820378,'esi'),(0x381834,0xfe8202fc,0)]
        voices=read(voice+0x64,1)[0];assert 1<=voices<=3
        for index in range(voices):
            for pc,address,value in commands:
                reach(pc)
                report['observed_device_writes'].append(dict(pc=f'{pc:08x}',address=f'{address:08x}',
                    value=f'{reg(value) if isinstance(value,str) else value:08x}',voice_index=index))
    reach(caller);assert reg('esp')==stack+4+count*4
    report['actual_return_eax']=f'{reg("eax"):08x}';assert reg('eax')==0
    for region in regions:region['after_hex']=read(int(region['address'],16),region['size']).hex()
    report.update(original_whole_setter_returned_normally=True,
        original_wrapper_lock_code_executed=True,device_writes_executed_normally=True,
        actual_return_pc=f'{reg("eip"):08x}',actual_return_esp=f'{reg("esp"):08x}')
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Original {kind} setter returned normally; {len(report["observed_device_writes"])} device writes; apply={arguments[-1] if kind!="filter" else "immediate"}')
