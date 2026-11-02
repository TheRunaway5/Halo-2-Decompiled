"""Keep the caller FP file in place for complete integer weapon owners."""
INCLUDE='#include "halo2/native_weapon_effect_dispatch_tu15.hpp"\n'


def install_native_sound_local(source):
    marker='static void run_18be00(WorldState &s) {\n'
    if marker not in source:return source
    assert source.count(marker)==1
    bridge='''    // Complete18BE00 uses only integer state. Keep the live FP/MMX file.
    if(s.image==SourcePhysicsContactImage::TitleUpdate15) {
        auto flags=s.other_flags|s.flags.bits();
        const auto publish=[&] {s.other_flags=flags&~0x8d5u;s.flags.load(flags);};
        try {native_weapon_effect_integer_tu15_run(s.memory,0x18be00u,s.gp,flags,s.services,s.observe);}
        catch(...) {publish();throw;}
        publish();return;
    }
'''
    if marker+bridge not in source:source=source.replace(marker,marker+bridge,1)
    if INCLUDE not in source:
        anchor='#include "physics/runtime/physics_world_runtime_internal.hpp"\n';assert source.count(anchor)==1
        source=source.replace(anchor,anchor+INCLUDE,1)
    return source
