"""Install qualified native region owners without altering retained literals."""
import hashlib

DELIVERY_INCLUDE='''// BEGIN native damage region eligibility include
#include "gameplay/objects/source_damage_region_eligibility_native_tu15_internal.hpp"
// END native damage region eligibility include
'''
REGIONS_INCLUDE='''// BEGIN native object region query include
#include "gameplay/objects/source_object_regions_native_tu15_internal.hpp"
// END native object region query include
'''


def strip(source,include,marker):
    assert source.count(include)<=1
    source=source.replace(include,'')
    first='// BEGIN '+marker+'\n';last='// END '+marker+'\n'
    if first in source:
        assert source.count(first)==source.count(last)==1
        a=source.index(first);b=source.index(last,a)+len(last);source=source[:a]+source[b:]
    assert first not in source and last not in source
    return source


def strip_damage_region_eligibility(source):
    return strip(source,DELIVERY_INCLUDE,'native TU1.5 damage region eligibility')


def install_damage_region_eligibility(source,observed=True):
    source=strip_damage_region_eligibility(source)
    kind='ObservedDamageState' if observed else 'WorldState'
    anchor='static void run_db570('+kind+' &s) {\n'
    assert source.count(anchor)==1
    start=source.index(anchor);end=source.index('\n}',start)+2
    expected=('0713033fd1414f8cd979c4b67c787df5ed37cab3bbf268a36e24ff8a24af32ad' if observed else
              '8d3ff8847e96d14914c6b2d2803d9788c942afddba3748acdbbbda9eeec08e8c')
    assert hashlib.sha256(source[start:end].encode()).hexdigest()==expected,'Retain and re-audit changed DC4C0 literal'
    pc='damage_observation_pc(canonical,actual,s)' if observed else 'canonical'
    route='''// BEGIN native TU1.5 damage region eligibility
if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
    damage_region_eligibility_native_tu15(s,[&](std::uint32_t canonical,std::uint32_t actual){
        (void)actual;const auto pc='''+pc+''';
        if(s.services.instruction)s.services.instruction(pc);
        return pc;
    });
    return;
}
// END native TU1.5 damage region eligibility
'''
    include='#include "halo2/source_damage_delivery_runtime.hpp"\n';assert source.count(include)==1
    return source.replace(include,include+DELIVERY_INCLUDE).replace(anchor,anchor+route)


def strip_object_region_query(source):
    return strip(source,REGIONS_INCLUDE,'native TU1.5 object region query')


def install_object_region_query(source):
    source=strip_object_region_query(source)
    anchor='static void run_bc6c0(WorldState &s) {\n';assert source.count(anchor)==1
    start=source.index(anchor);end=source.index('\n}',start)+2
    assert hashlib.sha256(source[start:end].encode()).hexdigest()=='a842a1493a954e82c6e7d0ef04fc84f280e379c8ac63bfc89fbfc6c07c910796','Retain and re-audit changed BC6C0 literal'
    route='''// BEGIN native TU1.5 object region query
if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
    object_regions_native_tu15(s,[&](std::uint32_t pc){
        if(s.services.instruction)s.services.instruction(pc);
        return pc;
    });
    return;
}
// END native TU1.5 object region query
'''
    include='#include "physics/runtime/physics_world_runtime_internal.hpp"\n';assert source.count(include)==1
    return source.replace(include,include+REGIONS_INCLUDE).replace(anchor,anchor+route)
