#!/usr/bin/env python3
"""Verify a real Needler contact update under the declared native resource ABI.

This verifies complete retained memory, ordered source writes and real native
resource ownership. Independent host FP/instruction-trace and Xbox asynchronous
I/O correspondence remain separate gates. No complete weapon parity is inferred.
"""
import argparse,hashlib,importlib.util,io,json,struct
from pathlib import Path
from audit_rendering import file_sha
from audit_damage_delivery_observation_pcs_tu15 import mapping
from research_mp_sandbox_update_functions import NEW_PIN

ROOT=Path(__file__).resolve().parents[1]


def stores(path):
    with path.open('rb') as stream:
        while header:=stream.read(12):
            assert len(header)==12
            pc,address,size=struct.unpack('<III',header);assert 0<size<=128*1024*1024
            before,after=stream.read(size),stream.read(size)
            assert len(before)==len(after)==size
            yield pc,address,before,after


def retained(path,base,expected):
    if path.is_file():raw=path.read_bytes()
    else:
        spec=importlib.util.spec_from_file_location('retained_banks',base/'bank-chunk-retention-v1.py')
        banks=importlib.util.module_from_spec(spec);spec.loader.exec_module(banks)
        member=str(path.relative_to(base));record=json.loads(banks.INDEX.read_text())['files'][member]
        manifest_path=banks.STORE/record['manifest'];assert file_sha(manifest_path)==record['manifest_sha256']
        manifest=json.loads(manifest_path.read_text());assert manifest['member']==member
        assert record['raw_sha256']==manifest['raw_sha256']==expected
        result=io.BytesIO();banks.reconstruct(manifest,result);raw=result.getvalue()
    assert hashlib.sha256(raw).hexdigest()==expected
    return raw


def regions(raw):
    result={};offset=0;view=memoryview(raw)
    while offset<len(view):
        address,size=struct.unpack_from('<II',view,offset);offset+=8
        assert size and offset+size<=len(view) and address not in result
        result[address]=view[offset:offset+size];offset+=size
    assert offset==len(view)
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('case','original','observation-audit','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--hardware',type=Path,help='Independent host instruction/store trace for this exact captured call and native resource contract')
    args=parser.parse_args();case=args.case.absolute();base=case.parents[1];original=args.original.absolute()
    assert not args.output.exists()
    build=json.loads((case/'build.json').read_text());capture=json.loads((case/'native.json').read_text())
    reference=json.loads((original/'report.json').read_text());flow=json.loads((case.parent/'report.json').read_text())
    run=json.loads((case.parent/'run.json').read_text());audit=json.loads(args.observation_audit.read_text())
    assert run['wrapper_returncode']==0 and run['requested_capture_completed']
    assert build['source_flow_report_sha256']==file_sha(case.parent/'report.json')==run['actual_report_sha256']
    assert build['source_flow_receipt_sha256']==file_sha(case.parent/'run.json')
    assert file_sha(Path(build['command'][0]))==build['binary_sha256']==run['binary_sha256']
    for name,digest in build['linked_files_sha256'].items():assert file_sha(Path(name))==digest
    assert flow['completed_ticks']==240 and flow['rounds_consumed']==11
    assert capture['completed'] and capture['entry']==0xfdeb0 and capture['sequence_tick']==118
    assert flow['ticks'][118]['target']['shield']<flow['ticks'][117]['target']['shield']
    assert reference['completed'] and reference['error'] is None and reference['entry']==0xfdeb0
    assert reference['origin_case']==str(case) and reference['origin_phase']=='before'
    assert reference['original_xbe_sha256']==NEW_PIN and not reference['instruction_patches']
    assert not reference['supplied_game_services'] and not reference['source_memory_faults']
    assert reference['native_synchronization_after_matches_actual_capture']
    assert reference['native_animation_resource_after_matches_actual_capture']
    assert not reference['original_animation_async_submission_worker_verified']
    context=reference['source_context']
    assert context['actual_native_projectile_update_API']==0xfdeb0 and context['selected_at_first_damage_tick']
    assert context['original_call_contract']==0xfde67 and context['original_caller_entry']==0xfdde0
    assert reference['original_functions']['000ffa60']==1
    for name in ('native-synchronization-before.json','native-synchronization-after.json','native-stores.bin','native-exit.bin','native.json'):
        assert file_sha(case/name)==build['files_sha256'][name]
    for name in ('original-stores.bin','original-exit.bin','original-trace.bin'):
        assert file_sha(original/name)==reference['files_sha256'][name]
    assert file_sha(original/'probe-source.py')==reference['tool_sha256']
    assert file_sha(original/'bank-validator-source.py')==context['validation_tool_sha256']
    assert reference['origin_bank_sha256']==build['files_sha256']['before.bin']

    native_raw=retained(case/'after.bin',base,build['files_sha256']['after.bin'])
    original_raw=retained(original/'original-after.bin',base,reference['files_sha256']['original-after.bin'])
    native,expected=regions(native_raw),regions(original_raw);assert native.keys()==expected.keys()
    compared=0
    for address,raw in native.items():
        assert len(raw)==len(expected[address])
        if 0x60000000<=address and address+len(raw)<=0x60200000:continue
        assert not (address<0x60200000 and address+len(raw)>0x60000000),'Split overlapping stack region explicitly'
        assert raw==expected[address],hex(address)
        compared+=len(raw)
    def read(address,size):
        hits=[raw[address-start:address-start+size] for start,raw in native.items() if start<=address and address+size<=start+len(raw)]
        assert len(hits)==1
        return bytes(hits[0])

    services=reference['native_platform_services'];assert len(services)==1
    event=services[0];assert event['kind']=='actual native synchronous animation resource read'
    assert event['caller_instruction']==0x126139 and event['target']==0x1a3b70
    before=json.loads((case/'native-synchronization-before.json').read_text())
    after=json.loads((case/'native-synchronization-after.json').read_text())
    assert before['source_resources_identity']==after['source_resources_identity']==event['actual_cache_catalog']
    old,new=before['native_resource_reads'],after['native_resource_reads']
    assert new['jobs'][:-1]==old['jobs'] and new['next']==old['next']+1
    assert event['allocated_job']==old['next']==new['jobs'][-1]['job']
    assert new['jobs'][-1]=={k:event[v] for k,v in [('job','allocated_job'),('descriptor','descriptor'),('destination','destination'),('bytes','bytes')]}
    location=event['encoded_offset']>>30;offset=event['encoded_offset']&0x3fffffff
    assert location in (1,2,3),'Audit balanced private-resident backing separately'
    resource=Path(build['command'][2])/('mainmenu.map','shared.map','single_player_shared.map')[location-1]
    assert file_sha(resource)==before['source_resources_identity']['shared'][location-1]
    with resource.open('rb') as stream:stream.seek(offset);payload=stream.read(event['bytes'])
    assert len(payload)==event['bytes'] and hashlib.sha256(payload).hexdigest()==event['payload_sha256']
    assert read(event['destination'],len(payload))==payload
    provider=Path(reference['native_animation_resource_provider']);manifest=json.loads((provider.parent/'manifest.json').read_text())
    assert file_sha(provider.parent/'manifest.json')==reference['native_animation_resource_manifest_sha256']
    for name,digest in manifest['files'].items():assert file_sha(Path(name))==digest

    assert audit['passed'] and file_sha(Path(audit['source']))==audit['source_sha256']
    pcs,owners,store_sites=mapping(Path(audit['source']))
    assert pcs=={int(p,16):q for p,q in audit['instruction_map'].items()}
    assert store_sites==set(audit['canonical_memory_store_pcs'])
    assert owners==audit['owners']
    # Only actual canonical memory-write sites are translated. Other modules
    # already report TU1.5 PCs, which can numerically overlap old read sites.
    selected={pc:pcs[pc] for pc in store_sites}
    source=list(stores(case/'native-stores.bin'));all_original=list(stores(original/'original-stores.bin'))
    platform=[row for row in all_original if row[0]==0x126139]
    assert len(platform)==2 and platform[0][1]==event['destination'] and platform[0][3]==payload
    assert platform[1][3]==b'\x01' and read(platform[1][1],1)==b'\x01'
    assert len(platform[1][2])==1
    comparable=[row for row in all_original if row[0]!=0x126139]
    converted=[(selected.get(pc,pc),address,before_bytes,after_bytes) for pc,address,before_bytes,after_bytes in source]
    assert converted==comparable
    native_cpu=(case/'native-exit.bin').read_bytes();reference_cpu=(original/'original-exit.bin').read_bytes()
    assert len(native_cpu)==len(reference_cpu)==180
    assert native_cpu[:172]==reference_cpu[:172] and native_cpu[174:]==reference_cpu[174:]
    native_status,reference_status=(struct.unpack_from('<H',raw,172)[0] for raw in (native_cpu,reference_cpu))
    result=dict(passed=True,scope=__doc__,original_report_sha256=file_sha(original/'report.json'),
        actual_API_build_sha256=file_sha(case/'build.json'),actual_flow_report_sha256=file_sha(case.parent/'report.json'),
        native_resource_provider_manifest_sha256=file_sha(provider.parent/'manifest.json'),
        observation_audit_sha256=file_sha(args.observation_audit),original_xbe_sha256=NEW_PIN,
        actual_source_projectile=context['actual_source_projectile'],tick=118,contact_branch_reached=True,
        original_instructions=reference['original_instructions'],complete_retained_nonstack_bytes_equal=compared,
        ordered_source_store_records_equal=len(source),canonical_store_labels_selected=sum(pc!=selected.get(pc,pc) for pc,_,_,_ in source),
        additional_declared_native_IO_store_records_checked=len(platform),actual_resource_bytes_checked=len(payload),
        native_resource_and_synchronization_after_exact=True,GP_XMM_flags_FS_depth_control_MXCSR_equal=True,
        native_x87_status=native_status,reference_emulated_x87_status=reference_status,
        independent_host_FP_and_complete_instruction_trace_verified=False,complete_original_native_call_verified=False,
        original_async_queue_worker_verified=False,physical_xbox_verified=False,full_weapon_parity=False,
        tool_sha256=file_sha(Path(__file__)))
    if args.hardware:
        hardware=args.hardware.absolute();host=json.loads((hardware/'report.json').read_text())
        assert host['passed'] and host['completed'] and host['returncode']==0
        assert host['original_report_sha256']==file_sha(original/'report.json')
        assert host['actual_source_build_sha256']==file_sha(case/'build.json')
        assert host['entire_native_CPU_FP_equal_except_platform_IF'] and host['native_resource_after_exact']
        assert host['hardware_instruction_trace_equal_original'] and host['hardware_ordered_stores_equal_original']
        for name in ('hardware-exit.bin','hardware-trace.bin','hardware-stores.bin','replay-x86'):
            assert file_sha(hardware/name)==host['files_sha256'][name]
        host_memory=regions(retained(hardware/'hardware-after.bin',base,host['files_sha256']['hardware-after.bin']))
        assert host_memory.keys()==native.keys()
        for address,raw in native.items():
            if 0x60000000<=address and address+len(raw)<=0x60200000:continue
            assert raw==host_memory[address],hex(address)
        host_cpu=bytearray((hardware/'hardware-exit.bin').read_bytes())
        flags=struct.unpack_from('<I',host_cpu,160)[0];assert flags&0x200
        assert not struct.unpack_from('<I',native_cpu,160)[0]&0x200
        struct.pack_into('<I',host_cpu,160,flags&~0x200);assert host_cpu==native_cpu
        assert file_sha(case/'native-instructions.bin')==build['files_sha256']['native-instructions.bin']
        observed=list(struct.iter_unpack('<II',(case/'native-instructions.bin').read_bytes()))
        assert all(domain in (0,1) and (not domain or pc in pcs) for pc,domain in observed)
        selected_trace=[pcs[pc] if domain else pc for pc,domain in observed]
        original_trace=[pc for pc, in struct.iter_unpack('<I',(original/'original-trace.bin').read_bytes())]
        assert selected_trace==original_trace
        host_stores=list(stores(hardware/'hardware-stores.bin'));assert host_stores==all_original
        result.update(independent_host_FP_and_complete_instruction_trace_verified=True,
            captured_call_under_declared_native_resource_ABI_verified=True,
            independent_host_report_sha256=file_sha(hardware/'report.json'),
            native_instruction_trace_sha256=file_sha(case/'native-instructions.bin'),
            selected_native_instruction_callbacks_equal_original=len(selected_trace),
            canonical_damage_instruction_callbacks=sum(domain for _,domain in observed),
            independent_host_instruction_callbacks=host['independent_single_step_trace']['instructions'],
            architectural_instruction_observations=host['architectural_instruction_observations'],
            REP_callback_policy=host['REP_callback_policy'],independent_host_ordered_stores_equal=len(host_stores),
            independent_host_full_CPU_FP_equal_except_platform_IF=True,
            independent_host_retained_nonstack_memory_equal=compared,
            emulated_x87_status_difference_resolved_by_actual_host=True,
            original_async_queue_worker_verified=False,complete_original_native_call_verified=False,
            remaining='Untaken projectile branches, original asynchronous resource scheduling, complete current-root runtime and matched Xbox TU1.5 visual/audio sequence remain unverified')
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ('passed','original_instructions','complete_retained_nonstack_bytes_equal','ordered_source_store_records_equal','native_x87_status','reference_emulated_x87_status','complete_original_native_call_verified')}))


if __name__=='__main__':main()
