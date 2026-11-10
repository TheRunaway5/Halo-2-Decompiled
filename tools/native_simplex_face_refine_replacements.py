"""Strict whole-body/context authentication for actual30D1A0's two owners."""
import hashlib
import re
BODY=re.compile(r'static void run_30d1a0\(WorldState &s\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_simplex_face_refine_owner_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_simplex_face_refine_source_owner.hpp"\n'
RAW_CONTEXTS={
    'primary':('2f64e35aeeb2232dfb02a2426a0bef6c5ac794e0c145a53e29bf2d107cb3ff06','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'),
    'death':('747e16bb89ef518bf8a9ea41b957b80be85b8c2e788071e581f79247ea8c91f7','cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61','2ddcb4ca932da7abcdbbbf8050bead73a3a18e4a4153021913f0555281b8aa20'),
    'primary-fresh':('75baea0c3838462f85031d77b2158c9c6f90cc48674885d31fb6925d0f280c34','1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4','aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'),
    'death-fresh':('75baea0c3838462f85031d77b2158c9c6f90cc48674885d31fb6925d0f280c34','1e70ace48573a64c67d51a9c7cde9f01c560e73795f8b089a918001d6bb1fbd6','1940da2e3890172390375a282ec9b1827d5a001fc02f5b85d0367efb9666ac45'),
}
# Exact prepared-output triples will be admitted only after independently
# deriving them from the strict raw installers; raw installers stay strict.
PREPARED_CONTEXTS={
    'primary':('28c6030081e58618fe70394e93d4619a259bcfb5c1a8f64db526ba28ce5e0ae9',RAW_CONTEXTS['primary'][1],'37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'),
    'death':('28c6030081e58618fe70394e93d4619a259bcfb5c1a8f64db526ba28ce5e0ae9',RAW_CONTEXTS['death'][1],'03799301083f53afa5eb43c5043509a12d3518626043f8cca11c6ce5e0e68ccc'),
    'primary-fresh':('fbdafe45671b0e758ec8545975647dff6ec79e7252b3790c8961e866edf0a694',RAW_CONTEXTS['primary-fresh'][1],'8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'),
    'death-fresh':('fbdafe45671b0e758ec8545975647dff6ec79e7252b3790c8961e866edf0a694',RAW_CONTEXTS['death-fresh'][1],RAW_CONTEXTS['death-fresh'][2]),
}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    at=body.index('{')+1
    return body[:at]+body[at+len(PREFIX):]if body[at:].startswith(PREFIX)else body
def context(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Missing complete simplex-face-refine owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete simplex-face-refine context')
    return found[0],resolver[0],dispatcher[0]
def authenticate(source):
    if not BODY.search(source):
        if PREFIX in source or INCLUDE in source:raise ValueError('Missing complete simplex-face-refine owner')
        return None
    match,resolver,dispatcher=context(source);body=canonical_body(match[0]);pins=tuple(digest(v)for v in(body,resolver,dispatcher))
    variant=next((name for name,value in RAW_CONTEXTS.items()if pins==value),None)
    if variant is None:raise ValueError('Complete simplex-face-refine body/resolver/dispatcher drift')
    if source.count(INCLUDE)>1 or(PREFIX in match[0])!=(INCLUDE in source):raise ValueError('Incomplete simplex-face-refine overlay')
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
    operands={int(a,16)for a in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',body)}
    if (variant!='death'and operands)or(variant=='death'and(len(operands)!=19 or body.count('selected(')!=54))or any(mapping.get(p)!=p for p in operands):raise ValueError('Simplex-face-refine identity mapping drift')
    return match,body,variant
def replace_simplex_face_refine(source):
    found=authenticate(source)
    if not found:return source
    match,body,_=found;at=body.index('{')+1
    source=source[:match.start()]+body[:at]+PREFIX+body[at:]+source[match.end():]
    return source if INCLUDE in source else INCLUDE+source
def remove_simplex_face_refine(source):
    found=authenticate(source)
    if not found:return source
    match,body,_=found
    return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    match,resolver,dispatcher=context(source)
    if source.count(INCLUDE)!=1 or match[0].count(PREFIX)!=1:raise ValueError('Missing complete simplex-face-refine overlay')
    if tuple(digest(v)for v in(match[0],resolver,dispatcher))not in PREPARED_CONTEXTS.values():authenticate(source)
    return['30d1a0']
