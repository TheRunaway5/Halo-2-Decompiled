"""Authenticate complete1F6690 bodies, selected resolver and dispatch owners."""
import hashlib
import re

BODY=re.compile(r'static void run_1f1da0\((State|WorldState|ObservedWorldState) &s\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_polygon_features_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_polygon_features_abi.hpp"\n'
BODY_PINS={
    'ObservedWorldState':'3e8f283d4b28809a1080ad526e6a03b338c0740b9f3cedde58b221a1ecc41df5',
    'State':'f41c8083fbc9cb4afbe3b0b5035ad570131bece54da7a5cb632b19a2575c6554',
    'WorldState':'d640af48f3db6706973fdb5ff07c40cd20ae56a35309b88e2d0ddfb35423a33c',
}
OWNER_PINS={
    'WorldState':('20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c70c675fd3a1b25f4ebc895e487cfb699548752eab54d0e25566e120f6ea848a'),
    'State':('d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e','915e87f07fb9cbb1d74ef1e80f5bbaf0b9cb4d8ea9582eeaf4e688df0ceb8fc9'),
    'ObservedWorldState':('20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c5ea0c5e4c241d9a0b448bea12cb9fc24cc8a208b0d861a57d22dd9dc0fc143c'),
}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    found=list(BODY.finditer(source))
    if not found:return None
    if len(found)!=1:raise ValueError('Ambiguous complete polygon-feature owner')
    match=found[0];state=match[1];body=canonical_body(match[0])
    if digest(body)!=BODY_PINS[state]:raise ValueError('Complete polygon-feature body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher or(digest(resolver[0]),digest(dispatcher[0]))!=OWNER_PINS[state]:raise ValueError('Complete polygon-feature selected resolver or dispatcher drift')
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver[0])}
    if mapping.get(0x1f1da0)!=0x1f6690:raise ValueError('Polygon-feature entry remapping changed')
    if state=='ObservedWorldState':
        pairs=re.findall(r'world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)',body)
        if len(pairs)!=26 or any(int(b,16)-int(a,16)!=0x48f0 for a,b in pairs):raise ValueError('Polygon-feature observation namespace changed')
    return match,body
def replace_polygon_features(source):
    found=authenticate(source)
    if not found:return source
    match,body=found
    if match[0]==body:
        opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:];source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate polygon-feature include')
    if INCLUDE not in source:source=INCLUDE+source
    return source
def remove_polygon_features(source):
    found=authenticate(source)
    if not found:return source
    match,body=found
    if match[0]!=body:source=source[:match.start()]+body+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate polygon-feature include')
    return source.replace(INCLUDE,'',1)
