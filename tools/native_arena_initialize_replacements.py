"""Exact TU1.5 Arena initialization overlay and owned const-table producer.

Only the Arena generator's mapped TU1.5 owner is accepted. The fixed 1.1
DeathStartup body and callback-bearing World construction remain outside it.
"""
import hashlib,re
from native_arena_owner_policy_prototype import (
    INCLUDES,OLD_CALL,PUBLISHED_CALL,RUN_PIN,SERVICES_PIN,prototype,run,services)

BODY=re.compile(r'static void run_125280\(WorldState &s\) \{.*?\n\}',re.S)
PIN='29de6f07a200826b98de37dc58b56b2bd9cd59f81e0c96e169e51de821af0551'
RESOLVER_PIN='ff495ade2e6eb33688d00bae80af91364ff47d20914d31c36d283928ae73bbae'
DISPATCH_PIN='151bef387336d665aa7ba909bf0996dabc061036415e327616763d104f0b3a6d'
EXPORT_PIN='4543981d9d679e199c1635f1b1f5d1d5e29d6db26dec69d92b603effb527dd4a'
INCLUDE='#include "physics/runtime/native_tu15_arena_initialize_owner.hpp"\n'
PREFIX='\n    if(halo2::native_arena_initialize_owner_detail::bridge(s,[](auto&current,auto target,auto site,bool tail){dispatch(current,target,site,tail);}))return;'
PRODUCER_PIN='2ee2b9a5f0cb0ff8a00f0f300e7944635c9230ef5868b9ef3b9be4685e6604e4'
INSTALLED_RUN_PIN='ad4e4237a0c4911a52079d962ba4d07e6d3edc65740555dd8b31836a1ae1abea'
PARENT_EXECUTOR='  const std::thread::id parent_executor=std::this_thread::get_id();\n'
BOOTSTRAP_INCLUDE='#include "physics/runtime/native_tu15_arena_schedule_policy.hpp"\n'
BOOTSTRAP_ALIAS='using native_arena_schedule_detail::WorkerIdleBarrier;'
BOOTSTRAP_CONSTRUCTOR_PIN='65c04c4917d370f0e2676223cdce77750b8532ad3bfd2c83d870c6de974da817'
BOOTSTRAP_CALLBACK='[idle](std::uint32_t handle,std::stop_token){idle->before_wait(handle);}'
BOOTSTRAP_NAMED='native_arena_schedule_detail::WorkerIdleBeforeWait{idle}'
BOOTSTRAP_BARRIER='''struct WorkerIdleBarrier {
    std::mutex mutex;
    std::condition_variable changed;
    unsigned waits=0;
    void before_wait(std::uint32_t handle) {
        if(handle!=0xf3000003u)return; // This owner's actual semaphore namespace.
        std::lock_guard lock(mutex);++waits;changed.notify_all();
    }
    void finish_startup() {
        std::unique_lock lock(mutex);
        // Initial idle wait, then one after each of the two source jobs for
        // each savegame file. A completion byte precedes free-list recycling;
        // wait for the source worker itself to finish before stopping it.
        if(!changed.wait_for(lock,std::chrono::seconds(5),[&]{return waits>=5;}))
            throw std::runtime_error("Native arena worker did not retire its four startup file jobs");
    }
};'''

def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def _world_overlay(source):
    return "native_arena_world_initialize_owner_detail" in source or "native_tu15_arena_world_initialize_owner.hpp" in source
def canonical_body(body):
    if _world_overlay(body):
        from native_arena_world_initialize_replacements import canonical_body as canonical_world_body
        return canonical_world_body(body)
    at=body.index('{')+1
    return body[:at]+body[at+len(PREFIX):]if body[at:].startswith(PREFIX)else body
def authenticate(source):
    if _world_overlay(source):
        from native_arena_world_initialize_replacements import authenticate as authenticate_world
        match,body,_,_,_=authenticate_world(source)
        return match,body
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Missing or ambiguous complete Arena initialization owner')
    match=matches[0];body=canonical_body(match[0])
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Incomplete Arena resolver/dispatcher')
    export='void source_run_physics_game_arena_runtime('
    if source.count(export)!=1:raise ValueError('Missing or ambiguous complete Arena export')
    if tuple(map(digest,(body,resolver[0],dispatcher[0],source[source.index(export):])))!=(PIN,RESOLVER_PIN,DISPATCH_PIN,EXPORT_PIN):
        raise ValueError('Complete Arena body/resolver/dispatcher/export drift')
    installed=match[0]!=body
    if source.count(INCLUDE)!=(1 if installed else 0):raise ValueError('Arena include/prefix ownership drift')
    if installed and not source.startswith(INCLUDE):raise ValueError('Relocated Arena owner include')
    return match,body
def replace_arena_initialize(source):
    if _world_overlay(source):
        from native_arena_world_initialize_replacements import replace_arena_world_initialize as world_operation
        return world_operation(source)
    match,body=authenticate(source)
    if match[0]!=body:return source
    at=body.index('{')+1;bound=body[:at]+PREFIX+body[at:]
    return INCLUDE+source[:match.start()]+bound+source[match.end():]
def remove_arena_initialize(source):
    if _world_overlay(source):
        from native_arena_world_initialize_replacements import remove_arena_world_initialize as world_operation
        return world_operation(source)
    match,body=authenticate(source)
    if match[0]==body:return source
    result=source[:match.start()]+body+source[match.end():]
    return result[len(INCLUDE):]
def installed_entries(source):
    if _world_overlay(source):
        from native_arena_world_initialize_replacements import installed_entries as world_operation
        return world_operation(source)
    match,body=authenticate(source)
    if match[0]==body:raise ValueError('Arena initialization native overlay missing')
    return ['126640']

def canonical_owner(source):
    if "native_tu15_arena_world_publication.hpp" in source or "matches_world_publication" in source:
        from native_arena_world_policy_prototype import canonical_arena
        source=canonical_arena(source)
    opening='  struct RequiredOwner {\n'
    if opening not in source:
        if INCLUDES in source or PUBLISHED_CALL in source:raise ValueError('Partial Arena producer publication')
        if digest(services(source))!=SERVICES_PIN or digest(run(source))!=RUN_PIN:
            raise ValueError('Complete original Arena producer/run drift')
        return source
    if source.count(opening)!=1 or not source.startswith(INCLUDES):raise ValueError('Arena producer boundary/include drift')
    start=source.index(opening);end=source.index('  bool platform(',start);current=source[start:end]
    if digest(current)!=PRODUCER_PIN or digest(run(source))!=INSTALLED_RUN_PIN:
        raise ValueError('Complete installed Arena producer/run drift')
    entry='  void required_entry(SourcePhysicsContactMemory &m,const SourcePhysicsContactRuntimeCall &call,\n                      SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &o,\n                      std::stop_token stop) {\n'
    ending='  }\n  SourcePhysicsWorldRuntimeServices services(std::stop_token stop = {}) {\n'
    if current.count(entry)!=1 or current.count(ending)!=1:raise ValueError('Arena required-owner extraction drift')
    body=current.split(entry,1)[1].split(ending,1)[0]
    original='  SourcePhysicsWorldRuntimeServices services(std::stop_token stop = {}) {\n    auto joined = lower;\n    joined.required_owner = [this,stop](auto &m,const auto &call,auto &abi,const auto &o) {\n'+body+'    };\n    return joined;\n  }\n'
    if digest(original)!=SERVICES_PIN:raise ValueError('Arena original required-owner reconstruction differs')
    if source.count(PARENT_EXECUTOR)!=1 or source.count(PUBLISHED_CALL)!=1:raise ValueError('Arena issuer field/call drift')
    restored=source[:start]+original+source[end:]
    restored=restored[len(INCLUDES):].replace(PARENT_EXECUTOR,'',1).replace(PUBLISHED_CALL,OLD_CALL,1)
    if prototype(restored)!=source:raise ValueError('Arena const-publication exact reconstruction differs')
    return restored
def replace_arena_owner(source):
    original=canonical_owner(source)
    return source if original!=source else prototype(original)

def canonical_bootstrap(source):
    installed=BOOTSTRAP_ALIAS in source
    if installed:
        if source.count(BOOTSTRAP_ALIAS+'\n')!=1 or not source.startswith(BOOTSTRAP_INCLUDE) or source.count(BOOTSTRAP_NAMED)!=1:
            raise ValueError('Arena bootstrap fixed-schedule ownership drift')
        original=source[len(BOOTSTRAP_INCLUDE):].replace(BOOTSTRAP_ALIAS,BOOTSTRAP_BARRIER,1).replace(BOOTSTRAP_NAMED,BOOTSTRAP_CALLBACK,1)
    else:
        if BOOTSTRAP_INCLUDE in source or BOOTSTRAP_NAMED in source:raise ValueError('Partial Arena bootstrap schedule overlay')
        original=source
    if original.count(BOOTSTRAP_BARRIER)!=1 or original.count(BOOTSTRAP_CALLBACK)!=1:
        raise ValueError('Complete actual Arena bootstrap barrier/callback drift')
    begin=original.index('NativeTu15ArenaBootstrap::NativeTu15ArenaBootstrap(')
    end=original.index('bool NativeTu15ArenaBootstrap::try_renderer',begin)
    if digest(original[begin:end])!=BOOTSTRAP_CONSTRUCTOR_PIN:raise ValueError('Complete actual Arena bootstrap constructor drift')
    return original
def replace_arena_bootstrap(source):
    original=canonical_bootstrap(source)
    if original!=source:return source
    return BOOTSTRAP_INCLUDE+original.replace(BOOTSTRAP_BARRIER,BOOTSTRAP_ALIAS,1).replace(BOOTSTRAP_CALLBACK,BOOTSTRAP_NAMED,1)
