"""Exact private 293290 overlay for the eight measured TU1.5 source contexts."""
import hashlib
import re

BODY = re.compile(r'^static void run_293290\(WorldState &s\) \{.*?^\}', re.M|re.S)
CALLER = re.compile(r'^static void run_284290\(WorldState &s\) \{.*?^\}', re.M|re.S)
RESOLVER = re.compile(r'^std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}', re.M)
DISPATCH = re.compile(r'^static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}', re.M)
STATE = re.compile(r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;', re.M|re.S)
ANCHOR = 'namespace halo2 {\nnamespace {\nusing namespace physics_world_runtime_detail;\n'
INCLUDE_BASENAME = 'native_tu15_codec_format_four_source_owner.hpp'
INCLUDE = '#include "physics/runtime/'+INCLUDE_BASENAME+'"\n'
OWNER = 'native_codec_format_four_owner_detail'
FRAME = 'native_codec_format_four_source_detail'
MATH = 'native_codec_format_four_math_detail'
LABEL = 'native_codec_format_four_resume_'
ORDERED = {
    0x293391:('_mm_mul_ps','multiply_packed'),0x293394:('_mm_mul_ps','multiply_packed'),
    0x2933a7:('_mm_add_ps','add_packed'),0x2933aa:('_mm_mul_ps','multiply_packed'),
    0x2933ba:('_mm_add_ps','add_packed'),0x2933bd:('_mm_mul_ps','multiply_packed'),
    0x2933c4:('_mm_sub_ps','subtract_packed'),0x293491:('_mm_add_ss','add_scalar'),
    0x29349f:('_mm_add_ss','add_scalar'),0x2934ad:('_mm_add_ss','add_scalar'),
    0x293573:('_mm_mul_ss','multiply_scalar'),
}

CONTEXTS = {
    ('80f4f97d339c22c9469bae67a3b4638860be3fac5f2c9ba205981c20f51e43d2', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46', 'bb04dfa70038ede46852f3ff480638c0005b43399cb01ee28deeb2727f54b3ad'): ('primary', 'current', 'raw'),
    ('baa8772442ff6230c5631e0139f3e243c987191c54a0435a73b85ff915cfad2f', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46', '6d98aceca1554ec47aa62ee0d49549800f323dba3fbc5fb6f04b789a5db5749e'): ('primary', 'current', 'prepared'),
    ('80f4f97d339c22c9469bae67a3b4638860be3fac5f2c9ba205981c20f51e43d2', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575', 'bb04dfa70038ede46852f3ff480638c0005b43399cb01ee28deeb2727f54b3ad'): ('primary', 'fresh', 'raw'),
    ('baa8772442ff6230c5631e0139f3e243c987191c54a0435a73b85ff915cfad2f', '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4', '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575', '6d98aceca1554ec47aa62ee0d49549800f323dba3fbc5fb6f04b789a5db5749e'): ('primary', 'fresh', 'prepared'),
    ('02f6aa94c9fe69acfb537b49a294b38543e21931b2123a891e4618193c02bc6d', '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75', '2f716e064af3a72690bbe766be88580a1927daa39319590d18444e94e00114ef', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46', 'f3f3c2f8b8223a7cb5dd307181b6814c23c11744c7ee4b8c785f54f00ac0fafb'): ('combat', 'current', 'raw'),
    ('baa8772442ff6230c5631e0139f3e243c987191c54a0435a73b85ff915cfad2f', '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75', 'ba92d4566306f2d5010c8f6b8276d209a573f7dfa97835e511faf7bc77644f5d', '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46', '6d98aceca1554ec47aa62ee0d49549800f323dba3fbc5fb6f04b789a5db5749e'): ('combat', 'current', 'prepared'),
    ('80f4f97d339c22c9469bae67a3b4638860be3fac5f2c9ba205981c20f51e43d2', '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75', 'f434da6ab6b8f8239fe575d6e75efb8e00f4a67a5188ac30cfff70e1f8d3a0ef', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575', 'bb04dfa70038ede46852f3ff480638c0005b43399cb01ee28deeb2727f54b3ad'): ('combat', 'fresh', 'raw'),
    ('baa8772442ff6230c5631e0139f3e243c987191c54a0435a73b85ff915cfad2f', '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75', 'f434da6ab6b8f8239fe575d6e75efb8e00f4a67a5188ac30cfff70e1f8d3a0ef', '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575', '6d98aceca1554ec47aa62ee0d49549800f323dba3fbc5fb6f04b789a5db5749e'): ('combat', 'fresh', 'prepared'),
}
SITES = (
    0x293291,0x29329f,0x2932a5,0x2932a6,0x2932a7,0x2932ac,0x2932b2,0x2932b6,
    0x2932bc,0x2932c2,0x2932c8,0x2932ce,0x2932d7,0x2932de,0x2932ef,0x2932fd,
    0x293303,0x293307,0x29332f,0x293347,0x29334d,0x293355,0x293359,0x29335d,
    0x293361,0x293365,0x293369,0x293371,0x293375,0x293378,0x29337c,0x29337f,
    0x2933a3,0x2933b6,0x2933c4,0x2933ca,0x2933cf,0x2933d3,0x2933d9,0x2933dd,
    0x2933ee,0x2933f8,0x293409,0x29340f,0x293414,0x293423,0x29342a,0x293439,
    0x293447,0x29344d,0x293451,0x29346d,0x293485,0x29348a,0x293491,0x293496,
    0x29349b,0x29349f,0x2934a8,0x2934ad,0x2934b6,0x2934bc,0x2934c0,0x2934c7,
    0x2934cb,0x2934db,0x2934e1,0x2934ee,0x2934f4,0x2934fd,0x293506,0x29350d,
    0x29351c,0x29352a,0x293530,0x293534,0x293550,0x293568,0x29356d,0x293573,
    0x293578,0x29357d,0x293583,0x293587,0x29358e,0x293592,0x2935a2,0x2935a8,
    0x2935b1,0x2935b2,0x2935b3,0x2935b6,
)


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def exact(pattern,source):
    matches=list(pattern.finditer(source))
    if len(matches)!=1:
        raise ValueError('Missing or ambiguous 293290 source context: '+pattern.pattern)
    return matches[0]


def name(pc):
    return LABEL+format(pc,'x')


def prefix(literal):
    tag=str(literal).lower()
    return ('\n    switch(halo2::'+OWNER+'::bridge<'+tag+'>'
            '(s,[](auto&current,auto target,auto site){dispatch(current,target,site,false);})){'
            'case 0u:break;case 1u:return;'+
            ''.join(f'case {pc:#x}u:goto {name(pc)};'for pc in SITES)+
            'default:throw std::logic_error("Unsupported native format-four continuation");}')


PREFIXES=(prefix(False),prefix(True))


def site_callback(pc,literal):
    if literal:return f's.instruction_literal({pc:#x}u);'
    return f'if(s.services.instruction){{s.services.instruction({pc:#x}u);}}'


def ordered_body(body,literal,*,reverse=False):
    for pc,(legacy,name) in ORDERED.items():
        ordered='halo2::'+MATH+'::'+name
        old,new=(ordered,legacy) if reverse else (legacy,ordered)
        callback=site_callback(pc,literal)
        if body.count(callback)!=1:raise ValueError('Exact 293290 arithmetic callback changed')
        start=body.index(callback)+len(callback);end=body.index('\n',start)
        expression=body[start:end]
        if expression.count(old+'(')!=1 or new+'('in expression:
            raise ValueError('Missing exact ordered 293290 expression '+hex(pc))
        body=body[:start]+expression.replace(old+'(',new+'(',1)+body[end:]
    return body


def canonical_body(body,*,allow_retag=False):
    at=body.index('{')+1
    matches=[(literal,value) for literal,value in enumerate(PREFIXES) if body.startswith(value,at)]
    if not matches:
        if OWNER in body or LABEL in body:raise ValueError('Incomplete or relocated 293290 entry')
        return body
    if len(matches)!=1:raise ValueError('Ambiguous 293290 source-format tag')
    literal,entry=matches[0]
    result=body[:at]+body[at+len(entry):]
    if allow_retag:literal='s.instruction_literal('in result
    for pc in SITES:
        label=name(pc)+': '
        if result.count(label)!=1 or result.count(label+site_callback(pc,literal))!=1:
            raise ValueError('Missing or relocated 293290 continuation '+hex(pc))
        result=result.replace(label,'',1)
    return ordered_body(result,literal,reverse=True)


def authenticate(source):
    match=exact(BODY,source);body=canonical_body(match[0]);installed=body!=match[0]
    values=(body,*(exact(pattern,source)[0]for pattern in (RESOLVER,DISPATCH,STATE,CALLER)))
    key=tuple(digest(value)for value in values)
    if key not in CONTEXTS:raise ValueError('Complete 293290 body/resolver/dispatcher/state/caller context changed')
    if (source.count(OWNER)!=int(installed) or FRAME in source or source.count(MATH)!=len(ORDERED)*int(installed) or
        source.count(LABEL)!=2*len(SITES)*int(installed) or
        source.count(INCLUDE_BASENAME)!=int(installed) or source.count(ANCHOR)!=1 or
        (installed and not source.startswith(INCLUDE,source.index(ANCHOR)-len(INCLUDE)))):
        raise ValueError('Foreign or misplaced 293290 source overlay')
    return CONTEXTS[key],match,body


def replace_format_four_decode(source):
    if not BODY.search(source):
        if any(marker in source for marker in (OWNER,FRAME,MATH,LABEL,INCLUDE_BASENAME)):
            raise ValueError('Format-four overlay lacks its real decoder')
        return source
    context,match,body=authenticate(source)
    if body!=match[0]:return source
    literal=context[-1]=='prepared'
    body=ordered_body(body,literal)
    for pc in SITES:
        site=site_callback(pc,literal)
        if body.count(site)!=1:raise ValueError('Exact 293290 source statement changed '+hex(pc))
        body=body.replace(site,name(pc)+': '+site,1)
    at=body.index('{')+1;body=body[:at]+prefix(literal)+body[at:]
    result=source[:match.start()]+body+source[match.end():]
    at=result.index(ANCHOR)
    return result[:at]+INCLUDE+result[at:]


def remove_format_four_decode(source):
    _,match,body=authenticate(source)
    if body==match[0]:return source
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    _,match,body=authenticate(source)
    if body==match[0]:raise ValueError('Missing 293290 source overlay')
    return ['293290']


def retag_specialized(original,specialized):
    context,match,body=authenticate(original)
    if context[-1]!='raw' or body==match[0]:raise ValueError('Expected authenticated raw 293290 overlay')
    prepared=exact(BODY,specialized)
    at=prepared[0].index('{')+1
    if not prepared[0].startswith(PREFIXES[0],at):
        raise ValueError('Missing raw 293290 prefix after preparation')
    canonical_body(prepared[0],allow_retag=True)
    start=prepared.start()+at
    result=specialized[:start]+PREFIXES[1]+specialized[start+len(PREFIXES[0]):]
    new_context,_,_=authenticate(result)
    if new_context!=(*context[:2],'prepared'):
        raise ValueError('Prepared 293290 owner changed')
    return result
