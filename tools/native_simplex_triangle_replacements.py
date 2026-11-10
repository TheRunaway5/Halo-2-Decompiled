"""Complete30D3F0 source overlay; no partial-body or dispatcher admission."""
import hashlib
import re

BODY=re.compile(r'static void run_309900\((State|WorldState|ObservedWorldState) &s\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_simplex_triangle_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_simplex_triangle_source_owner.hpp"\n'
BODY_PINS={
    'State':'bbc0adb751c6c4f44c2d3e19640c3ee7feca2e988c29d2bfe5e8103d15fc8141',
    'WorldState':'be0ba8dfd26a03f3f36f2ba0b9369a001d936cd0db6611054d5c891d92a400a7',
    'ObservedWorldState':'4e6518483279df6cc9969a6a52b0048995d21dcd3a1eb08e16e408ee1852db9b',
}
OWNER_PINS={
    'State':('d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e','915e87f07fb9cbb1d74ef1e80f5bbaf0b9cb4d8ea9582eeaf4e688df0ceb8fc9'),
    'WorldState':('20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c70c675fd3a1b25f4ebc895e487cfb699548752eab54d0e25566e120f6ea848a'),
    'ObservedWorldState':('20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c5ea0c5e4c241d9a0b448bea12cb9fc24cc8a208b0d861a57d22dd9dc0fc143c'),
}
PREPARED_CONTEXTS={
    'State':('9547a147ab03b9d9aca5e095342f740cdb224cedf84fe168ed018231bb45c5e7','d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e','232d5a082ea70455fc96d412dffb550cbb789b532b7e28b429fb4fedc8f63577'),
    'WorldState':('b53122b303aec18ce551baf7d6d757adfcab7cc46a0fee32cd0a2f07a35e5fec','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','65651152984258f6302a2c8d07a75d4f07d0ecd329a8a66347b82f502b3a6d12'),
    'ObservedWorldState':('b77223beefbb419a939bfc328d3af0efe839da2ad284f78d3b240defa8848447','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','9035cefe73aef945554d109aa7a2256ee0a3f52c1fc0499a416d8c4b9c52ab00'),
}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    if body[opening:].startswith(PREFIX):body=body[:opening]+body[opening+len(PREFIX):]
    return body
def authenticate(source):
    found=list(BODY.finditer(source))
    if not found:
        if INCLUDE in source or PREFIX in source:raise ValueError('Missing complete simplex-triangle owner')
        return None
    if len(found)!=1:raise ValueError('Ambiguous complete simplex-triangle owner')
    match=found[0];state=match[1];body=canonical_body(match[0])
    if digest(body)!=BODY_PINS[state]:raise ValueError('Complete simplex-triangle body drift')
    if source.count(INCLUDE)>1:raise ValueError('Duplicate simplex-triangle owner include')
    if (PREFIX in match[0])!=(INCLUDE in source):raise ValueError('Incomplete simplex-triangle source overlay')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher or (digest(resolver[0]),digest(dispatcher[0]))!=OWNER_PINS[state]:
        raise ValueError('Complete simplex-triangle selected resolver or dispatcher drift')
    if 'case 0x418ae0u:return 0x461960u;'not in resolver[0]:raise ValueError('Simplex-triangle sign mapping drift')
    if state=='ObservedWorldState':
        pairs=re.findall(r'world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)',body)
        if len(set(pairs))!=137 or any(int(b,16)-int(a,16)!=0x3af0 for a,b in pairs):raise ValueError('Simplex-triangle observation namespace drift')
    return match,body
def replace_simplex_triangle(source):
    found=authenticate(source)
    if not found:return source
    match,body=found;opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
    source=source[:match.start()]+bound+source[match.end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
def remove_simplex_triangle(source):
    found=authenticate(source)
    if not found:return source
    match,body=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    """Recognize only full pinned raw or full pinned prepared owner triples."""
    found=list(BODY.finditer(source))
    if len(found)!=1 or source.count(INCLUDE)!=1 or found[0][0].count(PREFIX)!=1:
        raise ValueError('Missing complete simplex-triangle source overlay')
    match=found[0];state=match[1]
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if resolver and dispatcher and tuple(digest(v)for v in(match[0],resolver[0],dispatcher[0]))==PREPARED_CONTEXTS[state]:
        return ['30d3f0']
    authenticate(source)
    return ['30d3f0']
