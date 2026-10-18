#!/usr/bin/env python3
"""Verify an actual Needler detonation call against independent host CPU.

Native locale/cache contracts remain explicit. Console EEPROM, asynchronous
I/O scheduling, full runtime delivery and Xbox rendered/audio parity are not
certified. The emulator's differing FP path is retained, never made a golden.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from audit_rendering import Cache,file_sha
from verify_captured_needler_projectile_tu15 import retained,regions,stores
from research_mp_sandbox_update_functions import NEW_PIN


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('case','original','host','baseline-flow','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--kind',choices=('super','standard'),default='super')
    args=parser.parse_args();case=args.case.absolute();base=case.parents[1];original=args.original.absolute();host=args.host.absolute()
    assert not args.output.exists()
    build=json.loads((case/'build.json').read_text());capture=json.loads((case/'native.json').read_text())
    run=json.loads((case.parent/'run.json').read_text());flow=json.loads((case.parent/'report.json').read_text())
    reference=json.loads((original/'report.json').read_text());hardware=json.loads((host/'report.json').read_text())
    baseline=json.loads((args.baseline_flow/'report.json').read_text())
    assert run['wrapper_returncode']==0 and run['requested_capture_completed'] and flow['passed']
    assert flow['completed_ticks']==240 and flow['ticks']==baseline['ticks'] and flow['rounds_consumed']==11
    assert file_sha(case.parent/'report.json')==build['source_flow_report_sha256']==run['actual_report_sha256']
    assert file_sha(case.parent/'run.json')==build['source_flow_receipt_sha256']
    assert file_sha(Path(build['command'][0]))==build['binary_sha256']
    for name,digest in build['linked_files_sha256'].items():assert file_sha(Path(name))==digest
    tick=capture['sequence_tick'];assert tick==run['capture_tick'] and 0<tick<240
    assert capture['completed'] and capture['entry']==0xfdeb0
    assert run['capture_projectile']==reference['source_context']['actual_source_projectile']
    assert reference['origin_case']==str(case) and reference['completed'] and reference['error'] is None
    selection='selected_at_first_target_death_tick' if args.kind=='super' else 'selected_at_first_standard_detonation_tick'
    assert reference['source_context'][selection]
    assert reference['original_xbe_sha256']==NEW_PIN and not reference['instruction_patches'] and not reference['supplied_game_services']
    assert reference['native_synchronization_after_matches_actual_capture'] and reference['native_animation_resource_after_matches_actual_capture']
    assert hardware['passed'] and hardware['completed'] and hardware['returncode']==0
    assert hardware['original_report_sha256']==file_sha(original/'report.json') and hardware['actual_source_build_sha256']==file_sha(case/'build.json')
    assert hardware['native_retained_memory_equal'] and hardware['entire_native_CPU_FP_equal_except_platform_IF']
    assert hardware['hardware_instruction_trace_equal_native'] and hardware['hardware_ordered_source_stores_equal_native']
    for name in ('native.json','native-input.bin','native-input-full.bin','native-stores.bin','native-instructions.bin','native-exit.bin','native-synchronization-before.json','native-synchronization-after.json'):
        assert file_sha(case/name)==build['files_sha256'][name]
    for name in ('hardware-exit.bin','hardware-trace.bin','hardware-stores.bin','replay-x86'):
        assert file_sha(host/name)==hardware['files_sha256'][name]
    before=regions(retained(case/'before.bin',base,build['files_sha256']['before.bin']))
    after=regions(retained(case/'after.bin',base,build['files_sha256']['after.bin']))
    host_after=regions(retained(host/'hardware-after.bin',base,hardware['files_sha256']['hardware-after.bin']))
    assert after.keys()==host_after.keys();compared=0
    for address,raw in after.items():
        if 0x60000000<=address and address+len(raw)<=0x60200000:continue
        assert raw==host_after[address],hex(address);compared+=len(raw)
    def reader(memory):
        def read(address,size):
            hits=[bytes(raw[address-start:address-start+size]) for start,raw in memory.items() if start<=address and address+size<=start+len(raw)]
            assert len(hits)==1
            return hits[0]
        return read,lambda p,n=4:int.from_bytes(read(p,n),'little')
    old_read,old_word=reader(before);new_read,new_word=reader(after)
    def object_body(word,datum):
        pool=word(0x4ecbd0);index=datum&65535
        assert word(pool+0x24)==12 and index<word(pool+0x38)<=word(pool+0x20)
        assert word(word(pool+0x48)+(index//32)*4)&(1<<(index%32))
        header=word(pool+0x44)+index*12;assert word(header,2)==datum>>16
        return word(header+8)
    projectile=run['capture_projectile'];target=flow['ticks'][tick]['target']['unit']
    old_body=object_body(old_word,target);new_body=object_body(new_word,target)
    old_health=struct.unpack('<f',old_read(old_body+0xec,4))[0];new_health=struct.unpack('<f',new_read(new_body+0xec,4))[0]
    new_shield=struct.unpack('<f',new_read(new_body+0xf0,4))[0]
    if args.kind=='super':assert old_health>0 and new_health<0 and not flow['ticks'][tick-1]['target']['dead']
    else:assert old_health<0 and new_health<old_health and flow['ticks'][tick-1]['target']['dead']
    assert new_shield==0 and flow['ticks'][tick]['target']['dead']
    # Other projectile/queued-effect work can still run after this captured
    # call, particularly when the selected projectile is visited first.
    end_tick_health=flow['ticks'][tick]['target']['health']
    if args.kind=='super':assert struct.pack('<f',end_tick_health)==struct.pack('<f',new_health)
    previous={p['datum'] for p in flow['ticks'][tick-1]['projectiles']};current={p['datum'] for p in flow['ticks'][tick]['projectiles']}
    assert previous-current=={projectile}
    projectile_before=object_body(old_word,projectile);projectile_after=object_body(new_word,projectile)
    assert projectile_before==projectile_after
    old_flags,new_flags=old_word(projectile_before+4),new_word(projectile_after+4)
    if args.kind=='super':assert not old_flags&0x100 and new_flags&0x100
    else:assert not old_flags&0x100 and not new_flags&0x100
    def effects(word):
        pool=word(0x4f720c);assert word(pool+0x24)==400 and word(pool+0x38)<=word(pool+0x20)
        result={}
        for index in range(word(pool+0x38)):
            if not word(word(pool+0x48)+(index//32)*4)&(1<<(index%32)):continue
            row=word(pool+0x44)+index*400;result[(word(row,2)<<16)|index]=word(row+4)
        return result
    old_effects,new_effects=effects(old_word),effects(new_word);born={p:d for p,d in new_effects.items() if p not in old_effects}
    map_path=Path(run['command'][1]);assert file_sha(map_path)==run['map_sha256'];cache=Cache(map_path)
    try:
        effect_name='effects\\impact\\explosion_small\\needler\\'+args.kind+'_detonation'
        effect_ids={datum for datum,tag in cache.tags.items() if tag['group']=='effe' and tag['name']==effect_name}
        assert len(effect_ids)==1 and effect_ids&set(born.values())
        born_names=[dict(instance=datum,definition=definition,name=cache.tags[definition]['name']) for datum,definition in born.items()]
    finally:cache.close()
    ownership_before=json.loads((case/'native-synchronization-before.json').read_text())
    ownership_after=json.loads((case/'native-synchronization-after.json').read_text())
    assert ownership_before['source_resources_identity']==ownership_after['source_resources_identity']
    reads=[event for event in reference['native_platform_services'] if event.get('target')==0x1a3b70]
    if args.kind=='super':assert [e['kind'] for e in reads]==['actual native synchronous animation resource read','actual native synchronous geometry resource read','actual native synchronous texture resource read','actual native synchronous texture resource read']
    else:assert not reads
    old,new=ownership_before['native_resource_reads'],ownership_after['native_resource_reads']
    assert new['jobs'][:len(old['jobs'])]==old['jobs'] and len(new['jobs'])==len(old['jobs'])+len(reads)
    assert new['next']==old['next']+len(reads) and hardware['native_resource_after']==new
    host_stores=list(stores(host/'hardware-stores.bin'));io_stores=[row for row in host_stores if row[0] in (0x126139,0x130f68,0x1301b0)]
    assert len(io_stores)==2*len(reads)
    for index,event in enumerate(reads):
        assert new['jobs'][len(old['jobs'])+index]==dict(job=event['allocated_job'],descriptor=event['descriptor'],destination=event['destination'],bytes=event['bytes'])
        record,done=io_stores[index*2:index*2+2]
        assert record[0]==done[0]==event['caller_instruction'] and record[1]==event['destination']
        assert len(record[3])==event['bytes'] and hashlib.sha256(record[3]).hexdigest()==event['payload_sha256'] and done[3]==b'\x01'
        location=event['encoded_offset']>>30;offset=event['encoded_offset']&0x3fffffff
        backing=map_path if not location else Path(run['command'][2])/('mainmenu.map','shared.map','single_player_shared.map')[location-1]
        digest=run['map_sha256'] if not location else ownership_before['source_resources_identity']['shared'][location-1]
        assert file_sha(backing)==digest
        with backing.open('rb') as stream:stream.seek(offset);assert stream.read(event['bytes'])==record[3]
    native_cpu=(case/'native-exit.bin').read_bytes();host_cpu=bytearray((host/'hardware-exit.bin').read_bytes())
    flags=struct.unpack_from('<I',host_cpu,160)[0];assert flags&0x200
    struct.pack_into('<I',host_cpu,160,flags&~0x200);assert native_cpu==host_cpu
    result=dict(passed=True,scope=__doc__,actual_source_build_sha256=file_sha(case/'build.json'),source_flow_report_sha256=file_sha(case.parent/'report.json'),
        original_report_sha256=file_sha(original/'report.json'),host_report_sha256=file_sha(host/'report.json'),original_xbe_sha256=NEW_PIN,
        tick=tick,detonation_kind=args.kind,actual_source_projectile=projectile,health_before=old_health,health_after=new_health,shield_after=new_shield,
        published_player_health_at_end_of_source_tick=end_tick_health,
        published_player_health_scope='Cached player Unit presentation; this is not a second raw corpse-body vitality capture',
        born_effects=born_names,authored_super_detonation_created_within_call=args.kind=='super',
        authored_standard_detonation_created_within_call=args.kind=='standard',target_health_depleted_within_call=old_health>0 and new_health<0,
        full_projectile_identity_still_retained_at_call_exit=True,projectile_body_flag_0x100_published=bool(new_flags&0x100),
        projectile_retired_by_end_of_source_tick=True,projectile_body_flags_before=old_flags,projectile_body_flags_after=new_flags,
        whole_240_tick_gameplay_rows_unchanged=True,independent_host_complete_CPU_FP_equal_except_platform_IF=True,
        independent_host_retained_nonstack_bytes_equal=compared,native_instruction_callbacks=hardware['native_instruction_callbacks'],
        host_instruction_callbacks=hardware['independent_single_step_trace']['instructions'],architectural_instruction_observations=hardware['architectural_instruction_observations'],
        ordered_native_source_store_records_equal=hardware['native_ordered_source_stores'],host_raw_store_records=hardware['independent_single_step_trace']['stores'],
        host_vector_stores_split_for_native_observer=hardware['host_vector_stores_split_for_native_observer'],
        separately_checked_native_IO_writes=len(io_stores),actual_cache_read_bytes=sum(e['bytes'] for e in reads),actual_cache_read_jobs=len(reads),
        native_resource_and_synchronization_after_exact=True,
        emulator_FP_and_retained_memory_match_host=hardware['raw_emulator_FP_equal'] and hardware['emulator_retained_memory_equal'],
        emulator_uses_additional_CRT_control_path=reference['original_instructions']>hardware['native_instruction_callbacks'],
        emulator_original_instruction_callbacks=reference['original_instructions'],
        original_async_resource_worker_verified=False,Xbox_EEPROM_discovery_verified=False,full_current_root_runtime_verified=False,
        physical_xbox_verified=False,rendered_audio_sequence_verified=False,full_weapon_parity=False,tool_sha256=file_sha(Path(__file__)))
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ('passed','detonation_kind','health_before','health_after','native_instruction_callbacks','host_instruction_callbacks','ordered_native_source_store_records_equal','actual_cache_read_bytes','authored_super_detonation_created_within_call','authored_standard_detonation_created_within_call')}))


if __name__=='__main__':main()
