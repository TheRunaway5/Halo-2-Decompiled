"""Authenticate actual Needler feedback, projectile, particle and queue callers."""
import json
import struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def validate(case, metadata, regions):
    entry=metadata['entry']
    assert entry in (0xfdeb0,0x1f5620,0xe3e20,0x178e30,0x179df0,0x1770c0) and metadata['actual_source_weapon_sequence']
    build = json.loads((case/'build.json').read_text())
    assert build['returncode'] == (0 if metadata['completed'] else 1)
    assert build['scope']['read_only_entry_capture'] and build['scope']['game_results_not_supplied']
    assert file_sha(Path(build['source'])) == build['source_sha256']
    assert file_sha(Path(build['command'][0])) == build['binary_sha256']
    for path, digest in build['linked_files_sha256'].items():
        assert file_sha(Path(path)) == digest
    for name in ('before.bin','native-input.bin','native-input-full.bin','native-fp-input.bin','native-stores.bin','native.json'):
        assert file_sha(case/name) == build['files_sha256'][name]
    if metadata['completed']:
        for name in ('after.bin','native-exit.bin'):
            assert file_sha(case/name) == build['files_sha256'][name]
    parent_path=case.parent/'report.json'
    assert file_sha(parent_path) == build['source_flow_report_sha256']
    assert file_sha(case.parent/'run.json') == build['source_flow_receipt_sha256']
    flow=json.loads(parent_path.read_text())
    assert flow['velocity_queries_use_private_scratch'] and not flow['source_state_or_game_results_supplied']
    assert flow['declared_native_inputs']['local_players'] == 2
    assert len(flow['ticks']) >= 165
    if not metadata['completed']:
        assert flow['failure_tick'] == flow['completed_ticks'] == metadata['sequence_tick'] == 165
        assert f'-> {entry:x} return {metadata["caller_return"]:x}' in flow['failure']

    def read(address,size):
        for base,raw in regions:
            if base <= address and address+size <= base+len(raw):
                return raw[address-base:address-base+size]
        raise AssertionError(hex(address))
    def word(address,size=4):return int.from_bytes(read(address,size),'little')
    raw=(case/'native-input.bin').read_bytes();fp=(case/'native-fp-input.bin').read_bytes()
    assert len(raw)==168 and raw+fp==(case/'native-input-full.bin').read_bytes()
    depth,status,control,mxcsr=struct.unpack('<IHHI',fp)
    assert depth==0 and control==0x27f and mxcsr&~0x3f==0x1f80
    gp=struct.unpack_from('<8I',raw);sp=gp[4]
    assert 0x60000000<=sp<0x601fffc0 and struct.unpack_from('<I',raw,164)[0]==0x60200000
    image=Xbe(NEW_DEFAULT);assert file_sha(NEW_DEFAULT)==NEW_PIN
    code=_Code(image,'.text')
    if entry==0xfdeb0:
        import re
        assert metadata['completed']
        assert flow['completed_ticks']==240 and flow['rounds_consumed']==11
        first_damage=next(row['tick'] for previous,row in zip(flow['ticks'],flow['ticks'][1:])
                          if row['target']['shield']<previous['target']['shield'])
        first_death=next(row['tick'] for row in flow['ticks'] if row['target']['dead'])
        from audit_rendering import Cache
        map_path=Path(build['command'][1]);cache=Cache(map_path)
        try:
            standard=[datum for datum,tag in cache.tags.items() if tag['group']=='effe' and tag['name']=='effects\\impact\\explosion_small\\needler\\standard_detonation']
            assert len(standard)==1
        finally:cache.close()
        first_standard=next(sample['tick'] for sample in flow['visual_samples']
                            if any(effect['definition']==standard[0] for effect in sample['effects']['active']))
        tick=metadata['sequence_tick'];assert tick in (first_damage,first_death,first_standard)
        assert first_damage==118 and first_death==165 and not flow['ticks'][first_death-1]['target']['dead']
        selected_tick=json.loads((case.parent/'run.json').read_text()).get('capture_tick')
        assert selected_tick is None or selected_tick==tick
        support=Path(__file__).resolve().parents[1]/'src/physics_live_support_tu15_generated.cpp'
        entries={int(value,16) for value in re.findall(r'static void run_([0-9a-f]+)\(WorldState &',support.read_text())}
        assert 0xfdde0 in entries and entry in entries
        caller=_trace(code,0xfdde0,entries)
        calls=[code.at(pc) for pc in caller.code if code.at(pc).mnemonic=='call'
               and code.at(pc).address+code.at(pc).size==metadata['caller_return']]
        assert len(calls)==1 and calls[0].operands[0].imm==entry
        call=calls[0];assert word(sp)==call.address+call.size==metadata['caller_return']
        projectile=word(sp+4);assert projectile!=0xffffffff and projectile==gp[5]==gp[7]
        selected_projectile=json.loads((case.parent/'run.json').read_text()).get('capture_projectile')
        assert selected_projectile is None or selected_projectile==projectile
        if tick==first_standard:
            assert tick>0 and selected_projectile==projectile
            prior={p['datum'] for p in flow['ticks'][tick-1]['projectiles']}
            current={p['datum'] for p in flow['ticks'][tick]['projectiles']}
            assert prior-current=={projectile}
        pool=word(0x4ecbd0);assert word(pool+0x24)==12
        high,capacity=word(pool+0x38),word(pool+0x20);index=projectile&65535
        assert 0<capacity<=65536 and index<high<=capacity
        assert word(word(pool+0x48)+(index//32)*4)&(1<<(index%32))
        header=word(pool+0x44)+index*12
        assert word(header,2)==projectile>>16 and word(header+3,1)==5
        body=word(header+8);definition=word(body);assert definition==flow['projectile_definition']
        tag=word(0x4f0414)+(definition&65535)*16
        # The original cache table stores FourCC words in canonical numeric
        # order; its little-endian bytes are reversed from the group name.
        assert word(tag)==int.from_bytes(b'proj','big') and word(tag+4)==definition
        section=next(s for s in image.sections if s.name=='.text')
        assert read(section.va,len(image.section_bytes(section)))==image.section_bytes(section)
        return dict(actual_native_projectile_update_API=entry,actual_native_caller=True,
            original_caller_entry=0xfdde0,original_call_contract=call.address,
            caller_stack=sp,callee_stack_arguments=[projectile],actual_source_projectile=projectile,
            source_projectile_definition=definition,source_projectile_body=body,
            sequence_tick=tick,selected_at_first_damage_tick=tick==first_damage,
            selected_at_first_target_death_tick=tick==first_death,
            selected_at_first_standard_detonation_tick=tick==first_standard,
            authored_standard_detonation_definition=standard[0],
            captured_call_contact_branch_verified=False,captured_call_detonation_branch_verified=False,
            local_render_metadata_fields_applicable=False,
            source_clock_tick=word(word(0x51df54)+8),
            map=str(Path(build['command'][1])),map_sha256=file_sha(Path(build['command'][1])),
            actual_source_weapon_sequence=True,native_completed=True,
            actual_API_instruction_trace_available=(case/'native-instructions.bin').is_file(),physical_xbox_caller_captured=False,
            rendered_sequence_verified=False,full_weapon_parity=False,
            validation_tool_sha256=file_sha(Path(__file__)))
    if entry==0x1770c0:
        from inspect_multiplayer import Cache
        assert metadata['completed'] and metadata['sequence_tick']==175
        assert metadata['sequence_view']==0 and not metadata['first_person']
        assert flow['completed_ticks']==flow['source_particle_frames_completed']==240
        assert word(sp)==metadata['caller_return']==0x60300000 and word(sp+4)==0
        assert word(0x4c66f8)==0 and word(0x4c2aa8)!=0
        assert flow['declared_native_inputs']['portal_mask_owner']['no_machine_owners_required']
        assert flow['declared_native_inputs']['portal_mask_owner']['authored_machine_associations']==0
        view=next(v for v in flow['native_view_samples'] if v['tick']==175 and v['view']==0)
        for field,offset in (('position',0),('forward',12),('up',24)):
            assert read(0x4c65c0+offset,12)==struct.pack('<3f',*view[field])
        assert word(0x4c65e8)==int.from_bytes(struct.pack('<f',view['projection_angle']),'little')
        call=code.at(0x40f1a)
        assert call.mnemonic=='call' and call.operands[0].imm==entry
        cache=Cache(Path(build['command'][1]))
        try:
            definitions={int(t['datum'],16):t['name'] for t in cache.tags.values() if t['group']=='effe' and
                t['name'] in ('effects\\impact\\explosion_small\\needler\\standard_detonation',
                    'effects\\impact\\explosion_small\\needler\\super_detonation')}
        finally:cache.data.close();cache.stream.close()
        effects=word(0x4f720c);assert word(effects+0x24)==400
        positive=[]
        for index in range(word(effects+0x38)):
            if word(word(effects+0x48)+(index//32)*4)&(1<<(index%32)):
                body=word(effects+0x44)+index*400;definition=word(body+4)
                if definition in definitions:positive.append(dict(instance=(word(body,2)<<16)|index,
                    definition=definition,name=definitions[definition]))
        assert positive,'World queue capture must retain actual positive Needler detonation records'
        section=next(s for s in image.sections if s.name=='.text')
        assert read(section.va,len(image.section_bytes(section)))==image.section_bytes(section)
        return dict(actual_native_particle_queue_API=entry,actual_native_caller=True,
            original_call_contract=call.address,callee_stack_arguments=[0],caller_stack=sp,
            sequence_tick=175,sequence_view=0,first_person=False,
            map=str(Path(build['command'][1])),map_sha256=file_sha(Path(build['command'][1])),
            actual_source_weapon_sequence=True,active_needler_detonations=positive,
            native_completed=True,rendered_sequence_verified=False,full_weapon_parity=False,
            validation_tool_sha256=file_sha(Path(__file__)))
    if entry==0x179df0:
        from inspect_multiplayer import Cache
        capture_tick=flow['declared_native_inputs'].get('particle_phase_capture_tick',180)
        assert metadata['completed'] and metadata['sequence_tick']==capture_tick
        assert capture_tick==180 or capture_tick==flow['declared_native_inputs']['fire_first_tick']==80
        assert flow['source_particle_frames_completed']==flow['completed_ticks']==240
        assert gp==(0,0,0,0,0x600f0000,0,0,0) and raw[32:160]==bytes(128)
        assert struct.unpack_from('<I',raw,160)[0]==2 and word(sp)==metadata['caller_return']==0x60300000
        seconds=flow['declared_native_inputs']['native_particle_frame_seconds']
        delta=int.from_bytes(struct.pack('<f',seconds),'little')
        assert word(sp+4)==delta==word(word(0x51df54)+4)
        for name in ('native-trace.bin','native-cpu-inputs.bin'):
            assert file_sha(case/name)==build['files_sha256'][name]
        cache=Cache(Path(build['command'][1]))
        try:
            effect_name=('effects\\objects\\weapons\\pistol\\needler\\firing' if capture_tick==80 else
                'effects\\impact\\explosion_small\\needler\\standard_detonation')
            effect=next(int(t['datum'],16) for t in cache.tags.values()
                if t['group']=='effe' and t['name']==effect_name)
        finally:cache.data.close();cache.stream.close()
        effects=word(0x4f720c);assert word(effects+0x24)==400 and word(effects+0x38)<=word(effects+0x20)==256
        active=[]
        for index in range(word(effects+0x38)):
            if word(word(effects+0x48)+(index//32)*4)&(1<<(index%32)):
                record=word(effects+0x44)+index*400
                if word(record+4)==effect:active.append((word(record,2)<<16)|index)
        assert active,'Particle reference must retain its actual positive Needler effect'
        section=next(s for s in image.sections if s.name=='.text')
        assert read(section.va,len(image.section_bytes(section)))==image.section_bytes(section)
        return dict(map=str(Path(build['command'][1])),map_sha256=file_sha(Path(build['command'][1])),
            actual_native_particle_API=entry,actual_native_caller=True,
            original_call_contract=0x13aa28,callee_stack_arguments=[delta],caller_stack=sp,
            caller_delta_bits=delta,sequence_tick=metadata['sequence_tick'],
            source_clock_tick=word(word(0x51df54)+8),actual_source_weapon_sequence=True,
            source_weapon=flow['weapon_definition'],source_projectile=flow['projectile_definition'],
            needler_effect_definition=effect,needler_effect_name=effect_name,active_needler_effects=active,
            **(dict(needler_detonation_definition=effect,active_needler_detonations=active) if capture_tick==180 else {}),
            actual_API_instruction_trace_available=True,native_completed=True,
            rendered_sequence_verified=False,full_weapon_parity=False,
            validation_tool_sha256=file_sha(Path(__file__)))
    call=code.at({0x1f5620:0xe0ddd,0xe3e20:0xe0de6,0x178e30:0x100c1c}[entry])
    assert call.mnemonic=='call' and call.operands[0].imm==metadata['entry']
    assert word(sp)==call.address+call.size==metadata['caller_return']
    if entry==0x178e30:
        request=word(sp+4)
        push=code.at(0x100c1b)
        assert push.mnemonic=='push' and push.op_str=='edx' and request==gp[2]
        section=next(s for s in image.sections if s.name=='.text')
        assert read(section.va,len(image.section_bytes(section)))==image.section_bytes(section)
        assert flow['weapon_definition']==3813343702 and flow['projectile_definition']==3818258977
        return dict(map=str(Path(build['command'][1])),map_sha256=file_sha(Path(build['command'][1])),
            actual_nested_caller=call.address,callee_stack_arguments=[request],caller_stack=sp,
            request=request,request_flags=word(request),request_effect=word(request+4),
            request_bytes=0x60,request_hex=read(request,0x60).hex(),
            sequence_tick=metadata['sequence_tick'],actual_source_weapon_sequence=True,
            source_weapon=flow['weapon_definition'],source_projectile=flow['projectile_definition'],
            source_clock_tick=word(word(0x51df54)+8),native_completed=metadata['completed'],
            full_weapon_parity=False,validation_tool_sha256=file_sha(Path(__file__)))
    unit=word(sp+4) if entry==0x1f5620 else gp[0]
    if entry==0xe3e20:
        flag=code.at(0xe0de2)
        assert flag.mnemonic=='push' and flag.operands[0].imm==word(sp+4)==1
    arguments=[unit] if entry==0x1f5620 else [1]
    pool=word(0x4ecbd0);header=word(pool+0x44)+(unit&65535)*12
    assert word(header,2)==unit>>16
    body=word(header+8)
    assert gp[0]==gp[5]==unit and gp[7]==body
    if entry==0x1f5620:assert gp[1]==body+0x3ec
    previous=next(row for row in flow['ticks'] if row['tick']==metadata['sequence_tick']-1)
    assert previous['target']['unit']==unit and previous['target']['shield']<1
    stuck={p['datum'] for row in flow['ticks'] if row['tick']<metadata['sequence_tick']
           for p in row['projectiles'] if p['parent']==unit}
    assert len(stuck)==11
    section=next(s for s in image.sections if s.name=='.text')
    assert read(section.va,len(image.section_bytes(section)))==image.section_bytes(section)
    return dict(map=str(Path(build['command'][1])),map_sha256=file_sha(Path(build['command'][1])),
        actual_nested_caller=call.address,callee_stack_arguments=arguments,unit=unit,body=body,
        caller_stack=sp,sequence_tick=metadata['sequence_tick'],actual_source_weapon_sequence=True,
        source_weapon=flow['weapon_definition'],source_projectile=flow['projectile_definition'],
        attached_projectiles_seen=len(stuck),source_clock_tick=word(word(0x51df54)+8),
        native_completed=metadata['completed'],full_weapon_parity=False,
        validation_tool_sha256=file_sha(Path(__file__)))
