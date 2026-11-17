"""Strict complete30DC10 Primary/Scene overlays; no loose prefix matching."""
import hashlib
import re
BODY=re.compile(r'static void run_30dc10\(WorldState &s\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_simplex_boundary_owner_detail::bridge(s,[](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_simplex_boundary_source_owner.hpp"\n'
RAW_CONTEXTS={
    'primary':('2894b1698fbc50d86cfbb6d63f47f24aafd79b622bc2585ee2909b2596818229','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'),
    'scene':('ddb1c7e82c16409c6684cc8b8b99b22a90284d69f77e02d279d9bef621254208','618bc56800beb42be63e189783dd7cf9a98d7b904e56fa92de6523bf86490d7a','1a68d583722bac0a950594a436cbb9f1b28b09624517589c9ee8ad507712f605'),
    'primary-fresh':('b0697322aca7283978c80ec2a67b7f99e0cb2f2cfffb67d877b9ecf93f639ffa','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'),
}
PREPARED_CONTEXTS={
    'primary':('835348cf4e0a54b90277e060cd1431bfc1f8c335b34b1040f29b03c2a41d006a',RAW_CONTEXTS['primary'][1],'37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'),
    'scene':('b04f5fe1c1425a0c523c92a018181305d07d38bc3ea47d2f783a420c06062ceb',RAW_CONTEXTS['scene'][1],'30d34d2c58b7adae1e2a3132d26e1da47e7278086c1e5ad7904fdfb58626241f'),
    'primary-fresh':('c8c3937292e6fd55ab3662c8478fd9547a547e5250a115491a0c7d0ca177a219',RAW_CONTEXTS['primary-fresh'][1],'8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'),
}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def context(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Missing or ambiguous complete simplex-boundary owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete simplex-boundary context')
    return found[0],resolver[0],dispatcher[0]
def authenticate(source):
    if not BODY.search(source):
        if PREFIX in source or INCLUDE in source:raise ValueError('Missing complete simplex-boundary owner')
        return None
    match,resolver,dispatcher=context(source);body=canonical_body(match[0]);pins=tuple(digest(v)for v in(body,resolver,dispatcher));variant=next((n for n,v in RAW_CONTEXTS.items()if pins==v),None)
    if variant is None:raise ValueError('Complete simplex-boundary body/resolver/dispatcher drift')
    if source.count(INCLUDE)>1 or(PREFIX in match[0])!=(INCLUDE in source):raise ValueError('Incomplete simplex-boundary source overlay')
    operands={int(v,16)for v in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',body)};mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
    if (variant!='scene'and operands)or any(mapping.get(v)!=v for v in operands):raise ValueError('Simplex-boundary literal identity mapping drift')
    return match,body,variant
def replace_simplex_boundary(source):
    found=authenticate(source)
    if not found:return source
    match,body,variant=found;opening=body.index('{')+1;source=source[:match.start()]+body[:opening]+PREFIX+body[opening:]+source[match.end():]
    if INCLUDE in source:return source
    if variant=='scene':
        marker='#include "gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp"\n'
        closest='#include "physics/runtime/native_tu15_closest_segments_source_owner.hpp"\n'
        lookup='#include "gameplay/objects/native_tu15_object_lookup_copies_source_owner.hpp"\n'
        # The final Scene lookup hook prepends its own include after Boundary
        # generation. Rebinding that installed source must preserve the same
        # Boundary insertion point, with either exact known sibling sequence.
        start=(lookup if lookup in source else '')+closest+marker
        if source.count(marker)!=1 or source.count(closest)!=1 or source.count(lookup)>1 or not source.startswith(start):raise ValueError('Simplex-boundary Scene sibling include order drift')
        at=source.index(marker)+len(marker);return source[:at]+INCLUDE+source[at:]
    return INCLUDE+source
def remove_simplex_boundary(source):
    found=authenticate(source)
    if not found:return source
    match,body,_=found;return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    match,resolver,dispatcher=context(source)
    if source.count(INCLUDE)!=1 or match[0].count(PREFIX)!=1:raise ValueError('Missing complete simplex-boundary overlay')
    if tuple(digest(v)for v in(match[0],resolver,dispatcher))not in PREPARED_CONTEXTS.values():authenticate(source)
    return['30dc10']
