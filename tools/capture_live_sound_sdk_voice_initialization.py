"""Observe whole original voice-data initialization during a genuine cold boot.

Real allocator, format, bin, floating-point and kernel owners run normally.
Hardware breakpoints only observe inputs, actual allocation returns and final
objects. No guest code, memory, registers or callee results are written.
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
read=lambda a,n:bytes(inferior.read_memory(a,n))
reg=lambda n:int(gdb.parse_and_eval('$'+n))&0xffffffff
word=lambda a:struct.unpack('<I',read(a,4))[0]
output=Path(os.environ['H2_SOUND_VOICE_INIT_OUTPUT']);output.mkdir(parents=True,exist_ok=False)
register_names=('eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags','fctrl','fstat','ftag','mxcsr')
def bp(pc):return gdb.Breakpoint(f'*0x{pc:x}',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
def reach(pc):
    b=bp(pc);gdb.execute('continue');assert reg('eip')==pc,(hex(reg('eip')),hex(pc));b.delete()
def capture():
    reports=[];seen=set()
    for attempt in range(256):
        reach(0x37cef1);sp=reg('esp');descriptor=word(sp+4);flags=word(descriptor+4)
        kind='spatial'if flags&0x10 else'nonspatial'
        if kind in seen:gdb.execute('stepi');continue
        directory=output/kind;directory.mkdir();owner=reg('ecx');caller=word(sp);regions=[]
        def backing(name,address,size):
            if address and not any(int(r['address'],16)<=address and address+size<=int(r['address'],16)+r['size']for r in regions):
                assert size<=4*1024*1024,(name,hex(size))
                regions.append(dict(name=name,address=f'{address:08x}',size=size,before_hex=read(address,size).hex()))
        backing('data',owner,0xe0);backing('descriptor',descriptor,24)
        format_identity=0x385748 if flags&0x182000 else word(descriptor+0xc)
        # Extensible format includes the GUID; ordinary WAVEFORMATEX is18B.
        format_extent=40 if int.from_bytes(read(format_identity,2),'little')==0xfffe else 18
        backing('format',format_identity,format_extent)
        bins=word(descriptor+0x10)
        if bins:
            backing('bins',bins,8);backing('bin_entries',word(bins+4),word(bins)*8)
        originals=[]
        for folder in ('sdk-voice-initialization-base-v1','sdk-voice-initialization-dependencies-v1'):
            for p in (ROOT/'analysis/audio-spatial-20261011'/folder).glob('*.asm'):
                for line in p.read_text().splitlines():
                    pc,raw,*_=line.split();raw=bytes.fromhex(raw)
                    assert image.read(int(pc,16),len(raw))==read(int(pc,16),len(raw))
                    originals.append(dict(pc=pc,bytes=raw.hex()))
        report=dict(boundary=__doc__,kind=kind,entry='0037cef1',owner=f'{owner:08x}',
            arguments=[f'{descriptor:08x}'],registers={n:reg(n)for n in register_names},regions=regions,
            image_sha256=sha(image_path.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),
            unchanged_instructions=originals,allocations=[],instruction_patches=[],guest_memory_writes=[],
            guest_register_writes=[],supplied_callee_returns=[],native_or_physical_DSP_claimed=False)
        (directory/'entry.json').write_text(json.dumps(report,indent=2)+'\n')
        returning=bp(caller);allocation=bp(0x37d952)
        for _ in range(16):
            gdb.execute('continue');pc=reg('eip')
            if pc==caller:break
            assert pc==0x37d952,hex(pc)
            asp=reg('esp');args=list(struct.unpack('<III',read(asp+4,12)));return_pc=word(asp)
            allocation.enabled=False;reach(return_pc);allocation.enabled=True
            identity=reg('eax');name=f'allocation_{len(report["allocations"])}'
            backing(name,identity,args[1])
            report['allocations'].append(dict(arguments=[f'{a:08x}'for a in args],identity=f'{identity:08x}',name=name))
        else:raise RuntimeError('Voice initialization allocation limit')
        returning.delete();allocation.delete();assert reg('esp')==sp+8
        for r in regions:r['after_hex']=read(int(r['address'],16),r['size']).hex()
        report.update(actual_return_eax=f'{reg("eax"):08x}',actual_return_pc=f'{reg("eip"):08x}',original_whole_initializer_returned_normally=True)
        (directory/'report.json').write_text(json.dumps(report,indent=2)+'\n');seen.add(kind)
        reports.append(dict(kind=kind,report_sha256=sha((directory/'report.json').read_bytes())))
        print(f'Original {kind} initialized normally; {len(report["allocations"])} real allocations; {len(regions)} regions',flush=True)
        if len(seen)==2:break
    else:raise RuntimeError('No complete spatial/nonspatial initializers observed')
    (output/'report.json').write_text(json.dumps(dict(boundary=__doc__,captures=reports,original_cold_boot=True,
        instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],supplied_callee_returns=[]),indent=2)+'\n')
