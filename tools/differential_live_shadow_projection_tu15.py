#!/usr/bin/env python3
"""Compare live native shadow root frames with the unchanged TU1.5 CPU prefix.

Stop at32340's first view-volume call (327D9), before GPU/caster/receiver work.
Only actual owner/history-fade call arguments are supplied. Original execution
reads retained native source banks; native projection outputs are comparison
only. The optional secondary-admission run retains original queue outputs, and
compares them when actual native secondary queues are present in the capture.
This is not full shadow submission or original framebuffer proof.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from differential_physics_native_frame_tu15_20261012 import regions, segment

ENTRY=0x32340
STOP=0x327d9
FIELDS=('center.x center.y center.z direction.x direction.y direction.z '
        'up.x up.y up.z left.x left.y left.z virtual_eye.x virtual_eye.y virtual_eye.z '
        'volume_center.x volume_center.y volume_center.z volume_half_extent.x '
        'volume_half_extent.y volume_half_extent.z transverse_radius axial_radius '
        'width near_offset far_offset virtual_distance cone_angle opacity').split()
REGS=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]


def sha(p):
    with Path(p).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def projection_words(read,sp):
    u=lambda p:struct.unpack('<I',read(p,4))[0]
    s=lambda o:u(0x48c530+o)
    v=lambda o:u(sp+o)
    return [*[s(i)for i in range(0,12,4)],*[s(i)for i in range(0x28,0x40,4)],
        *[v(i)for i in (0x3c,0x40,0x44,0x98,0x9c,0xa0,0x54,0x58,0x5c,0x60,0x64,0x68)],
        v(0x24),v(0x20),s(0xa8),s(0xb4),s(0xb8),v(0x94),s(0xb0),v(0x14)]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--hardware',action='store_true')
    p.add_argument('--view-volume',action='store_true')
    p.add_argument('--projection-bank',action='store_true')
    p.add_argument('--secondary-camera',action='store_true')
    p.add_argument('--secondary-admission',action='store_true')
    p.add_argument('--secondary-collection',action='store_true')
    p.add_argument('--native-synchronization',type=Path)
    p.add_argument('--native-synchronization-library',type=Path)
    a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    a.output.mkdir(parents=True)
    if a.secondary_collection:a.secondary_admission=True
    if a.secondary_admission:a.secondary_camera=True
    assert not(a.view_volume and a.secondary_camera)
    stop=0x32883 if a.secondary_collection else (0x32874 if a.secondary_admission else (0x32817 if a.secondary_camera else (0x327de if a.view_volume else STOP)))
    snapshot=a.capture/'shadow-selection'
    call=json.loads((snapshot/'call.json').read_text());assert call['authority']=='OG Xbox TU1.5'
    entry=json.loads((snapshot/'after-abi.json').read_text());assert entry['x87_depth']==0
    #333F0's stdcall return has popped its return/argument slots. Reuse the
    # actual retained entry return slot for this declared root call instead.
    entry['gp'][4]=json.loads((snapshot/'before-abi.json').read_text())['gp'][4]
    rows=[row for row in json.loads((a.capture/'report.json').read_text())['shadow_root_frames']
          if row['tick']==call['tick'] and row['view']==call['view']]
    if a.secondary_camera:rows=[row for row in rows if row['view_admitted']]
    assert rows and all(row['route']=='projected' for row in rows),rows
    image=Xbe(NEW_DEFAULT);results=[]
    for row in rows:
        banks=[(address,bytearray(data))for address,data in regions(snapshot/'after.bin')]
        def bank_read(address,size):
            for start,data in banks:
                if start<=address and address+size<=start+len(data):return bytes(data[address-start:address-start+size])
            raise AssertionError(('unretained read',hex(address),size))
        def bank_write(address,data):
            for start,retained in banks:
                if start<=address and address+len(data)<=start+len(retained):
                    retained[address-start:address-start+len(data)]=data;return
            raise AssertionError(('unretained call argument',hex(address)))
        # A declared ordinary32340 call in the same captured retained stack.
        # It does not consume caller GP registers besides SP; inputs are stack
        # arguments. Retain actual FS/SSE/x87/MXCSR state from the native caller.
        sp=entry['gp'][4];return_address=int.from_bytes(bank_read(sp,4),'little')
        arguments=struct.pack('<If',row['owner'],row['history_fade'])
        bank_write(sp+4,arguments)
        guest=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set();synchronization=None
        assert bool(a.native_synchronization)==bool(a.native_synchronization_library)
        if a.native_synchronization:
            from shadow_native_synchronization_adapter import NativeShadowSynchronization
            synchronization=NativeShadowSynchronization(a.native_synchronization,a.native_synchronization_library)

        def mount(address,data):
            pages=[p for p in range(address&~4095,(address+len(data)+4095)&~4095,4096)if p not in mapped]
            start=0
            while start<len(pages):
                end=start+1
                while end<len(pages)and pages[end]==pages[end-1]+4096:end+=1
                guest.mem_map(pages[start],(end-start)*4096);start=end
            mapped.update(pages);guest.mem_write(address,bytes(data))
        for address,data in banks:mount(address,data)
        if synchronization:
            assert 0x80000000 not in mapped
            mount(0x80000000,b'\x0f\x0b'*2048) # External kernel adapter only; no XBE instruction bytes change.

        gdt=0x7fff0000;assert gdt not in mapped;mount(gdt,bytes(4096))
        guest.mem_write(gdt+8,segment(0,0x9b)+segment(0,0x93)+segment(entry['fs_base'],0x93))
        guest.reg_write(R.UC_X86_REG_GDTR,(0,gdt,0xfff,0))
        for r,v in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):
            guest.reg_write(r,v)
        for r,v in zip(REGS,entry['gp']):guest.reg_write(r,v)
        for n,lanes in enumerate(entry['xmm']):guest.reg_write(R.UC_X86_REG_XMM0+n,sum(v<<(32*k)for k,v in enumerate(lanes)))
        for r,v in ((R.UC_X86_REG_EFLAGS,entry['eflags']),(R.UC_X86_REG_FPCW,entry['x87_control']),
                    (R.UC_X86_REG_FPSW,entry['x87_status']),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,entry['mxcsr'])):
            guest.reg_write(r,v)
        trace=[];volume={}
        def code(cpu,pc,size,_):
            if synchronization and 0x80000000<=pc<0x80001000:
                synchronization.kernel(cpu,pc);return
            assert bytes(cpu.mem_read(pc,size))==image.read(pc,size),hex(pc)
            trace.append(pc)
            if a.secondary_collection and pc==0x3287e:
                u=lambda p:int.from_bytes(cpu.mem_read(p,4),'little')
                count=u(0x4cd398)&65535
                assert count<=850
                volume['units_before_cull']=[list(struct.unpack('<8I',cpu.mem_read(0x4c6958+n*32,32)))for n in range(count)]
            if a.secondary_admission and pc==0x134c00:
                stack=cpu.reg_read(R.UC_X86_REG_ESP)
                volume['query_radius_word']=int.from_bytes(cpu.mem_read(stack+16,4),'little')
            if a.view_volume and pc==0x1681c0:
                stack=cpu.reg_read(R.UC_X86_REG_ESP)
                u=lambda p:int.from_bytes(cpu.mem_read(p,4),'little')
                assert u(stack+16)==24
                volume['endpoints_address']=u(stack+12)
                volume['bounds_address']=u(stack+28)
                volume['endpoints']=list(struct.unpack('<72I',cpu.mem_read(volume['endpoints_address'],288)))
        guest.hook_add(UC_HOOK_CODE,code);error=None;actual=None
        try:
            guest.emu_start(ENTRY,stop,count=5000000)
            assert guest.reg_read(R.UC_X86_REG_EIP)==stop,'Original root did not reach projection boundary'
            actual=projection_words(guest.mem_read,guest.reg_read(R.UC_X86_REG_ESP))
            if a.projection_bank or a.secondary_camera:
                root_sp=guest.reg_read(R.UC_X86_REG_ESP)
                actual += list(struct.unpack('<31I',guest.mem_read(root_sp+0x6c,124)))
                if a.secondary_camera:
                    actual += [int.from_bytes(guest.mem_read(root_sp+0x1c,2),'little'),
                        *struct.unpack('<111I',guest.mem_read(root_sp+0xe8,444))]
            if a.view_volume:
                assert volume,'Original root did not execute the view-volume owner'
                actual += [int(bool(guest.reg_read(R.UC_X86_REG_EAX)&255)),
                    *struct.unpack('<6I',guest.mem_read(volume['bounds_address'],24)),*volume['endpoints']]
        except (UcError,AssertionError,RuntimeError) as e:error=str(e)
        platform_calls=[]if synchronization is None else synchronization.events
        if synchronization:synchronization.close()
        failure_abi=None
        if error is not None:
            failure_abi=dict(gp=[guest.reg_read(r)for r in REGS],pc=guest.reg_read(R.UC_X86_REG_EIP))
            try:failure_abi['stack_words']=list(struct.unpack('<8I',guest.mem_read(failure_abi['gp'][4],32)))
            except UcError:pass

        directory=a.output/str(row['owner']);directory.mkdir()
        (directory/'instructions.json').write_text(json.dumps(trace)+'\n')
        if a.secondary_admission and error is None:
            u=lambda p:int.from_bytes(guest.mem_read(p,4),'little')
            queues={}
            for name,offset in [('clusters',12),('objects',20),('instances',24)]:
                header=u(0x5598f8+offset);capacity=u(header)&65535
                count=int.from_bytes(guest.mem_read(header+4,2),'little',signed=True)
                assert 0<=count<=capacity
                rows=[]
                for n in range(count):
                    rows.append(dict(identity=u(u(header+12)+4*n),
                        flags=int.from_bytes(guest.mem_read(u(header+8)+2*n,2),'little'),
                        association=int.from_bytes(guest.mem_read(u(header+20)+2*n,2),'little'),
                        other=u(u(header+16)+4*n)))
                queues[name]=dict(header=header,capacity=capacity,count=count,rows=rows)
            context=u(0x5598f8)
            admission=dict(context_descriptor=0x5598f8,visibility_bank=context,
                pass_mode=u(0x5598f8+0x2a78),query_mode=u(0x5598f8+0x2a7c),queues=queues,
                query_radius_word=volume['query_radius_word'],native_secondary_queues_verified=False)
            (directory/'original-secondary-queues.json').write_text(json.dumps(admission,indent=2)+'\n')
            if a.secondary_collection:
                count=u(0x4cd398)&65535;prefix=u(0x4cd39c)&65535
                assert prefix<=count<=850
                units=[list(struct.unpack('<8I',guest.mem_read(0x4c6958+n*32,32)))for n in range(count)]
                overrides=u(0x4cdd48);assert overrides<=192
                collection=dict(main_prefix=prefix,draw_count=count,units=units,units_before_cull=volume['units_before_cull'],source_draw_unit_address=0x4c6958,
                    coverage=list(guest.mem_read(0x4cd3a4,count)),override_slots=list(guest.mem_read(0x4cd6f6,count)),
                    overrides=list(struct.unpack(f'<{overrides}I',guest.mem_read(0x4cda48,overrides*4))),
                    section_highwater=u(0x4ce1cc),sections=list(struct.unpack('<288I',guest.mem_read(0x4cdd4c,1152))),
                    main_arena_address=0x557074,main_arena_used=int.from_bytes(guest.mem_read(0x556e18+0xa5c,2),'little'),
                    main_arena=list(struct.unpack('<512I',guest.mem_read(0x557074,2048))),
                    secondary_arena_address=0x559b54,secondary_arena_used=int.from_bytes(guest.mem_read(0x5598f8+0xa5c,2),'little'),
                    secondary_arena=list(struct.unpack('<512I',guest.mem_read(0x559b54,2048))),
                    native_draw_units_verified=False,external_native_synchronization_policy=bool(synchronization))
                (directory/'original-draw-units.json').write_text(json.dumps(collection,indent=2)+'\n')
        backend='Unicorn unchanged TU1.5';hardware_path=None
        if a.hardware and error is None:
            from generate_combat_action_tu15 import CombatCode
            import oracle_physics_game_frame_hardware_20261011 as hardware
            fixture=dict(id=0,entry=ENTRY,return_address=return_address,exit_entry=stop,
                initial_banks=[dict(address=start,hex=data.hex())for start,data in banks],exit={},
                entry_abi=dict(gp=entry['gp'],eflags=entry['eflags'],mxcsr=entry['mxcsr'],fs_base=entry['fs_base'],
                    x87_control_word=entry['x87_control'],x87_status=entry['x87_status'],x87_tag=0xffff,x87_values=[],
                    xmm=[f'{sum(value<<(32*k)for k,value in enumerate(lanes)):032x}'for lanes in entry['xmm']]))
            pair=dict(address_map={},entries={'00032340':ENTRY},instruction_map={f'{pc:08x}':pc for pc in trace})
            hardware._Code=lambda image,section:CombatCode(image)
            original=hardware.execute(dict(image='1.5',cases=[fixture]),pair,directory/'hardware')
            observed=original['cases'][0]
            assert observed['instructions']==trace,'Hardware original projection path differs'
            final=[(b['address'],bytes.fromhex(b['hex']))for b in observed['final_banks']]
            def read_final(address,size):
                for start,data in final:
                    if start<=address and address+size<=start+len(data):return data[address-start:address-start+size]
                raise AssertionError(('unretained hardware result',hex(address),size))
            actual=projection_words(read_final,observed['exit']['gp'][4])
            if a.projection_bank or a.secondary_camera:
                root_sp=observed['exit']['gp'][4]
                actual += list(struct.unpack('<31I',read_final(root_sp+0x6c,124)))
                if a.secondary_camera:
                    actual += [int.from_bytes(read_final(root_sp+0x1c,2),'little'),
                        *struct.unpack('<111I',read_final(root_sp+0xe8,444))]
            if a.view_volume:
                endpoints=list(struct.unpack('<72I',read_final(volume['endpoints_address'],288)))
                assert endpoints==volume['endpoints'],'Original view-volume owner mutated its endpoint inputs'
                actual += [int(bool(observed['exit']['gp'][0]&255)),
                    *struct.unpack('<6I',read_final(volume['bounds_address'],24)),*endpoints]
            backend=original['oracle_backend'];hardware_path=True
        # Expected native math is read only after unchanged original execution.
        expected=list(row['frame_words']);fields=list(FIELDS)
        if a.projection_bank or a.secondary_camera:
            expected += row['projection_words'];fields += [f'projection[{i}]'for i in range(31)]
            if a.secondary_camera:
                expected += [1,*row['secondary_camera_words']]
                fields += ['secondary_camera_count',*[f'secondary_camera[{i}]'for i in range(111)]]
        if a.view_volume:
            expected += [int(row['view_admitted']),*row['projected_bounds_words'],*row['world_endpoints_words']]
            fields += ['view_admitted',*[f'projected_bounds[{i}]'for i in range(6)],
                *[f'world_endpoints[{i}]'for i in range(72)]]
        assert len(expected)==len(fields)
        differences=[]if actual is None else [dict(field=name,original=o,native=n,
            original_float=struct.unpack('<f',struct.pack('<I',o))[0],native_float=struct.unpack('<f',struct.pack('<I',n))[0])
            for name,o,n in zip(fields,actual,expected)if o!=n]
        queues_match=None
        if a.secondary_admission and error is None and 'secondary_queues' in row:
            expected_queues=row['secondary_queues']
            actual_queues={name:[[r['identity'],r['flags'],r['association'],r['other']]for r in q['rows']]
                for name,q in queues.items()}
            queues_match=all(actual_queues[name]==expected_queues[name]for name in actual_queues)
            queues_match &= admission['pass_mode']==expected_queues['pass_mode'] and admission['query_mode']==expected_queues['query_mode']
            queues_match &= volume['query_radius_word']==row['query_radius_word']
            if not queues_match:differences.append(dict(field='secondary_queues_or_query_radius',
                original=admission,native=expected_queues))
        draws_match=None
        if a.secondary_collection and error is None and 'collected'in row:
            expected_draws=row['collected']
            draw_differences=[name for name,value in expected_draws.items()if collection[name]!=value]
            draws_match=not draw_differences
            if not draws_match:differences.append(dict(field='collected_draw_banks',keys=draw_differences))
        results.append(dict(owner=row['owner'],tick=row['tick'],route=row['route'],
            passed=error is None and not differences,native_secondary_queues_verified=queues_match,native_draw_banks_verified=draws_match,instructions=len(trace),error=error,original_pc=guest.reg_read(R.UC_X86_REG_EIP),failure_abi=failure_abi,platform_calls=platform_calls,
            original_words=actual,native_words=expected,differences=differences,
            oracle_backend=backend,hardware_original=hardware_path,
            declared_call_arguments=dict(owner=row['owner'],history_fade=row['history_fade'],stack_address=sp+4,hex=arguments.hex())))
        print(json.dumps({k:results[-1][k]for k in ('owner','passed','instructions','error','differences')}),flush=True)
    report=dict(scope=__doc__,passed=all(r['passed']for r in results),cases=results,entry=ENTRY,exit_entry=stop,view_volume=a.view_volume,projection_bank=a.projection_bank,secondary_camera=a.secondary_camera,secondary_admission=a.secondary_admission,secondary_collection=a.secondary_collection,
        original_sha256=NEW_PIN,tool_sha256=sha(__file__),input_bank_sha256=sha(snapshot/'after.bin'),
        native_capture_sha256=sha(a.capture/'report.json'),instruction_patches=[],replaced_game_services=[],
        replaced_services=['Xbox kernel wait/release imports use actual captured arena synchronization']if a.native_synchronization else [],
        external_native_synchronization_policy=bool(a.native_synchronization),
        synchronization_checkpoint_sha256=sha(a.native_synchronization)if a.native_synchronization else None,
        native_projection_output_used_as_original_input=False,native_secondary_queues_verified=a.secondary_admission and all(r["native_secondary_queues_verified"]is True for r in results),native_draw_banks_verified=a.secondary_collection and all(r["native_draw_banks_verified"]is True for r in results),
        shadow_gpu_submission=False,original_framebuffer_parity=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    assert report['passed'],'Live root projection differs from unchanged TU1.5 CPU prefix'


if __name__=='__main__':main()
