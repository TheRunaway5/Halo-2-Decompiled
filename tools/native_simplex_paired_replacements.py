"""Complete30D830 source overlays with exact whole-owner context pins."""
import hashlib
import re
BODY=re.compile(r'static void run_309d40\((State|WorldState|ObservedWorldState) &s\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_simplex_paired_owner_detail::bridge(s,[](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_simplex_paired_source_owner.hpp"\n'
RAW_CONTEXTS={
    'world':('d18b28718a1fcfe8fd416bcb137c0bfe1470da3ed0bda40d7217224af2540551','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c5ea0c5e4c241d9a0b448bea12cb9fc24cc8a208b0d861a57d22dd9dc0fc143c'),
    'contact':('77409385cdbdf6207b45a3c343e1bf1bef548deabe305b9150bc3800da71971f','d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e','915e87f07fb9cbb1d74ef1e80f5bbaf0b9cb4d8ea9582eeaf4e688df0ceb8fc9'),
    'baseline':('e883718d5683a19011a39a503a33c34f461ff4d3e260fe9edc09f6a1978d50d4','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c70c675fd3a1b25f4ebc895e487cfb699548752eab54d0e25566e120f6ea848a'),
}
PREPARED_CONTEXTS={
    'world':('ed47032d3939587446773c3a582dab7cd1cbf84e0183ca633f7ee43ae1717e37',RAW_CONTEXTS['world'][1],'9035cefe73aef945554d109aa7a2256ee0a3f52c1fc0499a416d8c4b9c52ab00'),
    'contact':('c3ca7e63c1efca46590f31c57f728729848d7df3abc8bfa0ff7faf67fd75e521',RAW_CONTEXTS['contact'][1],'232d5a082ea70455fc96d412dffb550cbb789b532b7e28b429fb4fedc8f63577'),
    'baseline':('f7204f46fe05aaaf92c5afa1539786e075ba2bb4d0b272cae5086f304f94bdc0',RAW_CONTEXTS['baseline'][1],'65651152984258f6302a2c8d07a75d4f07d0ecd329a8a66347b82f502b3a6d12'),
}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def context(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Missing or ambiguous complete simplex-paired owner')
    match=found[0];state=match[1]
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete simplex-paired context')
    return match,resolver[0],dispatcher[0]
def authenticate(source):
    if not BODY.search(source):
        if INCLUDE in source or PREFIX in source:raise ValueError('Missing complete simplex-paired owner')
        return None
    match,resolver,dispatcher=context(source);body=canonical_body(match[0]);pins=tuple(digest(v)for v in(body,resolver,dispatcher));variant=next((n for n,v in RAW_CONTEXTS.items()if pins==v),None)
    if variant is None:raise ValueError('Complete simplex-paired body/resolver/dispatcher drift')
    if source.count(INCLUDE)>1 or(PREFIX in match[0])!=(INCLUDE in source):raise ValueError('Incomplete simplex-paired overlay')
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
    if any(mapping.get(a)!=b for a,b in((0x418a60,0x41c500),(0x309900,0x30d3f0),(0x30eb30,0x312620))):raise ValueError('Simplex-paired selected owner mapping drift')
    if variant=='world':
        pairs=re.findall(r'world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)',body)
        if len(set(pairs))!=322 or any(int(b,16)-int(a,16)!=0x3af0 for a,b in pairs):raise ValueError('Simplex-paired observation namespace drift')
    return match,body,variant
def replace_simplex_paired(source):
    found=authenticate(source)
    if not found:return source
    match,body,_=found;opening=body.index('{')+1;source=source[:match.start()]+body[:opening]+PREFIX+body[opening:]+source[match.end():]
    return source if INCLUDE in source else INCLUDE+source
def remove_simplex_paired(source):
    found=authenticate(source)
    if not found:return source
    match,body,_=found;return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    match,resolver,dispatcher=context(source)
    if source.count(INCLUDE)!=1 or match[0].count(PREFIX)!=1:raise ValueError('Missing complete simplex-paired source overlay')
    if tuple(digest(v)for v in(match[0],resolver,dispatcher))not in PREPARED_CONTEXTS.values():authenticate(source)
    return['30d830']
