"""Observe genuine cold-boot SDK retained constructors and spatial binding.

Original constructors, reference methods, saved-setting owner and kernel code
execute normally. Hardware breakpoints observe actual initialized inputs,
whole retained objects, reached reference methods and normal returns. No guest
instruction, register, memory, kernel result or callee return is supplied.
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
image_path=ROOT/'assets/disc/default.xbe';sha=lambda raw:hashlib.sha256(raw).hexdigest()
assert sha(image_path.read_bytes())=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image=Xbe(image_path);inferior=gdb.selected_inferior()
read=lambda address,size:bytes(inferior.read_memory(address,size))
reg=lambda name:int(gdb.parse_and_eval('$'+name))&0xffffffff
word=lambda address:struct.unpack('<I',read(address,4))[0]
output=Path(os.environ['H2_SOUND_CONSTRUCTION_OUTPUT']);output.mkdir(parents=True,exist_ok=False)
specs={0x37a132:('device_object',0,0x2c),0x37cc6a:('device_data',0,0xb4),
       0x37c1d7:('buffer_object',1,0x24),0x382b22:('voice_object',2,0x158),
       0x37be6c:('buffer_binding',3,0x24)}
register_names=('eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags','fctrl','fstat','ftag','mxcsr')
remaining=dict(specs);skipped=[];reports=[]

def bp(pc):return gdb.Breakpoint(f'*0x{pc:x}',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
def reach(pc):
    selected=bp(pc);gdb.execute('continue');assert reg('eip')==pc,(hex(reg('eip')),hex(pc));selected.delete()

def observe(entry):
    kind,count,size=specs[entry];directory=output/kind;directory.mkdir()
    stack=reg('esp');caller=word(stack)
    arguments=list(struct.unpack('<'+'I'*count,read(stack+4,count*4))) if count else []
    owner=arguments[0] if kind=='buffer_binding' else reg('ecx')
    regions=[]
    def backing(name,address,extent):
        if not address:return
        if any(int(r['address'],16)<=address and address+extent<=int(r['address'],16)+r['size']for r in regions):return
        regions.append(dict(name=name,address=f'{address:08x}',size=extent,before_hex=read(address,extent).hex()))
    backing('object',owner,size)
    if kind=='device_object':backing('singleton',0x387198,4)
    if kind=='device_data':
        backing('speaker_override',0x386b08,4);backing('speaker_config',0x386b04,4)
    if kind=='buffer_object':backing('device_object',arguments[0],0x2c)
    if kind in ('voice_object','buffer_binding'):
        hardware=arguments[0] if kind=='voice_object' else word(arguments[1]+8)
        data=arguments[1] if kind=='voice_object' else arguments[2]
        backing('hardware_device',hardware,0x540);backing('voice_data',data,0xe0)
        backing('device_data',word(hardware+0xc),0xb4)
        backing('source',word(data+0xb4),0xa4);backing('parameters',word(data+0xb8),0x38)
    if kind=='buffer_binding':
        backing('voice_object',arguments[1],0x158)
        device=word(owner+8);backing('device_object',device,0x2c);backing('speaker_config',0x386b04,4)
        for name,at in (('list_next',device+0x10),('list_previous',device+0x14)):
            backing(name,word(at),8)
    backing('busy',0x386b0c,4)
    verified=[]
    entries={
        'device_object':(0x37a132,), 'device_data':(0x37cc6a,0x37a110),
        'buffer_object':(0x37c1d7,0x37be27,0x37b16f),
        'voice_object':(0x382b22,0x381e93,0x37b16f),
        'buffer_binding':(0x37be6c,0x37b16f)}[kind]
    for start in entries:
        directories=('sdk-construction-owners-base-v1','sdk-construction-state-base-v1','sdk-construction-state-dependencies-v1')
        paths=[ROOT/'analysis/audio-spatial-20261011'/d/f'{start:08x}.asm'for d in directories]
        path=next(p for p in paths if p.exists())
        for line in path.read_text().splitlines():
            fields=line.split();pc=int(fields[0],16);raw=bytes.fromhex(fields[1])
            assert raw==image.read(pc,len(raw))==read(pc,len(raw))
            verified.append(dict(pc=fields[0],bytes=fields[1]))
    report=dict(boundary=__doc__,kind=kind,image_sha256=sha(image_path.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),
        entry=f'{entry:08x}',owner=f'{owner:08x}',caller=f'{caller:08x}',registers={name:reg(name)for name in register_names},
        arguments=[f'{a:08x}'for a in arguments],regions=regions,unchanged_instructions=verified,
        reference_calls=[],saved_setting_calls=[],instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],
        supplied_callee_returns=[],native_or_physical_DSP_claimed=False)
    (directory/'entry.json').write_text(json.dumps(report,indent=2)+'\n')
    returning=bp(caller)
    reference=bp(0x37b16f) if kind in ('buffer_object','voice_object','buffer_binding') else None
    saved=bp(0x3739fa) if kind=='device_data' else None
    for _ in range(20):
        gdb.execute('continue');pc=reg('eip')
        if pc==caller:break
        if pc==0x37b16f:
            sp=reg('esp');target=word(sp+4);return_pc=word(sp)
            if target:
                table=word(target);method=word(table+4);backing(f'reference_target_{len(report["reference_calls"])}',target,8)
                call=dict(target=f'{target:08x}',method=f'{method:08x}',before_count=f'{word(target+4):08x}',
                    method_original8_hex=read(method,8).hex())
                assert read(method,8)==image.read(method,8)
            else:call=dict(target='00000000',method=None)
            reference.enabled=False;reach(return_pc);reference.enabled=True
            call.update(return_identity=f'{reg("eax"):08x}',after_count=f'{word(target+4):08x}' if target else None)
            assert reg('eax')==target
            report['reference_calls'].append(call)
        elif pc==0x3739fa:
            sp=reg('esp');return_pc=word(sp);saved.enabled=False;reach(return_pc)
            report['saved_setting_calls'].append(dict(return_value=f'{reg("eax"):08x}'));saved.enabled=True
        else:raise RuntimeError('Unexpected constructor observation stop '+hex(pc))
    else:raise RuntimeError('Constructor observation call limit')
    returning.delete()
    if reference:reference.delete()
    if saved:saved.delete()
    assert reg('esp')==stack+4+count*4,(kind,hex(reg('esp')),hex(stack))
    for r in regions:r['after_hex']=read(int(r['address'],16),r['size']).hex()
    report.update(actual_return_pc=f'{reg("eip"):08x}',actual_return_esp=f'{reg("esp"):08x}',
        actual_return_eax=f'{reg("eax"):08x}',original_whole_constructor_returned_normally=True)
    (directory/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    reports.append(dict(kind=kind,report_sha256=sha((directory/'report.json').read_bytes())))
    print(f'Original {kind} returned normally; {len(report["reference_calls"])} real reference calls; {len(regions)} retained regions',flush=True)

def capture_construction():
    for attempt in range(256):
        entries=list(remaining)[:4];observers=[bp(entry)for entry in entries]
        gdb.execute('continue');entry=reg('eip')
        assert entry in entries,hex(entry)
        for observer in observers:observer.delete()
        kind=remaining[entry][0]
        if kind in ('voice_object','buffer_binding'):
            data=word(reg('esp')+8) if kind=='voice_object' else word(reg('esp')+12)
            if not word(data+8)&0x10:
                skipped.append(dict(kind=kind,data=f'{data:08x}',flags=f'{word(data+8):08x}'))
                # Let the entry instruction execute before rearming the same
                # hardware address; no guest instruction is patched.
                gdb.execute('stepi');continue
        observe(entry);del remaining[entry]
        if not remaining:break
    else:raise RuntimeError('No complete spatial constructor set within256 observed entries')
    (output/'report.json').write_text(json.dumps(dict(boundary=__doc__,captures=reports,skipped_entries=skipped,
        original_cold_boot=True,instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],supplied_callee_returns=[]),indent=2)+'\n')

