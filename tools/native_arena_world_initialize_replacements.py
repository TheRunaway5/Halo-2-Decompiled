"""Private strict World/Arena source profiles; no owning generator hook yet."""
import hashlib
import re
import native_arena_initialize_replacements as legacy
import native_arena_initialize_continuation_replacements as continuations

BODY=legacy.BODY
INCLUDE='#include "physics/runtime/native_tu15_arena_world_initialize_owner.hpp"\n'
INCLUDES=(legacy.INCLUDE,INCLUDE)
LABELS=continuations.LABELS
REP=continuations.REP
REP_BOUND=continuations.REP_BOUND
REP_LABEL=continuations.REP_LABEL
REPEAT=continuations.REPEAT
RETAINED_PINS={False:legacy.PIN,True:'6a9545cf326781b62a75596ec5c226182037405f64d18f745228901bfe8a5f97'}
LEGACY_PINS={False:'f24a0aa219496fda4b3ca2c312871e9a73c630f593ae0d90ca742400930c2553',True:'3e35d8867b78f67439adecd98fef1332efa8471d404ee3a6fd4a6f6204236ebc'}
BOUND_PINS={False:'abd537a41738991c9599717a872b285cbaafe8fe2fd58d300936c5371fe90423',True:'acb9ab1e89dfb4949ba1be0b7f5ad998a37947db86abda6d800ad1412c333c91'}
INTERMEDIATE_PIN='c6ab1952432cbb6e8d0b60a306a21c7badd63f3912523ca2c4839ce24696eda2'
DISPATCH_PINS={False:legacy.DISPATCH_PIN,True:'58eb89178f54558f8a11c394f24d870aa37b65b3a2cdc92523eb7026dda7f31f'}

def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def prefix(prepared):
    return '\n    switch(halo2::native_arena_world_initialize_owner_detail::bridge<'+('true'if prepared else'false')+'>(s,[](auto&current,auto target,auto site,bool tail){dispatch(current,target,site,tail);})){case 0u:break;case 0xffffffffu:return;'+''.join('case 0x%xu:goto %s;'%(pc,label[:-2])for pc,label in LABELS.items())+'case 0x1252b8u:goto '+REP_LABEL[:-2]+';default:throw std::logic_error("Foreign World Arena source continuation");}'

def callback(prepared,pc):
    return 's.instruction_literal(0x%xu);'%pc if prepared else 'if(s.services.instruction){s.services.instruction(0x%xu);}'%pc

def format_of(body):
    raw=len(re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}',body))
    prepared=len(re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);',body))
    if body.count(REPEAT)!=1:raise ValueError('Authored Arena trailing REP callback drift')
    if(raw,prepared)==(45,0):return False
    if(raw,prepared)==(0,45):return True
    raise ValueError('Arena source callbacks are not one complete supported format')

def body_parts(body):
    prepared=format_of(body);at=body.index('{')+1
    found=[value for value in(prefix(False),prefix(True))if value in body]
    world=bool(found)
    if world:
        expected=prefix(prepared)
        if found!=[expected]or not body.startswith(expected,at)or digest(body)!=BOUND_PINS[prepared]:
            raise ValueError('Complete World Arena format/prefix/body drift')
        body=body[:at]+body[at+len(expected):]
        if body.count(REP_BOUND)!=1:raise ValueError('World Arena exact repeat retirement block drift')
        body=body.replace(REP_BOUND,REP,1)
        for pc,label in LABELS.items():
            if body.count(label)!=1 or body.count(label+callback(prepared,pc))!=1:
                raise ValueError('World Arena label/next-instruction adjacency drift '+hex(pc))
            body=body.replace(label,'',1)
    elif 'native_arena_source_'in body or 'native_arena_world_initialize_owner_detail::bridge'in body:
        raise ValueError('Partial World Arena source overlay')
    old=legacy.PREFIX in body
    if old:
        if body.count(legacy.PREFIX)!=1 or not body.startswith(legacy.PREFIX,at)or digest(body)!=LEGACY_PINS[prepared]:
            raise ValueError('Complete callback-free Arena fallback drift')
        body=body[:at]+body[at+len(legacy.PREFIX):]
    if digest(body)!=RETAINED_PINS[prepared]:raise ValueError('Complete retained Arena source profile drift')
    return body,prepared,world,old

def canonical_body(body):return body_parts(body)[0]

def one(source,pattern,name):
    found=re.findall(pattern,source,re.M|re.S)
    if len(found)!=1:raise ValueError('Missing/ambiguous World Arena '+name)
    return found[0]

def authenticate(source):
    found=list(BODY.finditer(source))
    if len(found)!=1:raise ValueError('Missing exact World Arena parent')
    match=found[0];retained,prepared,world,old=body_parts(match[0])
    resolver=one(source,r'^std::uint32_t selected\([^\n]+','resolver')
    dispatcher=one(source,r'^static void dispatch\(WorldState &s,[^\n]+','dispatcher')
    marker='void source_run_physics_game_arena_runtime('
    if source.count(marker)!=1:raise ValueError('Missing exact World Arena export')
    if(digest(resolver),digest(dispatcher),digest(source[source.index(marker):]))!=(legacy.RESOLVER_PIN,DISPATCH_PINS[prepared],legacy.EXPORT_PIN):
        raise ValueError('Complete World Arena resolver/dispatcher/export drift')
    if source.count(legacy.INCLUDE)!=int(old)or source.count(INCLUDE)!=int(world):
        raise ValueError('Partial World Arena source includes')
    if old and not source.startswith(legacy.INCLUDE):raise ValueError('Callback-free Arena include moved')
    if world and(not old or not source.startswith(legacy.INCLUDE+INCLUDE)):
        raise ValueError('World Arena include order drift')
    if source.count(legacy.PREFIX)!=int(old)or source.count('halo2::native_arena_initialize_owner_detail::bridge')!=int(old):
        raise ValueError('Callback-free Arena fallback contains an external prefix')
    if sum(source.count(value)for value in(prefix(False),prefix(True)))!=int(world)or source.count('halo2::native_arena_world_initialize_owner_detail::bridge')!=int(world):
        raise ValueError('World Arena source contains a foreign or relocated prefix')
    return match,retained,prepared,world,old

def replace_arena_world_initialize(source):
    match,retained,prepared,world,old=authenticate(source)
    if world:return source
    proposed=retained.replace(REP,REP_BOUND,1)
    for pc,label in LABELS.items():
        value=callback(prepared,pc)
        if proposed.count(value)!=1:raise ValueError('Missing actual Arena continuation '+hex(pc))
        proposed=proposed.replace(value,label+value,1)
    at=proposed.index('{')+1;proposed=proposed[:at]+prefix(prepared)+legacy.PREFIX+proposed[at:]
    result=source[:match.start()]+proposed+source[match.end():]
    if not old:result=legacy.INCLUDE+result
    result=result[:len(legacy.INCLUDE)]+INCLUDE+result[len(legacy.INCLUDE):]
    authenticate(result);return result

def remove_arena_world_initialize(source):
    match,retained,prepared,world,old=authenticate(source)
    result=source[:match.start()]+retained+source[match.end():]
    if old:result=result[len(legacy.INCLUDE):]
    if world:
        if not result.startswith(INCLUDE):raise ValueError('World Arena exact include removal drift')
        result=result[len(INCLUDE):]
    return result

def remove_world_only(source):
    """Recover the exact prior callback-free default for old-owner audits."""
    match,retained,prepared,world,old=authenticate(source)
    if not world:return source
    if not old:raise ValueError('World Arena overlay lost its independent callback-free fallback')
    at=retained.index('{')+1
    prior=retained[:at]+legacy.PREFIX+retained[at:]
    if digest(prior)!=LEGACY_PINS[prepared]:raise ValueError('Complete previous Arena owner profile drift')
    result=source[:match.start()]+prior+source[match.end():]
    begin=len(legacy.INCLUDE)
    if not result.startswith(legacy.INCLUDE+INCLUDE):raise ValueError('World Arena exact prior include boundary drift')
    result=result[:begin]+result[begin+len(INCLUDE):]
    authenticate(result)
    return result

def installed_entries(source):
    found=authenticate(source)
    if not found[3]:raise ValueError('World Arena continuation overlay is not installed')
    return ['126640']

def retag_specialized(source,output):
    original=authenticate(source)
    if not original[3]:return output
    match=BODY.search(output)
    if not match:raise ValueError('Specializer removed World Arena owner')
    if digest(match[0])==BOUND_PINS[True]:installed_entries(output);return output
    if original[2]or digest(match[0])!=INTERMEDIATE_PIN or not match[0].startswith(prefix(False),match[0].index('{')+1):
        raise ValueError('World Arena prepared intermediate body drift')
    result=output[:match.start()]+match[0].replace(prefix(False),prefix(True),1)+output[match.end():]
    installed_entries(result);return result
