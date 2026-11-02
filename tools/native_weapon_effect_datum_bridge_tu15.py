"""Install the complete native16E6A0 owner at real local caller boundaries."""
INCLUDE='#include "halo2/native_weapon_effect_dispatch_tu15.hpp"\n'
BEGIN='    // BEGIN native TU1.5 datum allocator\n'
END='    // END native TU1.5 datum allocator\n'


def bridge(mode):
    assert mode in ('tu15','canonical','pool','observed')
    text=BEGIN+'''    // Keep the live FP/MMX file in the caller. This owner uses integers only.
    if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
        auto flags=s.other_flags|s.flags.bits();
        const auto publish=[&] {s.other_flags=flags&~0x8d5u;s.flags.load(flags);};
'''
    if mode=='tu15':
        text+='''        try {native_weapon_effect_datum_allocate_tu15_run(s.memory,s.gp,flags,s.services,s.observe);}
'''
    else:
        text+='''        // Public canonical observations keep their existing address domain.
        auto services=s.services;auto observe=s.observe;
'''
        if mode=='canonical':text+='''        if(s.services.instruction)services.instruction=[&](auto pc){s.services.instruction(pc-0x13e0u);};
'''
        elif mode=='observed':text+='''        if(!s.selected_observation_pcs && s.services.instruction)
            services.instruction=[&](auto pc){s.services.instruction(pc-0x13e0u);};
'''
        condition='!s.selected_observation_pcs && s.observe' if mode=='observed' else 's.observe'
        text+='''        if(CONDITION)observe=[&](const auto &event){auto canonical=event;canonical.instruction-=0x13e0u;s.observe(canonical);};
        try {native_weapon_effect_datum_allocate_tu15_run(s.memory,s.gp,flags,services,observe);}
'''.replace('CONDITION',condition)
    return text+'''        catch(...) {publish();throw;}
        publish();return;
    }
'''+END


def install_native_effect_datum(source,mode='tu15'):
    entry='16e6a0' if mode=='tu15' else '16d2c0'
    state='ObservedWorldState' if mode=='observed' else 'WorldState'
    marker='static void run_'+entry+'('+state+' &s) {\n'
    if marker not in source:return source
    assert source.count(marker)==1
    replacement=marker+bridge(mode)
    if replacement not in source:
        assert BEGIN not in source,'Remove the old declared bridge before changing modes'
        source=source.replace(marker,replacement,1)
    if INCLUDE not in source:
        anchor='#include "physics/runtime/physics_world_runtime_internal.hpp"\n';assert source.count(anchor)==1
        source=source.replace(anchor,anchor+INCLUDE,1)
    return source


def remove_native_effect_datum(source):
    import re
    source=re.sub(re.escape(BEGIN)+r'.*?'+re.escape(END),'',source,flags=re.S)
    return source.replace(INCLUDE,'')
