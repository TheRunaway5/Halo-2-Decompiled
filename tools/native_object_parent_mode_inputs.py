"""Freeze genuine FC510 source owners and public predecessor into owned scratch."""
import re
import native_object_parent_mode_replacements as binding
UNIT='src/physics/physics_live_support_tu15_generated.cpp'
EFFECT='src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp'
HELPER='src/gameplay/weapons/native_weapon_marker_matrix_tu15_internal.hpp'
PUBLIC='src/campaign/ai/native_campaign_object_motion_refs_tu15.cpp'
def instrument(body):
    body,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::parent_mode_statement();',body)
    if count!=15:raise ValueError('Expected all15 FC510 retained statements')
    return body
def prepare(root,scratch,require_binding=False):
    from specialize_native_operands import specialize
    source=(root/UNIT).read_text();match,body=binding.authenticate(source);bound=binding.replace_object_parent_mode(source)
    if require_binding and source!=bound:raise ValueError('Installed Primary FC510 default missing')
    if binding.replace_object_parent_mode(bound)!=bound:raise ValueError('Primary FC510 installer is not idempotent')
    prepared,_=specialize(bound);prepared_body=binding.BODY.search(prepared)[0]
    if binding.PREFIX not in prepared_body:raise ValueError('Prepared FC510 prefix lost')
    text='namespace parent_source {\nusing namespace halo2::physics_world_runtime_detail;\nnamespace raw {\n'+instrument(binding.BODY.search(bound)[0])+'\n}\nnamespace prepared {\n'+instrument(prepared_body)+'\n}\n}\n'
    (scratch/'parent_mode_sources.inc').write_text(text)
    cpp=(root/EFFECT).read_text();helper=(root/HELPER).read_text();effect=binding.replace_effect(helper,cpp)
    if require_binding and effect!=helper:raise ValueError('Installed real Effect FC510 default missing')
    if binding.replace_effect(effect,cpp)!=effect:raise ValueError('Effect FC510 installer not idempotent')
    function=binding.effect_body(effect);opening=function.index('{')+1
    function=function[:opening]+'\n    ::parent_mode_enter(a);'+function[opening:]
    anchor='const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};'
    if function.count(anchor)!=1:raise ValueError('Exact Effect FC510 statement observer drift')
    function=function.replace(anchor,'const auto instruction=[&](auto pc){::parent_mode_statement();if(s.instruction)s.instruction(pc);};',1)
    effect=effect.replace(binding.effect_body(effect),function,1)
    (scratch/'native_weapon_marker_matrix_tu15_internal.hpp').write_text(effect)
    (scratch/'parent_mode_effect.cpp').write_text(binding.INCLUDE+'void parent_mode_statement();\nvoid parent_mode_enter(halo2::SourcePhysicsWorldRuntimeAbi&);\n'+cpp.replace(binding.INCLUDE,'',1))
    public=(root/PUBLIC).read_text();candidate=binding.replace_value(public)
    if require_binding and candidate!=public:raise ValueError('Installed public parent-mode shared DATA missing')
    if binding.replace_value(candidate)!=candidate:raise ValueError('Public parent-mode value installer is not idempotent')
    old=binding.function(public,'native_campaign_object_pickup_parent_tu15')
    if old==binding.VALUE_BODY:
        # Preserve only the exact exported predecessor for the independent
        # public-policy comparison; every other old CPP byte is still current.
        original='''bool native_campaign_object_pickup_parent_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    const auto record=object_record(m,object),flags=read(m,record+0x12cu,1);
    return (flags&1u) && !(flags&2u);
}'''
        if binding.digest(original)!=binding.PUBLIC_PIN:raise ValueError('Pinned public predecessor drift')
        public=public.replace(old,original,1).replace(binding.VALUE_INCLUDE,'',1)
    (scratch/'parent_mode_public.cpp').write_text(candidate)
    if public.count('namespace halo2 {')!=1:raise ValueError('Public parent-mode CPP namespace ownership drift')
    frozen=public.replace('namespace halo2 {','namespace frozen_parent_mode {using namespace halo2;',1)
    # Only unused sibling calls need qualification after the namespace move:
    # the Memory argument's halo2 ADL would otherwise add the current facade.
    for old,new,count in(
        ('&& native_campaign_object_pickup_parent_tu15(','&& ::frozen_parent_mode::native_campaign_object_pickup_parent_tu15(',1),
        ('else native_campaign_object_motion_component_tu15(','else ::frozen_parent_mode::native_campaign_object_motion_component_tu15(',2)):
        if count==1 and frozen.count(old)==0:
            # The independently authenticated inherited-marker facade no
            # longer calls this sibling. Accept only its exact whole body
            # and its strict installer context, never an arbitrary omission.
            import native_marker_owner_replacements as marker
            if marker.function(candidate,'native_campaign_object_marker_owner_tu15')!=marker.VALUE_BODY or marker.replace_value(candidate)!=candidate:
                raise ValueError('Public inherited-marker sibling ownership drift')
            continue
        if frozen.count(old)!=count:raise ValueError('Public predecessor sibling lookup drift')
        frozen=frozen.replace(old,new)
    (scratch/'parent_mode_public_reference.cpp').write_text(frozen)
    rejected=0
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
    dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
    for changed in(source.replace(match[0],match[0].replace('0x12cu','0x128u',1),1),
                   source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),
                   source.replace(dispatch,dispatch.replace('run_fc510(s);','run_fc280(s);',1),1),
                   bound.replace(binding.PREFIX,binding.EFFECT_PREFIX,1)):
        if changed==source:raise ValueError('Missing FC510 source mutation witness')
        try:binding.replace_object_parent_mode(changed)
        except ValueError:rejected+=1
        else:raise ValueError('Changed complete FC510 source accepted')
    original=binding.effect_body(helper)
    try:binding.replace_effect(helper.replace(original,original.replace('0x12c','0x128',1),1),cpp)
    except ValueError:rejected+=1
    else:raise ValueError('Changed complete Effect FC510 body accepted')
    try:binding.replace_effect(effect.replace(binding.EFFECT_PREFIX,binding.PREFIX,1),cpp)
    except ValueError:rejected+=1
    else:raise ValueError('Wrong Effect FC510 owner prefix accepted')
    return dict(primary_body=binding.BODY_PIN,effect_body=binding.EFFECT_PIN,public_predecessor=binding.PUBLIC_PIN,source_mutations_rejected=rejected,actual_effect_dispatcher=EFFECT,source_files=(UNIT,EFFECT,HELPER,PUBLIC))
