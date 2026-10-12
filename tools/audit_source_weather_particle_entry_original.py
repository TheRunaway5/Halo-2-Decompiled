#!/usr/bin/env python3
"""Execute actual captured391F0 on an owned immutable RAM clone to a real boundary.

Incoming general/XMM registers, flags, empty x87 state and the original stack
argument are retained. Only the return address is replaced with a stop token.
Every reached title/SDK instruction is checked against the pinned XBE; kernel,
uncaptured MMIO, unsupported aliasing and emulator boundaries stop without a
supplied return. No session, device, allocation, wait or I/O result is supplied.
"""
from pathlib import Path
import argparse, collections, csv, hashlib, json, re, struct, sys
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words, packed
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe, FUNCTIONS_CSV


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def replay(snapshot, image, output, return_snapshot=None):
    registers = snapshot.report['registers_before']
    captured_pc = int(re.search(r'EIP=([0-9a-fA-F]+)', registers)[1], 16)
    if captured_pc != 0x391f0:
        return dict(executed=False, captured_PC=f'{captured_pc:08X}',
            reason='Only actual captured391F0 entry is admitted; no active view is invented.')
    assert sha(snapshot.ram) == snapshot.report['physical_ram_sha256']
    assert snapshot.report['registers_before'] == snapshot.report['registers_after']
    assert sha(image.data) == PIN
    esp = int(re.search(r'ESP=([0-9a-fA-F]+)', registers)[1], 16)
    system = uint(snapshot.read(esp + 4, 4))
    entry_stack = snapshot.read(esp, 32)
    function_rows = {int(row['va'], 16): row for row in csv.DictReader(Path(FUNCTIONS_CSV).open())}
    function_extents = [(at, at+int(row['size'])) for at,row in function_rows.items()]
    stop = 0x220ff000
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(stop, 4096)
    pages, physical_pages, dirty, seen, calls, changes, boundaries, checkpoints, payloads = {}, {}, set(), {}, [], [], [], [], {}
    total_steps = [0]

    def boundary(kind, **extra):
        boundaries.append(dict(kind=kind, pc=f'{u.reg_read(UC_X86_REG_EIP):08X}',
            registers={name: f'{u.reg_read(reg):08X}' for name, reg in [('EAX',UC_X86_REG_EAX),
                ('EBX',UC_X86_REG_EBX),('ECX',UC_X86_REG_ECX),('EDX',UC_X86_REG_EDX),
                ('ESI',UC_X86_REG_ESI),('EDI',UC_X86_REG_EDI),('EBP',UC_X86_REG_EBP),('ESP',UC_X86_REG_ESP)]},
            **extra))
        u.emu_stop()

    def page(address):
        at = address & ~4095
        if at not in pages:
            physical = snapshot.physical(at)
            if physical in physical_pages and physical_pages[physical] != at:
                raise ValueError(f'unproved virtual alias {at:08X} to captured physical {physical:08X}, already mapped at{physical_pages[physical]:08X}')
            raw = snapshot.read(at, 4096)
            u.mem_map(at, 4096)
            u.mem_write(at, raw)
            pages[at] = raw
            physical_pages[physical] = at

    def read(address, size):
        for at in range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096):
            if at != stop:
                page(at)
        return bytes(u.mem_read(address, size))

    def missing(g, access, address, size, value, _):
        try:
            read(address, size)
            return True
        except ValueError as error:
            boundary('uncaptured memory/MMIO or unproved virtual alias; not replaced',
                access=access, address=f'{address:08X}', bytes=size, reason=str(error))
            return False

    def checkpoint(pc):
        device = uint(read(0x407488, 4))
        checkpoints.append(dict(pc=f'{pc:08X}', device=f'{device:08X}',
            dirty=f'{uint(read(0x407490,4)):08X}',
            point_words=[f'{v:08X}' for v in words(read(0x407668,32))],
            command_cursor=f'{uint(read(device,4)):08X}', command_limit=f'{uint(read(device+4,4)):08X}',
            device_flags=f'{uint(read(device+8,4)):08X}'))

    def code(g, pc, size, _):
        if pc == stop:
            g.emu_stop()
            return
        # A dynamic shader upload, title assert or kernel call is not an
        # invented success: only unchanged executable bytes may be executed.
        section = next((s for s in image.sections if s.va <= pc and pc + size <= s.va + s.rsize and s.flags & 4), None)
        if section is None:
            boundary('actual external/kernel or non-executable-title instruction; not executed')
            return
        source = image.read(pc, size)
        assert read(pc, size) == source, f'changed original instruction {pc:08X}'
        seen[pc] = size
        total_steps[0] += 1
        if pc in function_rows:
            calls.append(dict(pc=f'{pc:08X}', source=function_rows[pc]['source'],
                return_address=f'{uint(read(g.reg_read(UC_X86_REG_ESP),4)):08X}'))
        if pc in {0x3f7330,0x3fa3d0}:
            pointer=g.reg_read(UC_X86_REG_EDX)
            count=uint(read(g.reg_read(UC_X86_REG_ESP)+4,4)) if pc==0x3f7330 else 60
            assert count == (48 if pc==0x3f7330 else 60)
            payloads['vertex_constants_c0_c11' if pc==0x3f7330 else 'pixel_definition_39780']=dict(
                callee=f'{pc:08X}',address=f'{pointer:08X}',word_count=count,
                words=[f'{v:08X}' for v in words(read(pointer,count*4))],
                sha256=sha(read(pointer,count*4)))
        if pc in {0x391f0,0x39217,0x39279,0x393bc,0x393cd,0x39436,0x39467,0x396cd,0x39713,0x3976e,
            0x3ff920,0x403400,0x400fa0}:
            checkpoint(pc)
        # GPU command allocation/pump may reach MMIO even when the hardware
        # registers happen to have CPU page-table entries. The read mapper
        # rejects those uncaptured addresses; no zero register is supplied.

    def writing(g, access, address, size, value, _):
        if address == stop:
            boundary('unexpected write to diagnostic stop storage', address=f'{address:08X}')
            return
        try:
            previous = read(address, size)
        except ValueError as error:
            boundary('uncaptured write/MMIO or unproved virtual alias; not replaced',
                address=f'{address:08X}', bytes=size, reason=str(error))
            return
        if any(at < address+size and address < end for at,end in function_extents):
            raise AssertionError(f'source code mutation attempted at{address:08X}')
        dirty.update(range(address & ~4095, ((address + size - 1) & ~4095) + 4096, 4096))
        changes.append(dict(pc=f'{g.reg_read(UC_X86_REG_EIP):08X}', address=f'{address:08X}', bytes=size,
            before=previous.hex(), after=(value & ((1 << (size * 8)) - 1)).to_bytes(size,'little').hex(),
            original_stack_write=esp - 0x10000 <= address < esp + 0x1000))

    u.hook_add(UC_HOOK_MEM_UNMAPPED, missing)
    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, writing)
    ga, gl = (int(v, 16) for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)', registers).groups())
    read(ga, gl+1)
    u.reg_write(UC_X86_REG_GDTR, (0, ga, gl, 0))
    for reg, value in [(UC_X86_REG_CS,8),(UC_X86_REG_SS,16),(UC_X86_REG_DS,16),
        (UC_X86_REG_ES,16),(UC_X86_REG_FS,32)]:
        u.reg_write(reg,value)
    for reg, name in [(UC_X86_REG_EAX,'EAX'),(UC_X86_REG_EBX,'EBX'),(UC_X86_REG_ECX,'ECX'),
        (UC_X86_REG_EDX,'EDX'),(UC_X86_REG_ESI,'ESI'),(UC_X86_REG_EDI,'EDI'),(UC_X86_REG_EBP,'EBP')]:
        u.reg_write(reg,int(re.search(name+r'=([0-9a-fA-F]+)',registers)[1],16))
    u.reg_write(UC_X86_REG_ESP,esp)
    u.reg_write(UC_X86_REG_EFLAGS,int(re.search(r'EFL=([0-9a-fA-F]+)',registers)[1],16))
    fpcw = int(re.search(r'FCW=([0-9a-fA-F]+)',registers)[1],16)
    fpsw = int(re.search(r'FSW=([0-9a-fA-F]+)',registers)[1],16)
    ftw = int(re.search(r'FTW=([0-9a-fA-F]+)',registers)[1],16)
    assert ftw == 0, 'Only the captured empty x87 stack is admitted.'
    u.reg_write(UC_X86_REG_FPCW,fpcw)
    u.reg_write(UC_X86_REG_FPSW,fpsw)
    u.reg_write(UC_X86_REG_FPTAG,0xffff)
    mxcsr = int(re.search(r'MXCSR=([0-9a-fA-F]+)',registers)[1],16)
    u.reg_write(UC_X86_REG_MXCSR,mxcsr)
    for i in range(8):
        a,b = re.search(r'XMM'+f'{i:02d}'+r'=([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',registers).groups()
        u.reg_write(globals()['UC_X86_REG_XMM'+str(i)],int(a+b,16))
    read(esp,32)
    u.mem_write(esp,packed([stop]))
    dirty.add(esp & ~4095)
    try:
        u.emu_start(0x391f0,stop,count=2000000)
    except UcError as error:
        if not boundaries:
            boundary('emulator execution limitation; no source return supplied', reason=str(error))
    complete = not boundaries and u.reg_read(UC_X86_REG_EIP) == stop
    if not complete and not boundaries:
        boundary('diagnostic instruction budget exhausted; no source return supplied')
    if complete:
        assert u.reg_read(UC_X86_REG_ESP) == esp + 8
    raw_dir = output.parent / (output.stem+'-clone-inputs')
    raw_dir.mkdir(exist_ok=True)
    mutations = {}
    for at in sorted(dirty):
        before, after = pages[at], bytes(u.mem_read(at,4096))
        raw_dir.joinpath(f'{at:08X}-before.bin').write_bytes(before)
        raw_dir.joinpath(f'{at:08X}-after.bin').write_bytes(after)
        mutations[f'{at:08X}'] = dict(before_sha256=sha(before),after_sha256=sha(after),
            before_path=str(raw_dir.joinpath(f'{at:08X}-before.bin').relative_to(ROOT)),
            after_path=str(raw_dir.joinpath(f'{at:08X}-after.bin').relative_to(ROOT)),
            differing_bytes=sum(a!=b for a,b in zip(before,after)))
    for name,value in payloads.items():
        raw=packed([int(v,16) for v in value['words']])
        path=raw_dir.joinpath(name+'.bin')
        path.write_bytes(raw)
        value['path']=str(path.relative_to(ROOT))
    packet_report=None
    if complete:
        start=int(checkpoints[0]['command_cursor'],16)
        end=int(checkpoints[-1]['command_cursor'],16)
        assert 0 <= end-start <= int(checkpoints[0]['command_limit'],16)-start
        raw=read(start,end-start)
        path=raw_dir/'appended-command-span.bin'
        path.write_bytes(raw)
        packet_words=words(raw)
        i,packets=0,[]
        while i<len(packet_words):
            header=packet_words[i]
            count=(header>>18)&2047
            assert not header&0xe0030000 and count>0 and i+1+count<=len(packet_words)
            packets.append(dict(word_offset=i,address=f'{start+i*4:08X}',header=f'{header:08X}',
                method=f'{header&8191:04X}',count=count,words=[f'{v:08X}' for v in packet_words[i+1:i+1+count]]))
            i+=1+count
        assert i==len(packet_words)
        packet_report=dict(start=f'{start:08X}',end=f'{end:08X}',bytes=len(raw),word_count=len(packet_words),
            path=str(path.relative_to(ROOT)),sha256=sha(raw),packet_count=len(packets),packets=packets,
            authority='Exact CPU-appended command words, not GPU consumption, presentation or physical raster coverage.')
        constants=[v for packet in packets if packet['method']=='0B80' for v in packet['words']]
        assert constants==payloads['vertex_constants_c0_c11']['words']
    output_registers={name:f'{u.reg_read(reg):08X}' for name,reg in [('EAX',UC_X86_REG_EAX),
        ('EBX',UC_X86_REG_EBX),('ECX',UC_X86_REG_ECX),('EDX',UC_X86_REG_EDX),
        ('ESI',UC_X86_REG_ESI),('EDI',UC_X86_REG_EDI),('EBP',UC_X86_REG_EBP),
        ('ESP',UC_X86_REG_ESP),('EFLAGS',UC_X86_REG_EFLAGS),('FPCW',UC_X86_REG_FPCW),
        ('FPSW',UC_X86_REG_FPSW),('MXCSR',UC_X86_REG_MXCSR)]}
    output_xmm={f'XMM{i:02d}':f'{u.reg_read(globals()["UC_X86_REG_XMM"+str(i)]):032X}' for i in range(8)}
    result = dict(executed=True,captured_PC='000391F0',incoming_ESP=f'{esp:08X}',
        incoming_stack_words=[f'{v:08X}' for v in words(entry_stack)],system_argument=f'{system:08X}',
        completed_whole391F0=complete,AL=u.reg_read(UC_X86_REG_EAX)&255 if complete else None,
        boundaries=boundaries,source_calls=calls,checkpoints=checkpoints,
        source_payloads=payloads,appended_commands=packet_report,output_registers=output_registers,output_XMM=output_xmm,
        original_writes=changes,mutation_pages=mutations,
        source_call_counts=dict(collections.Counter(x['pc'] for x in calls)),
        verified_original_instructions=len(seen),executed_instruction_steps=total_steps[0],
        instruction_sha256=sha(b''.join(packed([pc])+image.read(pc,n) for pc,n in sorted(seen.items()))),
        captured_GPR_XMM_flags_preserved=True,captured_fpcw=f'{fpcw:04X}',captured_fpsw=f'{fpsw:04X}',
        captured_empty_x87_stack=True,captured_mxcsr=f'{mxcsr:04X}',
        only_incoming_return_replaced=f'{stop:08X}',incoming_argument_or_data_changed=False,
        source_instructions_patched=[],services_replaced=[],kernel_or_MMIO_executed=False,
        original_readiness_or_completion_result_supplied=False,live_original_advanced=False,
        GPU_execution=False,native_activation=False)
    if return_snapshot is not None:
        assert complete, 'Return comparison requires complete original caller.'
        assert sha(return_snapshot.ram)==return_snapshot.report['physical_ram_sha256']
        assert return_snapshot.report['registers_before']==return_snapshot.report['registers_after']
        rr=return_snapshot.report['registers_before']
        assert int(re.search(r'EIP=([0-9a-fA-F]+)',rr)[1],16)==uint(entry_stack)
        returned_registers={name:f'{int(re.search(source+r"=([0-9a-fA-F]+)",rr)[1],16):08X}'
            for name,source in [('EAX','EAX'),('EBX','EBX'),('ECX','ECX'),('EDX','EDX'),
                ('ESI','ESI'),('EDI','EDI'),('EBP','EBP'),('ESP','ESP'),('EFLAGS','EFL'),
                ('FPCW','FCW'),('FPSW','FSW'),('MXCSR','MXCSR')]}
        returned_xmm={}
        for i in range(8):
            a,b=re.search(r'XMM'+f'{i:02d}'+r'=([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',rr).groups()
            returned_xmm[f'XMM{i:02d}']=(a+b).upper()
        last_written={}
        for change in changes:
            if not change['original_stack_write']:
                for i,b in enumerate(bytes.fromhex(change['after'])):
                    last_written[int(change['address'],16)+i]=b
        mismatch=[f'{at:08X}' for at,b in last_written.items() if return_snapshot.read(at,1)[0]!=b]
        page_differences={}
        for at in sorted(dirty):
            if esp-0x10000 <= at < esp+0x1000:
                continue
            clone,actual=bytes(u.mem_read(at,4096)),return_snapshot.read(at,4096)
            differences=[dict(address=f'{at+i:08X}',clone=f'{a:02X}',actual=f'{b:02X}',
                source_written=(at+i) in last_written) for i,(a,b) in enumerate(zip(clone,actual)) if a!=b]
            if differences:
                path=raw_dir/f'{at:08X}-actual-return.bin'
                path.write_bytes(actual)
                page_differences[f'{at:08X}']=dict(actual_return_sha256=sha(actual),
                    actual_return_path=str(path.relative_to(ROOT)),differences=differences)
        group=uint(snapshot.read(system+0x68,4))
        count,render,state_count,state=words(snapshot.read(group,16))
        arrays={}
        for name,at,n in [('published_group',group,16),('render_inputs',render,count*20),('state_inputs',state,state_count*32)]:
            before=snapshot.read(at,n)
            clone=read(at,n)
            actual=return_snapshot.read(at,n)
            arrays[name]=dict(address=f'{at:08X}',bytes=n,incoming_sha256=sha(before),
                clone_return_sha256=sha(clone),actual_return_sha256=sha(actual),
                source_unchanged=before==clone,actual_return_unchanged=before==actual)
        actual_packet=return_snapshot.read(int(packet_report['start'],16),packet_report['bytes'])
        path=raw_dir/'appended-command-span-actual-return.bin'
        path.write_bytes(actual_packet)
        result['actual_return_comparison']=dict(snapshot=str(return_snapshot.directory.relative_to(ROOT)),
            ram_sha256=sha(return_snapshot.ram),manifest_sha256=sha(return_snapshot.directory.joinpath('export.json').read_bytes()),
            actual_return_EIP=f'{uint(entry_stack):08X}',returned_registers=returned_registers,returned_XMM=returned_xmm,
            register_mismatches={k:dict(clone=v,actual=returned_registers[k]) for k,v in output_registers.items() if v!=returned_registers[k]},
            XMM_mismatches={k:dict(clone=v,actual=returned_xmm[k]) for k,v in output_xmm.items() if v!=returned_xmm[k]},
            nonstack_source_written_bytes=len(last_written),source_written_mismatches=mismatch,
            input_arrays=arrays,appended_command_equal=sha(actual_packet)==packet_report['sha256'],
            actual_command_sha256=sha(actual_packet),actual_command_path=str(path.relative_to(ROOT)),
            surrounding_page_differences=page_differences,
            scope='All last source-written non-stack bytes and appended commands are compared. Surrounding page differences are retained separately; complete RAM history and GPU presentation are not inferred.')
    for at in dirty:
        u.mem_write(at,pages[at])
    for at,raw in pages.items():
        assert bytes(u.mem_read(at,4096))==raw
    result['restored_pages_checked']=len(pages)
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot',type=Path,default=ROOT/'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/weather-particle-entry-original.json')
    parser.add_argument('--compare-return',type=Path,default=ROOT/'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-return')
    args=parser.parse_args()
    snapshot=Snapshot(args.snapshot)
    image=Xbe(ROOT/'assets/disc/default.xbe')
    result=replay(snapshot,image,args.output,Snapshot(args.compare_return))
    report=dict(authority=__doc__,source_xbe_sha256=PIN,
        snapshot=str(args.snapshot.relative_to(ROOT)),snapshot_ram_sha256=sha(snapshot.ram),
        snapshot_manifest_sha256=sha(args.snapshot.joinpath('export.json').read_bytes()),
        tool_sha256=sha(Path(__file__).read_bytes()),original_draw_entry=result)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(completed=result['completed_whole391F0'],boundaries=result['boundaries'],
        source_call_counts=result['source_call_counts'],verified_instructions=result['verified_original_instructions'],
        steps=result['executed_instruction_steps'],restored_pages=result['restored_pages_checked']),indent=2))


if __name__=='__main__':
    main()
