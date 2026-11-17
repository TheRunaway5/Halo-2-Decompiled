"""Complete312620 private source overlays with exact whole-owner contexts."""
import hashlib
import re
BODY=re.compile(r'static void run_(30eb30|312620)\((State|WorldState|ObservedWorldState) &s\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_closest_segments_owner_detail::bridge(s))return;'
ACTUAL_PREFIX='\n    if(halo2::native_closest_segments_owner_detail::bridge_actual(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_closest_segments_source_owner.hpp"\n'
RAW_CONTEXTS={
    'world':('9490b95842d933fdfbcd672f0213f7ab559a34b06e6e859690ba8a35d235f8fb','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c5ea0c5e4c241d9a0b448bea12cb9fc24cc8a208b0d861a57d22dd9dc0fc143c'),
    'contact':('31497d7846de3376c18650a94a954597ec6752e28653351f874f97457a27f15e','d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e','915e87f07fb9cbb1d74ef1e80f5bbaf0b9cb4d8ea9582eeaf4e688df0ceb8fc9'),
    'baseline':('e5e5f4bf9e30bc5fcc895cb863873af58db4c60a32b8fdbfc75a5daa15e1f720','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','c70c675fd3a1b25f4ebc895e487cfb699548752eab54d0e25566e120f6ea848a'),
    'scene':('a7a05a52fffc9429cae4f7bd1591070621bd69a1aaf7edee8e3d47adae57bdbb','618bc56800beb42be63e189783dd7cf9a98d7b904e56fa92de6523bf86490d7a','1a68d583722bac0a950594a436cbb9f1b28b09624517589c9ee8ad507712f605'),
}
PREPARED_CONTEXTS={
    'world':('b35234461f871b397a3d1d2f4046f2f41b8593a97cc8fad31d73e25a2f66f8cb',RAW_CONTEXTS['world'][1],'9035cefe73aef945554d109aa7a2256ee0a3f52c1fc0499a416d8c4b9c52ab00'),
    'contact':('687554b5fe991d6b64ea07dd1f4b55050be7914a3eebf4a966d35b2b72634009',RAW_CONTEXTS['contact'][1],'232d5a082ea70455fc96d412dffb550cbb789b532b7e28b429fb4fedc8f63577'),
    'baseline':('4781a1ee5811db04d56e4ff89dca7734dedeee122cfd05167fd77e91012fdfbf',RAW_CONTEXTS['baseline'][1],'65651152984258f6302a2c8d07a75d4f07d0ecd329a8a66347b82f502b3a6d12'),
    'scene':('eeea6a836b7c739b38407ec0738621466bb042da95b59d6d73f26a83e47c08bc',RAW_CONTEXTS['scene'][1],'30d34d2c58b7adae1e2a3132d26e1da47e7278086c1e5ad7904fdfb58626241f'),
}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def prefix(body):return ACTUAL_PREFIX if body.startswith('static void run_312620(')else PREFIX
def canonical_body(body):
    opening=body.index('{')+1;p=prefix(body)
    return body[:opening]+body[opening+len(p):]if body[opening:].startswith(p)else body
def context(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Missing or ambiguous complete closest-segments owner')
    match=found[0];state=match[2]
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete closest-segments context')
    return match,resolver[0],dispatcher[0]
def authenticate(source):
    if not BODY.search(source):
        if INCLUDE in source or PREFIX in source or ACTUAL_PREFIX in source:raise ValueError('Missing complete closest-segments owner')
        return None
    match,resolver,dispatcher=context(source);body=canonical_body(match[0]);pins=tuple(digest(v)for v in(body,resolver,dispatcher));variant=next((n for n,v in RAW_CONTEXTS.items()if pins==v),None)
    if variant is None:raise ValueError('Complete closest-segments body/resolver/dispatcher drift')
    if source.count(INCLUDE)>1 or(prefix(body)in match[0])!=(INCLUDE in source):raise ValueError('Incomplete closest-segments overlay')
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
    if variant=='scene':
        operands={int(v,16)for v in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',body)}
        if len(operands)!=21 or any(mapping.get(v)!=v for v in operands):raise ValueError('Closest-segments Scene identity mapping drift')
    else:
        if any(mapping.get(a)!=b for a,b in((0x465030,0x46a0d0),(0x465048,0x46a0e8),(0x4651bc,0x46a25c))):raise ValueError('Closest-segments selected scalar mapping drift')
    if variant=='world':
        pairs=re.findall(r'world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)',body)
        if len(set(pairs))!=190 or any(int(b,16)-int(a,16)!=0x3af0 for a,b in pairs):raise ValueError('Closest-segments observation namespace drift')
    return match,body,variant
def replace_closest_segments(source):
    found=authenticate(source)
    if not found:return source
    match,body,_=found;opening=body.index('{')+1;source=source[:match.start()]+body[:opening]+prefix(body)+body[opening:]+source[match.end():]
    return source if INCLUDE in source else INCLUDE+source
def remove_closest_segments(source):
    found=authenticate(source)
    if not found:return source
    match,body,_=found;return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    match,resolver,dispatcher=context(source)
    if source.count(INCLUDE)!=1 or match[0].count(prefix(match[0]))!=1:raise ValueError('Missing complete closest-segments source overlay')
    if tuple(digest(v)for v in(match[0],resolver,dispatcher))not in PREPARED_CONTEXTS.values():authenticate(source)
    return['312620']
