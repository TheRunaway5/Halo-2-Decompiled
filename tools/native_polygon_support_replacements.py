"""Authenticate complete polygon-support bodies and selected observation owners."""
import hashlib
import re

BODY=re.compile(r'static void run_1f1d20\((State|WorldState|ObservedWorldState) &s\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_polygon_support_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_polygon_support_frame.hpp"\n'
BODY_PINS={
    'WorldState':'d39f7c9fc604b84b50d796d14395f16f398577ac82ab42f9f13aee50c6718d45',
    'State':'fc76df4d1afb2b53dddfa7896127febcd955b6f9f3f06570bb9f8977120f41e0',
    'ObservedWorldState':'6f18b3095a81713cf772bb35c8998180fe0ab700f7a8ebede710354de44fe164',
}
OWNER_PINS={
    'WorldState':('20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c70c675fd3a1b25f4ebc895e487cfb699548752eab54d0e25566e120f6ea848a'),
    'State':('d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e','915e87f07fb9cbb1d74ef1e80f5bbaf0b9cb4d8ea9582eeaf4e688df0ceb8fc9'),
    'ObservedWorldState':('20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c5ea0c5e4c241d9a0b448bea12cb9fc24cc8a208b0d861a57d22dd9dc0fc143c'),
}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
SHIFT='s.flags.shift(s.reg(7,4,0),0x4u,4)'
ORDERED_SHIFT='halo2::native_polygon_support_detail::ordered_shift(s,s.reg(7,4,0))'
ORDERED=((SHIFT,ORDERED_SHIFT),
    ('_mm_mul_ps(s.xmm[0],s.xmm[2])','halo2::native_polygon_support_detail::ordered_product(s,s.xmm[0],s.xmm[2])'),
    ('_mm_add_ss(s.xmm[3],s.xmm[0])','halo2::native_polygon_support_detail::ordered_sum(s,s.xmm[3],s.xmm[0])'),
    ('_mm_add_ss(s.xmm[4],s.xmm[3])','halo2::native_polygon_support_detail::ordered_sum(s,s.xmm[4],s.xmm[3])'),
    ('vw(s.memory,s.observe,','halo2::native_polygon_support_detail::ordered_store(s,'))
def ordered_body(body):
    for old,new in ORDERED:body=body.replace(old,new)
    return body
def canonical_body(body):
    opening=body.index('{')+1
    if body[opening:].startswith(PREFIX):body=body[:opening]+body[opening+len(PREFIX):]
    for old,new in ORDERED:body=body.replace(new,old)
    return body

def authenticate(source):
    found=list(BODY.finditer(source))
    if not found:return None
    if len(found)!=1:raise ValueError('Ambiguous complete polygon-support owner')
    match=found[0];state=match[1];body=canonical_body(match[0])
    if state not in BODY_PINS or digest(body)!=BODY_PINS[state]:raise ValueError('Complete polygon-support body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher or (digest(resolver[0]),digest(dispatcher[0]))!=OWNER_PINS[state]:
        raise ValueError('Complete polygon-support selected resolver or dispatcher drift')
    # The sole selected constant has a genuine remapping. Authenticate both
    # entire owners above before checking its exact DATA and instruction map.
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver[0])}
    if mapping.get(0x447020)!=0x44aac8:raise ValueError('Polygon-support initial bound remapping changed')
    if state=='ObservedWorldState':
        pairs=re.findall(r'world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)',body)
        if not pairs or any(int(actual,16)-int(canonical,16)!=0x48f0 for canonical,actual in pairs):
            raise ValueError('Polygon-support observation namespace changed')
    return match,body

def replace_polygon_support(source):
    found=authenticate(source)
    if not found:return source
    match,body=found
    opening=body.index('{')+1;bound=ordered_body(body[:opening]+PREFIX+body[opening:])
    if match[0]!=bound:
        source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate native polygon-support include')
    if INCLUDE not in source:source=INCLUDE+source
    return source

def remove_polygon_support(source):
    found=authenticate(source)
    if not found:return source
    match,body=found
    if match[0]!=body:source=source[:match.start()]+body+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate native polygon-support include')
    source=source.replace(INCLUDE,'',1)
    return source
