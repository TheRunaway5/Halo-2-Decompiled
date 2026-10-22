#!/usr/bin/env python3
"""Execute a captured TU1.5 projectile update on independent host i386.

The declared native animation I/O boundary reads authenticated cache sectors and
allocates real completed-read identities. Original code bytes are unchanged.
The lower original async submission/worker and Xbox kernel clobbers are outside
this native contract. This does not certify a whole weapon or physical Xbox.
"""
import argparse,hashlib,json,os,resource,select,struct,subprocess,time
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from shadow_native_animation_resource_tu15 import NativeShadowAnimationResources
from verify_captured_needler_projectile_tu15 import retained,regions

ROOT=Path(__file__).resolve().parents[1]


def send(fd,payload):
    view=memoryview(payload)
    while view:
        length=os.write(fd,view);assert length>0;view=view[length:]


def receive(fd,size,deadline):
    data=bytearray()
    while len(data)<size:
        remaining=deadline-time.monotonic()
        if remaining<=0:raise TimeoutError('Independent i386 resource bridge timed out')
        if not select.select([fd],[],[],min(remaining,1))[0]:continue
        block=os.read(fd,size-len(data))
        if not block:
            assert not data,'Partial native resource request'
            return None
        data.extend(block)
    return bytes(data)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',type=Path,required=True,help='Completed original reference case')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--trace',action='store_true',help='Single-step original host instructions and record actual stores; parent supplies the same declared real native I/O boundary')
    args=parser.parse_args();case=args.case.absolute();out=args.output.absolute();base=case.parent
    assert not out.exists();out.mkdir(parents=True)
    original=json.loads((case/'report.json').read_text());native_case=Path(original['origin_case'])
    captured=json.loads((native_case/'native.json').read_text());build=json.loads((native_case/'build.json').read_text())
    run=json.loads((native_case.parent/'run.json').read_text());command=run['command']
    assert original['completed'] and original['error'] is None and original['entry']==0xfdeb0
    assert captured['completed'] and captured['entry']==0xfdeb0
    assert original['source_context']['sequence_tick']==captured['sequence_tick']
    assert any(original['source_context'].get(key) for key in ('selected_at_first_damage_tick','selected_at_first_target_death_tick','selected_at_first_standard_detonation_tick'))
    assert not original['instruction_patches'] and not original['supplied_game_services']
    assert original['original_xbe_sha256']==NEW_PIN==file_sha(NEW_DEFAULT)
    assert original['native_synchronization_after_matches_actual_capture']
    assert original['native_animation_resource_after_matches_actual_capture']
    generic=bool(original.get('native_weapon_resource_contracts'))
    services=[s for s in original['native_platform_services'] if s.get('target')==0x1a3b70]
    language_services=[s for s in original['native_platform_services'] if s['kind']=='actual native interface-language input']
    assert len(services)+len(language_services)==len(original['native_platform_services'])
    assert all(s['caller_instruction'] in (0x126139,0x130f68,0x1301b0) for s in services)
    interface_language=None
    if language_services:
        from shadow_native_interface_language_tu15 import NativeShadowInterfaceLanguage
        interface_language=NativeShadowInterfaceLanguage(native_case)
    assert not original['original_animation_async_submission_worker_verified']
    for name in ('native-input.bin','native-fp-input.bin','original-exit.bin'):
        assert file_sha(case/name)==original['files_sha256'][name]
    assert (case/'native-input.bin').read_bytes()==(native_case/'native-input.bin').read_bytes()
    for name in ('native-synchronization-before.json','native-synchronization-after.json','native-exit.bin'):
        assert file_sha(native_case/name)==build['files_sha256'][name]
    before=retained(case/'before.bin',base,original['files_sha256']['before.bin'])
    memory=regions(before);image=Xbe(NEW_DEFAULT);owners=[_Code(image,'.text'),_Code(image,'D3D')]
    for code in owners:assert any(p<=code.lo and code.hi<=p+len(raw) and bytes(raw[code.lo-p:code.hi-p])==code.bytes for p,raw in memory.items())
    forbidden={'pushf','pushfd','pushfw','popf','popfd','popfw','cli','sti','iret','iretd','iretw','hlt'}
    for address,raw in original['executed_instruction_bytes'].items():
        pc=int(address,16);code=next(code for code in owners if code.inside(pc));instruction=code.at(pc)
        assert instruction.bytes.hex()==raw and instruction.mnemonic not in forbidden
        assert pc&~4095!=0x1a3000,'The whole protected page must stay outside original execution in this contract'
        if generic or interface_language:assert pc&~4095!=0x33a000,'The locale boundary page must stay outside original execution'
    helper=ROOT/'tools'/('replay_original_host_weapon_resource_x86.c' if generic or interface_language else 'replay_original_host_animation_resource_x86.c');assembly=ROOT/'tools/replay_charged_effect_x86.S'
    binary=out/'replay-x86';compile=['cc','-m32','-fno-pie','-no-pie','-Wl,-Ttext-segment=0x70000000','-O1',str(helper),str(assembly),'-o',str(binary)]
    with (out/'build.log').open('w') as log:subprocess.run(compile,stdout=log,stderr=subprocess.STDOUT,check=True)
    provider=Path(original['native_animation_resource_provider'])
    ResourceOwner=NativeShadowAnimationResources
    if generic:from shadow_native_weapon_resources_tu15 import NativeShadowWeaponResources as ResourceOwner
    resources=ResourceOwner(native_case/'native-synchronization-before.json',provider,*command[1:4])
    bank_fd=os.memfd_create('authenticated-tu15-projectile-before',os.MFD_CLOEXEC)
    send(bank_fd,before);os.lseek(bank_fd,0,os.SEEK_SET)
    request_read,request_write=os.pipe();reply_read,reply_write=os.pipe()
    execute=[str(binary),str(case),'fdeb0',str(out),str(request_write),str(reply_read),str(bank_fd)]
    resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    records=[];language_records=[];pending=None;error=None;resource_after=None;child=None;trace_result=None
    with (out/'hardware.log').open('w') as log:
        try:
            if args.trace:from trace_native_animation_resource_cpu_tu15 import trace_me
            child=subprocess.Popen(execute,stdout=log,stderr=subprocess.STDOUT,pass_fds=(request_write,reply_read,bank_fd),
                preexec_fn=trace_me if args.trace else None)
            os.close(request_write);request_write=None;os.close(reply_read);reply_read=None;os.close(bank_fd);bank_fd=None
            if args.trace:
                from trace_native_animation_resource_cpu_tu15 import execute as trace_execute
                trace_result=trace_execute(child,original,owners,resources,out,interface_language)
                records=[dict(event,job=event['allocated_job']) for event in resources.events]
                if interface_language:language_records=interface_language.events
            deadline=time.monotonic()+45
            while not args.trace and (request:=receive(request_read,20,deadline)):
                operation,descriptor,destination,size,encoded=struct.unpack('<5I',request)
                if operation in (1,4,5):
                    assert pending is None
                    assert operation==1 or generic
                    answer=resources.request(dict(op='read' if operation==1 else 'read-sectors',descriptor=descriptor,destination=destination,bytes=size,encoded_offset=encoded))
                    authored=bytes.fromhex(answer['descriptor_hex']);payload=bytes.fromhex(answer['payload_hex'])
                    if operation!=1:assert not authored;authored=bytes(20)
                    assert len(authored)==20 and len(payload)==size
                    pending=dict(descriptor=descriptor,destination=destination,bytes=size,encoded_offset=encoded,
                        payload_sha256=hashlib.sha256(payload).hexdigest(),authored_bytes=answer['authored_bytes'])
                    send(reply_write,struct.pack('<2I',0,answer['authored_bytes'])+authored+struct.pack('<I',size)+payload)
                elif operation==2:
                    assert pending and (descriptor,destination,size,encoded)==tuple(pending[k] for k in ('descriptor','destination','bytes','encoded_offset'))
                    job=resources.request(dict(op='commit'))['job'];pending['job']=job
                    records.append(pending);pending=None;send(reply_write,struct.pack('<2I',0,job))
                elif operation==3:
                    assert interface_language and descriptor in (0x1a9da2,0x1a82ab) and destination==descriptor+5
                    language_records.append(dict(caller_instruction=descriptor,returned=destination,raw_language=interface_language.raw))
                    send(reply_write,struct.pack('<2I',0,interface_language.raw))
                else:raise RuntimeError('Unknown independent host resource request')
            assert pending is None
            child.wait(timeout=max(1,deadline-time.monotonic()))
            resource_after=resources.checkpoint()
        except Exception as failure:
            error=str(failure)
            if child and child.returncode is None:child.kill();child.wait(timeout=10)
        finally:
            for fd in (bank_fd,request_read,request_write,reply_read,reply_write):
                if fd is not None:os.close(fd)
            try:resources.close()
            except Exception as failure:error=error or str(failure)
    after_expected=json.loads((native_case/'native-synchronization-after.json').read_text())['native_resource_reads']
    report=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),original_report_sha256=file_sha(case/'report.json'),
        actual_source_build_sha256=file_sha(native_case/'build.json'),original_xbe_sha256=NEW_PIN,
        compile=compile,command=execute,binary_sha256=file_sha(binary),helper_source_sha256={str(p.relative_to(ROOT)):file_sha(p) for p in (helper,assembly)},
        native_resource_provider_sha256=file_sha(provider),native_resource_manifest_sha256=file_sha(provider.parent/'manifest.json'),
        native_resource_adapter_sha256=file_sha(ROOT/'tools/shadow_native_animation_resource_tu15.py'),
        independent_actual_resource_reads=records,native_resource_after=resource_after,
        independent_native_interface_language_inputs=language_records,
        native_resource_after_exact=resource_after==after_expected,returncode=child.returncode if child else None,error=error,
        source_instruction_patches=[],source_page_execute_disabled=0x1a3000,native_IO_entry=0x1a3b70,
        ABI='EAX actual retained job; consume28 stack bytes; preserve other GP/FP/flags through Linux signal context',
        source_bank_input_from_authenticated_memfd=True,expanded_input_bank_written=False,
        independent_single_step_trace=trace_result,
        original_async_submission_worker_verified=False,Xbox_kernel_clobbers_verified=False,
        physical_xbox_verified=False,full_weapon_parity=False,core_dumps_disabled=True,completed=False)
    if child and child.returncode==0 and error is None:
        receipt=list(struct.iter_unpack('<7I',(out/'host-animation-reads.bin').read_bytes()))
        assert len(records)==len(services) and len(language_records)==len(language_services)
        if args.trace:assert not receipt
        else:assert len(receipt)==len(records)
        for actual,backed,old in zip(receipt,records,services):
            assert actual[:4]==tuple(backed[k] for k in ('descriptor','destination','bytes','encoded_offset'))
            assert actual[5]==backed['job']==old['allocated_job'] and actual[6]==old['returned']
            assert all(backed[k]==old[k] for k in ('descriptor','destination','bytes','encoded_offset','payload_sha256'))
        for actual,old in zip(language_records,language_services):
            assert all(actual[k]==old[k] for k in ('caller_instruction','returned','raw_language'))
        raw=(out/'hardware-exit.bin').read_bytes();native=(native_case/'native-exit.bin').read_bytes();emulated=(case/'original-exit.bin').read_bytes()
        assert len(raw)==len(native)==len(emulated)==180
        flags=struct.unpack_from('<I',raw,160)[0];assert flags&0x200 and not struct.unpack_from('<I',native,160)[0]&0x200
        normalized=bytearray(raw);struct.pack_into('<I',normalized,160,flags&~0x200)
        expected=regions(retained(case/'original-after.bin',base,original['files_sha256']['original-after.bin']))
        hardware=regions((out/'hardware-after.bin').read_bytes());assert expected.keys()==hardware.keys()
        native_memory=regions(retained(native_case/'after.bin',base,build['files_sha256']['after.bin']))
        assert native_memory.keys()==hardware.keys();native_differences=[]
        compared=0;differences=[]
        for address,bytes_ in expected.items():
            other=hardware[address];assert len(bytes_)==len(other)
            if 0x60000000<=address and address+len(bytes_)<=0x60200000:continue
            compared+=len(bytes_)
            if bytes_!=other:differences.append(dict(address=address,first_difference=next(n for n,(a,b) in enumerate(zip(bytes_,other)) if a!=b)))
            if native_memory[address]!=other:native_differences.append(dict(address=address,first_difference=next(n for n,(a,b) in enumerate(zip(native_memory[address],other)) if a!=b)))
        report.update(completed=True,retained_bytes_compared=compared,retained_bank_differences=differences,
            emulator_retained_memory_equal=not differences,entire_native_CPU_FP_equal_except_platform_IF=native==normalized,
            native_retained_memory_equal=not native_differences,native_retained_bank_differences=native_differences,
            emulator_GP_XMM_flags_FS_depth_equal=emulated[:172]==normalized[:172],
            raw_emulator_FP_equal=emulated[172:]==raw[172:],native_FP_status=struct.unpack_from('<H',native,172)[0],
            emulator_FP_status=struct.unpack_from('<H',emulated,172)[0],hardware_FP_status=struct.unpack_from('<H',raw,172)[0])
        if args.trace:
            from verify_captured_needler_projectile_tu15 import stores
            def architectural_trace(path):
                result=[]
                for pc, in struct.iter_unpack('<I',path.read_bytes()):
                    code=next(code for code in owners if code.inside(pc));instruction=code.at(pc)
                    # Unicorn reports one extra zero-count exit callback for
                    # a completed REP. Linux TF reports completed iterations.
                    # Keep one architectural instruction per contiguous REP
                    # execution and separately compare every actual store.
                    if result and result[-1]==pc and instruction.mnemonic.startswith('rep '):continue
                    result.append(pc)
                return result
            report['hardware_raw_instruction_callbacks_equal_original']=(out/'hardware-trace.bin').read_bytes()==(case/'original-trace.bin').read_bytes()
            original_path=architectural_trace(case/'original-trace.bin');hardware_path=architectural_trace(out/'hardware-trace.bin')
            report['hardware_instruction_trace_equal_original']=original_path==hardware_path
            report['architectural_instruction_observations']=len(hardware_path)
            report['REP_callback_policy']='Collapse contiguous observations of the same original REP into one architectural instruction; compare every memory store separately'
            report['hardware_ordered_stores_equal_original']=list(stores(out/'hardware-stores.bin'))==list(stores(case/'original-stores.bin'))
            if (native_case/'native-instructions.bin').is_file():
                from audit_damage_delivery_observation_pcs_tu15 import mapping
                pcs,_,write_sites=mapping(ROOT/'src/gameplay/units/source_damage_delivery_runtime_generated.cpp')
                native_rows=list(struct.iter_unpack('<II',(native_case/'native-instructions.bin').read_bytes()))
                assert all(domain in (0,1) and (not domain or pc in pcs) for pc,domain in native_rows)
                selected_native=[pcs[pc] if domain else pc for pc,domain in native_rows]
                normalized_native=[]
                for pc in selected_native:
                    code=next(code for code in owners if code.inside(pc));instruction=code.at(pc)
                    if normalized_native and normalized_native[-1]==pc and instruction.mnemonic.startswith('rep '):continue
                    normalized_native.append(pc)
                report['hardware_instruction_trace_equal_native']=hardware_path==normalized_native
                report['native_instruction_callbacks']=len(selected_native)
                source_stores=list(stores(native_case/'native-stores.bin'))
                from collections import defaultdict
                domains=defaultdict(set)
                for pc,domain in native_rows:domains[pc].add(domain)
                native_stores=[]
                for pc,address,before,after in source_stores:
                    selected=pc
                    if pc in write_sites and 1 in domains[pc]:
                        if 0 in domains[pc]:
                            # A shared number can name a canonical write and
                            # an actual-image comparison. Reject ambiguous
                            # actual writes instead of selecting by expected
                            # values or erasing the observation domain.
                            code=next(code for code in owners if code.inside(pc));op=code.at(pc)
                            assert op.mnemonic=='cmp',f'Capture per-store domains for ambiguous site {pc:x}'
                        selected=pcs[pc]
                    native_stores.append((selected,address,before,after))
                host_stores=[];wide_stores=0
                for pc,address,before,after in stores(out/'hardware-stores.bin'):
                    if pc in (0x126139,0x130f68,0x1301b0):continue
                    if len(after)==16:
                        code=next(code for code in owners if code.inside(pc));op=code.at(pc)
                        assert op.mnemonic=='movaps' and op.operands[0].type==3 and op.operands[0].size==16
                        # Native physics_step_internal::vw retains a128-bit
                        # instruction as two ordered8-byte observer writes.
                        wide_stores+=1
                        host_stores.extend((pc,address+n,before[n:n+8],after[n:n+8]) for n in (0,8))
                    else:host_stores.append((pc,address,before,after))
                report['hardware_ordered_source_stores_equal_native']=host_stores==native_stores
                report['native_ordered_source_stores']=len(native_stores)
                report['host_vector_stores_split_for_native_observer']=wide_stores
    report['passed']=bool(report['completed'] and report['native_resource_after_exact'] and report['native_retained_memory_equal'] and report['entire_native_CPU_FP_equal_except_platform_IF'])
    if args.trace:
        report['passed']=report['passed'] and report.get('hardware_instruction_trace_equal_native',report.get('hardware_instruction_trace_equal_original',False)) and report.get('hardware_ordered_source_stores_equal_native',report.get('hardware_ordered_stores_equal_original',False))
    report['files_sha256']={p.name:file_sha(p) for p in out.iterdir() if p.is_file()}
    (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','completed','returncode','error','retained_bytes_compared','native_FP_status','emulator_FP_status','hardware_FP_status','entire_native_CPU_FP_equal_except_platform_IF') if k in report}))
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
