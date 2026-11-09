"""Authenticate the complete82-owner canonical-PC TU1.5 MOPP context."""
import hashlib
import re
from native_mopp_aabb_replacements import SHIFT_PAIRS,ordered_body

PIN='ffd0d98959366dea0236a09a56c6c0573c338efbd942241d01756ddba0968da7'
SPECIALIZED_BODY_PIN='264e2623e2aa2bcf4ff3eb4aef9f6628bff68befb233d83a98fd2c7dfccfb240'
SPECIALIZED_DISPATCH_PIN='232d5a082ea70455fc96d412dffb550cbb789b532b7e28b429fb4fedc8f63577'
PREFIX='\n    if(halo2::native_mopp_aabb_abi_detail::bridge_canonical(s,[](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_mopp_aabb_traversal.hpp"\n'
BODY=re.compile(r'static void run_305e40\(State &s\) \{.*?\n\}',re.S)
OWNER_PINS={
    r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}':'d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e',
    r'std::uint32_t canonical_branch\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}':'4e328c43da59895ab6c3f14a24129e538924d7467cb173bd65cdc3373d2a703d',
    r'static void dispatch\(State &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}':'915e87f07fb9cbb1d74ef1e80f5bbaf0b9cb4d8ea9582eeaf4e688df0ceb8fc9',
}


def canonical_body(body):
    start=body.index('{')+1
    if body[start:].startswith(PREFIX):body=body[:start]+body[start+len(PREFIX):]
    for original,ordered in SHIFT_PAIRS:body=body.replace(ordered,original)
    return body


def replace_mopp_aabb_canonical(source):
    matches=list(BODY.finditer(source))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete canonical309940 owner')
    current=matches[0][0];original=canonical_body(current)
    body_pin=hashlib.sha256(original.encode()).hexdigest()
    if body_pin not in(PIN,SPECIALIZED_BODY_PIN):raise ValueError('Complete canonical309940 MOPP body drift')
    for pattern,pin in OWNER_PINS.items():
        if pattern.startswith('static void dispatch')and body_pin==SPECIALIZED_BODY_PIN:pin=SPECIALIZED_DISPATCH_PIN
        owner=re.search(pattern,source,re.S)
        if not owner or hashlib.sha256(owner[0].encode()).hexdigest()!=pin:raise ValueError('Complete canonical309940 MOPP supporting owner drift')
    start=original.index('{')+1;bound=ordered_body(original[:start]+PREFIX+original[start:])
    source=source[:matches[0].start()]+bound+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source


def installed_entries(source):
    if not BODY.search(source):return []
    return ['309940']if replace_mopp_aabb_canonical(source)==source else []
