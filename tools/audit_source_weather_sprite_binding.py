#!/usr/bin/env python3
"""Replay unchanged143C0 over the actual391F0 sprite/input cache banks.

Only a fresh return address and the literal393AD..393B7 caller inputs are
supplied. Readiness/cache/pool/header state remains the immutable entry export.
No original instruction, callee, allocation, wait, I/O, device or result is
replaced. Raw GPU physical bytes are captured RAM, not a new GPU readback.
"""
from pathlib import Path
import argparse,collections,hashlib,json,re,struct,sys
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,words,packed
from audit_rendering import Cache
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
SYNTH,STACK,STOP=0x20000000,0x200fe000,0x200ff000

def sha(raw):return hashlib.sha256(raw).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot',type=Path,default=ROOT/'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/weather-sprite-particle-entry.json')
    parser.add_argument('--return-snapshot',type=Path,default=ROOT/'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-return')
    args=parser.parse_args();args.snapshot=args.snapshot.resolve();args.output=args.output.resolve()
    s=Snapshot(args.snapshot);image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
    registers=s.report['registers_before'];assert int(re.search(r'EIP=([0-9a-fA-F]+)',registers)[1],16)==0x391f0
    captured_sp=int(re.search(r'ESP=([0-9a-fA-F]+)',registers)[1],16)
    system=uint(s.read(captured_sp+4,4));system_raw=s.read(system,140);bitmap=uint(system_raw,4)
    table=uint(s.read(0x4e3b44,4));tag_address=table+(bitmap&65535)*16
    tag=s.read(tag_address,16);root=uint(tag,8);definition=s.read(root,84)
    assert uint(definition,0x44)>0 and bitmap!=0xffffffff
    record=uint(definition,0x48);record_raw=s.read(record,116)
    cache_pool=uint(s.read(0x4e6454,4));image_rows=uint(s.read(cache_pool+0x44,4))
    manager=uint(s.read(0x4e6464,4));physical_pool=uint(s.read(manager+0x64,4));physical_rows=uint(s.read(physical_pool+0x44,4))
    outputdir=args.output.parent/(args.output.stem+'-inputs');outputdir.mkdir(parents=True,exist_ok=True)
    banks=[]
    def save(name,address,raw,space='CPU0 virtual'):
        path=outputdir/(name+'.bin');path.write_bytes(raw)
        banks.append(dict(name=name,address=f'{address:08X}',bytes=len(raw),sha256=sha(raw),space=space,file=str(path.relative_to(ROOT))))
    for name,address,raw in [('system',system,system_raw),('bitmap-tag-row',tag_address,tag),('bitmap-definition',root,definition),
        ('bitmap-runtime-record',record,record_raw),('full-view-485600',0x485600,s.read(0x485600,0x298)),
        ('image-pool',cache_pool,s.read(cache_pool,76)),('physical-manager',manager,s.read(manager,112)),
        ('physical-pool',physical_pool,s.read(physical_pool,76)),('entry-stage-bindings',0x51f3d8,s.read(0x51f3d8,16)),
        ('entry-stage-dimensions',0x485af4,s.read(0x485af4,32))]:save(name,address,raw)
    partitions=[]
    for i in range(3):
        handle=uint(record_raw,0x28+i*4);row=dict(partition=i,handle=f'{handle:08X}',
            authored_resource_pointer=f'{uint(record_raw,0x1c+i*4):08X}',authored_resource_size=uint(record_raw,0x34+i*4))
        if handle!=0xffffffff:
            slot=handle&65535;at=image_rows+slot*40;raw=s.read(at,40);physical_at=physical_rows+slot*24;physical=s.read(physical_at,24)
            save(f'partition-{i}-image-row',at,raw);save(f'partition-{i}-physical-row',physical_at,physical)
            fmt=uint(raw,0x20);header=words(raw[20:40]);data=header[1]&0x0fffffff
            row.update(slot=slot,ready=raw[3],image_row=f'{at:08X}',header_pointer=f'{at+20:08X}',
                raw_header_words=[f'{v:08X}'for v in header],physical_row=f'{physical_at:08X}',
                last_use=uint(physical,20),decoded_header=dict(width=1<<((fmt>>20)&15),height=1<<((fmt>>24)&15),
                    depth=1<<((fmt>>28)&15),mips=(fmt>>16)&15,dimensions=(fmt>>4)&15,hardware_format=(fmt>>8)&255))
            n=uint(record_raw,0x34+i*4);assert data+n<=len(s.ram)
            gpu_ram=s.ram[data:data+n];save(f'partition-{i}-captured-physical-pixels',data,gpu_ram,'captured physical RAM')
            row['captured_physical_bytes_sha256']=sha(gpu_ram)
        partitions.append(row)
    c=Cache(ROOT/'assets/disc/maps/lockout.map')
    try:
        authored_tag,authored_root=c.tag(bitmap,'bitm');authored_record=c.pointer(c.u32(authored_root+0x48),c.magic,116)
        authored_raw=bytes(c.data[authored_record:authored_record+116])
        save('authored-bitmap-record',authored_record,authored_raw,'immutable lockout.map file')
        for p in partitions:
            if 'captured_physical_bytes_sha256'not in p:continue
            location=int(p['authored_resource_pointer'],16);assert location>>30==0
            n=p['authored_resource_size'];c.require(location,n);resource=bytes(c.data[location:location+n])
            save(f"partition-{p['partition']}-authored-resource",location,resource,'immutable lockout.map file')
            p['authored_resource_sha256']=sha(resource);p['captured_physical_resource_exact']=sha(resource)==p['captured_physical_bytes_sha256']
        authored_name=authored_tag['name']
    finally:c.close()
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000)
    pages,dirty,seen,calls,writes,boundaries={},set(),{},collections.Counter(),[],[];count=[0]
    allowed=[(record,record+116),(0x51f3d8,0x51f3e8),(0x485af4,0x485b14)]
    for p in partitions:
        if 'physical_row'in p:
            at=int(p['physical_row'],16);allowed.append((at,at+24))
    def page(address):
        at=address&~4095
        if at not in pages:
            raw=s.read(at,4096);u.mem_map(at,4096);u.mem_write(at,raw);pages[at]=raw
    def read(address,size):
        for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):
            if not SYNTH<=at<SYNTH+0x100000:page(at)
        return bytes(u.mem_read(address,size))
    def missing(g,access,address,size,value,_):
        try:read(address,size);return True
        except ValueError as error:
            boundaries.append(dict(kind='uncaptured/MMIO memory; not replaced',pc=f'{g.reg_read(UC_X86_REG_EIP):08X}',address=f'{address:08X}',reason=str(error)));g.emu_stop();return False
    def code(g,pc,size,_):
        if pc==STOP:g.emu_stop();return
        try:raw=image.read(pc,size)
        except(ValueError,KeyError):
            boundaries.append(dict(kind='actual external instruction not executed',pc=f'{pc:08X}'));g.emu_stop();return
        assert read(pc,size)==raw,f'original instruction changed at{pc:X}'
        count[0]+=1;seen[pc]=size
        if pc in[0x143c0,0x137550,0x12310,0x12ccf0,0x12ce00,0x12d5a0,0x120ce0]:calls[f'{pc:08X}']+=1
    def write(g,access,address,size,value,_):
        if SYNTH<=address<SYNTH+0x100000:return
        assert any(at<=address and address+size<=end for at,end in allowed),(hex(g.reg_read(UC_X86_REG_EIP)),hex(address),size)
        writes.append(dict(pc=f'{g.reg_read(UC_X86_REG_EIP):08X}',address=f'{address:08X}',bytes=size,
            before=read(address,size).hex(),after=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
        dirty.update(range(address&~4095,((address+size-1)&~4095)+4096,4096))
    u.hook_add(UC_HOOK_MEM_UNMAPPED,missing);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write)
    ga,gl=(int(v,16)for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',registers).groups())
    read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
    for reg,value in[(UC_X86_REG_CS,8),(UC_X86_REG_SS,16),(UC_X86_REG_DS,16),(UC_X86_REG_ES,16),(UC_X86_REG_FS,32)]:u.reg_write(reg,value)
    for reg,name in[(UC_X86_REG_EAX,'EAX'),(UC_X86_REG_EBX,'EBX'),(UC_X86_REG_ECX,'ECX'),(UC_X86_REG_EDX,'EDX'),(UC_X86_REG_ESI,'ESI'),(UC_X86_REG_EDI,'EDI'),(UC_X86_REG_EBP,'EBP')]:
        u.reg_write(reg,int(re.search(name+r'=([0-9a-fA-F]+)',registers)[1],16))
    # These are the exact393AD..393B7 instructions, not a fitted service result.
    u.reg_write(UC_X86_REG_EAX,bitmap);u.reg_write(UC_X86_REG_ECX,0);u.reg_write(UC_X86_REG_XMM2,0)
    u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2)
    u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1fa0)
    u.mem_write(STACK,packed([STOP,3]))
    try:u.emu_start(0x143c0,STOP,count=2000000)
    except UcError:
        if not boundaries:raise
    complete=not boundaries and u.reg_read(UC_X86_REG_EIP)==STOP
    if complete:assert u.reg_read(UC_X86_REG_ESP)==STACK+8
    selected=uint(read(0x51f3d8+12,4));selected_header=words(read(selected,20))if selected else[]
    for name,address,n in [('after-runtime-record',record,116),('after-stage-bindings',0x51f3d8,16),('after-stage-dimensions',0x485af4,32)]:save(name,address,read(address,n))
    before_stage=[f'{v:08X}'for v in words(s.read(0x51f3d8,16))]
    result=dict(completed=complete,AL=(u.reg_read(UC_X86_REG_EAX)&255)if complete else None,
        selected_header_pointer=f'{selected:08X}',selected_header_words=[f'{v:08X}'for v in selected_header],
        before_stage_words=before_stage,after_stage_words=[f'{v:08X}'for v in words(read(0x51f3d8,16))],
        after_stage_dimensions=words(read(0x485af4,32)),before_cached_until=uint(record_raw,112),
        after_cached_until=uint(read(record,116),112),source_calls=dict(calls),source_writes=writes,
        verified_distinct_instructions=len(seen),executed_instructions=count[0],boundaries=boundaries,
        source_instruction_sha256=sha(b''.join(packed([pc])+image.read(pc,n)for pc,n in sorted(seen.items()))),
        source_instructions_patched=[],services_replaced=[],readiness_result_supplied=False)
    actual_return=None
    if args.return_snapshot:
        args.return_snapshot=args.return_snapshot.resolve()
        returned=Snapshot(args.return_snapshot)
        assert int(re.search(r'EIP=([0-9a-fA-F]+)',returned.report['registers_before'])[1],16)==uint(s.read(captured_sp,4))==0x36f00
        compared=[]
        # These words are written by143C0 and remain unchanged through the
        # later real device binding/draw/cleanup in the captured whole call.
        for address,n in [(0x51f3d8,16),(0x485af4,32),(record+0x70,4),(record+0x50,4)]:
            expected=read(address,n);observed=returned.read(address,n)
            assert expected==observed
            compared.append(dict(address=f'{address:08X}',bytes=n,words=[f'{v:08X}'for v in words(observed)]))
        for p in partitions:
            if 'physical_row'in p:
                address=int(p['physical_row'],16)+20
                assert read(address,4)==returned.read(address,4)
                compared.append(dict(address=f'{address:08X}',bytes=4,words=[f'{uint(returned.read(address,4)):08X}']))
        observed_header=returned.read(selected,20)
        assert observed_header[4:]==packed(selected_header)[4:]
        for name,address,n in [('return-selected-sprite-header',selected,20),
            ('return-stage-bindings',0x51f3d8,16),('return-device-committed-textures',0x51f3c8,16),
            ('return-camera-c0-c11',0x407738,192),('entry-camera-c0-c11',0x407738,192),
            ('return-point-retained-state',0x407668,32)]:
            raw=(s if name.startswith('entry-')else returned).read(address,n)
            save(name,address,raw)
        actual_return=dict(snapshot=str(args.return_snapshot.relative_to(ROOT)),snapshot_ram_sha256=sha(returned.ram),
            snapshot_manifest_sha256=sha((args.return_snapshot/'export.json').read_bytes()),captured_PC='00036F00',
            verified_unchanged_after_143C0=compared,
            selected_header_words=[f'{v:08X}'for v in words(observed_header)],
            selected_common_entry=f'{selected_header[0]:08X}',selected_common_return=f'{uint(observed_header):08X}',
            common_scope='Later complete1CF80/SDK binding changes Common flags;143C0 does not perform this mutation. Other four header words are exact.',
            staged_pushbuffer_not_presented_image=True)
    for at in dirty:u.mem_write(at,pages[at])
    for at,raw in pages.items():assert bytes(u.mem_read(at,4096))==raw
    result['restored_pages_checked']=len(pages)
    report=dict(scope=__doc__,snapshot=str(args.snapshot.relative_to(ROOT)),snapshot_ram_sha256=sha(s.ram),
        snapshot_manifest_sha256=sha((args.snapshot/'export.json').read_bytes()),source_xbe_sha256=PIN,tool_sha256=sha(Path(__file__).read_bytes()),
        captured_PC='000391F0',actual_system=f'{system:08X}',sprite=dict(datum=f'{bitmap:08X}',name=authored_name,
            definition=f'{root:08X}',record=f'{record:08X}',index=0,raw_record_hex=record_raw.hex(),
            authored_dimensions=list(struct.unpack_from('<3h',record_raw,4)),authored_type=struct.unpack_from('<h',record_raw,10)[0],
            authored_format=struct.unpack_from('<h',record_raw,12)[0],authored_flags=f'{struct.unpack_from("<H",record_raw,14)[0]:04X}',
            authored_mipmap_count=struct.unpack_from('<h',record_raw,20)[0],partitions=partitions),
        bitmap_globals={f'{at:08X}':s.read(at,n).hex()for at,n in[(0x4e647b,1),(0x468841,1),(0x4e647c,4),(0x4e6488,4)]},
        manager_use_clock=uint(s.read(manager+0x38,4)),binding_replay=result,actual_return=actual_return,raw_banks=banks,
        limitations=['Fresh whole143C0 binding uses actual391F0 entry residency with the literal caller inputs; it does not replay the intervening SDK setup or claim their historical side effects.',
            'Entry stage3 belongs to the preceding plate draw. Only replay output claims the sprite stage3 binding.',
            'Captured physical sprite bytes match the immutable authored partition. No new GPU readback/filter/raster/physicalXbox appearance is claimed.',
            'This single ready source bank does not create native allocation/residency/scheduling history.'],guest_IO=False,GPU=False,production_activation=False)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(sprite=report['sprite'],binding=result),indent=2))

if __name__=='__main__':main()
