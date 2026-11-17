"""Exact private binding proposal for the complete Primary 1046C0 owner."""
import hashlib
import re

ENTRY='1046c0'
HEADER='static void run_1046c0(WorldState &s) {'
BODY=re.compile(r'^'+re.escape(HEADER)+r'.*?^\}',re.M|re.S)
OWNER='native_object_ratio_caller_owner_detail'
DETAIL='native_object_ratio_caller_detail'
BASENAME='native_tu15_object_ratio_caller_source_owner.hpp'
INCLUDE=f'#include "gameplay/objects/{BASENAME}"\n'
ANCHOR='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
PINS={
    False:'84a26140524e26d4dddc5eea8b3bf3e7205f8fcd161c034d49e55ef5f6fcfe9c',
    True:'32e7c8d5885c51767fdc5bba6dd66cf4ed67516d133fb25ab16e0c77cad36a42',
}
STATE=re.compile(r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;',re.M|re.S)
DISPATCH=re.compile(r'^static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]+\}$',re.M)
RESOLVER=re.compile(r'^std::uint32_t selected\([^\n]+',re.M)
CONTEXT_PINS={
    False:(
        ('086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
         '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'),
        ('457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
         'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849')),
    True:(
        ('086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
         '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'),
        ('457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
         '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851')),
}
RESOLVER_PIN='1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4'
ARITHMETIC=(
    ('s.set(1,s.flags.shift(s.reg(1,4,0),0x4u,4),4,0);',f'halo2::{DETAIL}::shift_definition_index<true>(s);',3),
    ('s.quotient(s.reg(1,4,0),true);',f'halo2::{DETAIL}::divide_clock_digit<true>(s);',2),
)


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def prefix(literal):
    return ('\n    if(halo2::'+OWNER+'::bridge<'+('true' if literal else 'false')+
            '>(s,halo2::'+OWNER+'::Routes{'
            '[](auto&state){run_fc510(state);},[](auto&state){run_107140(state);},'
            '[](auto&state,std::uint32_t target,std::uint32_t site){dispatch(state,target,site,false);}}))return;')


def arithmetic(body,*,remove=False):
    for old,new,count in ARITHMETIC:
        if remove:old,new=new,old
        if body.count(old)!=count or new in body:
            raise ValueError('Missing, duplicate, or foreign 1046C0 arithmetic sites')
        body=body.replace(old,new)
    return body


def split_parent(body):
    markers=[(literal,prefix(literal)) for literal in (False,True) if prefix(literal) in body]
    if not markers:
        if OWNER in body or DETAIL in body or BASENAME in body:
            raise ValueError('Unknown native 1046C0 marker')
        return body,None
    if len(markers)!=1:
        raise ValueError('Conflicting native 1046C0 callback tags')
    tag,marker=markers[0]
    if body.count(marker)!=1 or not body.startswith(HEADER+marker):
        raise ValueError('Native 1046C0 prefix moved')
    return arithmetic(HEADER+body[len(HEADER)+len(marker):],remove=True),tag


def canonical_body(body):
    """Recover the exact retained owner for the catalog and the 107140 caller pin."""
    return split_parent(body)[0]


canonical_parent=canonical_body


def authenticate(source,allow_old_tag=False):
    names=(OWNER,DETAIL,BASENAME)
    if 'void source_run_physics_live_support_tu15(' not in source:
        if any(name in source for name in names):
            raise ValueError('Native 1046C0 markers outside Primary')
        return None
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing or ambiguous complete 1046C0')
    match=matches[0]
    body,installed=split_parent(match[0])
    raw=re.findall(r's\.services\.instruction\((0x[0-9a-f]+u)\)',body)
    prepared=re.findall(r's\.instruction_literal\((0x[0-9a-f]+u)\)',body)
    if (len(raw),len(prepared)) not in ((406,0),(0,406)):
        raise ValueError('1046C0 needs 406 uniform original callbacks')
    literal=bool(prepared)
    if digest(body)!=PINS[literal] or len(set(prepared or raw))!=406:
        raise ValueError('Complete retained 1046C0 changed')
    if installed is not None and installed!=literal and not allow_old_tag:
        raise ValueError('1046C0 callback tag drift')
    context=[]
    for label,pattern in (('LiveState',STATE),('dispatcher',DISPATCH),('resolver',RESOLVER)):
        rows=pattern.findall(source)
        if len(rows)!=1:raise ValueError('Missing exact 1046C0 '+label)
        context.append(digest(rows[0]))
    if tuple(context[:2]) not in CONTEXT_PINS[literal] or context[2]!=RESOLVER_PIN:
        raise ValueError('Correlated 1046C0 owner context changed')
    active=int(installed is not None)
    if (source.count(OWNER)!=2*active or source.count(DETAIL)!=5*active or
        source.count(BASENAME)!=active or source.count(INCLUDE)!=active):
        raise ValueError('Foreign or duplicate 1046C0 native marker')
    if source.count(ANCHOR)!=1 or (active and source.count(ANCHOR+INCLUDE)!=1):
        raise ValueError('1046C0 include moved across its exact Primary runtime dependency')
    return match,body,literal,installed

def replace_object_ratio_caller(source):
    found=authenticate(source)
    if found is None or found[3] is not None:
        return source
    match,body,literal,_=found
    bound=HEADER+prefix(literal)+arithmetic(body)[len(HEADER):]
    result=(source[:match.start()]+bound+source[match.end():]).replace(ANCHOR,ANCHOR+INCLUDE,1)
    installed_entries(result)
    return result


def remove_object_ratio_caller(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        return source
    match,body,_,_=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def installed_entries(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        raise ValueError('Complete 1046C0 default missing')
    return [ENTRY]


def retag_specialized(source,output):
    before=authenticate(source)
    if before is None or before[3] is None:
        return output
    found=authenticate(output,allow_old_tag=True)
    if found is None:
        raise ValueError('Specializer dropped complete 1046C0')
    match,body,literal,old=found
    if not literal or old!=before[3]:
        raise ValueError('Specializer moved the 1046C0 marker')
    result=output[:match.start()]+HEADER+prefix(True)+arithmetic(body)[len(HEADER):]+output[match.end():]
    installed_entries(result)
    return result
