"""Authenticate the paired-image309940 MOPP owner before native admission."""
import hashlib
import re

PIN='e305c6cd745e83016af45747938f2b2101cbcc768920a429fed51857ea2112cb'
SPECIALIZED_BODY_PIN='c1937c5b0ea7934242365a63ad4604f1550a9fe7b450f2d6c6c276872a032654'
SPECIALIZED_DISPATCH_PIN='9035cefe73aef945554d109aa7a2256ee0a3f52c1fc0499a416d8c4b9c52ab00'
PREFIX='\n    if(halo2::native_mopp_aabb_abi_detail::bridge(s,[](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);}))return;'
INCLUDE='#include "physics/runtime/native_tu15_mopp_aabb_traversal.hpp"\n'
BODY=re.compile(r'static void run_305e40\(ObservedWorldState &s\) \{.*?\n\}',re.S)
OWNER_PINS={
    r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}':'20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2',
    r'std::uint32_t canonical_branch\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}':'e5779c8ed411532bc15334539a7814df2a584d81946b32d413bb5346d166e5c6',
    r'static void dispatch\(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}':'c5ea0c5e4c241d9a0b448bea12cb9fc24cc8a208b0d861a57d22dd9dc0fc143c',
    r'// BEGIN authenticated physics world observation addresses.*?// END authenticated physics world observation addresses':'0958a95ded1d1b87c184bc6e047cf4a7536b7011cd0f2e8391512d78f8dabcee',
}
SHIFT_PAIRS=[]
for reg in range(8):
    value=f's.reg({reg},4,0)'
    for count in (1,8):
        SHIFT_PAIRS.append((f's.flags.shift({value},0x{count:x}u,4)',f'halo2::native_mopp_aabb_abi_detail::shifted_immediate<{count}>(s,{value})'))
    SHIFT_PAIRS.append((f's.flags.shift({value},s.reg(1,1,0),4)',f'halo2::native_mopp_aabb_abi_detail::shifted(s,{value},s.reg(1,1,0))'))
    SHIFT_PAIRS.append((f's.flags.right({value},s.reg(1,1,0),4,true)',f'halo2::native_mopp_aabb_abi_detail::shifted(s,{value},s.reg(1,1,0),true)'))


def canonical_body(body):
    start=body.index('{')+1
    if body[start:].startswith(PREFIX):body=body[:start]+body[start+len(PREFIX):]
    for original,ordered in SHIFT_PAIRS:body=body.replace(ordered,original)
    return body


def ordered_body(body):
    for original,ordered in SHIFT_PAIRS:body=body.replace(original,ordered)
    return body


def replace_mopp_aabb(source):
    matches=list(BODY.finditer(source))
    if not matches:return source
    if len(matches)!=1:raise ValueError('Expected one complete309940 MOPP owner')
    current=matches[0][0];original=canonical_body(current)
    body_pin=hashlib.sha256(original.encode()).hexdigest()
    if body_pin not in(PIN,SPECIALIZED_BODY_PIN):raise ValueError('Complete309940 MOPP body drift')
    for pattern,pin in OWNER_PINS.items():
        if pattern.startswith('static void dispatch')and body_pin==SPECIALIZED_BODY_PIN:pin=SPECIALIZED_DISPATCH_PIN
        owner=re.search(pattern,source,re.S)
        if not owner or hashlib.sha256(owner[0].encode()).hexdigest()!=pin:raise ValueError('Complete309940 MOPP resolver/dispatcher/observation owner drift')
    start=original.index('{')+1;bound=ordered_body(original[:start]+PREFIX+original[start:])
    source=source[:matches[0].start()]+bound+source[matches[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source


def remove_mopp_aabb(source):
    return BODY.sub(lambda match:canonical_body(match[0]),source).replace(INCLUDE,'')


def installed_entries(source):
    match=BODY.search(source)
    if not match:return []
    bound=replace_mopp_aabb(source)
    return ['309940']if bound==source else []
