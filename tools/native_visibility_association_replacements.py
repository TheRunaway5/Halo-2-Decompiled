"""Authenticate complete136E10 association owners before any native overlay."""
import hashlib,re
BODY_PIN='dca96b0bb38177d553c8e2fa9e4810925db950704beed846367adc81207d623a'
LITERAL_PIN='5ea1d1ac4638934c2bbeaf0ac8c2f8953f961acbba19c80990894a760342f1bc'
CONTEXTS={BODY_PIN:{
 ('dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981','fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e'),
 ('6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9','f754c70cbc7f2e502ea1e757ef67688c89a38f38b665e7e2ba66f711571dc84c'),
},LITERAL_PIN:{('5dda1bc027ca22bb0dfb680fea8c6ac399b3de4002aea0733e29373ed7f7b08d','e32f2927198be567ac60d36c8f00767d8941832313a643fd964f3297e651f5f4')}}
PREFIX='\n    if(halo2::native_visibility_association_detail::bridge(s))return;'
INCLUDE='#include "rendering/native_tu15_visibility_association.hpp"\n'
BODY=re.compile(r'static void run_136e10\(WorldState &s\) \{.*?\n\}',re.S)
ZERO_OLD='s.set(1,s.flags.shift(s.reg(1,4,0),selected(0x4u,s.image),4),4,0);'
ZERO_LITERAL='s.set(1,s.flags.shift(s.reg(1,4,0),0x4u,4),4,0);'
BIT_OLD='s.set(6,s.flags.shift(s.reg(6,4,0),s.reg(1,1,0),4),4,0);'
ZERO_CALL='halo2::native_visibility_association_detail::retained_mask_zero_offset(s);'
BIT_CALL='halo2::native_visibility_association_detail::retained_membership_bit(s);'
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    if body[opening:].startswith(PREFIX):body=body[:opening]+body[opening+len(PREFIX):]
    return body.replace(ZERO_CALL,ZERO_OLD if 'selected('in body else ZERO_LITERAL).replace(BIT_CALL,BIT_OLD)
def ordered_reference(body):
    lines=body.splitlines(keepends=True);zero=bit=0
    for index,line in enumerate(lines):
        if 's.services.instruction(0x136e51u)'in line:
            if line.count(ZERO_OLD)+line.count(ZERO_LITERAL)!=1:raise ValueError('Exact136E51 shift drift')
            lines[index]=line.replace(ZERO_OLD,ZERO_CALL).replace(ZERO_LITERAL,ZERO_CALL);zero+=1
        if 's.services.instruction(0x136e77u)'in line:
            if line.count(BIT_OLD)!=1:raise ValueError('Exact136E77 shift drift')
            lines[index]=line.replace(BIT_OLD,BIT_CALL);bit+=1
    if(zero,bit)!=(1,1):raise ValueError('Complete association shift sites missing')
    return ''.join(lines)
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete136E10 association owner')
    match=matches[0];body=canonical_body(match[0])
    pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('Complete136E10 association body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatch or(digest(resolver[0]),digest(dispatch[0]))not in CONTEXTS[pin]:raise ValueError('Complete136E10 association context drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{pair[0]for pair in CONTEXTS[pin]});return match,body
def replace_visibility_association(source):
    if not BODY.search(source):return source
    match,body=authenticate(source)
    expected=ordered_reference(body);opening=expected.index('{')+1;expected=expected[:opening]+PREFIX+expected[opening:]
    if match[0]!=expected:source=source[:match.start()]+expected+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate association include')
    return source if INCLUDE in source else INCLUDE+source
