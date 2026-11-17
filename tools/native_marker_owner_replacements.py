"""Strict BACD0 source/Effect/public ownership and exact default installation."""
import importlib.util,re
from pathlib import Path
def sibling(name):
    spec=importlib.util.spec_from_file_location('_marker_owner_'+name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
shared=sibling('native_model_marker_lookup_replacements')
parent_mode=sibling('native_object_parent_mode_replacements')
function=shared.function
digest=shared.digest
BODY=re.compile(r'static void run_(bacd0|b9b90)\((WorldState|MotionState) &s\) \{.*?\n\}',re.S)
PINS={
    'primary':{'a327e414690738d61eabcbc7c95297ef5f752a7b7f7b80bb57d5821a16dc5222'},
    'ai':{'570240e23649e8df0ad11b613983da6b1cea1bf34968180444ff3c2309b5b084','20d72c3eb4a0b573c35eed0f6f6e4ad54afed36bd89923694227dd0fea03b0ac'},
    'scene':{'570240e23649e8df0ad11b613983da6b1cea1bf34968180444ff3c2309b5b084','20d72c3eb4a0b573c35eed0f6f6e4ad54afed36bd89923694227dd0fea03b0ac'},
    'motion':{'6ecd097ada4ddf3335b7a00b986686209c177bb65d79cecec8fcdd6d2bbac2d7'},
}
EFFECT_PIN='45017299ec0141aef9c752416f59969b435967a21c5d8274f0f477e6cb90fb1c'
CALL_PIN='bf8c5dac6aa38efc04ec5852ea318d273991bb87f6d3b41c35a541fac506ef70'
PUBLIC_PIN='aec6b1930a1cfa0964842ae9f9dc507fff9acf32583b9c32412ca5288147ddc0'
PUBLIC_ORIGINAL='''std::uint32_t native_campaign_object_marker_owner_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    while(true) {
        const auto record=object_record(m,object),parent=read(m,record+0x14u);
        if(parent!=none) {
            const auto tag=read(m,record),tags=read(m,0x4f0414u),definition=read(m,tags+(tag&0xffffu)*16u+8u);
            if(read(m,definition+2u,1)&0x10u){object=parent;continue;}
        }
        if(read(m,record+4u,1)&1u) {
            const auto kind=read(m,record+0xaau,1);
            if(((1u<<(kind&31u))&0x1cu) && native_campaign_object_pickup_parent_tu15(m,object))return read(m,record+0x154u);
        }
        return object;
    }
}'''
PREFIX='\n    if(halo2::native_marker_owner_detail::bridge(s))return;'
CANONICAL_PREFIX='\n    if(halo2::native_marker_owner_detail::bridge(s,halo2::native_marker_owner_detail::Observation::canonical))return;'
EFFECT_PREFIX='\n    if(halo2::native_marker_owner_detail::effect(m,a,s,o))return;'
INCLUDE='#include "gameplay/weapons/native_tu15_marker_owner_admission.hpp"\n'
SCENE_ANCHOR='#include "physics/runtime/native_tu15_simplex_boundary_source_owner.hpp"\n'
VALUE_INCLUDE='#include "halo2/native_tu15_marker_owner_value.hpp"\n'
VALUE_BODY='''std::uint32_t native_campaign_object_marker_owner_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    return native_marker_owner_detail::value([&](auto address,unsigned width){return read(m,address,width);},object);
}'''
def canonical_body(body):
    start=body.index('{')+1
    for current in(PREFIX,CANONICAL_PREFIX,EFFECT_PREFIX):
        if body[start:].startswith(current):
            expected=EFFECT_PREFIX if body.startswith('void marker_parent_unit(')else CANONICAL_PREFIX if body.startswith('static void run_b9b90(')else PREFIX
            if current!=expected:raise ValueError('Wrong inherited-marker owner prefix')
            return body[:start]+body[start+len(current):]
    return body
def prefix(owner):return CANONICAL_PREFIX if owner=='motion'else PREFIX
def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Expected one complete BACD0 owner')
    match=found[0];body=canonical_body(match[0])
    # The closed sibling authenticates complete unit identity, resolver,
    # dispatcher and the exact fresh-FA context. No partial dispatcher guess.
    _,_,observation,owner=shared.authenticate(source)
    if owner not in PINS or observation!=('canonical'if owner=='motion'else'actual') or digest(body)not in PINS[owner]:
        raise ValueError('Complete BACD0 body/context drift')
    return match,body,owner
def replace_marker_owner(source):
    if not BODY.search(source):
        if PREFIX in source or CANONICAL_PREFIX in source or INCLUDE in source:raise ValueError('BACD0 overlay has no owner')
        return source
    match,body,owner=authenticate(source)
    if match[0]==body:
        start=body.index('{')+1;source=source[:match.start()]+body[:start]+prefix(owner)+body[start:]+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate BACD0 include')
    if owner=='scene':
        if source.count(SCENE_ANCHOR)!=1:raise ValueError('Complete Scene boundary include ownership drift')
        source=source.replace(INCLUDE,'',1);at=source.index(SCENE_ANCHOR)+len(SCENE_ANCHOR)
        return source[:at]+INCLUDE+source[at:]
    return source if INCLUDE in source else INCLUDE+source
def effect_body(source):return function(source,'marker_parent_unit')
def replace_effect(source,dispatcher):
    body=effect_body(source);original=canonical_body(body)
    if digest(original)!=EFFECT_PIN:raise ValueError('Complete Effect BACD0 body drift')
    if digest(function(dispatcher,'call'))!=CALL_PIN:raise ValueError('Complete actual Effect required-owner transport drift')
    # Reuse the exact real aliases, owns/run routes and matrix-include proof;
    # FC510 must already be installed and reconstruct its unchanged body.
    if parent_mode.replace_effect(source,dispatcher)!=source:
        raise ValueError('Effect BACD0 requires its qualified FC510 child prefix')
    if body==original:
        start=body.index('{')+1;source=source.replace(body,body[:start]+EFFECT_PREFIX+body[start:],1)
    return source
def replace_value(source):
    body=function(source,'native_campaign_object_marker_owner_tu15')
    if body!=VALUE_BODY and digest(body)!=PUBLIC_PIN:raise ValueError('Complete public inherited-marker body drift')
    if parent_mode.replace_value(source)!=source:raise ValueError('Inherited-marker public DATA requires qualified parent-mode facade')
    source=source.replace(body,VALUE_BODY,1)
    if source.count(VALUE_INCLUDE)>1:raise ValueError('Duplicate inherited-marker DATA include')
    return source if VALUE_INCLUDE in source else VALUE_INCLUDE+source
