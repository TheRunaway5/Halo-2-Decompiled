#!/usr/bin/env python3
"""Execute an untouched TU1.5 particle frame over a captured charge-effect bank.

The source world and charge callback produced the input records. The probe
supplies only a declared caller ABI and frame delta, and supplies no function
returns or game/platform service policies. This is original consumer evidence,
not a native implementation, a boot or a complete rendered weapon sequence.
Optional native platform boundaries require real captured resource backing;
they do not establish Xbox kernel instructions or register-clobber behavior.
"""
import argparse,csv,hashlib,json,shutil,struct
from collections import Counter
from pathlib import Path
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code
from oracle_charged_effect_lifetime_bank_tu15 import regions,segment
from validate_charge_effect_bank_tu15 import validate
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ROOT=Path(__file__).resolve().parents[1]
REGISTERS=[getattr(R,'UC_X86_REG_'+s) for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--case',type=Path,required=True)
    ap.add_argument('--captured-weapon-feedback',action='store_true',help='Use an actual Needler callback/particle bank, or an explicitly audited camera CRT leaf with that bank and complete FP input')
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--entry',type=lambda x:int(x,0),choices=(0xfdeb0,0x382360,0x382380,0x1f5620,0xe3e20,0x171f40,0x171e10,0x228260,0x22570,0x10fa30,0x10fa90,0x1888f0,0x11e550,0x126730,0x1a59d0,0x1a5410,0x191df0,0x27fa20,0x2afa50,0x218020,0x1d63c0,0x26f260,0x1d1880,0x2035b0,0x2035f0,0x2039b0,0x2658b0,0x266d30,0x18b4c0,0xb9090,0xb8f40,0x11ddf0,0x178e30,0x116400,0x116440,0x1164d0,0xb9180,0x179df0,0x13a9b0,0x179cd0,0x22c440,0x22c480,0x17ead0,0x180520,0xd72f0,0x18ee50,0x219b90,0x1378e0,0x3f3e0,0x1770c0,0x1772c0,0x1e0f0,0x1373f0,0x136ea0,0x176b30,0x219080,0x2190c0,0x175f90,0x380fa0,0x108db0,0x1030f0,0x1e7e40,0x1e9000,0x1e8410,0x1d0670,0x1d03a0,0x11e660,*range(0x381a60,0x381ed0,0x10)),default=0x179df0)
    ap.add_argument('--phase',choices=('before','after'),default='after')
    ap.add_argument('--previous',type=Path,help='Continue from an authenticated original hardware result')
    ap.add_argument('--lifetime-frame',type=int,choices=range(1,129),metavar='1..128',help='Replay a captured source-world particle API frame')
    ap.add_argument('--native-call',choices=['visibility-consumer','transparent-init','view-reset','view-admission','world-draw',*(f'active-frame{n}' for n in range(1,17))],help='Replay an authenticated native visibility or active-particle input bank')
    ap.add_argument('--queued-callback',type=int,help='Invoke this actual transparent-command callback from a preceding authenticated hardware bank')
    ap.add_argument('--captured-weapon-tick',action='store_true',help='Use the retained actual nested weapon caller, including its empty-stack FP state')
    ap.add_argument('--captured-contact',action='store_true',help='Use the actual unresolved public D1878 contact caller and its complete FP input')
    ap.add_argument('--captured-level-handles-new-map',action='store_true',help='Use the actual13A7BA registered row13 new-map callback')
    ap.add_argument('--captured-resource-cache-new-map',action='store_true',help='Use the actual13A7BA registered row16 resource-cache callback')
    ap.add_argument('--captured-registered-new-map',action='store_true',help='Use an authenticated actual cold registered new-map callback')
    ap.add_argument('--captured-shader-function-startup',action='store_true',help='Use the actual native registered row29 shader-function constructor')
    ap.add_argument('--captured-render-object-startup',action='store_true',help='Use actual native registered row36/row43 constructors')
    ap.add_argument('--captured-level-handles-startup',action='store_true',help='Use the actual registered row13 level-handles constructor caller')
    ap.add_argument('--host-pool-pages',action='store_true',help='Declare the existing native raw-pool mmap ABI for the captured row13 constructor; no Xbox kernel-clobber claim')
    ap.add_argument('--host-game-start-memory',action='store_true',help='Use actual host-backed pages and mprotect at the declared game-start physical-protection boundary')
    ap.add_argument('--native-synchronization-library',type=Path,help='Replay the actual captured projectile arena mutex/semaphore ownership using real native waits; no prescribed success or Xbox kernel claim')
    ap.add_argument('--native-animation-resource-provider',type=Path,help='Use actual authenticated cache sectors and restored read ownership at the existing native126139 animation I/O boundary; excludes original asynchronous queue scheduling')
    ap.add_argument('--native-interface-language',action='store_true',help='Use the actual compiled native World English-language input at its declared XGetLanguage callers; excludes Xbox EEPROM discovery')
    ap.add_argument('--native-weapon-resource-provider',type=Path,help='Use the existing native animation, geometry and texture I/O contracts with actual cache sectors and restored read ownership')
    ap.add_argument('--declared-game-start',action='store_true',help='Use the explicit native191DF0 startup bridge input before object birth')
    ap.add_argument('--captured-script-builtin',action='store_true',help='Use the actual public211C1B descriptor callback and complete incoming ABI')
    ap.add_argument('--captured-unit-notification',action='store_true',help='Use the actual public11750D unit callback with its register operands and retained FP input')
    ap.add_argument('--captured-animation',action='store_true',help='Use the actual public1D5897 animation caller and complete retained FP input')
    ap.add_argument('--captured-damage-notification',action='store_true',help='Use the actual public CC9CD damage notification caller and complete FP input')
    ap.add_argument('--captured-recorded-animation',action='store_true',help='Use an authenticated actual row55 callback or public Hunter query input')
    ap.add_argument('--captured-notification',action='store_true',help='Use the actual public204553 notification caller, six stack operands and complete FP input')
    ap.add_argument('--material-control',choices=('alive','dead','model-fallback','empty-model-damage','no-model','non-unit-weapon'),help='Declared disposable material-query operand control over the authenticated captured caller')
    ap.add_argument('--recorded-query-control',choices=('active-match','active-mismatch','flag-excluded','inactive-bitmap'),help='Declared disposable recorded-animation query operands over the authenticated initialized caller')
    ap.add_argument('--captured-projectile-effect',action='store_true',help='Use the actual nested projectile hit-effect creator caller and retained FP state')
    ap.add_argument('--captured-unit-action',action='store_true',help='Use the actual nested public Hunter unit-action caller and FP state')
    ap.add_argument('--captured-hunter-particle-frame',action='store_true',help='Use the actual Hunter particle API caller and retained FP state')
    a=ap.parse_args();case=a.case.resolve();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    assert not a.host_pool_pages or a.captured_level_handles_startup
    assert not a.host_game_start_memory or ((a.declared_game_start or a.captured_resource_cache_new_map) and not a.host_pool_pages)
    assert not a.native_synchronization_library or (a.captured_weapon_feedback and a.entry==0xfdeb0 and not a.host_pool_pages and not a.host_game_start_memory)
    assert not a.native_animation_resource_provider or a.native_synchronization_library
    assert not a.native_weapon_resource_provider or (a.native_synchronization_library and not a.native_animation_resource_provider)
    assert not a.native_interface_language or (a.captured_weapon_feedback and a.entry==0xfdeb0)
    metadata=json.loads((case/'native.json').read_text())
    behavior_initializer=a.entry in range(0x381a60,0x381ed0,0x10)
    actor_constructor=a.entry in (0x1e7e40,0x1e9000,0x1e8410,0x1d0670,0x1d03a0) or behavior_initializer
    captured_public_frame_fp=(a.entry in (0x1d0670,0xb8f40,0xb9090,0xb9180) and metadata['entry']==a.entry and
        metadata.get('public_campaign_placement',False) and not a.previous)
    if a.captured_weapon_feedback:
        from validate_captured_weapon_feedback_tu15 import validate as validate_weapon_feedback
        assert a.entry in (0xfdeb0,0x1f5620,0xe3e20,0x178e30,0x179df0,0x1770c0,0x382360,0x382380) and a.phase=='before'
        assert not any((value for name,value in vars(a).items() if name.startswith('captured_') and name!='captured_weapon_feedback'))
        assert not any((a.previous,a.native_call,a.lifetime_frame,a.declared_game_start,a.host_pool_pages,a.host_game_start_memory))
        context=validate_weapon_feedback(case,metadata,list(regions(case/'before.bin')))
        if a.entry in (0x382360,0x382380):
            assert metadata['entry']==0x179df0 and metadata['completed']
            row,operand,destination=({0x382360:(0x46dc90,0x46a164,0x56d324),
                0x382380:(0x46dc94,0x46a5f8,0x56d328)})[a.entry]
            image=Xbe(NEW_DEFAULT)
            assert int.from_bytes(image.read(row,4),'little')==a.entry
            assert image.read(a.entry,17)==(bytes.fromhex('f30f1005')+operand.to_bytes(4,'little')+
                bytes.fromhex('f30f1105')+destination.to_bytes(4,'little')+bytes.fromhex('c3'))
            context=dict(derived_from_actual_native_particle_bank=metadata['entry'],
                actual_captured_initializer_call=False,actual_native_caller=False,
                original_crt_registration_row=row,original_operand=operand,
                original_destination=destination,callee_stack_arguments=[],
                caller_stack=context['caller_stack'],map=context['map'],map_sha256=context['map_sha256'],
                validation_tool_sha256=context['validation_tool_sha256'],
                declared_isolated_constructor_bridge=True,full_weapon_parity=False)
        else:
            assert metadata['entry']==a.entry
        origin_hardware_sha256=None
    elif a.captured_render_object_startup:
        from validate_captured_render_object_startup_tu15 import validate as validate_render_startup
        assert a.entry in (0x171e10,0x228260) and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.declared_game_start,a.captured_shader_function_startup,a.captured_registered_new_map,a.captured_resource_cache_new_map,a.captured_level_handles_new_map,a.captured_level_handles_startup,a.captured_script_builtin,a.captured_unit_notification,a.captured_animation,a.captured_damage_notification,a.captured_recorded_animation,a.captured_notification,a.captured_contact,a.captured_hunter_particle_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick))
        context=validate_render_startup(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_shader_function_startup:
        from validate_captured_shader_function_startup_tu15 import validate as validate_shader_startup
        assert a.entry==0x10fa30 and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.declared_game_start,a.captured_registered_new_map,a.captured_resource_cache_new_map,a.captured_level_handles_new_map,a.captured_level_handles_startup,a.captured_script_builtin,a.captured_unit_notification,a.captured_animation,a.captured_damage_notification,a.captured_recorded_animation,a.captured_notification,a.captured_contact,a.captured_hunter_particle_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick))
        context=validate_shader_startup(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_registered_new_map:
        from validate_captured_registered_new_map_tu15 import validate as validate_registered_map
        assert a.entry in (0x1888f0,0x11e550,0x10fa90,0x22570,0x171f40) and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.declared_game_start,a.captured_resource_cache_new_map,a.captured_level_handles_new_map,a.captured_level_handles_startup,a.captured_script_builtin,a.captured_unit_notification,a.captured_animation,a.captured_damage_notification,a.captured_recorded_animation,a.captured_notification,a.captured_contact,a.captured_hunter_particle_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick))
        context=validate_registered_map(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_resource_cache_new_map:
        from validate_captured_resource_cache_new_map_tu15 import validate as validate_resource_map
        assert a.entry==0x126730 and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.declared_game_start,a.captured_level_handles_new_map,a.captured_level_handles_startup,a.captured_script_builtin,a.captured_unit_notification,a.captured_animation,a.captured_damage_notification,a.captured_recorded_animation,a.captured_notification,a.captured_contact,a.captured_hunter_particle_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick))
        context=validate_resource_map(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_level_handles_new_map:
        from validate_captured_level_handles_new_map_tu15 import validate as validate_level_map
        assert a.entry==0x1a59d0 and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.declared_game_start,a.captured_level_handles_startup,a.captured_script_builtin,a.captured_unit_notification,a.captured_animation,a.captured_damage_notification,a.captured_recorded_animation,a.captured_notification,a.captured_contact,a.captured_hunter_particle_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick))
        context=validate_level_map(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_level_handles_startup:
        from validate_captured_level_handles_startup_tu15 import validate as validate_level_handles
        assert a.entry==0x1a5410 and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.declared_game_start,a.captured_script_builtin,a.captured_unit_notification,a.captured_animation,a.captured_damage_notification,a.captured_recorded_animation,a.captured_notification,a.captured_contact,a.captured_hunter_particle_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick))
        context=validate_level_handles(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.declared_game_start:
        from validate_declared_game_start_tu15 import validate as validate_game_start
        assert a.entry==0x191df0 and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.captured_script_builtin,a.captured_unit_notification,a.captured_animation,a.captured_damage_notification,a.captured_recorded_animation,a.captured_notification,a.captured_contact,a.captured_hunter_particle_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick))
        context=validate_game_start(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_script_builtin:
        from validate_captured_hunter_script_builtin_tu15 import validate as validate_builtin
        assert a.entry in (0x2afa50,0x27fa20) and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.captured_unit_notification,a.captured_animation,a.captured_damage_notification,a.captured_recorded_animation,a.captured_notification,a.captured_contact,a.captured_hunter_particle_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick))
        context=validate_builtin(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_unit_notification:
        from validate_captured_hunter_unit_notification_tu15 import validate as validate_unit_notification
        assert a.entry==0x218020 and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick,a.captured_hunter_particle_frame,a.captured_contact,a.captured_notification,a.captured_recorded_animation,a.captured_damage_notification,a.captured_animation))
        context=validate_unit_notification(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_animation:
        from validate_captured_hunter_animation_tu15 import validate as validate_animation
        assert a.entry==0x1d63c0 and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick,a.captured_hunter_particle_frame,a.captured_contact,a.captured_notification,a.captured_recorded_animation,a.captured_damage_notification))
        context=validate_animation(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_damage_notification:
        from validate_captured_hunter_damage_notification_tu15 import validate as validate_damage_notification
        assert a.entry in (0x1d1880,0x26f260) and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick,a.captured_hunter_particle_frame,a.captured_contact,a.captured_notification,a.captured_recorded_animation))
        context=validate_damage_notification(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_recorded_animation:
        from validate_captured_recorded_animation_tu15 import validate as validate_recorded_animation
        assert a.entry in (0x2035b0,0x2035f0,0x2039b0) and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick,a.captured_hunter_particle_frame,a.captured_contact,a.captured_notification))
        context=validate_recorded_animation(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_notification:
        from validate_captured_hunter_notification_tu15 import validate as validate_notification
        assert a.entry in (0x266d30,0x2658b0) and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick,a.captured_hunter_particle_frame,a.captured_contact))
        context=validate_notification(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_contact:
        from validate_captured_hunter_contact_tu15 import validate as validate_contact
        assert a.entry==0x18b4c0 and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick,a.captured_hunter_particle_frame))
        context=validate_contact(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_hunter_particle_frame:
        from validate_captured_hunter_particle_frame_tu15 import validate as validate_hunter_particle
        assert a.entry==0x179df0 and a.phase=='before' and not any((a.previous,a.native_call,a.lifetime_frame,a.captured_unit_action,a.captured_projectile_effect,a.captured_weapon_tick))
        context=validate_hunter_particle(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_unit_action:
        from validate_captured_hunter_unit_action_tu15 import validate as validate_unit_action
        assert a.entry==0x11ddf0 and a.phase=='before' and not a.previous and not a.native_call and not a.lifetime_frame and not a.captured_weapon_tick and not a.captured_projectile_effect
        context=validate_unit_action(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_projectile_effect:
        from validate_captured_hunter_impact_tu15 import validate as validate_impact
        assert a.entry==0x178e30 and a.phase=='before' and not a.previous and not a.native_call and not a.lifetime_frame and not a.captured_weapon_tick
        context=validate_impact(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.captured_weapon_tick:
        from validate_captured_hunter_weapon_tick_tu15 import validate as validate_weapon
        assert a.entry==0x1030f0 and a.phase=='before' and not a.previous and not a.native_call and not a.lifetime_frame
        context=validate_weapon(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.entry in (0xb8f40,0xb9090,0xb9180):
        from validate_hunter_ai_frame_bank_tu15 import validate as validate_public_entry
        assert metadata['public_campaign_placement'] and a.phase=='before' and not a.previous and not a.native_call and not a.lifetime_frame
        context=validate_public_entry(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.entry in (0x1164d0,0x116440,0x116400):
        from validate_hunter_unit_query_bank_tu15 import validate as validate_unit_query
        assert a.phase=='before' and not a.previous and not a.native_call and not a.lifetime_frame
        context=validate_unit_query(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif a.entry==0x11e660:
        from validate_ai_cluster_query_bank_tu15 import validate as validate_cluster
        assert a.phase=='before' and not a.previous and not a.native_call and not a.lifetime_frame
        context=validate_cluster(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None
    elif actor_constructor:
        assert not a.native_call and not a.lifetime_frame
        if (a.entry==0x1d0670 or behavior_initializer) and metadata['entry']==0x1d0670:
            from validate_hunter_ai_frame_bank_tu15 import validate as validate_actor
            assert a.phase=='before' and not a.previous
        else:
            from validate_actor_constructor_bank_tu15 import validate as validate_actor
            if a.entry==0x1d0670:assert a.previous and a.phase=='after' and metadata['entry']==0x1e8410
            else:assert a.phase=='before' and not a.previous
        context=validate_actor(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None # This probe establishes the first original actor comparison.
    else:
        context=validate(case,metadata,list(regions(case/'before.bin')))
        hardware=json.loads((case/'hardware-verification/report.json').read_text())
        assert hardware['passed'] and not hardware['source_instruction_patches'] and not hardware['supplied_game_services']
        origin_hardware_sha256=sha(case/'hardware-verification/report.json')
    build=json.loads((case/'build.json').read_text());origin=case/(a.phase+'.bin');previous_evidence=None
    synchronization=None;synchronization_before=None;synchronization_after_expected=None
    if a.native_synchronization_library:
        from shadow_native_synchronization_adapter import NativeShadowSynchronization
        for name in ('native-synchronization-before.json','native-synchronization-after.json'):
            assert sha(case/name)==build['files_sha256'][name]
            captured=json.loads((case/name).read_text())
            assert captured['exported'] and captured['actual_native_owner'] and captured['source_memory_unchanged']
            assert captured['source_entry']==a.entry and captured['sequence_tick']==metadata['sequence_tick']
            shutil.copy2(case/name,out/name)
        synchronization_before=json.loads((case/'native-synchronization-before.json').read_text())
        captured=json.loads((case/'native-synchronization-after.json').read_text())
        synchronization_after_expected={key:captured[key] for key in ('version','first','next','limit','objects')}
        synchronization=NativeShadowSynchronization(case/'native-synchronization-before.json',a.native_synchronization_library)
    animation_resources=None;animation_after_expected=None
    resource_provider=a.native_weapon_resource_provider or a.native_animation_resource_provider
    if resource_provider:
        if a.native_weapon_resource_provider:
            from shadow_native_weapon_resources_tu15 import NativeShadowWeaponResources as ResourceOwner
        else:
            from shadow_native_animation_resource_tu15 import NativeShadowAnimationResources as ResourceOwner
        captured=json.loads((case/'native-synchronization-after.json').read_text())
        animation_after_expected=captured['native_resource_reads']
        flow=json.loads((case.parent/'run.json').read_text());command=flow['command']
        assert sha(command[1])==flow['map_sha256'] and sha(command[3])==NEW_PIN
        animation_resources=ResourceOwner(case/'native-synchronization-before.json',
            resource_provider,command[1],command[2],command[3])
    interface_language=None
    if a.native_interface_language:
        from shadow_native_interface_language_tu15 import NativeShadowInterfaceLanguage
        interface_language=NativeShadowInterfaceLanguage(case)
    api_frame=None;assert sum(bool(v) for v in (a.previous,a.lifetime_frame,a.native_call))<=1
    if a.native_call:
        assert a.entry in ({'visibility-consumer':(0x3f3e0,),'transparent-init':(0x1e0f0,),'view-reset':(0x1373f0,),'view-admission':(0x136ea0,),'world-draw':(0x1770c0,)}.get(a.native_call,(0x179df0,0x1770c0,0x1772c0)))
        native_root=Path(build['command'][3]);name=a.native_call+'/before.bin';origin=native_root/name
        assert sha(origin)==build['active_banks_sha256'][name]
        after_name=a.native_call+'/after.bin';assert sha(native_root/after_name)==build['active_banks_sha256'][after_name]
        api_frame=dict(native_root=str(native_root),native_call=a.native_call,native_after=str(native_root/after_name),native_after_sha256=build['active_banks_sha256'][after_name],native_build_sha256=sha(case/'build.json'))
    elif a.lifetime_frame:
        assert a.entry==0x179df0
        native_root=Path(build['command'][3]);name=f'particle-frame{a.lifetime_frame}/before.bin';origin=native_root/name
        assert sha(origin)==build['lifetime_banks_sha256'][name]
        after_name=f'particle-frame{a.lifetime_frame}/after.bin';assert sha(native_root/after_name)==build['lifetime_banks_sha256'][after_name]
        api_frame=dict(native_root=str(native_root),frame=a.lifetime_frame,native_after=str(native_root/after_name),native_after_sha256=build['lifetime_banks_sha256'][after_name],native_build_sha256=sha(case/'build.json'))
    elif a.previous:
        previous=a.previous.resolve();prior=json.loads((previous/'report.json').read_text());h=json.loads((previous/'hardware/report.json').read_text())
        assert prior['completed'] and prior['source_context']==context and prior['origin_case']==str(case)
        assert h['completed'] and h['emulator_retained_state_matches_hardware'] and not h['source_instruction_patches'] and not h['supplied_game_services']
        assert h['original_report_sha256']==sha(previous/'report.json') and h['original_xbe_sha256']==NEW_PIN
        origin=previous/'hardware/hardware-after.bin';assert sha(origin)==h['hardware_after_sha256']
        previous_evidence=dict(report=str(previous/'report.json'),report_sha256=sha(previous/'report.json'),hardware_report_sha256=sha(previous/'hardware/report.json'))
    else:assert sha(origin)==build['files_sha256'][origin.name]
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    # D3D is owned original executable code too. Admit its unchanged bytes;
    # genuine kernel/device imports still stop without supplied policies.
    owners=[code,_Code(image,'D3D')]
    data=list(regions(origin));g=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set();native_source_backing=[]
    def map_range(p,n):
        pages=[q for q in range(p&~4095,(p+n+4095)&~4095,4096) if q not in mapped]
        i=0
        while i<len(pages):
            j=i+1
            while j<len(pages) and pages[j]==pages[j-1]+4096:j+=1
            size=(j-i)*4096
            if a.host_game_start_memory:
                import ctypes,mmap
                assert mmap.PAGESIZE==4096
                backing=mmap.mmap(-1,size,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,
                                  prot=mmap.PROT_READ|mmap.PROT_WRITE)
                native=ctypes.addressof(ctypes.c_char.from_buffer(backing))
                g.mem_map_ptr(pages[i],size,7,native)
                native_source_backing.append((pages[i],size,backing,native))
            else:g.mem_map(pages[i],size)
            i=j
        mapped.update(pages)
    for p,b in data:map_range(p,len(b));g.mem_write(p,b)
    for owner in owners:assert bytes(g.mem_read(owner.lo,len(owner.bytes)))==owner.bytes
    for p in (0x70000000,0x60300000):map_range(p,4096)
    if a.host_pool_pages or a.host_game_start_memory or synchronization:
        # An emulator entry gate for the declared external ABI; no original
        # executable section occupies this page or is changed by this map.
        assert 0x80000000 not in mapped
        map_range(0x80000000,4096)
    g.mem_write(0x70000008,segment(0,0x9b)+segment(0,0x93)+segment(0x60200000,0x93))
    g.reg_write(R.UC_X86_REG_GDTR,(0,0x70000000,0xfff,0))
    for reg,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):g.reg_write(reg,value)
    def u(p,n=4):return int.from_bytes(g.mem_read(p,n),'little')
    clock=u(0x51df54);delta_bits=0 if a.captured_level_handles_startup or a.captured_shader_function_startup or a.captured_render_object_startup else u(clock+4)
    if not (a.captured_level_handles_startup or a.captured_shader_function_startup or a.captured_render_object_startup or (a.captured_recorded_animation and a.entry in (0x2035b0,0x2035f0))):
        assert struct.unpack('<f',struct.pack('<I',delta_bits))[0]>0
    gp=[0,0,0,0,0x600f0000,0,0,0]
    raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
    for reg,value in zip(REGISTERS,gp):g.reg_write(reg,value)
    for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),0)
    for reg,value in ((R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):g.reg_write(reg,value)
    captured_fp=None
    if a.captured_weapon_feedback or a.captured_weapon_tick or a.captured_projectile_effect or a.captured_unit_action or a.captured_hunter_particle_frame or a.captured_contact or a.captured_notification or a.captured_recorded_animation or a.captured_damage_notification or captured_public_frame_fp or a.captured_animation or a.captured_unit_notification or a.captured_script_builtin or a.declared_game_start or a.captured_level_handles_startup or a.captured_level_handles_new_map or a.captured_resource_cache_new_map or a.captured_registered_new_map or a.captured_shader_function_startup or a.captured_render_object_startup:
        raw_input=(case/'native-input.bin').read_bytes();gp=list(struct.unpack_from('<8I',raw_input))
        for reg,value in zip(REGISTERS,gp):g.reg_write(reg,value)
        for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),int.from_bytes(raw_input[32+i*16:48+i*16],'little'))
        depth,status,cw,mxcsr=struct.unpack('<IHHI',(case/'native-fp-input.bin').read_bytes());assert depth==0
        for reg,value in ((R.UC_X86_REG_EFLAGS,struct.unpack_from('<I',raw_input,160)[0]),(R.UC_X86_REG_FPCW,cw),(R.UC_X86_REG_FPSW,status),(R.UC_X86_REG_MXCSR,mxcsr)):g.reg_write(reg,value)
        captured_fp=dict(depth=depth,status=status,control=cw,mxcsr=mxcsr)
        shutil.copy2(case/'native-fp-input.bin',out/'native-fp-input.bin')
    arguments=[delta_bits] if a.entry in (0x179df0,0x13a9b0,0x17ead0,0xd72f0,0x18ee50,0x1378e0) else []
    if a.captured_projectile_effect or a.captured_unit_action:arguments=[metadata['request']]
    if a.captured_hunter_particle_frame:arguments=[metadata['delta_bits']]
    if a.captured_contact:arguments=context.get('callee_stack_arguments',context['actual_arguments'])
    if a.captured_weapon_feedback or a.captured_notification or a.captured_recorded_animation or a.captured_damage_notification or a.captured_animation or a.captured_unit_notification or a.captured_script_builtin or a.declared_game_start or a.captured_level_handles_startup or a.captured_level_handles_new_map or a.captured_resource_cache_new_map or a.captured_registered_new_map or a.captured_shader_function_startup or a.captured_render_object_startup:arguments=context['callee_stack_arguments']
    recorded_query_control=None
    if a.recorded_query_control:
        assert a.captured_recorded_animation and a.entry==0x2039b0
        pool=u(0x5021fc);assert pool and u(pool+0x20)==1 and u(pool+0x24)==0xa0
        record,bitmap=u(pool+0x44),u(pool+0x48)
        assert record and bitmap and u(pool+0x38)==0
        declared=[]
        def query_operand(address,size,value):
            before=bytes(g.mem_read(address,size));after=value.to_bytes(size,'little')
            g.mem_write(address,after)
            declared.append(dict(address=address,size=size,before_hex=before.hex(),after_hex=after.hex()))
        query_operand(pool+0x34,4,1)
        query_operand(pool+0x38,4,1)
        query_operand(bitmap,4,0 if a.recorded_query_control=='inactive-bitmap' else 1)
        query_operand(record,2,u(pool+0x40,2))
        query_operand(record+4,4,metadata['unit'] if a.recorded_query_control!='active-mismatch' else metadata['player_unit'])
        query_operand(record+0xa,1,1 if a.recorded_query_control=='flag-excluded' else 0)
        recorded_query_control=dict(case=a.recorded_query_control,explicit_memory_operands=declared,
            scope='Declared disposable query branch controls; not additional actual native gameplay histories')
    material_control=None
    if a.material_control:
        assert a.captured_contact and a.entry==0x18b4c0
        declared=[]
        def operand(address,size,value):
            before=bytes(g.mem_read(address,size));after=value.to_bytes(size,'little')
            g.mem_write(address,after)
            declared.append(dict(address=address,size=size,before_hex=before.hex(),after_hex=after.hex()))
        headers=u(u(0x4ecbd0)+0x44)
        target=u(headers+(gp[0]&65535)*12+8)
        tag=u(u(0x4f0414)+(u(target)&65535)*16+8)
        model=u(tag+0x38);assert model!=0xffffffff
        model_root=u(u(0x4f0414)+(model&65535)*16+8)
        if a.material_control=='dead':operand(target+0x10a,1,u(target+0x10a,1)|4)
        if a.material_control in ('model-fallback','empty-model-damage','no-model'):
            operand(tag+0x280,2,0xffff);operand(tag+0x282,2,0xffff)
        if a.material_control=='empty-model-damage':operand(model_root+0x60,4,0)
        if a.material_control=='no-model':operand(tag+0x38,4,0xffffffff)
        register_controls=[]
        if a.material_control=='non-unit-weapon':
            register_controls.append(dict(register='EAX',before=gp[0],after=metadata['weapon']))
            gp[0]=metadata['weapon'];g.reg_write(REGISTERS[0],gp[0])
            raw_input=struct.pack('<8I',*gp)+raw_input[32:]
        material_control=dict(case=a.material_control,explicit_memory_operands=declared,
            explicit_register_operands=register_controls,actual_native_caller_unchanged=a.material_control=='alive',
            scope='Declared disposable branch controls; not additional actual native gameplay histories')
    if a.entry==0x11e660:
        gp[0]=metadata['location'];g.reg_write(REGISTERS[0],gp[0])
        raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
    if a.entry in (0x1164d0,0x116440,0x116400):
        gp[0]=metadata.get('caller_unit',metadata['unit']);g.reg_write(REGISTERS[0],gp[0])
        if a.entry in (0x116440,0x116400):arguments=[metadata['duration_bits']]
        raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
    if behavior_initializer:
        slot=0x46dad8+(a.entry-0x381a60)//4
        assert u(slot)==a.entry and image.read(slot,4)==struct.pack('<I',a.entry)
    if a.entry==0x1373f0:gp[2]=0x556e18;g.reg_write(REGISTERS[2],gp[2]);raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
    if a.entry==0x108db0:
        assert a.phase=='before' and not a.previous and metadata['entry']==0x108db0
        gp[1]=metadata['weapon'];g.reg_write(REGISTERS[1],gp[1]);arguments=[0]
        raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
    if a.entry==0x1030f0:
        assert a.phase=='before' and not a.previous and metadata['entry']==0x1030f0
        arguments=[metadata['weapon']]
    if actor_constructor:
        if a.entry==0x1e7e40:arguments=[metadata['character']]
        elif a.entry==0x1e8410:
            gp[0]=metadata['declared_squad'];gp[2]=metadata['character'];arguments=[metadata['unit'],0]
            g.reg_write(REGISTERS[0],gp[0]);g.reg_write(REGISTERS[2],gp[2])
            raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
        elif a.entry==0x1e9000:
            gp[7]=metadata['actor'];g.reg_write(REGISTERS[7],gp[7]);arguments=[metadata['unit']]
            raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
        elif a.entry==0x1d03a0:
            gp[6]=metadata['player'];g.reg_write(REGISTERS[6],gp[6])
            raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
    if a.entry==0x136ea0:arguments=[0x556e18]
    portal_caller=None
    if a.entry==0x175f90:
        assert a.previous and u(0x505e7c)!=0
        visibility=json.loads((case/'visibility.json').read_text())
        assert sha(case/'visibility.json')==build['files_sha256']['visibility.json']
        collector=u(0x556e18);count=u(collector,2)
        assert 0<count<=6 and visibility['containing_cluster']>=0
        arguments=[count,collector+4,visibility['containing_cluster'],collector]
        portal_caller=dict(collector=collector,group_count=count,containing_cluster=visibility['containing_cluster'],visibility_sha256=sha(case/'visibility.json'),mask_pointer=u(0x505e7c))
    if a.entry==0x1770c0:arguments=[0] # Explicit ordinary world particle layer.
    if a.entry==0x1772c0:
        effect_pool=u(0x4f720c);instance=metadata['effect_instance'];effect_record=u(effect_pool+0x44)+(instance&65535)*400
        assert u(effect_record,2)==instance>>16
        particle=u(effect_record+0xfc);particle_pool=u(0x51df74);particle_record=u(particle_pool+0x44)+(particle&65535)*84
        assert particle!=0xffffffff and u(particle_record,2)==particle>>16
        arguments=[particle,0,0,0] # The actual graph's ordinary world draw input.
    queued_callback=None
    if a.queued_callback is not None:
        assert a.previous and a.entry==0x176b30
        assert 0<=a.queued_callback<u(0x4c2aa0)<=1024
        command=u(0x4c2aa8)+a.queued_callback*112
        assert u(command)==3 and u(command+32)==a.entry
        system,location=u(command+36),u(command+40)
        for slot,record,stride in ((0x51df74,system,84),(0x52d71c,location,52)):
            pool=u(slot);first=u(pool+0x44)
            assert first<=record<first+u(pool+0x38)*stride and (record-first)%stride==0
            assert u(record,2)!=0
        arguments=[command+36]
        queued_callback=dict(index=a.queued_callback,command_address=command,callback=a.entry,payload=command+36,system_record=system,location_record=location)
    else:assert a.entry!=0x176b30
    caller=struct.pack('<'+'I'*(1+len(arguments)),0x60300000,*arguments)
    old_caller=bytes(g.mem_read(gp[4],len(caller)));g.mem_write(gp[4],caller)
    (out/'native-input.bin').write_bytes(raw_input)
    def dump(name):
        with (out/name).open('wb') as f:
            for p,b in data:f.write(struct.pack('<II',p,len(b)));f.write(bytes(g.mem_read(p,len(b))))
    dump('before.bin')
    inventory=ROOT/'analysis/multiplayer/title-update-1.5/functions.json'
    if inventory.exists():
        starts={int(row['ea'],16) for row in json.loads(inventory.read_text())}
    else:
        inventory=ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv'
        with inventory.open() as stream:starts={int(row['address'],16) for row in csv.DictReader(stream,delimiter='\t')}
    visited=Counter();reached={};count=0;pc=0;writes=0;callers=[]
    native_platform_services=[];host_pool_backing=[];next_pool_token=0x10000000
    stores=(out/'original-stores.bin').open('wb');trace=(out/'original-trace.bin').open('wb')
    def hook(uc,p,n,user):
        nonlocal count,pc,next_pool_token
        if interface_language and p==0x33a4a6:
            start=len(interface_language.events);interface_language.apply(g,pc)
            native_platform_services.extend(interface_language.events[start:]);return
        if animation_resources and p==0x1a3b70 and pc in getattr(animation_resources,'caller_sites',(0x126139,)):
            start=len(animation_resources.events)
            def platform_store(address,payload):
                nonlocal writes
                old=bytes(g.mem_read(address,len(payload)))
                stores.write(struct.pack('<III',pc,address,len(payload)));stores.write(old);stores.write(payload);writes+=1
                g.mem_write(address,payload)
            if a.native_weapon_resource_provider:animation_resources.resource(g,pc,platform_store)
            else:animation_resources.animation(g,pc,platform_store)
            native_platform_services.extend(animation_resources.events[start:]);return
        if synchronization and p>=0x80000000:
            start=len(synchronization.events)
            try:synchronization.kernel(g,p)
            finally:
                for event in synchronization.events[start:]:
                    native_platform_services.append(dict(kind='actual native synchronization ownership',
                        caller_instruction=pc,**event,
                        ABI='EAX native status; consume original stdcall arguments; preserve other GP/FP/flags',
                        Xbox_kernel_instructions_or_clobbers_verified=False))
            return
        if a.host_game_start_memory and p==0x800000b6:
            import ctypes,mmap
            sp=g.reg_read(R.UC_X86_REG_ESP)
            returned,address,size,protection=struct.unpack('<4I',g.mem_read(sp,16))
            assert size and protection in (2,4,0x404)
            assert 0x80061000<=address and address+size<=0x83145000
            start=address&~4095;end=(address+size+4095)&~4095
            backing=next((row for row in native_source_backing
                          if row[0]<=start and end<=row[0]+row[1]),None)
            assert backing is not None,'Physical request escapes its retained actual host mapping'
            host=backing[3]+start-backing[0]
            permissions=mmap.PROT_READ if protection==2 else mmap.PROT_READ|mmap.PROT_WRITE
            libc=ctypes.CDLL(None,use_errno=True)
            libc.mprotect.argtypes=(ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int)
            libc.mprotect.restype=ctypes.c_int
            if libc.mprotect(host,end-start,permissions):
                error=ctypes.get_errno();raise OSError(error,'Actual source physical mprotect failed')
            g.mem_protect(start,end-start,1 if protection==2 else 3)
            native_platform_services.append(dict(kind='actual host physical page protection',
                import_target=p,caller_instruction=pc,returned=returned,address=address,bytes=size,
                protection=protection,rounded_source=start,rounded_bytes=end-start,
                actual_host_address=host,actual_mprotect_succeeded=True,
                ABI='preserve GP/FP/flags; consume the original stdcall stack',
                Xbox_kernel_address_or_clobber_behavior_verified=False,
                Xbox_GPU_cache_coherency_verified=False))
            g.reg_write(R.UC_X86_REG_ESP,sp+16);g.reg_write(R.UC_X86_REG_EIP,returned);return
        if a.host_pool_pages and p==0x800000b8:
            # This is the explicitly declared native raw-page boundary used
            # by SourcePhysicsPoolHostStorage, not an Xbox allocator golden.
            # No pool header, source return, or captured output is installed.
            import ctypes,mmap
            sp=g.reg_read(R.UC_X86_REG_ESP)
            returned,bp,zero,np,kind,protection=struct.unpack('<6I',g.mem_read(sp,24))
            requested=u(np)
            assert returned==0x33b3c5 and u(bp)==0 and zero==0
            assert kind==0x101000 and protection==4
            assert len(native_platform_services)<2 and requested==(0xf0,0x1e4)[len(native_platform_services)]
            assert mmap.PAGESIZE==4096
            rounded=(requested+4095)&~4095;base=next_pool_token
            assert not any(page in mapped for page in range(base,base+rounded,4096))
            raw=mmap.mmap(-1,rounded,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS,
                          prot=mmap.PROT_READ|mmap.PROT_WRITE)
            native=ctypes.addressof(ctypes.c_char.from_buffer(raw))
            assert raw[:]==bytes(rounded)
            g.mem_map_ptr(base,rounded,3,native);mapped.update(range(base,base+rounded,4096))
            host_pool_backing.append(raw);data.append((base,raw));data.sort(key=lambda row:row[0])
            g.mem_write(bp,struct.pack('<I',base));g.mem_write(np,struct.pack('<I',rounded))
            native_platform_services.append(dict(kind='actual anonymous host raw-pool allocation',
                import_target=p,caller_instruction=pc,requested=requested,rounded=rounded,
                source_token=base,actual_host_address=native,actual_host_initial_bytes_zero=True,
                token_policy='fresh native pool namespace starts10000000 and advances by host pages',
                ABI='preserve GP/FP/flags except EAX status and consumed stdcall stack',
                Xbox_kernel_address_or_clobber_behavior_verified=False))
            next_pool_token=base+rounded
            g.reg_write(R.UC_X86_REG_EAX,0);g.reg_write(R.UC_X86_REG_ESP,sp+24)
            g.reg_write(R.UC_X86_REG_EIP,returned);return
        if not any(owner.inside(p) for owner in owners):raise RuntimeError('Original reached an unsupplied external service at '+hex(p))
        raw=bytes(g.mem_read(p,n));assert raw==image.read(p,n),hex(p)
        pc=p;count+=1;trace.write(struct.pack('<I',p));reached.setdefault(p,raw.hex())
        if p in starts:
            visited[p]+=1
            if len(callers)<4096:
                sp=g.reg_read(R.UC_X86_REG_ESP)
                callers.append(dict(entry=p,order=count,gp=[g.reg_read(r) for r in REGISTERS],stack_words=[u(sp+i*4) for i in range(5)]))
    def write(uc,access,p,n,value,user):
        nonlocal writes
        if 0x60000000<=p<0x60200000:return
        old=bytes(g.mem_read(p,n));new=(value&((1<<(n*8))-1)).to_bytes(n,'little')
        stores.write(struct.pack('<III',pc,p,n));stores.write(old);stores.write(new);writes+=1
    source_faults=[]
    def invalid(uc,access,address,size,value,user):
        source_faults.append(dict(access=access,address=address,size=size,instruction=pc,gp=[g.reg_read(r) for r in REGISTERS]))
        return False
    g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
    error=None
    try:g.emu_start(a.entry,0x60300000,count=10000000)
    except (UcError,RuntimeError,AssertionError) as e:error=str(e)
    stores.close();trace.close();completed=g.reg_read(R.UC_X86_REG_EIP)==0x60300000
    if not completed and error is None:error='Instruction limit reached without returning'
    boundary=None
    if not completed and g.reg_read(R.UC_X86_REG_EIP)>=0x80000000:
        target=g.reg_read(R.UC_X86_REG_EIP);sp=g.reg_read(R.UC_X86_REG_ESP)
        boundary=dict(kind='unsupplied Xbox kernel import',target=target,ordinal=target&0x7fffffff,caller_instruction=pc,gp=[g.reg_read(r) for r in REGISTERS],stack_words=[u(sp+i*4) for i in range(8)])
        error=f'Original reached an unsupplied Xbox kernel import at {target:#x}'
    dump('original-after.bin')
    tags=g.reg_read(R.UC_X86_REG_FPTAG);depth=sum(((tags>>(2*i))&3)!=3 for i in range(8))
    exit_=struct.pack('<8I',*(g.reg_read(r) for r in REGISTERS))
    exit_+=b''.join(g.reg_read(getattr(R,f'UC_X86_REG_XMM{i}')).to_bytes(16,'little') for i in range(8))
    exit_+=struct.pack('<IIIHHI',g.reg_read(R.UC_X86_REG_EFLAGS),g.reg_read(R.UC_X86_REG_FS_BASE),depth,g.reg_read(R.UC_X86_REG_FPSW),g.reg_read(R.UC_X86_REG_FPCW),g.reg_read(R.UC_X86_REG_MXCSR))
    (out/'original-exit.bin').write_bytes(exit_)
    synchronization_after=None;synchronization_error=None;synchronization_exact=True
    if synchronization:
        try:
            synchronization_after=synchronization.export_checkpoint()
            synchronization_exact=synchronization_after==synchronization_after_expected
        except Exception as failure:
            synchronization_error=str(failure);synchronization_exact=False
        finally:synchronization.close()
    animation_after=None;animation_error=None;animation_exact=True
    if animation_resources:
        try:
            animation_after=animation_resources.checkpoint()
            animation_exact=animation_after==animation_after_expected
        except Exception as failure:
            animation_error=str(failure);animation_exact=False
        finally:animation_resources.close()
    report=dict(scope=__doc__,tool_sha256=sha(__file__),original_xbe_sha256=NEW_PIN,source_context=context,
        function_inventory=str(inventory),function_inventory_sha256=sha(inventory),
        material_control=material_control,recorded_query_control=recorded_query_control,
        origin_case=str(case),origin_phase=a.phase,origin_bank_sha256=sha(origin),origin_hardware_report_sha256=origin_hardware_sha256,previous_evidence=previous_evidence,native_api_frame=api_frame,
        entry=a.entry,captured_weapon_tick=a.captured_weapon_tick,captured_projectile_effect=a.captured_projectile_effect,captured_unit_action=a.captured_unit_action,captured_hunter_particle_frame=a.captured_hunter_particle_frame,captured_caller_fp=captured_fp,queued_callback=queued_callback,portal_caller=portal_caller,source_memory_faults=source_faults,frame_delta_bits=delta_bits,caller_argument_words=arguments,caller_frame_address=gp[4],caller_before_hex=old_caller.hex(),caller_after_hex=caller.hex(),
        supplied_game_services=[],instruction_patches=[],native_platform_services=native_platform_services,
        native_platform_contract_source_sha256={str(p.relative_to(ROOT)):sha(p) for p in
            ((ROOT/'src/physics/physics_pool_host_storage.cpp',ROOT/'src/runtime/source_runtime_host_storage.cpp') if a.host_pool_pages else
             (ROOT/'src/physics/physics_physical_storage_host.cpp',ROOT/'src/runtime/source_runtime_host_physical_memory.cpp'))}
            if a.host_pool_pages or a.host_game_start_memory else {},
        completed=completed,error=error,unowned_boundary=boundary,original_instructions=count,retained_stores=writes,exit_pc=g.reg_read(R.UC_X86_REG_EIP),
        original_functions={f'{p:08x}':n for p,n in sorted(visited.items())},original_function_entries=callers,
        executed_instruction_bytes={f'{p:08x}':v for p,v in sorted(reached.items())},
        files_sha256={p.name:sha(p) for p in out.iterdir() if p.is_file()},native_implementation_verified=False,physical_xbox_verified=False,full_weapon_parity=False)
    (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    if synchronization:
        report.update(native_synchronization_before_sha256=sha(case/'native-synchronization-before.json'),
            native_synchronization_expected_after_sha256=sha(case/'native-synchronization-after.json'),
            native_synchronization_after=synchronization_after,native_synchronization_export_error=synchronization_error,
            native_synchronization_after_matches_actual_capture=synchronization_exact,
            native_synchronization_library=str(a.native_synchronization_library.resolve()),
            native_synchronization_library_sha256=sha(a.native_synchronization_library),
            native_synchronization_adapter_sha256=sha(ROOT/'tools/shadow_native_synchronization_adapter.py'))
        report['native_platform_contract_source_sha256'].update({str(p.relative_to(ROOT)):sha(p) for p in
            (ROOT/'src/platform/native_synchronization.cpp',ROOT/'include/halo2/native_synchronization.hpp',
             ROOT/'tools/shadow_native_synchronization_adapter.cpp')})
        (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    if animation_resources:
        report.update(native_animation_resource_after=animation_after,
            native_animation_resource_after_matches_actual_capture=animation_exact,
            native_animation_resource_export_error=animation_error,
            native_animation_resource_provider=str(resource_provider.absolute()),
            native_animation_resource_provider_sha256=sha(resource_provider),
            native_animation_resource_manifest_sha256=sha(resource_provider.parent/'manifest.json'),
            native_animation_resource_adapter_sha256=sha(ROOT/'tools/shadow_native_animation_resource_tu15.py'),
            original_animation_async_submission_worker_verified=False)
        if a.native_weapon_resource_provider:
            report['native_weapon_resource_contracts']=['animation','geometry','texture']
            report['native_weapon_resource_adapter_sha256']=sha(ROOT/'tools/shadow_native_weapon_resources_tu15.py')
        (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    if interface_language:
        report.update(native_interface_language_input=interface_language.raw,
            native_interface_language_adapter_sha256=sha(ROOT/'tools/shadow_native_interface_language_tu15.py'),
            native_interface_language_configuration_source_sha256=interface_language.source_sha,
            Xbox_EEPROM_language_discovery_verified=False)
        (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    if context.get('validation_tool_sha256'):
        validators=['validate_captured_weapon_feedback_tu15.py','validate_captured_render_object_startup_tu15.py','validate_captured_shader_function_startup_tu15.py','validate_captured_registered_new_map_tu15.py','validate_captured_resource_cache_new_map_tu15.py','validate_captured_level_handles_new_map_tu15.py','validate_captured_level_handles_startup_tu15.py','validate_declared_game_start_tu15.py','validate_captured_hunter_script_builtin_tu15.py','validate_captured_hunter_unit_notification_tu15.py','validate_captured_hunter_animation_tu15.py','validate_captured_hunter_damage_notification_tu15.py','validate_captured_recorded_animation_tu15.py','validate_captured_hunter_notification_tu15.py','validate_captured_hunter_contact_tu15.py','validate_actor_constructor_bank_tu15.py','validate_hunter_ai_frame_bank_tu15.py',
            'validate_hunter_unit_query_bank_tu15.py','validate_ai_cluster_query_bank_tu15.py',
            'validate_captured_hunter_weapon_tick_tu15.py','validate_captured_hunter_particle_frame_tu15.py','validate_captured_hunter_unit_action_tu15.py','validate_captured_hunter_impact_tu15.py','validate_charge_effect_bank_tu15.py']
        matching=[ROOT/'tools'/p for p in validators if sha(ROOT/'tools'/p)==context['validation_tool_sha256']]
        assert len(matching)==1,'Retain the exact admitted bank validator'
        shutil.copy2(matching[0],out/'bank-validator-source.py')
    shutil.copyfile(__file__,out/'probe-source.py')
    print(json.dumps({k:report[k] for k in ('completed','error','original_instructions','retained_stores','exit_pc','original_functions')}));return 0 if completed and synchronization_exact and animation_exact else 1
if __name__=='__main__':raise SystemExit(main())
