"""Authenticate the complete FC510 owner and its actual Effect/public facades."""
import importlib.util,re
from pathlib import Path
def sibling(name):
    spec=importlib.util.spec_from_file_location('_parent_mode_'+name,Path(__file__).with_name(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
shared=sibling('native_model_marker_lookup_replacements')
function=shared.function
digest=shared.digest
BODY=re.compile(r'static void run_fc510\(WorldState &s\) \{.*?\n\}',re.S)
BODY_PIN='bacc19016419b095481b799760732fac22e6adac07ee6f06c8ba83d87e6b7f18'
RESOLVER_PIN='1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4'
DISPATCH_PIN='5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'
FRESH_PIN='aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'
EFFECT_PIN='e9ca702eba386bd27186d9925a1b01781203b2cc5e2b0ad7a6757a5c22f5a9f4'
PUBLIC_PIN='8fe8369075cb041744940ce13609aa53dfccf5a245311e8fd2710e071604c623'
PUBLIC_HELPERS={'read':'60ed535d13b20c09c6f29c10ff3d121f4f793b0f1e69e7fec6ed353bbf8704e6','object_record':'a2431fe22c5a598bbd2ad7e286255ed687779d071ccfd8b0ae4218d3282fd57f'}
PREFIX='\n    if(halo2::native_object_parent_mode_detail::bridge(s))return;'
EFFECT_PREFIX='\n    if(halo2::native_object_parent_mode_detail::effect(m,a,s))return;'
INCLUDE='#include "gameplay/weapons/native_tu15_object_parent_mode_admission.hpp"\n'
VALUE_INCLUDE='#include "halo2/native_tu15_object_parent_mode_value.hpp"\n'
VALUE_BODY='''bool native_campaign_object_pickup_parent_tu15(const SourcePhysicsContactMemory &m,std::uint32_t object) {
    return native_object_parent_mode_detail::value([&](auto address,unsigned width){return read(m,address,width);},object);
}'''
def canonical_body(body):
    start=body.index('{')+1
    for prefix in(PREFIX,EFFECT_PREFIX):
        if body[start:].startswith(prefix):
            expected=EFFECT_PREFIX if body.startswith('void marker_parent_mode(')else PREFIX
            if prefix!=expected:raise ValueError('Wrong FC510 owner prefix')
            return body[:start]+body[start+len(prefix):]
    return body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete FC510 owner')
    current=matches[0];body=canonical_body(current[0])
    if digest(body)!=BODY_PIN:raise ValueError('Complete FC510 body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or digest(resolver[0])!=RESOLVER_PIN or not dispatcher:
        raise ValueError('Complete FC510 resolver/dispatcher drift')
    pin=digest(dispatcher[0])
    if pin!=DISPATCH_PIN:
        case='case 0xfa5d0u:run_fa5d0(s);return;';halfword=sibling('native_halfword_bit_query_replacements')
        if pin!=FRESH_PIN or dispatcher[0].count(case)!=1 or digest(dispatcher[0].replace(case,'',1))!=DISPATCH_PIN or digest(halfword.canonical_body(function(source,'run_fa5d0')))!=halfword.PIN:
            raise ValueError('Complete fresh FC510 dispatcher/FA owner drift')
    return current,body
def replace_object_parent_mode(source):
    if not BODY.search(source):return source
    match,body=authenticate(source)
    if match[0]==body:
        start=body.index('{')+1;source=source[:match.start()]+body[:start]+PREFIX+body[start:]+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate FC510 include')
    return source if INCLUDE in source else INCLUDE+source
def effect_body(source):return function(source,'marker_parent_mode')
def replace_effect(source,dispatcher):
    body=effect_body(source);original=canonical_body(body)
    if digest(original)!=EFFECT_PIN:raise ValueError('Complete Effect FC510 body drift')
    aliases='using Memory=SourcePhysicsContactMemory;\nusing Abi=SourcePhysicsWorldRuntimeAbi;\nusing Observer=SourcePhysicsContactRecordObserver;\nusing Services=SourcePhysicsWorldRuntimeServices;'
    if dispatcher.count(aliases)!=1 or dispatcher.count('constexpr auto none=0xffffffffu;')!=1:
        raise ValueError('Actual Effect FC510 API aliases/sentinel drift')
    for name,pin in shared.EFFECT_ROUTE_PINS.items():
        if digest(function(dispatcher,name))!=pin:raise ValueError('Actual Effect FC510 route drift '+name)
    if dispatcher.count('#include "native_weapon_marker_matrix_tu15_internal.hpp"')!=1:
        raise ValueError('Actual Effect FC510 private include ownership drift')
    if body==original:
        start=body.index('{')+1;source=source.replace(body,body[:start]+EFFECT_PREFIX+body[start:],1)
    return source
def replace_value(source):
    body=function(source,'native_campaign_object_pickup_parent_tu15')
    if body!=VALUE_BODY and digest(body)!=PUBLIC_PIN:raise ValueError('Complete public pickup-parent value owner drift')
    for name,pin in PUBLIC_HELPERS.items():
        if digest(function(source,name))!=pin:raise ValueError('Complete public pickup-parent dependency drift '+name)
    source=source.replace(body,VALUE_BODY,1)
    if source.count(VALUE_INCLUDE)>1:raise ValueError('Duplicate parent-mode value include')
    return source if VALUE_INCLUDE in source else VALUE_INCLUDE+source
