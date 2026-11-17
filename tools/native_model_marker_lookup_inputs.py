"""Freeze authenticated genuine marker owners into disposable source fixtures."""
import re
from pathlib import Path
import native_model_marker_lookup_replacements as binding

UNITS=('src/physics/physics_live_support_tu15_generated.cpp',
       'src/campaign/ai/source_ai_frame_tu15_generated.cpp',
       'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp',
       'src/gameplay/objects/source_sandbox_special_tu15_generated.cpp',
       'src/physics/vehicles/physics_vehicle_motion_generated.cpp',
       'src/gameplay/units/death_startup_runtime_generated.cpp')
EFFECT='src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp'
HELPER='src/gameplay/weapons/native_weapon_marker_lookup_tu15_internal.hpp'

def instrument(body):
    body,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::marker_retained_statement();',body)
    if count!=28:raise ValueError('Expected all28 retained marker statements')
    return body

def prepare(root,scratch,require_binding=False):
    from specialize_native_operands import specialize
    bodies=[];rows=[];controls=0
    for index,unit in enumerate(UNITS):
        source=(root/unit).read_text();match,original,mode,owner=binding.authenticate(source)
        bound=binding.replace_model_marker_lookup(source)
        if require_binding and source!=bound:raise ValueError('Installed marker prefix missing '+unit)
        if binding.replace_model_marker_lookup(bound)!=bound:raise ValueError('Marker prefix is not idempotent')
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
        variants=[source.replace(match[0],match[0].replace('0x58u','0x54u',1),1),
                  source.replace(match[0],match[0].replace('0xcu','0x10u',1),1),
                  bound.replace(binding.prefix(mode),binding.prefix('canonical'if mode=='actual'else'actual'),1)]
        if resolver:variants.append(source.replace(resolver[0],resolver[0].replace('return p;','return p+1u;',1),1))
        dispatch=re.search(r'static void dispatch\('+match[2]+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        variants.append(source.replace(dispatch,dispatch.replace('run_'+match[1]+'(s);','run_30bd0(s);',1),1))
        for changed in variants:
            if changed in(source,bound):raise ValueError('Marker source mutation witness missing '+owner)
            try:binding.replace_model_marker_lookup(changed)
            except ValueError:controls+=1
            else:raise ValueError('Changed complete marker source was admitted '+owner)
        if owner=='death-startup':
            state=re.search(r'struct State : physics_contact_runtime_detail::State \{.*?\n\};',source,re.S)[0]
            text='namespace death_actual {\nusing namespace halo2::physics_contact_runtime_detail;\n'+state+'\nnamespace bound {\n'+instrument(binding.BODY.search(bound)[0])+'\n}\nnamespace retained {\n'+instrument(original)+'\n}\n}\n'
            (scratch/'marker_death_actual.inc').write_text(text)
            prepared_body='fixed literal1.1'
        else:
            prepared,receipt=specialize(bound);prepared_body=binding.BODY.search(prepared)[0]
            if binding.prefix(mode)not in prepared_body:raise ValueError('Prepared marker prefix missing '+owner)
            text='namespace copy'+str(index)+' {\nusing namespace halo2::physics_world_runtime_detail;\nusing WorldState=InstrumentedState;using MotionState=InstrumentedState;\n'+resolver[0]+'\nnamespace bound {\n'+instrument(binding.BODY.search(bound)[0])+'\n}\nnamespace prepared {\n'+instrument(prepared_body)+'\n}\n}\n'
            bodies.append(text)
        rows.append(dict(unit=unit,owner=owner,mode=mode,original_body=binding.digest(original),prepared_body=binding.digest(prepared_body)))
    source=(root/HELPER).read_text();cpp=(root/EFFECT).read_text();bound=binding.replace_effect(source,cpp)
    if require_binding and bound!=source:raise ValueError('Installed real Effect marker prefix missing')
    if binding.replace_effect(bound,cpp)!=bound:raise ValueError('Real Effect marker prefix not idempotent')
    # This observer records entry/statement identity only. It supplies no CPU,
    # geometry, memory, or child outcome to the candidate or original body.
    anchor='const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};'
    if bound.count(anchor)!=1:raise ValueError('Exact Effect marker statement witness drift')
    bound=bound.replace(anchor,'const auto instruction=[&](auto pc){::marker_retained_statement();if(s.instruction)s.instruction(pc);};',1)
    opening=bound.index('{')+1;bound=bound[:opening]+'\n    ::marker_enter(a);'+bound[opening:]
    (scratch/Path(HELPER).name).write_text(bound)
    declarations='void marker_retained_statement();\nvoid marker_enter(halo2::SourcePhysicsWorldRuntimeAbi&);\n'
    cpp=binding.INCLUDE+declarations+cpp
    (scratch/'marker_effect.cpp').write_text(cpp)
    (scratch/'marker_sources.inc').write_text('\n'.join(bodies))
    return dict(owners=rows,source_auth_mutants_rejected=controls,effect_body=binding.EFFECT_BODY_PIN,effect_routes=binding.EFFECT_ROUTE_PINS,source_files=(EFFECT,HELPER,*UNITS),genuine_fixed1_1_state=binding.DEATH_STATE_PIN)
