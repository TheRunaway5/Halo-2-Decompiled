"""Install the shared native17DCE0 owner in complete TU1.5 caller modules."""

ENTRY='static void run_17dce0(WorldState &s) {'
INCLUDE='#include "halo2/native_weapon_effect_dispatch_tu15.hpp"\n'
BRIDGE='''
    // Native17DCE0 owns the cursor/GP/defined-flags boundary. The caller's
    // live FP/MMX file remains in place; this integer-only kernel preserves it.
    if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
        auto flags=s.other_flags|s.flags.bits();
        const auto publish=[&] {
            s.other_flags=flags&~0x8d5u;s.flags.load(flags);
        };
        try {native_weapon_effect_iterator_tu15_run(s.memory,s.gp,flags,s.services,s.observe);}
        catch(...) {publish();throw;}
        publish();return;
    }
'''


def install_native_effect_iterator(source):
    if ENTRY not in source:return source
    assert source.count(ENTRY)==1
    if ENTRY+BRIDGE not in source:source=source.replace(ENTRY,ENTRY+BRIDGE,1)
    if INCLUDE not in source:
        anchor='#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
        assert source.count(anchor)==1
        source=source.replace(anchor,anchor+INCLUDE,1)
    return source
