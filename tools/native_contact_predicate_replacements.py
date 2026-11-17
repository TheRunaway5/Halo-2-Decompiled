"""Private exact current LiveSupport 303800 prototype binding."""
import hashlib
import re

SIGNATURE = 'static void run_303800(WorldState &s) {'
BODY = re.compile(re.escape(SIGNATURE)+r'.*?\n\}',re.S)
RESOLVER = re.compile(r'^std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',re.M)
DISPATCHER = re.compile(r'^static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',re.M)
EXPORT = re.compile(r'^void source_run_physics_live_support_tu15\(M &m,std::uint32_t entry,\n.*?^\}',re.S|re.M)
OWNS = re.compile(r'^bool source_physics_live_support_tu15_owns_entry\(std::uint32_t p\) \{[^\n]*\}',re.M)
STATE = re.compile(r'^struct LiveState[^\n]*\{\n.*?^\};',re.S|re.M)
RESOLVER_PIN = '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4'
EXPORT_PIN = '912e109873b4775f13979a32e325106d74712a3e4a0bf43d5485d911cd01f35a'
OWNS_PIN = 'f53eb05fed15f9f03fb04925d9a466a46992c9c79539232e8c54f76a96b43fab'
STATE_PIN = '68accf0054ca64fec14dc244a1c1d00e6015e79ef2e0182717e8b2fe4cf99fb3'
FRESH_EXPORT_PIN = '7e227c0aa7f7b21a540fa749c52d95a75432e51a9513543be7bddc25fa58ca09'
FRESH_OWNS_PIN = '5a8d30ef7f8b2b214d9e32e5e4cdb91d43c2556853b5ad03692086703cec016c'
FRESH_STATE_PIN = 'c3e351629369a688a5ca40be8dfcaf725482d008a3050759060cacd2a2bed275'
CONTEXTS = {
    ('79b2196359a4ff74201eb00415220048669183c05087f6ecd9a42325f852bc87',
     RESOLVER_PIN,'5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0',
     EXPORT_PIN,OWNS_PIN,STATE_PIN): False,
    ('6856c5049553512dc212013bb7e791ed21948ea28e4143d9f255690a1b91b673',
     RESOLVER_PIN,'37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052',
     EXPORT_PIN,OWNS_PIN,STATE_PIN): True,
    ('89d7da38867c4aa88439f76786039e85d134714924cf0b30932fe432a41515c6',
     RESOLVER_PIN,'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849',
     FRESH_EXPORT_PIN,FRESH_OWNS_PIN,FRESH_STATE_PIN): False,
    ('999d5b9290123fe306c099de877c47a219ad6288b6e39040bcb71b884b0fd804',
     RESOLVER_PIN,'8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851',
     FRESH_EXPORT_PIN,FRESH_OWNS_PIN,FRESH_STATE_PIN): True,
}
HEADER = 'native_tu15_contact_predicate_source_owner.hpp'
INCLUDE = f'#include "physics/runtime/{HEADER}"\n'
ANCHOR = '#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
NAMESPACE = 'native_contact_predicate_owner_detail'
ORDERED = 'native_contact_predicate_ordered_detail'
LABEL = 'NativeContactPredicateResume_'
NEXT = tuple('3038'+value for value in (
    '01','0c','0f','10','16','24','28','2c','2f','53','57','5a',
    '64','67','8e','92','95','9f','a2','a3','a6','ac','af','b0','b3'))
ORDERED_EXPRESSIONS = tuple((old,f'halo2::{ORDERED}::{operation}({args})')
    for old,operation,args in (
        ('_mm_mul_ps(s.xmm[0],s.xmm[2])','multiply','s.xmm[0],s.xmm[2]'),
        ('_mm_add_ss(s.xmm[2],s.xmm[0])','sum_low','s.xmm[2],s.xmm[0]'),
        ('_mm_add_ss(s.xmm[3],s.xmm[2])','sum_low','s.xmm[3],s.xmm[2]'),
        ('_mm_sub_ps(s.xmm[2],s.xmm[0])','subtract','s.xmm[2],s.xmm[0]'),
        ('_mm_mul_ps(s.xmm[0],s.xmm[1])','multiply','s.xmm[0],s.xmm[1]'),
        ('_mm_add_ss(s.xmm[1],s.xmm[0])','sum_low','s.xmm[1],s.xmm[0]'),
        ('_mm_add_ss(s.xmm[2],s.xmm[1])','sum_low','s.xmm[2],s.xmm[1]')))


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def prefix(literal):
    tag='true' if literal else 'false'
    return (f'\n    switch(halo2::{NAMESPACE}::bridge<{tag}>(s)){{case 0u:break;case 1u:return;'
            + ''.join(f'case 0x{pc}u:goto {LABEL}{pc};' for pc in NEXT)
            + 'default:throw std::logic_error("Foreign contact-predicate continuation");}')


def callback(pc, literal):
    if literal:
        return f's.instruction_literal(0x{pc}u);'
    return f'if(s.services.instruction){{s.services.instruction(0x{pc}u);}}'


def only(pattern, text):
    matches=list(pattern.finditer(text))
    if len(matches)!=1:
        raise ValueError('Expected one complete contact-predicate context: '+pattern.pattern[:80])
    return matches[0]


def retained_body(body, prefix_tag, literal):
    at=len(SIGNATURE)
    body=body[:at]+body[at+len(prefix(prefix_tag)):]
    for pc in NEXT:
        original=callback(pc,literal)
        anchor=f'{LABEL}{pc}: '+original
        if body.count(anchor)!=1:
            raise ValueError('Misplaced contact-predicate continuation '+pc)
        body=body.replace(anchor,original,1)
    for original,ordered in ORDERED_EXPRESSIONS:
        if body.count(ordered)!=1:
            raise ValueError('Contact-predicate ordered fallback expression drift')
        body=body.replace(ordered,original,1)
    return body


def canonical_body(body):
    tags=[tag for tag in (False,True) if body.startswith(SIGNATURE+prefix(tag))]
    if (len(tags)!=1 or body.count(NAMESPACE)!=1 or body.count(LABEL)!=2*len(NEXT) or
        body.count(ORDERED)!=len(ORDERED_EXPRESSIONS)):
        raise ValueError('Contact-predicate canonical body requires its one exact owner prefix')
    result=retained_body(body,tags[0],tags[0])
    if not any(digest(result)==context[0] and tag==tags[0] for context,tag in CONTEXTS.items()):
        raise ValueError('Complete contact-predicate canonical body drift')
    return result


def authenticate(source, *, intermediate=False):
    body_match=only(BODY,source)
    body=body_match[0]
    at=len(SIGNATURE)
    tags=[tag for tag in (False,True) if body.startswith(prefix(tag),at)]
    if (source.count(NAMESPACE)!=len(tags) or source.count(HEADER)!=len(tags) or
        source.count(LABEL)!=len(tags)*2*len(NEXT) or
        source.count(ORDERED)!=len(tags)*len(ORDERED_EXPRESSIONS) or source.count(ANCHOR)!=1 or
        (tags and source.count(INCLUDE+ANCHOR)!=1)):
        raise ValueError('Unowned, incomplete or misplaced contact-predicate overlay')
    identity=tuple(digest(only(regex,source)[0]) for regex in (RESOLVER,DISPATCHER,EXPORT,OWNS,STATE))
    possible=[(context[0],tag) for context,tag in CONTEXTS.items() if context[1:]==identity]
    if len(possible)!=1:
        raise ValueError('Complete contact-predicate resolver/dispatcher/export/owns/State context drift')
    body_pin, literal=possible[0]
    if tags:
        if tags[0]!=literal and not (intermediate and literal and tags==[False]):
            raise ValueError('Contact-predicate prefix does not match its authenticated callback format')
        body=retained_body(body,tags[0],literal)
    if digest(body)!=body_pin:
        raise ValueError('Complete retained contact-predicate body drift')
    return body_match,body,literal,bool(tags)


def install(source):
    match,body,literal,installed=authenticate(source)
    if installed:
        return source
    for pc in NEXT:
        old=callback(pc,literal)
        if body.count(old)!=1:
            raise ValueError('Contact-predicate continuation site drift '+pc)
        body=body.replace(old,f'{LABEL}{pc}: '+old,1)
    for original,ordered in ORDERED_EXPRESSIONS:
        if body.count(original)!=1:
            raise ValueError('Contact-predicate ordered fallback expression drift')
        body=body.replace(original,ordered,1)
    at=len(SIGNATURE)
    candidate=(source[:match.start()]+body[:at]+prefix(literal)+body[at:]+source[match.end():])
    candidate=candidate.replace(ANCHOR,INCLUDE+ANCHOR,1)
    authenticate(candidate)
    return candidate


def remove(source):
    match,body,_,installed=authenticate(source)
    if not installed:
        return source
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    return ['303800'] if authenticate(source)[3] else []


def rebind_prepared(source):
    match,_,literal,installed=authenticate(source,intermediate=True)
    if not literal or not installed:
        raise ValueError('Contact-predicate retag requires an already bound prepared owner')
    body=match[0]; at=len(SIGNATURE)
    if body.startswith(prefix(False),at):
        body=body[:at]+prefix(True)+body[at+len(prefix(False)):]
    result=source[:match.start()]+body+source[match.end():]
    authenticate(result)
    return result
