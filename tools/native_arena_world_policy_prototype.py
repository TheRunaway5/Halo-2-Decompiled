"""Exact, uninstalled World-to-Arena construction/publication prototype."""
import hashlib
import native_arena_initialize_replacements as original
import native_arena_world_producer_inputs as world

INCLUDE='#include "physics/runtime/native_tu15_arena_world_publication.hpp"\n'
WORLD_PIN='28de58b3676095b1c011a293f10714dcd786662da8e52b6880bf54d8932f4911'
ARENA_PIN='251cb01e96265b8fd56415d5837e1fb6511a8150c58f816a8cdcf0aa9c7ddeea'
WORLD_BOUND_PIN='7140a0f0d69c53d5abd3d6e2061facf908b94bcd597246ca0fde20609b38f994'
ARENA_BOUND_PIN='63d56965ee26855c0abfcb7bed60242cc83fe99820a05f52c60208c4f675ee63'
WORLD_CALL='            a=frame({});arena->run(entry,a);'
WORLD_SCOPED='''            a=frame({});
            if(entry==0x125280u) {
                native_arena_world_publication_detail::Construction published_world_arena{*arena,bank,services(),this};
                arena->run(entry,a);
            }else arena->run(entry,a);'''
OLD_CALL='  const auto joined=impl_->services();\n'
NEW_CALL=OLD_CALL+'''  native_arena_world_publication_detail::Borrow published_world{*this,impl_->shared,joined,impl_.get(),&Impl::RequiredOwner::matches_world_publication};
'''
MATCHER='''    static bool matches_world_publication(const SourcePhysicsWorldRuntimeServices&services,
        const void*expected,const SourcePhysicsGameArenaNative&arena)noexcept {
      const auto*actual=services.required_owner.target<RequiredOwner>();
      if(!actual||actual->owner!=expected||!actual->owner)return false;
      const auto&owner=*actual->owner;
      return owner.parent_executor==std::this_thread::get_id()&&!owner.observe&&
          !owner.config.schedule.before_worker_wait&&!owner.config.schedule.before_main_yield&&
          native_arena_world_publication_detail::Construction::qualified(arena,owner.shared.bank,owner.lower)&&
          native_arena_world_publication_detail::same_instruction(services,owner.lower)&&
          native_required_owner_policy_detail::preserves_instruction_policy(services);
    }
'''

# The independently qualified D3950 World admission adds exactly these two
# siblings, each at its exact position. Removing them recovers the pinned World.
D3950_NAME='owns_d3950_node_progress_call'
D3950_SIBLINGS=(
    ('constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;\n',
     'constexpr bool owns_d3950_node_progress_call(SourcePhysicsContactImage selected_image,\n'
     '                                               const SourcePhysicsContactRuntimeCall &call) noexcept {\n'
     '    return selected_image==SourcePhysicsContactImage::TitleUpdate15 && !call.tail &&\n'
     '        call.target==0x1cf0e0u && (call.instruction==0xd3998u || call.instruction==0xd39b3u);\n'
     '}\n',
     'constexpr SourcePhysicsFactoryFloatingPoint fp{0x27f,0x1f80};\n'),
    ('        }\n',
     '        if(owns_d3950_node_progress_call(image,c)) {\n'
     '            source_run_ai_frame_tu15(m,c.target,a,fp,services(),o);return;\n'
     '        }\n',
     '        if(c.target==0x11dc00u || c.target==0x11dd90u || source_ai_frame_tu15_owns_entry(c.target)) {\n'),
)

def digest(value):return hashlib.sha256(value.encode()).hexdigest()

def world_core(source):
    if D3950_NAME not in source:return source
    for before,text,after in D3950_SIBLINGS:
        if source.count(text)!=1 or source.count(before+text+after)!=1:
            raise ValueError('D3950 World admission sibling drift')
        source=source.replace(text,'',1)
    if D3950_NAME in source:raise ValueError('Foreign D3950 World admission marker')
    return source

def canonical_world(source):
    core=world_core(source)
    if digest(core)==WORLD_PIN:return source
    if digest(core)!=WORLD_BOUND_PIN or not source.startswith(INCLUDE):
        raise ValueError('Complete World construction publication profile drift')
    if source.count(INCLUDE)!=1 or source.count(WORLD_SCOPED)!=1:
        raise ValueError('World construction publication scope drift')
    restored=source[len(INCLUDE):].replace(WORLD_SCOPED,WORLD_CALL,1)
    if digest(world_core(restored))!=WORLD_PIN:
        raise ValueError('World construction publication original reconstruction drift')
    return restored

def canonical_arena(source):
    if digest(source)==ARENA_PIN:
        original.canonical_owner(source)
        return source
    if digest(source)!=ARENA_BOUND_PIN or not source.startswith(INCLUDE):
        raise ValueError('Complete Arena World-publication profile drift')
    if source.count(INCLUDE)!=1 or source.count(MATCHER)!=1 or source.count(NEW_CALL)!=1:
        raise ValueError('Arena World-publication scope drift')
    restored=source[len(INCLUDE):].replace(MATCHER,'',1).replace(NEW_CALL,OLD_CALL,1)
    if digest(restored)!=ARENA_PIN:
        raise ValueError('Arena World-publication original reconstruction drift')
    original.canonical_owner(restored)
    return restored

def world_source(source):
    canonical=canonical_world(source)
    if canonical!=source:return source
    if digest(world_core(source))!=WORLD_PIN:raise ValueError('Complete accepted public World baseline drift')
    if INCLUDE in source:raise ValueError('World construction prototype is already present')
    start=source.index('        arena=std::make_unique<SourcePhysicsGameArenaNative>')
    end=source.index('        // Actual32CBF8',start)
    span=source[start:end]
    if digest(span)!=world.PINS['construction'] or span.count(WORLD_CALL)!=1:
        raise ValueError('Complete actual World/Arena construction span drift')
    proposed=INCLUDE+source[:start]+span.replace(WORLD_CALL,WORLD_SCOPED,1)+source[end:]
    if proposed[len(INCLUDE):].replace(WORLD_SCOPED,WORLD_CALL,1)!=source:
        raise ValueError('World construction prototype changed unrelated source')
    if canonical_world(proposed)!=source:raise ValueError('World publication output profile drift')
    return proposed

def arena_source(source):
    canonical=canonical_arena(source)
    if canonical!=source:return source
    if digest(source)!=ARENA_PIN:raise ValueError('Complete accepted public Arena baseline drift')
    original.canonical_owner(source)
    if INCLUDE in source or source.count(OLD_CALL)!=1:
        raise ValueError('Arena source publication call scope drift')
    marker='  };\n  bool preserves_parent_instruction_policy()const noexcept {'
    if source.count(marker)!=1:raise ValueError('Exact private named Arena owner boundary drift')
    proposed=INCLUDE+source.replace(marker,MATCHER+marker,1).replace(OLD_CALL,NEW_CALL,1)
    restored=proposed[len(INCLUDE):].replace(MATCHER,'',1).replace(NEW_CALL,OLD_CALL,1)
    if restored!=source:raise ValueError('Arena source publication prototype changed unrelated bytes')
    original.canonical_owner(restored)
    if canonical_arena(proposed)!=source:raise ValueError('Arena publication output profile drift')
    return proposed
