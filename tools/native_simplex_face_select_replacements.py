"""Complete30D5D0 source overlay with full resolver/dispatcher pins."""
import hashlib
import re
BODY=re.compile(r'static void run_309ae0\((State|WorldState|ObservedWorldState) &s\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_simplex_face_select_owner_detail::bridge(s,[](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_simplex_face_select_source_owner.hpp"\n'
BODY_PINS={
    'State':'87cb283fd90aca633c7c28f738a0115f2dc9a57d5ce9e5b5fcce277a085f84c5',
    'WorldState':'32fbd9c78b36eaa188bb52e5934d24fbc07f0205dea27be273f12821bca5adcd',
    'ObservedWorldState':'14092a34987ac851b948c1a49773cf6e32e9a2b93b27d47e7be504f6d3944e12',
}
OWNER_PINS={
    'State':('d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e','915e87f07fb9cbb1d74ef1e80f5bbaf0b9cb4d8ea9582eeaf4e688df0ceb8fc9'),
    'WorldState':('20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c70c675fd3a1b25f4ebc895e487cfb699548752eab54d0e25566e120f6ea848a'),
    'ObservedWorldState':('20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c5ea0c5e4c241d9a0b448bea12cb9fc24cc8a208b0d861a57d22dd9dc0fc143c'),
}
PREPARED_CONTEXTS={
    'ObservedWorldState':('17d690a1dd8807f1517831dc5aa3e3141fdcac237a33dffd49adfa72461af9fa',OWNER_PINS['ObservedWorldState'][0],'9035cefe73aef945554d109aa7a2256ee0a3f52c1fc0499a416d8c4b9c52ab00'),
    'State':('b51128bb360415cd56754ea0db56a21c8399c70c618d4b933025ba60d1d92959',OWNER_PINS['State'][0],'232d5a082ea70455fc96d412dffb550cbb789b532b7e28b429fb4fedc8f63577'),
    'WorldState':('3107b0541e2992f8684c5d62f7c78b9712cf2f39fbb5fba4849f7a82ff92f7f1',OWNER_PINS['WorldState'][0],'65651152984258f6302a2c8d07a75d4f07d0ecd329a8a66347b82f502b3a6d12'),
}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    if body[opening:].startswith(PREFIX):body=body[:opening]+body[opening+len(PREFIX):]
    return body
def authenticate(source):
    found=list(BODY.finditer(source))
    if not found:
        if INCLUDE in source or PREFIX in source:raise ValueError('Missing complete simplex-face-select owner')
        return None
    if len(found)!=1:raise ValueError('Ambiguous complete simplex-face-select owner')
    match=found[0];state=match[1];body=canonical_body(match[0])
    if digest(body)!=BODY_PINS[state]:raise ValueError('Complete simplex-face-select body drift')
    if source.count(INCLUDE)>1:raise ValueError('Duplicate simplex-face-select owner include')
    if (PREFIX in match[0])!=(INCLUDE in source):raise ValueError('Incomplete simplex-face-select source overlay')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher or (digest(resolver[0]),digest(dispatcher[0]))!=OWNER_PINS[state]:
        raise ValueError('Complete simplex-face-select selected resolver or dispatcher drift')
    if any(needle not in resolver[0]for needle in('case 0x3096b0u:return 0x30d1a0u;','case 0x309d34u:return 0x30d824u;','case 0x45cac0u:return 0x461950u;')):raise ValueError('Simplex-face-select literal mapping drift')
    if state=='ObservedWorldState':
        pairs=re.findall(r'world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)',body)
        if len(set(pairs))!=189 or any(int(b,16)-int(a,16)!=0x3af0 for a,b in pairs):raise ValueError('Simplex-face-select observation namespace drift')
    return match,body
def replace_simplex_face_select(source):
    found=authenticate(source)
    if not found:return source
    match,body=found;opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
    source=source[:match.start()]+bound+source[match.end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
def remove_simplex_face_select(source):
    found=authenticate(source)
    if not found:return source
    match,body=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    """Recognize only full pinned raw or full pinned prepared owner triples."""
    found=list(BODY.finditer(source))
    if len(found)!=1 or source.count(INCLUDE)!=1 or found[0][0].count(PREFIX)!=1:
        raise ValueError('Missing complete simplex-face-select source overlay')
    match=found[0];state=match[1]
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if resolver and dispatcher and tuple(digest(v)for v in(match[0],resolver[0],dispatcher[0]))==PREPARED_CONTEXTS[state]:
        return ['30d5d0']
    authenticate(source)
    return ['30d5d0']
