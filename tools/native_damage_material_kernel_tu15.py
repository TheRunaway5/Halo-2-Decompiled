#!/usr/bin/env python3
"""Install the native D8110 route without changing preserved literal bodies."""
import hashlib
from pathlib import Path

INCLUDE='''// BEGIN native material damage include
#include "gameplay/objects/source_damage_material_native_tu15_internal.hpp"
// END native material damage include
'''
LEGACY_ROUTE='''// BEGIN native TU1.5 material damage
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
    damage_material_lookup_native_tu15(s,
        [&](std::uint32_t canonical,std::uint32_t actual,bool publish=true){
            const auto pc=damage_observation_pc(canonical,actual,s);
            if(publish && s.services.instruction)s.services.instruction(pc);
            return pc;
        },[&]{run_13f540(s);});
    return;
}
#endif
// END native TU1.5 material damage
'''
ROUTE='''// BEGIN native TU1.5 material damage
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
    const auto site=[&](std::uint32_t canonical,std::uint32_t actual,bool publish=true){
        const auto pc=damage_observation_pc(canonical,actual,s);
        if(publish && s.services.instruction)s.services.instruction(pc);
        return pc;
    };
    damage_material_lookup_native_tu15(s,site,[&]{
        damage_material_search_native_tu15(s,site,[&](std::uint32_t target){
            if(target==0xd8100u)damage_material_compare_native_tu15(s,site);
            else dispatch(s,target,0x13f566u,false);
        });
    });
    return;
}
#endif
// END native TU1.5 material damage
'''
LEGACY_GETTER='''// BEGIN native material damage activity
SourceDamageMaterialNativeActivity source_damage_material_native_activity() {
    using namespace physics_world_runtime_detail;
    return {damage_material_native_calls.load(std::memory_order_relaxed),
        damage_material_groups_found.load(std::memory_order_relaxed),
        damage_material_modifiers_found.load(std::memory_order_relaxed)};
}
// END native material damage activity
'''
GETTER=LEGACY_GETTER.replace(
    'damage_material_modifiers_found.load(std::memory_order_relaxed)};',
    'damage_material_modifiers_found.load(std::memory_order_relaxed),\n'
    '        damage_material_search_calls.load(std::memory_order_relaxed),\n'
    '        damage_material_comparator_calls.load(std::memory_order_relaxed)};')


def strip_native_material_lookup(source):
    for part in (INCLUDE,ROUTE,LEGACY_ROUTE,GETTER,LEGACY_GETTER):
        if source.count(part)>1:raise ValueError('Duplicate native material hook')
        source=source.replace(part,'')
    if '// BEGIN native material damage' in source or '// BEGIN native TU1.5 material damage' in source:
        raise ValueError('Changed native material hook; retain and re-audit')
    return source


def install_native_material_lookup(source):
    source=strip_native_material_lookup(source)
    anchor='static void run_d71c0(ObservedDamageState &s) {\n'
    if source.count(anchor)!=1:raise ValueError('Material damage owner changed')
    start=source.index(anchor);end=source.index('\n}',start)+2
    if hashlib.sha256(source[start:end].encode()).hexdigest()!='50889bb9b5f00ba4a1eb60a60e3496ba4f3aaa2edbe95459a31f3e4ea1c9b102':
        raise ValueError('Preserved material damage body changed')
    include='#include "halo2/source_damage_delivery_runtime.hpp"\n'
    if source.count(include)!=1 or source.count('} // namespace halo2\n')!=1:
        raise ValueError('Damage module layout changed')
    # Keep an existing voice observer's include order. The regeneration route
    # installs that observer afterward and must produce the same final source.
    voice_end='// END native damage voice request include\n'
    include_anchor=voice_end if voice_end in source else include
    if source.count(include_anchor)!=1:raise ValueError('Damage include ownership changed')
    source=source.replace(anchor,anchor+ROUTE).replace(include_anchor,include_anchor+INCLUDE)
    return source.replace('} // namespace halo2\n',GETTER+'} // namespace halo2\n')
