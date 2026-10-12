"""Observe whole original382F58 voice initialization during cold boot.

Original dirty/virtual format dispatch, channel packing, slot allocation,
activation, kernel locks, FIFO and device writes run to normal returns.
Hardware breakpoints only observe actual state and device words. No guest
code, memory, registers, FIFO result or callee return is supplied. These
captures are Xbox-in-xemu evidence, not native output or physical DSP proof.
"""
import hashlib,json,os,struct,sys
from pathlib import Path
import gdb
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
image_path=ROOT/'assets/disc/default.xbe'
sha=lambda raw:hashlib.sha256(raw).hexdigest()
assert sha(image_path.read_bytes())=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image=Xbe(image_path);inferior=gdb.selected_inferior()
read=lambda a,n:bytes(inferior.read_memory(a,n))
reg=lambda n:int(gdb.parse_and_eval('$'+n))&0xffffffff
word=lambda a:struct.unpack('<I',read(a,4))[0]
def breakpoint(pc):return gdb.Breakpoint(f'*0x{pc:x}',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
def reach(pc):
    b=breakpoint(pc);gdb.execute('continue');assert reg('eip')==pc,(hex(reg('eip')),hex(pc));b.delete()
output=Path(os.environ['H2_SOUND_VOICE_DISPATCH_OUTPUT']);output.mkdir(parents=True,exist_ok=False)
commands=[(0x381b43,0xfe8202f8,'ecx'),(0x381b53,0xfe820304,'ecx'),
    (0x381b5c,0xfe820318,'ecx'),(0x381b65,0xfe820308,'ecx'),
    (0x381b6e,0xfe82030c,'ecx'),(0x381b77,0xfe820310,'ecx'),
    (0x381b80,0xfe820314,'ecx'),(0x381b89,0xfe820350,'ecx'),
    (0x381b92,0xfe82036c,'ecx'),(0x381b9b,0xfe820370,'ecx'),
    (0x381ba4,0xfe820374,'ecx'),(0x381bad,0xfe820378,'ecx'),
    (0x381bb6,0xfe820300,'ecx'),(0x381bc0,0xfe820360,'ecx'),
    (0x381bca,0xfe820364,'ecx'),(0x381bd4,0xfe820368,'ecx'),
    (0x381bdd,0xfe82037c,'ecx'),(0x381be3,0xfe82031c,0xffff)]
def capture():
    seen=set();reports=[]
    for attempt in range(512):
        reach(0x382f58);voice=reg('ecx');data=word(voice+0x70);flags=word(data+8)
        kind='spatial'if flags&0x200010 else'nonspatial'
        if kind in seen or read(voice+0x12,1)[0]&1 or flags&0x40000:
            gdb.execute('stepi');continue
        directory=output/kind;directory.mkdir();stack=reg('esp');caller=word(stack)
        regions=[]
        def backing(name,address,size):
            if address and not any(int(r['address'],16)<=address and address+size<=int(r['address'],16)+r['size']for r in regions):
                regions.append(dict(name=name,address=f'{address:08x}',size=size,before_hex=read(address,size).hex()))
        device=word(voice+8)
        backing('voice',voice,0x158);backing('data',data,0xe0);backing('device',device,0x540)
        backing('source',word(data+0xb4),0xa4);backing('parameters',word(data+0xb8),0x38)
        backing('ordinary_count',word(device+0x58),4);backing('spatial_count',word(device+0x5c),4)
        backing('voice_mapping_pointer',0x3872c0,4);backing('speaker_config',0x386b04,4)
        parent=word(data+0xb0);backing('parent_link',parent,0x24)
        if parent:
            owner=word(parent+0x20);backing('parent_owner',owner,0x84)
            if owner:
                pd=word(owner+0x80);backing('parent_data',pd,0xe0);backing('parent_parameters',word(pd+0xb8),0x38)
        originals=[]
        for folder in ('sdk-hardware-voice-dependencies-base-v1','sdk-hardware-voice-deeper-base-v1','sdk-hardware-voice-dispatch-base-v1','sdk-hardware-initialization-base-v1'):
            for p in (ROOT/'analysis/audio-spatial-20261011'/folder).glob('*.asm'):
                for line in p.read_text().splitlines():
                    pc,raw,*_=line.split();encoded=bytes.fromhex(raw)
                    assert image.read(int(pc,16),len(encoded))==read(int(pc,16),len(encoded))
                    originals.append(dict(pc=pc,bytes=raw))
        report=dict(boundary=__doc__,kind=kind,entry='00382f58',voice=f'{voice:08x}',
            registers={n:reg(n)for n in ('eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags','fctrl','fstat','ftag','mxcsr')},
            image_sha256=sha(image_path.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),regions=regions,
            unchanged_instructions=originals,selected_sdk_table_hex=read(0x3871c8,44).hex(),
            observed_device_writes=[],instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],
            supplied_callee_returns=[],native_or_physical_DSP_claimed=False)
        (directory/'entry.json').write_text(json.dumps(report,indent=2)+'\n')
        reach(0x381aa9)
        report['after_format_hex']=read(voice,0x158).hex()
        reach(0x37e737);allocation_stack=reg('esp');assert word(allocation_stack)==0x381ad8
        report['slot_allocation']=dict(device=f'{reg("ecx"):08x}',voice=f'{word(allocation_stack+4):08x}')
        reach(0x381ad8);assert reg('esp')==allocation_stack+8
        result=reg('eax');report['slot_allocation']['result']=f'{result:08x}'
        assert result==0,'Expected a genuine successful fresh activation'
        count=read(voice+0x64,1)[0];assert 1<=count<=3,count
        mapping=word(0x3872c0)
        #381A92 maps four16B records beginning at (first_slot*4+2).
        # This is distinct from the number of activated hardware voices.
        first_slot=int.from_bytes(read(voice+0xc,2),'little')
        backing('voice_mapping',mapping+(first_slot*4+2)*16,64)
        def pending(pc,address,value):
            reach(pc);report['observed_device_writes'].append(dict(pc=f'{pc:08x}',address=f'{address:08x}',value=f'{reg(value)if isinstance(value,str)else value:08x}'))
        for index in range(count):
            for pc,address,value in commands:pending(pc,address,value)
        if flags&0x200010:
            reach(0x3812e0);hrtf_stack=reg('esp');assert word(hrtf_stack)==0x381c14
            pending(0x38139f,0xfe820160,'eax')
            for index in range(15):
                reach(0x3813ec);assert reg('ebx')==0xfe820400+index*4
                report['observed_device_writes'].append(dict(pc='003813ec',address=f'{reg("ebx"):08x}',value=f'{reg("edx"):08x}'))
            for pc,address in ((0x381427,0xfe82043c),(0x381433,0xfe8202f8),(0x38143e,0xfe82031c)):pending(pc,address,'eax')
            reach(0x381c14);assert reg('esp')==hrtf_stack+12
        reach(caller);assert reg('esp')==stack+4 and reg('eax')==0
        for region in regions:region['after_hex']=read(int(region['address'],16),region['size']).hex()
        report.update(actual_return_pc=f'{caller:08x}',actual_return_eax=f'{reg("eax"):08x}',original_whole_initialization_returned_normally=True,device_writes_executed_normally=True)
        (directory/'report.json').write_text(json.dumps(report,indent=2)+'\n');seen.add(kind)
        reports.append(dict(kind=kind,report_sha256=sha((directory/'report.json').read_bytes())))
        print(f'Original {kind} voice initialized normally; {count} slots; {len(report["observed_device_writes"])} device writes',flush=True)
        if len(seen)==2:break
    else:raise RuntimeError('No complete spatial/nonspatial voice initializers in512 entries')
    (output/'report.json').write_text(json.dumps(dict(boundary=__doc__,captures=reports,original_cold_boot=True,instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],supplied_callee_returns=[]),indent=2)+'\n')
