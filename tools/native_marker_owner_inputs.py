"""Freeze genuine inherited-marker sources and actual required-child owners."""
import re
import native_marker_owner_replacements as binding
UNITS=(
    'src/physics/physics_live_support_tu15_generated.cpp',
    'src/campaign/ai/source_ai_frame_tu15_generated.cpp',
    'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp',
    'src/physics/vehicles/physics_vehicle_motion_generated.cpp',
)
EFFECT='src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp'
HELPER='src/gameplay/weapons/native_weapon_marker_matrix_tu15_internal.hpp'
PUBLIC='src/campaign/ai/native_campaign_object_motion_refs_tu15.cpp'
def prepare_value(root,scratch,require_binding=False):
    source=(root/PUBLIC).read_text();candidate=binding.replace_value(source)
    if require_binding and source!=candidate:raise ValueError('Installed public inherited-marker DATA missing')
    if binding.digest(binding.PUBLIC_ORIGINAL)!=binding.PUBLIC_PIN:raise ValueError('Pinned public inherited-marker predecessor drift')
    old=binding.function(source,'native_campaign_object_marker_owner_tu15')
    if old==binding.VALUE_BODY:
        source=source.replace(old,binding.PUBLIC_ORIGINAL,1).replace(binding.VALUE_INCLUDE,'',1)
    if source.count('namespace halo2 {')!=1:raise ValueError('Public inherited-marker namespace ownership drift')
    frozen=source.replace('namespace halo2 {','namespace frozen_marker_owner {using namespace halo2;',1)
    for old,new,count in(
        ('&& native_campaign_object_pickup_parent_tu15(','&& ::frozen_marker_owner::native_campaign_object_pickup_parent_tu15(',1),
        ('else native_campaign_object_motion_component_tu15(','else ::frozen_marker_owner::native_campaign_object_motion_component_tu15(',2)):
        if frozen.count(old)!=count:raise ValueError('Public inherited-marker sibling lookup drift')
        frozen=frozen.replace(old,new)
    (scratch/'marker_owner_public.cpp').write_text(candidate)
    (scratch/'marker_owner_public_reference.cpp').write_text(frozen)
    return dict(predecessor_sha256=binding.PUBLIC_PIN,shared_value_facade=True,frozen_unused_sibling_qualifications_only=True)
def instrument(body):
    body,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::marker_owner_statement();',body)
    if count!=44:raise ValueError('Expected all44 inherited-marker statements')
    return body
def prepare(root,scratch,require_binding=False):
    from specialize_native_operands import specialize
    child_source=(root/UNITS[0]).read_text()
    child=binding.parent_mode.BODY.search(child_source)[0]
    binding.parent_mode.authenticate(child_source)
    if binding.parent_mode.PREFIX not in child:raise ValueError('Inherited-marker source requires its installed actual FC510 owner')
    sources=[];rejected=0
    for index,name in enumerate(UNITS):
        source=(root/name).read_text();match,body,owner=binding.authenticate(source);bound=binding.replace_marker_owner(source)
        if require_binding and source!=bound:raise ValueError('Installed inherited-marker source default missing '+owner)
        if binding.replace_marker_owner(bound)!=bound:raise ValueError('Inherited-marker installer not idempotent '+owner)
        prepared,_=specialize(bound);emitted=binding.BODY.search(prepared)[0]
        if binding.prefix(owner)not in emitted:raise ValueError('Prepared inherited-marker default missing')
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
        if resolver is None:raise ValueError('Missing authenticated inherited-marker resolver')
        # Keep the exact resolver rather than substituting an identity hook.
        # The canonical default fallback is the real State.required service.
        dispatch='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}\n'
        namespace='physics_vehicle_motion_detail'if owner=='motion'else'physics_world_runtime_detail'
        text='namespace marker_source'+str(index)+' {using namespace halo2::'+namespace+';\n'+resolver[0]+'\n'+dispatch+'namespace raw {\n'+child+'\n'+instrument(binding.BODY.search(bound)[0])+'\n}\nnamespace prepared {\n'+child+'\n'+instrument(emitted)+'\n}\n}\n'
        (scratch/('marker_owner_source'+str(index)+'.inc')).write_text(text)
        for changed in(
            source.replace(match[0],match[0].replace('0x14u','0x18u',1),1),
            bound.replace(binding.prefix(owner),binding.EFFECT_PREFIX,1)):
            if changed in(source,bound):raise ValueError('Missing inherited-marker source mutation')
            try:binding.replace_marker_owner(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Changed inherited-marker source accepted')
        sources.append(dict(source=name,owner=owner,body_sha256=binding.digest(body)))
    cpp=(root/EFFECT).read_text();helper=(root/HELPER).read_text();effect=binding.replace_effect(helper,cpp)
    if require_binding and(effect!=helper or cpp.count(binding.INCLUDE)!=1):raise ValueError('Installed real Effect inherited-marker default missing')
    if binding.replace_effect(effect,cpp)!=effect:raise ValueError('Effect inherited-marker installer not idempotent')
    body=binding.effect_body(effect);opening=body.index('{')+1
    body=body[:opening]+'\n    ::marker_owner_enter(a);'+body[opening:]
    anchor='const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};'
    if body.count(anchor)!=1:raise ValueError('Actual inherited-marker instruction observer drift')
    body=body.replace(anchor,'const auto instruction=[&](auto pc){::marker_owner_statement();if(s.instruction)s.instruction(pc);};',1)
    effect=effect.replace(binding.effect_body(effect),body,1)
    # The real already-qualified FC510 child remains in this complete owner.
    child=binding.parent_mode.effect_body(effect);opening=child.index('{')+1
    child=child[:opening]+'\n    ::marker_child_enter(a);'+child[opening:]
    effect=effect.replace(binding.parent_mode.effect_body(effect),child,1)
    (scratch/'native_weapon_marker_matrix_tu15_internal.hpp').write_text(effect)
    declarations='void marker_owner_statement();\nvoid marker_owner_enter(halo2::SourcePhysicsWorldRuntimeAbi&);\nvoid marker_child_enter(halo2::SourcePhysicsWorldRuntimeAbi&);\n'
    (scratch/'marker_owner_effect.cpp').write_text(binding.INCLUDE+declarations+cpp.replace(binding.INCLUDE,'',1))
    original=binding.effect_body(helper)
    for changed_helper,changed_cpp in(
        (helper.replace(original,original.replace('0x14','0x18',1),1),cpp),
        (helper,cpp.replace('entry==0xbacd0u','entry==0xbacd1u',1)),
        (helper,cpp.replace('s.required_owner(m,{pc,target,false},a,o);','s.required_owner(m,{pc+1u,target,false},a,o);',1))):
        if changed_helper==helper and changed_cpp==cpp:raise ValueError('Missing Effect inherited-marker mutation')
        try:binding.replace_effect(changed_helper,changed_cpp)
        except ValueError:rejected+=1
        else:raise ValueError('Changed Effect inherited-marker transport admitted')
    return dict(sources=sources,effect_body=binding.EFFECT_PIN,required_call_helper=binding.CALL_PIN,strict_source_mutations_rejected=rejected,source_files=(*UNITS,EFFECT,HELPER,PUBLIC))
