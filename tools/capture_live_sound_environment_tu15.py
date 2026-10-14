"""Observe genuine TU1.5 environment/mapping callers and their real I/O.

Cold boot and complete original callees run normally in an isolated xemu.
Hardware breakpoints only observe state. No original code, guest memory,
register, service return, device, DSP result or source availability is supplied.
This is original Xbox code in an emulator, not physical-console PCM proof.
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
IMAGE=Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
assert hashlib.sha256(IMAGE.read_bytes()).hexdigest()==PIN
image=Xbe(IMAGE);inferior=gdb.selected_inferior()
out=Path(os.environ['H2_SOUND_ENVIRONMENT_OUTPUT']);out.mkdir(parents=True,exist_ok=False)
read=lambda a,n:bytes(inferior.read_memory(a,n))
word=lambda a:struct.unpack('<I',read(a,4))[0]
reg=lambda n:int(gdb.parse_and_eval('$'+n))&0xffffffff
GP=('eax','ecx','edx','ebx','esp','ebp','esi','edi')
entries=(0x1972c0,0x1973e0)
stack_anchor=None


def abi():
    tag=reg('ftag');status=reg('fstat')
    assert tag in (0,0xffff), f'Sparse x87 tags need their actual owner: {tag:04x}'
    floating=[gdb.parse_and_eval(f'$st{i}').bytes.hex() for i in range(8)]
    assert all(len(raw)==20 for raw in floating)
    xmm=[]
    for i in range(8):
        raw=gdb.parse_and_eval(f'$xmm{i}').bytes;assert len(raw)==16
        xmm.append(list(struct.unpack('<4I',raw)))
    return dict(gp=[reg(n)for n in GP],eflags=reg('eflags'),xmm=xmm,
        x87_depth=0 if tag==0xffff else 8,x87_status=status,x87_control=reg('fctrl'),mxcsr=reg('mxcsr'),
        x87_tag=tag,x87_logical_hex=floating)


def bank_state():
    regions=[]
    def retain(label,address,size,writable=True):
        assert address and size
        if any(r['address']<=address and address+size<=r['address']+r['size']for r in regions):return
        assert not any(address<r['address']+r['size'] and r['address']<address+size for r in regions)
        raw=read(address,size);assert len(raw)==size
        regions.append(dict(label=label,address=address,size=size,writable=writable,hex=raw.hex()))
    for name in ('.text','.rdata','.data'):
        s=image.section(name);retain(name,s.va,s.vsize,name=='.data')
    arena=word(0x51db00);assert arena;retain('sound_arena',arena,0x23000)
    assert stack_anchor is not None
    page=stack_anchor
    for n in range(-8,5):
        address=page+n*4096
        try:retain(f'stack{n:+}',address,4096)
        except gdb.MemoryError:pass
    environment=word(0x51df90)
    if environment:
        descriptor=word(environment)
        if descriptor:retain('effects_descriptor',descriptor,4)
    return sorted(regions,key=lambda r:r['address'])


def state():
    return dict(pc=reg('eip'),abi=abi(),regions=bank_state())


def breakpoint(pc):
    return gdb.Breakpoint(f'*0x{pc:x}',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)


def verify(entry):
    lines=(ROOT/'work/sandbox-audio-v7'/f'{entry:08x}.asm').read_text().splitlines()
    for line in lines:
        fields=line.split();pc=int(fields[0],16);raw=bytes.fromhex(fields[1])
        assert read(pc,len(raw))==raw==image.read(pc,len(raw)),f'Original instruction differs at {pc:08x}'
    return len(lines)


def request(entry):
    sp=reg('esp');return_pc=word(sp)
    sizes={0x2df851:1,0x2df86c:1,0x38594d:4,0x3856c6:5,0x38759e:4,
        0x38574f:2,0x3866c4:2,0x3857bf:4,0x386700:3,0x386724:3}
    args=list(struct.unpack('<'+'I'*sizes[entry],read(sp+4,sizes[entry]*4)))
    row=dict(entry=entry,return_pc=return_pc,arguments=args)
    if entry in (0x2df851,0x2df86c,0x38594d):
        assert read(args[0],9)==b'DSPImage\0'
    if entry==0x38594d:
        row['effect_locations']=list(struct.unpack('<2I',read(args[1],8)))
    if entry==0x3856c6:
        row['output_before_hex']=read(args[3],args[4]).hex()
    if entry==0x38759e:
        row['descriptor24_hex']=read(args[1],24).hex()
    if entry==0x3866c4:
        count,pairs=struct.unpack('<2I',read(args[1],8))
        row['mix_bins']=list(struct.unpack('<'+'I'*(count*2),read(pairs,count*8)))
    return row


def observe(entry):
    global stack_anchor
    stack_anchor=reg('esp')&~4095
    name={0x1972c0:'environment',0x1973e0:'mapping'}[entry]
    directory=out/name;directory.mkdir();parent_sp=reg('esp');parent_return=word(parent_sp)
    initial=state();(directory/'before.json').write_text(json.dumps(initial,separators=(',',':'))+'\n')
    verified=verify(entry)
    if entry==0x1973e0:verified+=verify(0x2275a0)
    returned=breakpoint(parent_return)
    expected=(0x2df851,0x38594d,0x2df86c,0x3856c6,0x3856c6,0x3856c6,0x3856c6)if name=='environment'else None
    calls=[]
    candidates=(0x3856c6,0x38759e,0x38574f,0x3866c4,0x3857bf,0x386700,0x386724)
    # Hardware has four slots. Mapping's external methods are observed in
    # source call order: first GetEffectData, then optional buffer construction,
    # then its volume/mix-bin/play. The next GetEffectData re-arms that sequence.
    stage='effect_data'
    for attempt in range(64):
        if expected:
            if len(calls)==len(expected):active=[]
            else:active=[breakpoint(expected[len(calls)])]
        else:
            targets={
                'effect_data':(0x3856c6,0x38759e),
                'buffer_create':(0x3856c6,0x38574f),
                'buffer_volume':(0x3866c4,),
                'buffer_mix':(0x3857bf,),
                'buffer_play':(0x3856c6,),
            }[stage]
            active=[breakpoint(p)for p in targets]
        gdb.execute('continue');pc=reg('eip')
        for b in active:b.delete()
        if pc==parent_return:break
        assert pc in candidates or (expected and pc==expected[len(calls)]),hex(pc)
        row=request(pc);row['id']=len(calls)+1
        row['before']=state()
        exiting=breakpoint(row['return_pc']);gdb.execute('continue')
        assert reg('eip')==row['return_pc'],f'Unexpected original return at {reg("eip"):08x}'
        exiting.delete();row['after']=state();row['return_value']=reg('eax')
        if pc==0x3856c6:
            a=row['arguments'];row['output_after_hex']=read(a[3],a[4]).hex()
        (directory/f'call-{row["id"]:02}.json').write_text(json.dumps(row,separators=(',',':'))+'\n')
        calls.append(dict(id=row['id'],entry=pc,return_value=row['return_value'],arguments=row['arguments']))
        if not expected:
            stage={0x3856c6:'effect_data',0x38759e:'buffer_create',0x38574f:'buffer_volume',
                   0x3866c4:'buffer_mix',0x3857bf:'buffer_play'}[pc]
        print(f'Original TU1.5 {name} I/O {len(calls)} entry={pc:08x} result={row["return_value"]:08x}',flush=True)
    else:raise RuntimeError('Original sound owner exceeded observation limit')
    returned.delete();final=state();(directory/'after.json').write_text(json.dumps(final,separators=(',',':'))+'\n')
    assert reg('esp')==parent_sp+(8 if name=='environment'else 4)
    if name=='environment':assert reg('eax')&255, 'Original environment constructor did not succeed naturally'
    report=dict(scope=__doc__,entry=entry,caller=parent_return,original_sha256=PIN,
        original_returned_normally=True,verified_original_instruction_rows=verified,calls=calls,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        original_code_patches=[],original_memory_writes=[],original_register_writes=[],supplied_callee_returns=[],
        native_or_physical_pcm_verified=False)
    (directory/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS original TU1.5 {name}: real caller returned, {len(calls)} actual I/O operations',flush=True)


def capture_environment():
    for entry in entries:
        stop=breakpoint(entry);gdb.execute('continue');assert reg('eip')==entry,hex(reg('eip'));stop.delete()
        observe(entry)
    (out/'report.json').write_text(json.dumps(dict(scope=__doc__,original_sha256=PIN,
        original_cold_boot=True,original_code_patches=[],original_memory_writes=[],original_register_writes=[],
        supplied_callee_returns=[],native_or_physical_pcm_verified=False),indent=2)+'\n')
